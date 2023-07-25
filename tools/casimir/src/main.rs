use anyhow::Result;
use argh::FromArgs;
use futures::future::poll_fn;
use std::future::Future;
use std::net::{Ipv4Addr, SocketAddrV4};
use std::pin::Pin;
use std::task::Context;
use std::task::Poll;
use tokio::io::AsyncReadExt;
use tokio::io::AsyncWriteExt;
use tokio::net::{tcp, TcpListener, TcpStream};
use tokio::select;
use tokio::sync::mpsc;

pub mod controller;
pub mod packets;

use controller::Controller;
use packets::nci;

const MAX_DEVICES: usize = 2;
type Id = usize;

/// Read NCI Control and Data packets received on the NCI transport.
/// Performs recombination of the segmented packets.
pub struct NciReader {
    socket: tcp::OwnedReadHalf,
}

/// Write NCI Control and Data packets received to the NCI transport.
/// Performs segmentation of the packets.
pub struct NciWriter {
    socket: tcp::OwnedWriteHalf,
}

impl NciReader {
    pub fn new(socket: tcp::OwnedReadHalf) -> Self {
        NciReader { socket }
    }

    pub async fn read(&mut self) -> Result<Vec<u8>> {
        let mut complete_packet = vec![0, 0, 0];

        // Note on reassembly:
        // - for each segment of a Control Message, the header of the
        //   Control Packet SHALL contain the same MT, GID and OID values,
        // - for each segment of a Data Message the header of the Data
        //   Packet SHALL contain the same MT and Conn ID.
        // Thus it is correct to keep only the last header of the segmented
        // packet.
        loop {
            // Read the common packet header.
            self.socket.read_exact(&mut complete_packet[0..3]).await?;
            let header = nci::PacketHeader::parse(&complete_packet[0..3])?;

            // Read the packet payload.
            let payload_length = header.get_payload_length() as usize;
            let mut payload_bytes = vec![0; payload_length];
            self.socket.read_exact(&mut payload_bytes).await?;
            complete_packet.extend(payload_bytes);

            // Check the Packet Boundary Flag.
            match header.get_pbf() {
                nci::PacketBoundaryFlag::CompleteOrFinal => return Ok(complete_packet),
                nci::PacketBoundaryFlag::Incomplete => (),
            }
        }
    }
}

impl NciWriter {
    pub fn new(socket: tcp::OwnedWriteHalf) -> Self {
        NciWriter { socket }
    }

    async fn write(&mut self, mut packet: &[u8]) -> Result<()> {
        let mut header_bytes = [packet[0], packet[1], 0];
        packet = &packet[3..];

        loop {
            // Update header with framing information.
            let chunk_length = std::cmp::min(255, packet.len());
            let pbf = if chunk_length < packet.len() {
                nci::PacketBoundaryFlag::Incomplete
            } else {
                nci::PacketBoundaryFlag::CompleteOrFinal
            };
            header_bytes[0] &= !0x10;
            header_bytes[0] |= (pbf as u8) << 4;
            header_bytes[2] = chunk_length as u8;

            // Write the header and payload segment bytes.
            self.socket.write_all(&header_bytes).await?;
            self.socket.write_all(&packet[..chunk_length]).await?;
            packet = &packet[chunk_length..];

            if packet.is_empty() {
                return Ok(());
            }
        }
    }
}

pub struct Device {
    // Async task running the controller main loop.
    task: Pin<Box<dyn Future<Output = Result<()>>>>,
    // Channel for injecting RF data packets into the controller instance.
    rf_tx: mpsc::Sender<Vec<u8>>,
}

impl Device {
    fn new(id: Id, socket: TcpStream, controller_rf_tx: mpsc::Sender<(Id, Vec<u8>)>) -> Device {
        let (rf_tx, rf_rx) = mpsc::channel(2);
        Device {
            rf_tx,
            task: Box::pin(async move {
                let (nci_rx, nci_tx) = socket.into_split();
                let mut controller = Controller::new(
                    id,
                    NciReader::new(nci_rx),
                    NciWriter::new(nci_tx),
                    rf_rx,
                    controller_rf_tx,
                );
                controller.run().await
            }),
        }
    }
}

#[derive(Default)]
struct Scene {
    devices: [Option<Device>; MAX_DEVICES],
}

impl Scene {
    fn new() -> Scene {
        Default::default()
    }

    fn add_device(&mut self, socket: TcpStream, rf_tx: mpsc::Sender<(Id, Vec<u8>)>) -> Result<Id> {
        for id in 0..MAX_DEVICES {
            if self.devices[id].is_none() {
                self.devices[id] = Some(Device::new(id, socket, rf_tx));
                return Ok(id);
            }
        }
        Err(anyhow::anyhow!("max number of connections reached"))
    }

    fn poll(&mut self, cx: &mut Context<'_>) -> Poll<()> {
        for id in 0..MAX_DEVICES {
            if let Some(ref mut device) = &mut self.devices[id] {
                match device.task.as_mut().poll(cx) {
                    Poll::Ready(Ok(_)) => unreachable!(),
                    Poll::Ready(Err(err)) => {
                        println!("dropping device {}: {}", id, err);
                        self.devices[id] = None;
                    }
                    Poll::Pending => (),
                }
            }
        }
        Poll::Pending
    }

    async fn send(&self, sender_id: Id, packet: &[u8]) -> Result<()> {
        for id in 0..MAX_DEVICES {
            if id == sender_id {
                continue;
            }
            if let Some(ref device) = self.devices[id] {
                device.rf_tx.send(packet.to_owned()).await?;
            }
        }

        Ok(())
    }
}

#[derive(FromArgs, Debug)]
/// Nfc emulator.
struct Opt {
    #[argh(option, default = "7000")]
    /// configure the TCP port for the NCI server.
    nci_port: u16,
}

async fn run() -> Result<()> {
    let opt: Opt = argh::from_env();
    let mut scene = Scene::new();
    let nci_address = SocketAddrV4::new(Ipv4Addr::LOCALHOST, opt.nci_port);
    let nci_listener = TcpListener::bind(nci_address).await?;
    let (rf_tx, mut rf_rx) = mpsc::channel(2);
    println!("Listening at address 127.0.0.1:{}", opt.nci_port);
    loop {
        select! {
            result = nci_listener.accept() => {
                let (socket, addr) = result?;
                println!("Incoming connection from {}", addr);
                match scene.add_device(socket, rf_tx.clone()) {
                    Ok(id) => println!("Accepted connection from {} in slot {}", addr, id),
                    Err(err) => println!("Failed to accept connection from {}: {}", addr, err)
                }
            },
            _ = poll_fn(|cx| scene.poll(cx)) => (),
            result = rf_rx.recv() => {
                let (sender_id, packet) = result.ok_or(anyhow::anyhow!("rf_rx channel closed"))?;
                scene.send(sender_id, &packet).await?
            }
        }
    }
}

#[tokio::main]
async fn main() -> Result<()> {
    run().await
}

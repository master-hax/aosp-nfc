// Copyright 2023, The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! NFCC and RF emulator.

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
use tokio::sync::Mutex;

pub mod controller;
pub mod packets;

use controller::Controller;
use packets::{nci, rf};

const MAX_DEVICES: usize = 16;
type Id = u16;

/// Read NCI Control and Data packets received on the NCI transport.
/// Performs recombination of the segmented packets.
pub struct NciReader {
    socket: Mutex<tcp::OwnedReadHalf>,
}

/// Write NCI Control and Data packets received to the NCI transport.
/// Performs segmentation of the packets.
pub struct NciWriter {
    socket: Mutex<tcp::OwnedWriteHalf>,
}

impl NciReader {
    /// Create a new NCI reader from the TCP socket half.
    pub fn new(socket: tcp::OwnedReadHalf) -> Self {
        NciReader { socket: Mutex::new(socket) }
    }

    /// Read a single NCI packet from the reader. The packet is automatically
    /// re-assembled if segmented on the NCI transport.
    pub async fn read(&self) -> Result<Vec<u8>> {
        const HEADER_SIZE: usize = 3;
        let mut socket = self.socket.lock().await;
        let mut complete_packet = vec![0; HEADER_SIZE];

        // Note on reassembly:
        // - for each segment of a Control Message, the header of the
        //   Control Packet SHALL contain the same MT, GID and OID values,
        // - for each segment of a Data Message the header of the Data
        //   Packet SHALL contain the same MT and Conn ID.
        // Thus it is correct to keep only the last header of the segmented
        // packet.
        loop {
            // Read the common packet header.
            socket.read_exact(&mut complete_packet[0..HEADER_SIZE]).await?;
            let header = nci::PacketHeader::parse(&complete_packet[0..HEADER_SIZE])?;

            // Read the packet payload.
            let payload_length = header.get_payload_length() as usize;
            let mut payload_bytes = vec![0; payload_length];
            socket.read_exact(&mut payload_bytes).await?;
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
    /// Create a new NCI writer from the TCP socket half.
    pub fn new(socket: tcp::OwnedWriteHalf) -> Self {
        NciWriter { socket: Mutex::new(socket) }
    }

    /// Write a single NCI packet to the writer. The packet is automatically
    /// segmented if the payload exceeds the maximum size limit.
    async fn write(&self, mut packet: &[u8]) -> Result<()> {
        let mut socket = self.socket.lock().await;
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
            const PBF_MASK: u8 = 0x10;
            header_bytes[0] &= !PBF_MASK;
            header_bytes[0] |= (pbf as u8) << 4;
            header_bytes[2] = chunk_length as u8;

            // Write the header and payload segment bytes.
            socket.write_all(&header_bytes).await?;
            socket.write_all(&packet[..chunk_length]).await?;
            packet = &packet[chunk_length..];

            if packet.is_empty() {
                return Ok(());
            }
        }
    }
}

/// Read RF Control and Data packets received on the RF transport.
/// Performs recombination of the segmented packets.
pub struct RfReader {
    socket: tcp::OwnedReadHalf,
}

/// Write RF Control and Data packets received to the RF transport.
/// Performs segmentation of the packets.
pub struct RfWriter {
    socket: tcp::OwnedWriteHalf,
}

impl RfReader {
    /// Create a new RF reader from the TCP socket half.
    pub fn new(socket: tcp::OwnedReadHalf) -> Self {
        RfReader { socket }
    }

    /// Read a single RF packet from the reader.
    /// RF packets are framed with the byte size encoded as little-endian u16.
    pub async fn read(&mut self) -> Result<Vec<u8>> {
        const HEADER_SIZE: usize = 2;
        let mut header_bytes = [0; HEADER_SIZE];

        // Read the header bytes.
        self.socket.read_exact(&mut header_bytes[0..HEADER_SIZE]).await?;
        let packet_length = u16::from_le_bytes(header_bytes) as usize;

        // Read the packet data.
        let mut packet_bytes = vec![0; packet_length];
        self.socket.read_exact(&mut packet_bytes).await?;

        Ok(packet_bytes)
    }
}

impl RfWriter {
    /// Create a new RF writer from the TCP socket half.
    pub fn new(socket: tcp::OwnedWriteHalf) -> Self {
        RfWriter { socket }
    }

    /// Write a single RF packet to the writer.
    /// RF packets are framed with the byte size encoded as little-endian u16.
    async fn write(&mut self, packet: &[u8]) -> Result<()> {
        let packet_length: u16 = packet.len().try_into()?;
        let header_bytes = packet_length.to_le_bytes();

        // Write the header bytes.
        self.socket.write_all(&header_bytes).await?;

        // Write the packet data.
        self.socket.write_all(packet).await?;

        Ok(())
    }
}

/// Represent a generic NFC device interacting on the RF transport.
/// Devices communicate together through the RF mpsc channel.
/// NFCCs are an instance of Device.
pub struct Device {
    // Async task running the controller main loop.
    task: Pin<Box<dyn Future<Output = Result<()>>>>,
    // Channel for injecting RF data packets into the controller instance.
    rf_tx: mpsc::Sender<rf::RfPacket>,
}

impl Device {
    fn nci(id: Id, socket: TcpStream, controller_rf_tx: mpsc::Sender<rf::RfPacket>) -> Device {
        let (rf_tx, rf_rx) = mpsc::channel(2);
        Device {
            rf_tx,
            task: Box::pin(async move {
                let (nci_rx, nci_tx) = socket.into_split();
                Controller::run(
                    id,
                    NciReader::new(nci_rx),
                    NciWriter::new(nci_tx),
                    rf_rx,
                    controller_rf_tx,
                )
                .await
            }),
        }
    }

    fn rf(id: Id, socket: TcpStream, controller_rf_tx: mpsc::Sender<rf::RfPacket>) -> Device {
        let (rf_tx, mut rf_rx) = mpsc::channel(2);
        Device {
            rf_tx,
            task: Box::pin(async move {
                let (socket_rx, socket_tx) = socket.into_split();
                let mut rf_reader = RfReader::new(socket_rx);
                let mut rf_writer = RfWriter::new(socket_tx);

                loop {
                    select! {
                        result = rf_reader.read() => {
                            // Replace the sender identifier in the packet
                            // with the assigned number for the RF connection.
                            // TODO: currently the generated API does not allow
                            // modifying the parsed fields so the change needs to be
                            // applied to the unparsed packet.
                            let mut packet_bytes = result?;
                            packet_bytes[0..2].copy_from_slice(&id.to_le_bytes());

                            // Parse the input packet.
                            let packet = rf::RfPacket::parse(&packet_bytes)?;

                            // Forward the packet to other devices.
                            controller_rf_tx.send(packet).await?;
                        }
                        result = rf_rx.recv() => {
                            // Forward the packet to the socket connection.
                            use packets::rf::Packet;
                            let packet = result.ok_or(anyhow::anyhow!("rf_rx channel closed"))?;
                            rf_writer.write(&packet.to_vec()).await?;
                        }
                    }
                }
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

    fn add_device(&mut self, builder: impl FnOnce(Id) -> Device) -> Result<Id> {
        //socket: TcpStream, rf_tx: mpsc::Sender<(Id, Vec<u8>)>) -> Result<Id> {
        for id in 0..MAX_DEVICES {
            if self.devices[id].is_none() {
                self.devices[id] = Some(builder(id as Id));
                return Ok(id as Id);
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

    async fn send(&self, packet: &rf::RfPacket) -> Result<()> {
        for id in 0..MAX_DEVICES {
            if id as Id == packet.get_sender()
                || (packet.get_receiver() != u16::MAX && packet.get_receiver() != id as Id)
            {
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
    #[argh(option, default = "7001")]
    /// configure the TCP port for the RF server.
    rf_port: u16,
}

async fn run() -> Result<()> {
    let opt: Opt = argh::from_env();
    let mut scene = Scene::new();
    let nci_listener =
        TcpListener::bind(SocketAddrV4::new(Ipv4Addr::LOCALHOST, opt.nci_port)).await?;
    let rf_listener =
        TcpListener::bind(SocketAddrV4::new(Ipv4Addr::LOCALHOST, opt.rf_port)).await?;
    let (rf_tx, mut rf_rx) = mpsc::channel(2);
    println!("Listening for NCI connections at address 127.0.0.1:{}", opt.nci_port);
    println!("Listening for RF connections at address 127.0.0.1:{}", opt.rf_port);
    loop {
        select! {
            result = nci_listener.accept() => {
                let (socket, addr) = result?;
                println!("Incoming NCI connection from {}", addr);
                match scene.add_device(|id| Device::nci(id, socket, rf_tx.clone())) {
                    Ok(id) => println!("Accepted NCI connection from {} in slot {}", addr, id),
                    Err(err) => println!("Failed to accept NCI connection from {}: {}", addr, err)
                }
            },
            result = rf_listener.accept() => {
                let (socket, addr) = result?;
                println!("Incoming RF connection from {}", addr);
                match scene.add_device(|id| Device::rf(id, socket, rf_tx.clone())) {
                    Ok(id) => println!("Accepted RF connection from {} in slot {}", addr, id),
                    Err(err) => println!("Failed to accept RF connection from {}: {}", addr, err)
                }
            },
            _ = poll_fn(|cx| scene.poll(cx)) => (),
            result = rf_rx.recv() => {
                let packet = result.ok_or(anyhow::anyhow!("rf_rx channel closed"))?;
                scene.send(&packet).await?
            }
        }
    }
}

#[tokio::main]
async fn main() -> Result<()> {
    run().await
}

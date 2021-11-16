//! Rootcanal HAL
//! This connects to "rootcanal" which provides a simulated
//! Nfc chip as well as a simulated environment.

use crate::internal::{InnerHal, RawHal};
use crate::Result;
use bytes::{BufMut, BytesMut};
use log::{debug, error};
use nfc_packets::nci::{DataPacket, NciMsgType, NciPacket, Packet};
use std::convert::TryInto;
use tokio::io::{AsyncReadExt, AsyncWriteExt, BufReader};
use tokio::net::TcpStream;
use tokio::select;
use tokio::sync::mpsc::{UnboundedReceiver, UnboundedSender};

/// Initialize the module
pub async fn init() -> RawHal {
    let (raw_hal, inner_hal) = InnerHal::new();
    let (reader, writer) = TcpStream::connect("127.0.0.1:54323")
        .await
        .expect("unable to create stream to rootcanal")
        .into_split();

    let reader = BufReader::new(reader);
    tokio::spawn(dispatch_incoming(inner_hal.in_cmd_tx, inner_hal.in_dta_tx, reader));
    tokio::spawn(dispatch_outgoing(inner_hal.out_cmd_rx, inner_hal.out_dta_rx, writer));

    raw_hal
}

/// Send NCI events received from the HAL to the NCI layer
async fn dispatch_incoming<R>(
    in_cmd_tx: UnboundedSender<NciPacket>,
    in_dta_tx: UnboundedSender<DataPacket>,
    mut reader: R,
) -> Result<()>
where
    R: AsyncReadExt + Unpin,
{
    loop {
        let mut buffer = BytesMut::with_capacity(1024);
        let len: usize = reader.read_u16().await?.into();
        buffer.resize(len, 0);
        reader.read_exact(&mut buffer).await?;
        let frozen = buffer.freeze();
        debug!("{:?}", &frozen);
        let t: u8 = (frozen[0] >> 5) & 0x7;
        debug!("packet {} received len={}", &t, &len);
        if t == NciMsgType::Response as u8 || t == NciMsgType::Notification as u8 {
            match NciPacket::parse(&frozen) {
                Ok(p) => in_cmd_tx.send(p)?,
                Err(e) => error!("dropping invalid cmd event packet: {}: {:02x}", e, frozen),
            }
        } else if t == NciMsgType::Data as u8 {
            match DataPacket::parse(&frozen) {
                Ok(p) => in_dta_tx.send(p)?,
                Err(e) => error!("dropping invalid data event packet: {}: {:02x}", e, frozen),
            }
        } else {
            error!("Packet type is not supported");
        }
    }
}

/// Send commands received from the NCI later to rootcanal
async fn dispatch_outgoing<W>(
    mut out_cmd_rx: UnboundedReceiver<NciPacket>,
    mut out_dta_rx: UnboundedReceiver<DataPacket>,
    mut writer: W,
) -> Result<()>
where
    W: AsyncWriteExt + Unpin,
{
    loop {
        select! {
            Some(cmd) = out_cmd_rx.recv() => write_nci(&mut writer, cmd).await?,
            Some(dta) = out_dta_rx.recv() => write_nci(&mut writer, dta).await?,
            else => break,
        }
    }

    Ok(())
}

async fn write_nci<W, P>(writer: &mut W, cmd: P) -> Result<()>
where
    W: AsyncWriteExt + Unpin,
    P: Packet,
{
    let b = cmd.to_bytes();
    let mut data = BytesMut::with_capacity(b.len() + 2);
    data.put_u16(b.len().try_into().unwrap());
    data.extend(b);
    writer.write_all(&data[..]).await?;
    debug!("Sent {:?}", data);
    Ok(())
}

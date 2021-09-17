//! This connects to "rootcanal" and provides a simulated
//! Nfc chip as well as a simulated environment.

use bytes::{BufMut, BytesMut};
use nfc_packets::nci;
use nfc_packets::nci::CommandChild::{InitCommand, ResetCommand};
use nfc_packets::nci::PacketBoundaryFlag;
use nfc_packets::nci::{
    CommandPacket, Packet, ResetResponseBuilder, ResponseBuilder, ResponsePacket,
};
use num_derive::{FromPrimitive, ToPrimitive};
use std::convert::TryInto;
use std::io::{Error, ErrorKind};
use tokio::io;
use tokio::io::{AsyncReadExt, AsyncWriteExt, BufReader};
use tokio::net::TcpListener;

/// Result type
type Result<T> = std::result::Result<T, Box<dyn std::error::Error + Send + Sync>>;

#[derive(FromPrimitive, ToPrimitive)]
enum NciPacketType {
    Data = 0x00,
    Command = 0x01,
    Response = 0x02,
    Notification = 0x03,
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:54323").await.unwrap();

    loop {
        let (mut sock, _) = listener.accept().await?;

        tokio::spawn(async move {
            let (mut rd, mut wr) = sock.split();
            process(&mut rd, &mut wr).await.unwrap();
        });
    }
}

async fn write_cmd<W>(writer: &mut W, cmd: ResponsePacket) -> Result<()>
where
    W: AsyncWriteExt + Unpin,
{
    let b = cmd.to_bytes();
    let mut data = BytesMut::with_capacity(b.len() + 3);
    data.put_u8(NciPacketType::Response as u8);
    data.put_u16(b.len().try_into().unwrap());
    data.extend(b);
    writer.write_all(&data[..]).await?;
    Ok(())
}

async fn command_response<W>(out: &mut W, cmd: CommandPacket) -> Result<()>
where
    W: AsyncWriteExt + Unpin,
{
    let packet_boundary_flag = PacketBoundaryFlag::CompleteOrFinal;
    match cmd.specialize() {
        ResetCommand(_) => {
            write_cmd(
                out,
                (ResetResponseBuilder { packet_boundary_flag, status: nci::Status::Ok })
                    .build()
                    .into(),
            )
            .await
        }
        InitCommand(_) => {
            write_cmd(
                out,
                (ResponseBuilder { packet_boundary_flag, op: cmd.get_op(), payload: None }).build(),
            )
            .await
        }
        _ => {
            log::error!("Cannot process cmd: {:?}", cmd);
            Err(Error::new(ErrorKind::Unsupported, "unsupported command").into())
        }
    }
}
/// Send NCI events received from the HAL to the NCI layer
async fn process<R, W>(reader: &mut R, writer: &mut W) -> Result<()>
where
    R: AsyncReadExt + Unpin,
    W: AsyncWriteExt + Unpin,
{
    let mut reader = BufReader::new(reader);
    let mut buffer = BytesMut::with_capacity(1024);
    let btype = reader.read_u8().await?;
    if btype == NciPacketType::Command as u8 {
        let len: usize = reader.read_u16().await?.into();
        buffer.resize(len, 0);
        reader.read_exact(&mut buffer).await?;
        let frozen = buffer.freeze();
        match CommandPacket::parse(&frozen) {
            Ok(p) => command_response(writer, p).await,
            Err(e) => {
                log::error!("dropping invalid command packet: {}: {:02x}", e, frozen);
                Err(Error::new(ErrorKind::InvalidInput, "invalid command packet").into())
            }
        }
    } else {
        log::error!("dropping invalid packet:{:02x}", buffer[0]);
        Err(Error::new(ErrorKind::Unsupported, "unsupported packet").into())
    }
}

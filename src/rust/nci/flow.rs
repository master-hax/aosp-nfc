//! NCI flow control modure
//! The module ensures flow control rules and timing imposed by NCIForum standard

use log::error;
use nfc_packets::nci::{NciMsgType, NciPacket};
use std::sync::Arc;
use tokio::select;
use tokio::sync::mpsc::{
    channel, unbounded_channel, Receiver, Sender, UnboundedReceiver, UnboundedSender,
};
use tokio::sync::Notify;
use tokio::time::{timeout, Duration};

/// Result type
type Result<T> = std::result::Result<T, Box<dyn std::error::Error + Send + Sync>>;

/// Initialize the module and connect the channels
pub async fn init() -> (Sender<NciPacket>, Receiver<NciPacket>) {
    let (out_tx, in_rx) = nfc_hal::init().await;
    let cmd_out_tx = out_tx.clone();
    let (cmd_tx, cmd_rx) = unbounded_channel();
    // Channel to handle downstream messages
    let (out_ext, out_int) = channel::<NciPacket>(10);
    // Channel to handle upstream messages
    let (in_int, in_ext) = channel::<NciPacket>(10);
    let cmd_resp = Arc::new(Notify::new());

    tokio::spawn(command_stream(cmd_out_tx, cmd_rx, cmd_resp.clone()));
    tokio::spawn(main_stream(out_int, out_tx, in_rx, in_int, cmd_tx, cmd_resp));
    (out_ext, in_ext)
}

async fn main_stream(
    mut out_int: Receiver<NciPacket>,
    out_tx: UnboundedSender<NciPacket>,
    mut in_rx: UnboundedReceiver<NciPacket>,
    in_int: Sender<NciPacket>,
    cmd_tx: UnboundedSender<NciPacket>,
    cmd_resp: Arc<Notify>,
) -> Result<()> {
    loop {
        select! {
            Some(cmd) = in_rx.recv() => {
                if cmd.get_mt() == NciMsgType::Response {
                    cmd_resp.notify_one();
                }
                in_int.send(cmd).await?;
            }
            Some(cmd) = out_int.recv() => {
                if cmd.get_mt() == NciMsgType::Command {
                    cmd_tx.send(cmd)?;
                } else {
                    out_tx.send(cmd)?;
                }
            }
            else => break,
        }
    }
    Ok(())
}

async fn command_stream(
    out_tx: UnboundedSender<NciPacket>,
    mut cmd_rx: UnboundedReceiver<NciPacket>,
    cmd_resp: Arc<Notify>,
) -> Result<()> {
    let to = Duration::from_millis(2 * 10);
    loop {
        if let Some(cmd) = cmd_rx.recv().await {
            out_tx.send(cmd)?;
        } else {
            break;
        }
        if let Err(e) = timeout(to, cmd_resp.notified()).await {
            error!("Command flow interrupted {:?}", e);
        }
    }
    Ok(())
}

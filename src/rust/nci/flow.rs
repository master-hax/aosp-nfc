//! NCI flow control modure
//! The module ensures flow control rules and timing imposed by NCIForum standard

use log::error;
use nfc_packets::nci::{NciMsgType, NciPacket};
use std::sync::Arc;
use tokio::select;
use tokio::sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender};
use tokio::sync::Notify;
use tokio::time::{timeout, Duration};

/// Result type
type Result<T> = std::result::Result<T, Box<dyn std::error::Error + Send + Sync>>;

/// Initialize the module and connect the channels
pub async fn init() -> (UnboundedSender<NciPacket>, UnboundedReceiver<NciPacket>) {
    let (out_tx, in_rx) = nfc_hal::init().await;
    let cmd_out_tx = out_tx.clone();
    let (cmd_tx, cmd_rx) = unbounded_channel();
    let (out_ext, out_int) = unbounded_channel();
    let (in_int, in_ext) = unbounded_channel();
    let rdy = Arc::new(Notify::new());

    tokio::spawn(command_flow(cmd_out_tx, cmd_rx, rdy.clone()));
    tokio::spawn(straight_flow(out_int, out_tx, in_rx, in_int, cmd_tx, rdy));
    (out_ext, in_ext)
}

async fn straight_flow(
    mut out_int: UnboundedReceiver<NciPacket>,
    out_tx: UnboundedSender<NciPacket>,
    mut in_rx: UnboundedReceiver<NciPacket>,
    in_int: UnboundedSender<NciPacket>,
    cmd_tx: UnboundedSender<NciPacket>,
    rdy: Arc<Notify>,
) -> Result<()> {
    loop {
        select! {
            Some(cmd) = in_rx.recv() => {
                if cmd.get_mt() == NciMsgType::Response {
                    rdy.notify_one();
                }
                in_int.send(cmd)?;
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

async fn command_flow(
    out_tx: UnboundedSender<NciPacket>,
    mut cmd_rx: UnboundedReceiver<NciPacket>,
    rdy: Arc<Notify>,
) -> Result<()> {
    let to = Duration::from_millis(2 * 10);
    loop {
        if let Some(cmd) = cmd_rx.recv().await {
            out_tx.send(cmd)?;
        } else {
            break;
        }
        if let Err(e) = timeout(to, rdy.notified()).await {
            error!("Command flow interrupted {:?}", e);
        }
    }
    Ok(())
}

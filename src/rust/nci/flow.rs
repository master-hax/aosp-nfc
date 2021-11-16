//! NCI flow control modure
//! The module ensures flow control rules and timing imposed by NCIForum standard

use log::error;
use nfc_hal::Hal;
use nfc_packets::nci::{DataPacket, NciMsgType, NciPacket};
use tokio::select;
use tokio::sync::mpsc::{channel, Receiver, Sender};
use tokio::time::{sleep, Duration, Instant};

/// Result type
type Result<T> = std::result::Result<T, Box<dyn std::error::Error + Send + Sync>>;

/// Initialize the module and connect the channels
pub async fn init(
) -> (Sender<NciPacket>, Receiver<NciPacket>, Sender<DataPacket>, Receiver<DataPacket>) {
    let hc = nfc_hal::init().await;
    // Channel to handle nci downstream messages
    let (out_cmd_ext, out_cmd_int) = channel::<NciPacket>(10);
    // Channel to handle data downstream messages
    let (out_dta_ext, out_dta_int) = channel::<DataPacket>(10);
    // Channel to handle nci upstream messages
    let (in_cmd_int, in_cmd_ext) = channel::<NciPacket>(10);
    // Channel to handle data upstream messages
    let (in_dta_int, in_dta_ext) = channel::<DataPacket>(10);

    let ic = InternalChannels { out_cmd_int, in_cmd_int, out_dta_int, in_dta_int };
    tokio::spawn(dispatch(hc, ic));
    (out_cmd_ext, in_cmd_ext, out_dta_ext, in_dta_ext)
}

struct InternalChannels {
    out_cmd_int: Receiver<NciPacket>,
    in_cmd_int: Sender<NciPacket>,
    out_dta_int: Receiver<DataPacket>,
    in_dta_int: Sender<DataPacket>,
}

async fn dispatch(mut hc: Hal, mut ic: InternalChannels) -> Result<()> {
    let mut has_pending_cmd = false;
    let time_out = sleep(Duration::MAX);
    let max_deadline = time_out.deadline();
    tokio::pin!(time_out);
    loop {
        select! {
            Some(cmd) = hc.in_cmd_rx.recv() => {
                if cmd.get_mt() == NciMsgType::Response {
                    has_pending_cmd = false;
                    time_out.as_mut().reset(max_deadline);
                }
                ic.in_cmd_int.send(cmd).await?;
            }
            Some(dta) = hc.in_dta_rx.recv() => ic.in_dta_int.send(dta).await?,
            Some(cmd) = ic.out_cmd_int.recv(), if !has_pending_cmd  => {
                if cmd.get_mt() == NciMsgType::Command {
                    has_pending_cmd = true;
                    time_out.as_mut().reset(Instant::now() + Duration::from_millis(20));
                }
                hc.out_cmd_tx.send(cmd)?;
            }
            Some(dta) = ic.out_dta_int.recv() => hc.out_dta_tx.send(dta)?,
            () = &mut time_out => {
                error!("Command processing timeout");
                time_out.as_mut().reset(max_deadline);
                has_pending_cmd = false;
            }
            else => break,
        }
    }
    Ok(())
}

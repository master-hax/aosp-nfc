//! Rootcanal HAL
//! This connects to "rootcanal" which provides a simulated
//! Nfc chip as well as a simulated environment.

use log::{debug, Level};
use logger::{self, Config};
use nfc_packets::nci::{FeatureEnable, PacketBoundaryFlag, ResetType};
use nfc_packets::nci::{InitCommandBuilder, ResetCommandBuilder};
use nfc_packets::nci::{NciChild, NciPacket};
use tokio::select;
use tokio::sync::mpsc::{Receiver, Sender};

/// Result type
type Result<T> = std::result::Result<T, Box<dyn std::error::Error + Send + Sync>>;

#[tokio::main]
async fn main() -> Result<()> {
    logger::init(Config::default().with_tag_on_device("lnfc").with_min_level(Level::Trace));
    let (out_tx, in_rx, _out_dta, _in_dta) = nfc_rnci::init().await;
    let task = tokio::spawn(command_response(in_rx));
    send_reset(&out_tx).await?;
    send_init(&out_tx).await?;
    task.await.unwrap();
    Ok(())
}

async fn command_response(mut in_rx: Receiver<NciPacket>) {
    loop {
        select! {
            Some(cmd) = in_rx.recv() => {
                match cmd.specialize() {
                    NciChild::Response(rsp) =>
                        debug!("{} - response received", rsp.get_cmd_op()),
                    NciChild::Notification(ntfy) =>
                        debug!("{} - notification received", ntfy.get_cmd_op()),
                    _ => debug!("Unexpected packet"),
                }
            }
            else => break,
        }
    }
}

async fn send_reset(out: &Sender<NciPacket>) -> Result<()> {
    let pbf = PacketBoundaryFlag::CompleteOrFinal;
    out.send(
        (ResetCommandBuilder { gid: 0, pbf, reset_type: ResetType::ResetConfig }).build().into(),
    )
    .await?;
    Ok(())
}
async fn send_init(out: &Sender<NciPacket>) -> Result<()> {
    let pbf = PacketBoundaryFlag::CompleteOrFinal;
    out.send(
        (InitCommandBuilder { gid: 0, pbf, feature_eneble: FeatureEnable::Rfu }).build().into(),
    )
    .await?;
    Ok(())
}

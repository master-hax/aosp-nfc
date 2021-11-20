//! Rootcanal HAL
//! This connects to "rootcanal" which provides a simulated
//! Nfc chip as well as a simulated environment.

use log::{debug, error, Level};
use logger::{self, Config};
use nfc_packets::nci::CommandPacket;
use nfc_packets::nci::Opcode::{self, CoreInit, CoreReset};
use nfc_packets::nci::{FeatureEnable, PacketBoundaryFlag, ResetType};
use nfc_packets::nci::{InitCommandBuilder, ResetCommandBuilder};

/// Result type
type Result<T> = std::result::Result<T, Box<dyn std::error::Error + Send + Sync>>;

#[tokio::main]
async fn main() -> Result<()> {
    logger::init(Config::default().with_tag_on_device("lnfc").with_min_level(Level::Trace));
    let mut nci = nfc_rnci::init().await;
    let (rrsp, ntfyc) = nci.commands.send_with_notification(build_cmd(CoreReset).unwrap()).await?;
    let irsp = nci.commands.send(build_cmd(CoreInit).unwrap()).await?;
    let rrp = rrsp.specialize();
    debug!("Received {:?}", rrp);
    let irp = irsp.specialize();
    debug!("Received {:?}", irp);
    let np = ntfyc.await?;
    debug!("Received {:?}", np.specialize());
    Ok(())
}

fn build_cmd(cmd_op_code: Opcode) -> Option<CommandPacket> {
    let pbf = PacketBoundaryFlag::CompleteOrFinal;
    match cmd_op_code {
        CoreReset => Some(
            ResetCommandBuilder { gid: 0, pbf, reset_type: ResetType::ResetConfig }.build().into(),
        ),
        CoreInit => Some(
            InitCommandBuilder { gid: 0, pbf, feature_eneble: FeatureEnable::Rfu }.build().into(),
        ),
        _ => {
            error!("Unsupported command: {}", cmd_op_code);
            None
        }
    }
}

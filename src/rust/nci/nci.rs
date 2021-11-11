//! NCI Protocol Abstraction Layer
//! Supports sending NCI commands to the HAL and receiving
//! NCI messages back

use nfc_packets::nci::NciPacket;
use tokio::sync::mpsc::{Receiver, Sender};

pub mod flow;

/// Initialize the modules
pub async fn init() -> (Sender<NciPacket>, Receiver<NciPacket>) {
    let ch = flow::init().await;
    ch
}

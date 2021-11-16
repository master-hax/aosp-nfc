//! NCI Hardware Abstraction Layer
//! Supports sending NCI commands to the HAL and receiving
//! NCI events from the HAL

use nfc_packets::nci::{DataPacket, NciPacket};
use thiserror::Error;
use tokio::sync::mpsc::{UnboundedReceiver, UnboundedSender};

#[cfg(target_os = "android")]
#[path = "hidl_hal.rs"]
pub mod ihal;

#[cfg(not(target_os = "android"))]
#[path = "rootcanal_hal.rs"]
pub mod ihal;

/// Initialize the module and connect the channels
pub async fn init() -> (
    UnboundedSender<NciPacket>,
    UnboundedReceiver<NciPacket>,
    UnboundedSender<DataPacket>,
    UnboundedReceiver<DataPacket>,
) {
    let ch = ihal::init().await;
    (ch.out_cmd_tx, ch.in_cmd_rx, ch.out_dta_tx, ch.in_dta_rx)
}

mod internal {
    use nfc_packets::nci::{DataPacket, NciPacket};
    use tokio::sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender};

    pub struct RawHal {
        pub out_cmd_tx: UnboundedSender<NciPacket>,
        pub in_cmd_rx: UnboundedReceiver<NciPacket>,
        pub out_dta_tx: UnboundedSender<DataPacket>,
        pub in_dta_rx: UnboundedReceiver<DataPacket>,
    }

    pub struct InnerHal {
        pub out_cmd_rx: UnboundedReceiver<NciPacket>,
        pub in_cmd_tx: UnboundedSender<NciPacket>,
        pub out_dta_rx: UnboundedReceiver<DataPacket>,
        pub in_dta_tx: UnboundedSender<DataPacket>,
    }

    impl InnerHal {
        pub fn new() -> (RawHal, Self) {
            let (out_cmd_tx, out_cmd_rx) = unbounded_channel();
            let (in_cmd_tx, in_cmd_rx) = unbounded_channel();
            let (out_dta_tx, out_dta_rx) = unbounded_channel();
            let (in_dta_tx, in_dta_rx) = unbounded_channel();
            (
                RawHal { out_cmd_tx, in_cmd_rx, out_dta_tx, in_dta_rx },
                Self { out_cmd_rx, in_cmd_tx, out_dta_rx, in_dta_tx },
            )
        }
    }
}

/// Result type
type Result<T> = std::result::Result<T, Box<dyn std::error::Error + Send + Sync>>;

/// Errors that can be encountered while dealing with the HAL
#[derive(Error, Debug)]
pub enum HalError {
    /// Invalid rootcanal host error
    #[error("Invalid rootcanal host")]
    InvalidAddressError,
    /// Error while connecting to rootcanal
    #[error("Connection to rootcanal failed: {0}")]
    RootcanalConnectError(#[from] tokio::io::Error),
}

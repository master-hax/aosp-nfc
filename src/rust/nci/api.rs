// Copyright 2021, The Android Open Source Project
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

//! NCI API module

use crate::{CommandSender, Result};
use lazy_static::lazy_static;
use log::debug;
use nfc_hal::{HalEvent, HalEventRegistry, HalEventStatus};
use nfc_packets::nci::{FeatureEnable, PacketBoundaryFlag, ResetType};
use nfc_packets::nci::{InitCommandBuilder, ResetCommandBuilder};
use tokio::sync::{oneshot, Mutex};

lazy_static! {
    /// Command Sender external interface
    pub static ref COMMANDS: Mutex<Option<CommandSender>> = Mutex::new(None);
    /// The NFC response callback
    pub static ref CALLBACK: Mutex<Option<fn(u16, &[u8])>> = Mutex::new(None);
    /// HalEventRegistry is used to register for HAL events
    pub static ref HAL_EVENTS: Mutex<Option<HalEventRegistry>> = Mutex::new(None);
}

/** ****************************************************************************
**
** Function         nfc_enable
**
** Description      This function enables NFC. Prior to calling NFC_Enable:
**                  - the NFCC must be powered up, and ready to receive
**                    commands.
**
**                  This function opens the NCI transport (if applicable),
**                  resets the NFC controller, and initializes the NFC
**                  subsystems.
**
**                  When the NFC startup procedure is completed, an
**                  NFC_ENABLE_REVT is returned to the application using the
**                  tNFC_RESPONSE_CBACK.
**
** Returns          tNFC_STATUS
**
*******************************************************************************/
/// extern tNFC_STATUS NFC_Enable(tNFC_RESPONSE_CBACK* p_cback);
pub async fn nfc_enable(callback: fn(u16, &[u8])) {
    let nci = crate::init().await;

    *COMMANDS.lock().await = Some(nci.commands);
    *CALLBACK.lock().await = Some(callback);
    *HAL_EVENTS.lock().await = Some(nci.hal_events);
}
/** ****************************************************************************
**
** Function         NFC_Disable
**
** Description      This function performs clean up routines for shutting down
**                  NFC and closes the NCI transport (if using dedicated NCI
**                  transport).
**
**                  When the NFC shutdown procedure is completed, an
**                  NFC_DISABLED_REVT is returned to the application using the
**                  tNFC_RESPONSE_CBACK.
**
** Returns          nothing
**
*******************************************************************************/
// extern void NFC_Disable(void);
pub async fn nfc_disable() {
    let (tx, rx) = oneshot::channel::<HalEventStatus>();
    if let Some(mut her) = HAL_EVENTS.lock().await.take() {
        her.register(HalEvent::CloseComplete, tx).await;

        if let Some(cmd) = COMMANDS.lock().await.take() {
            drop(cmd);
        }
        let status = rx.await.unwrap();
        debug!("Shutdown complete {:?}.", status);

        if let Some(cb) = CALLBACK.lock().await.take() {
            cb(1, &[]);
        }
    }
}

/** ****************************************************************************
**
** Function         NFC_Init
**
** Description      This function initializes control blocks for NFC
**
** Returns          nothing
**
*******************************************************************************/
/// extern void NFC_Init(tHAL_NFC_ENTRY* p_hal_entry_tbl);
pub async fn nfc_init() -> Result<()> {
    let pbf = PacketBoundaryFlag::CompleteOrFinal;
    if let Some(cmd) = COMMANDS.lock().await.as_mut() {
        let reset = cmd
            .send_and_notify(
                ResetCommandBuilder { gid: 0, pbf, reset_type: ResetType::ResetConfig }
                    .build()
                    .into(),
            )
            .await?;
        let _notification_packet = reset.notification.await?;
        let _init = cmd
            .send(
                InitCommandBuilder { gid: 0, pbf, feature_enable: FeatureEnable::Rfu }
                    .build()
                    .into(),
            )
            .await?;
    }
    Ok(())
}

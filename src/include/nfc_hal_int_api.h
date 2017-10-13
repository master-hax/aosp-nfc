/******************************************************************************
 *
 *  Copyright (C) 2009-2014 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Internal NFC HAL API functions.
 *
 ******************************************************************************/
#ifndef NFC_HAL_INT_API_H
#define NFC_HAL_INT_API_H

/****************************************************************************
** Device Configuration definitions
****************************************************************************/

#define NFC_HAL_PLL_325_SETCONFIG_PARAM_LEN (2 + NCI_PARAM_LEN_PLL325_CFG_PARAM)

/* Broadcom specific device initialization before sending NCI reset */

typedef struct {
  uint32_t brcm_hw_id;
  uint16_t xtal_freq;
  uint8_t xtal_index;
} tNFC_HAL_DEV_INIT_XTAL_CFG;

#define NFC_HAL_DEV_INIT_MAX_XTAL_CFG 5

/*****************************************************************************
**  Low Power Mode definitions
*****************************************************************************/

/* Snooze mode disabled    */
#define NFC_HAL_LP_SNOOZE_MODE_NONE NFC_SNOOZE_MODE_NONE
/* Snooze mode for UART    */
#define NFC_HAL_LP_SNOOZE_MODE_UART NFC_SNOOZE_MODE_UART
/* Snooze mode for SPI/I2C */
#define NFC_HAL_LP_SNOOZE_MODE_SPI_I2C NFC_SNOOZE_MODE_SPI_I2C

/* high to low voltage is asserting */
#define NFC_HAL_LP_ACTIVE_LOW NFC_SNOOZE_ACTIVE_LOW
/* low to high voltage is asserting */
#define NFC_HAL_LP_ACTIVE_HIGH NFC_SNOOZE_ACTIVE_HIGH

/*****************************************************************************
**  Patch RAM Constants
*****************************************************************************/

/* patch format type */
#define NFC_HAL_PRM_FORMAT_BIN 0x00
#define NFC_HAL_PRM_FORMAT_HCD 0x01
#define NFC_HAL_PRM_FORMAT_NCD 0x02
typedef uint8_t tNFC_HAL_PRM_FORMAT;

/*****************************************************************************
**  Patch RAM Callback for event notificaton
*****************************************************************************/

typedef void(tNFC_HAL_PRM_CBACK)(uint8_t event);

typedef uint8_t tNFC_HAL_NCI_EVT; /* MT + Opcode */

/*******************************************************************************
**
** Function         HAL_NfcPrmSetSpdNciCmdPayloadSize
**
** Description      Set Host-to-NFCC NCI message size for secure patch download
**
**                  This API must be called before calling
**                  HAL_NfcPrmDownloadStart. If the API is not called, then PRM
**                  will use the default message size.
**
**                  Typically, this API is only called for platforms that have
**                  message-size limitations in the transport/driver.
**
**                  Valid message size range:
**                  NFC_HAL_PRM_MIN_NCI_CMD_PAYLOAD_SIZE to 255.
**
** Returns          HAL_NFC_STATUS_OK if successful
**                  HAL_NFC_STATUS_FAILED otherwise
**
**
*******************************************************************************/
tHAL_NFC_STATUS HAL_NfcPrmSetSpdNciCmdPayloadSize(uint8_t max_payload_size);

#endif /* NFC_HAL_INT_API_H */

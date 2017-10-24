/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
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
#ifndef __CONFIG_H
#define __CONFIG_H

int GetStrValue(const char* name, char* p_value, unsigned long len);
int GetNumValue(const char* name, void* p_value, unsigned long len);

#define NAME_POLLING_TECH_MASK "POLLING_TECH_MASK"
#define NAME_APPL_TRACE_LEVEL "APPL_TRACE_LEVEL"
#define NAME_USE_RAW_NCI_TRACE "USE_RAW_NCI_TRACE"
#define NAME_PROTOCOL_TRACE_LEVEL "PROTOCOL_TRACE_LEVEL"
#define NAME_NFA_DM_CFG "NFA_DM_CFG"
#define NAME_SCREEN_OFF_POWER_STATE "SCREEN_OFF_POWER_STATE"
#define NAME_NFA_STORAGE "NFA_STORAGE"
#define NAME_UICC_LISTEN_TECH_MASK "UICC_LISTEN_TECH_MASK"
#define NAME_NFA_DM_DISC_DURATION_POLL "NFA_DM_DISC_DURATION_POLL"
#define NAME_AID_FOR_EMPTY_SELECT "AID_FOR_EMPTY_SELECT"
#define NAME_PRESERVE_STORAGE "PRESERVE_STORAGE"
#define NAME_NFA_MAX_EE_SUPPORTED "NFA_MAX_EE_SUPPORTED"
#define NAME_POLL_FREQUENCY "POLL_FREQUENCY"
#define NAME_PRESENCE_CHECK_ALGORITHM "PRESENCE_CHECK_ALGORITHM"
#define NAME_DEVICE_HOST_WHITE_LIST "DEVICE_HOST_WHITE_LIST"
#define NAME_NFA_POLL_BAIL_OUT_MODE "NFA_POLL_BAIL_OUT_MODE"
#define NAME_NFA_PROPRIETARY_CFG "NFA_PROPRIETARY_CFG"
#define NAME_NFA_AID_BLOCK_ROUTE "NFA_AID_BLOCK_ROUTE"
#define NAME_ISO_DEP_MAX_TRANSCEIVE "ISO_DEP_MAX_TRANSCEIVE"

struct tUART_CONFIG {
  int m_iBaudrate;  // 115200
  int m_iDatabits;  // 8
  int m_iParity;    // 0 - none, 1 = odd, 2 = even
  int m_iStopbits;
};

extern struct tUART_CONFIG uartConfig;

void readOptionalConfig(const char* option);

/* Snooze mode configuration structure */
typedef struct {
  unsigned char snooze_mode;         /* Snooze Mode */
  unsigned char idle_threshold_dh;   /* Idle Threshold Host */
  unsigned char idle_threshold_nfcc; /* Idle Threshold NFCC   */
  unsigned char
      nfc_wake_active_mode; /* NFC_LP_ACTIVE_LOW or NFC_LP_ACTIVE_HIGH */
  unsigned char
      dh_wake_active_mode; /* NFC_LP_ACTIVE_LOW or NFC_LP_ACTIVE_HIGH */
} tSNOOZE_MODE_CONFIG;
#endif

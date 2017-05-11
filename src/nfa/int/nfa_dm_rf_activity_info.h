/*
 * Copyright (C) 2017 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 *
 *  This is the private interface file for the NFA device manager.
 *
 ******************************************************************************/
#ifndef NFA_DM_RF_ACTIVITY_INFO_H
#define NFA_DM_RF_ACTIVITY_INFO_H

#include <string.h>
#include <sys/time.h>
#include "nfa_dm_int.h"
#include "nfa_sys.h"
#include "nfa_sys_int.h"
#include "nfc_api.h"

#define NFA_DM_RF_QOS_ACTIVITY_POLL_START 0xAA
#define NFA_DM_RF_QOS_ACTIVITY_POLL_STOP 0x00
#define NFA_DM_RF_QOS_LISTEN_PATTERN_TIMEOUT 10000
#define NFA_DM_RF_QOS_EVENT_TIMEOUT 500
#define NFA_DM_RF_QOS_ERROR_DURATION 5000
#define NFA_DM_RF_QOS_LISTEN_MODE 0x80
#define NFA_DM_RF_QOS_POLL_MODE 0x00
/*QOS Listen functions*/
extern void nfa_dm_rf_qos_listen_capture_onevent(void);
extern void nfa_dm_rf_qos_listen_capture_offevent(void);
extern void nfa_dm_rf_qos_activation_info(tNFC_RF_TECH_N_MODE tech_n_mode,
                                          tNFC_PROTOCOL protocol,
                                          tNFC_INTF_TYPE interface);
void nfa_dm_rf_qos_clear_listen_data();
/*QOS Poll functions*/
extern void nfa_dm_rf_qos_record_poll_error();
void nfa_dm_rf_qos_clear_poll_data();
void nfa_dm_rf_qos_poll_activation_time();

#endif /* NFA_DM_RF_ACTIVITY_INFO_H */

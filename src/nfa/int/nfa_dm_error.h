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
#ifndef NFA_DM_ERROR_H
#define NFA_DM_ERROR_H

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "nfa_api.h"
#include "nfa_dm_int.h"
#include "nfa_sys_int.h"
#include "nfc_int.h"

#define NFA_DM_FALSE_DETECTION_ERR_CODE 0x01
#define NFA_DM_ACTIVATION_FAILED_ERR_CODE 0x02
#define NFA_DM_PROTOCOL_ERR_CODE 0x03
#define NFA_DM_TIME_OUT_ERR_CODE 0x04
#define NFA_DM_TRANSMISSION_ERR_CODE 0x05
#define NFA_DM_FIELD_OFF_ERR_CODE 0x06
#define NFA_NO_ACTIVATION_CODE 0x07

#define NFA_STATUS_FALSE_DETECTION 0xA3
#define NFA_DM_FIELD_OFF_ERROR 0xFF /* TO DO */

#endif /* NFA_DM_ERROR_H */

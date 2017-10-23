/******************************************************************************
 *
 *  Copyright (C) 2012 Broadcom Corporation
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

#include <cutils/properties.h>
#include <string.h>
#include "_OverrideLog.h"
#include "android_logmsg.h"
#include "config.h"

/*******************************************************************************
**
** Function:        initializeGlobalAppLogLevel
**
** Description:     Initialize and get global logging level from .conf or
**                  Android property nfc.app_log_level.  The Android property
**                  overrides .conf variable.
**
** Returns:         Global log level:
**                  0 * No trace messages to be generated
**                  1 * Debug messages
**
*******************************************************************************/
unsigned char initializeGlobalAppLogLevel() {
  unsigned long num = 0;
  char valueStr[PROPERTY_VALUE_MAX] = {0};

  num = 1;
  if (GetNumValue(NAME_APPL_TRACE_LEVEL, &num, sizeof(num)))
    nfc_debug_enabled = (num == 0) ? false : true;

  int len = property_get("nfc.app_log_level", valueStr, "");
  if (len > 0) {
    // let Android property override .conf variable
    sscanf(valueStr, "%lu", &num);
    nfc_debug_enabled = (num == 0) ? false : true;
  }

  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("%s: level=%u", __func__, nfc_debug_enabled);

  return nfc_debug_enabled;
}

uint32_t initializeProtocolLogLevel() {
  uint32_t num = 0;
  char valueStr[PROPERTY_VALUE_MAX] = {0};

  if (GetNumValue(NAME_PROTOCOL_TRACE_LEVEL, &num, sizeof(num)))
    ScrProtocolTraceFlag = num;

  int len = property_get("nfc.enable_protocol_log", valueStr, "");
  if (len > 0) {
    if (strncmp("0", valueStr, 1) == 0) {
      ScrProtocolTraceFlag = 0;
    } else {
      ScrProtocolTraceFlag = ~0;
    }
  }

  return ScrProtocolTraceFlag;
}

void initializeGlobalAppDtaMode() {
  appl_dta_mode_flag = 0x01;
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: DTA Enabled", __func__);
}

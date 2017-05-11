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
 *  This file contains the action functions for device manager error scenarios
 *
 ******************************************************************************/

#include "nfa_dm_error.h"

/*******************************************************************************
**
** Function         nfa_dm_rf_qos_check_error_code
**
** Description      function to detect the NFCC error code
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_rf_qos_check_error_code(uint8_t* p_data) {
  NFA_TRACE_DEBUG0("nfa_dm_rf_qos_check_error_code ()");
  uint8_t rf_error_code;
  uint8_t rf_error_info;
  uint32_t millisec;
  struct timezone tz;
  struct tm* tm;
  tNFC_STATUS status = NFC_STATUS_OK;

  nfa_dm_cb.disc_cb.discovery_error_info.rf_error_code = *p_data;

  NFA_TRACE_DEBUG1("nfa_dm_rf_qos_check_error_code () Error Code: 0x%X",
                   nfa_dm_cb.disc_cb.discovery_error_info.rf_error_code);

  if ((nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_DISCOVERY) ||
      (nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_W4_HOST_SELECT) ||
      (nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_POLL_ACTIVE) ||
      (nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_LISTEN_ACTIVE)) {
    gettimeofday(&nfa_dm_cb.disc_cb.discovery_error_info.rf_error_tv, &tz);

    millisec =
        (nfa_dm_cb.disc_cb.discovery_error_info.rf_error_tv.tv_usec / 1000.0);
    if (millisec >= 1000) {
      millisec -= 1000;
      nfa_dm_cb.disc_cb.discovery_error_info.rf_error_tv.tv_sec++;
    }

    tm = localtime(&nfa_dm_cb.disc_cb.discovery_error_info.rf_error_tv.tv_sec);

    NFA_TRACE_DEBUG6("RF Error logged at : %02d:%02d:%02d:%02d:%02d:%03d  ",
                     (tm->tm_mon) + 1, (tm->tm_mday), tm->tm_hour, tm->tm_min,
                     tm->tm_sec, millisec);

    switch (nfa_dm_cb.disc_cb.discovery_error_info.rf_error_code) {
      case NFA_STATUS_FALSE_DETECTION:
        nfa_dm_cb.disc_cb.discovery_error_info.rf_error_info
            .rf_error_reason_code = NFA_DM_FALSE_DETECTION_ERR_CODE;
        break;
      case NFA_STATUS_ACTIVATION_FAILED:
        nfa_dm_cb.disc_cb.discovery_error_info.rf_error_info
            .rf_error_reason_code = NFA_DM_ACTIVATION_FAILED_ERR_CODE;
        nfa_dm_rf_qos_record_poll_error();
        break;
      case NFA_STATUS_RF_PROTOCOL_ERR:
        nfa_dm_cb.disc_cb.discovery_error_info.rf_error_info
            .rf_error_reason_code = NFA_DM_PROTOCOL_ERR_CODE;
        break;
      case NFA_STATUS_TIMEOUT:
        nfa_dm_cb.disc_cb.discovery_error_info.rf_error_info
            .rf_error_reason_code = NFA_DM_TIME_OUT_ERR_CODE;
        break;
      case NFA_STATUS_RF_TRANSMISSION_ERR:
        nfa_dm_cb.disc_cb.discovery_error_info.rf_error_info
            .rf_error_reason_code = NFA_DM_TRANSMISSION_ERR_CODE;
        break;
      case NFA_DM_FIELD_OFF_ERROR:
        nfa_dm_cb.disc_cb.discovery_error_info.rf_error_info
            .rf_error_reason_code = NFA_DM_FIELD_OFF_ERR_CODE;
        break;
      default:
        status = NFC_STATUS_FAILED;
        break;
    }
    // if(status == NFC_STATUS_OK)
    // Todo report the errors
  }
}

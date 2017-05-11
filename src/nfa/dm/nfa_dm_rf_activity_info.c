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
 *  This file contains the action functions for device manager RF Activity Info
 *
 ******************************************************************************/

#include "nfa_dm_rf_activity_info.h"
#include "nfa_dm_error.h"

/*******************************************************************************
**
** Function         nfa_dm_rf_qos_activation_info
**
** Description      function to record the Activation info for Poll / Listen
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_rf_qos_activation_info(tNFC_RF_TECH_N_MODE tech_n_mode,
                                   tNFC_PROTOCOL protocol,
                                   tNFC_INTF_TYPE interface) {
  NFA_TRACE_DEBUG3(
      "nfa_dm_rf_qos_activation_info (): tech_n_mode:0x%X,"
      "protocol:0x%X, interface:0x%X",
      tech_n_mode, protocol, interface);

  nfa_dm_cb.disc_cb.activation_info.rf_interface = interface;
  nfa_dm_cb.disc_cb.activation_info.rf_protocol = protocol;
  nfa_dm_cb.disc_cb.activation_info.rf_tech_mode = tech_n_mode;
  nfa_dm_cb.disc_cb.activation_info.rf_activation_type =
      (tech_n_mode & NFA_DM_RF_QOS_LISTEN_MODE) ? NFA_DM_RF_QOS_LISTEN_MODE
                                                : NFA_DM_RF_QOS_POLL_MODE;
  nfa_dm_cb.disc_cb.activation_info.rf_listen_active = true;
  if (nfa_dm_cb.disc_cb.activation_info.rf_activation_type ==
      NFA_DM_RF_QOS_LISTEN_MODE) {
    if (nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval >
        NFA_DM_RF_QOS_ERROR_DURATION) {
      nfa_dm_cb.disc_cb.discovery_error_info.rf_error_code =
          NFA_NO_ACTIVATION_CODE;
      nfa_dm_cb.disc_cb.discovery_error_info.rf_error_info
          .rf_on_off_burst_period =
          nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval;
      NFA_TRACE_DEBUG2(
          "nfa_dm_rf_qos_activation_info(): listen time"
          "in msecs : %ld, RF Error code: %x",
          nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval,
          nfa_dm_cb.disc_cb.discovery_error_info.rf_error_code);
      // Todo report the errors
    } else {
      NFA_TRACE_DEBUG0(
          "nfa_dm_rf_qos_activation_info ():Activated in"
          "Listen Mode within time frame!!!");
      // Todo report activation
    }
    /* Clear the QOS listen data after activation */
    nfa_dm_rf_qos_clear_listen_data();
  } else if (nfa_dm_cb.disc_cb.activation_info.rf_activation_type ==
             NFA_DM_RF_QOS_POLL_MODE) {
    nfa_dm_rf_qos_poll_activation_time();
    if (nfa_dm_cb.disc_cb.activation_info.rf_poll_interval >
        NFA_DM_RF_QOS_ERROR_DURATION) {
      nfa_dm_cb.disc_cb.discovery_error_info.rf_error_code =
          NFA_NO_ACTIVATION_CODE;
      nfa_dm_cb.disc_cb.discovery_error_info.rf_error_info
          .rf_on_off_burst_period =
          nfa_dm_cb.disc_cb.activation_info.rf_poll_interval;
      NFA_TRACE_DEBUG2(
          "nfa_dm_rf_qos_activation_info (): polltime in"
          "msecs : %ld, RF Error code: %x",
          nfa_dm_cb.disc_cb.activation_info.rf_poll_interval,
          nfa_dm_cb.disc_cb.discovery_error_info.rf_error_code);
      // Todo report the errors
    } else {
      NFA_TRACE_DEBUG0(
          "nfa_dm_rf_qos_activation_info ():Activated in"
          "Poll Mode within time frame!!!");
      // Todo report activation
    }
    nfa_dm_rf_qos_clear_poll_data();
  }
}
/*******************************************************************************
**
** Function         nfa_dm_rf_qos_poll_activation_time
**
** Description      function to calculates time taken for activation
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_rf_qos_poll_activation_time() {
  long secs_recorded;
  long usecs_recorded;
  nfa_dm_cb.disc_cb.activation_info.rf_poll_interval = 0;
  if (nfa_dm_cb.disc_cb.activation_info.rf_poll_record_flag ==
      NFA_DM_RF_QOS_ACTIVITY_POLL_START) {
    NFA_TRACE_DEBUG0("nfa_dm_rf_qos_poll_activation_time ()");
    nfa_dm_cb.disc_cb.activation_info.rf_poll_record_flag =
        NFA_DM_RF_QOS_ACTIVITY_POLL_STOP;
    gettimeofday(&nfa_dm_cb.disc_cb.activation_info.endRFPollActivity, NULL);
    secs_recorded =
        (nfa_dm_cb.disc_cb.activation_info.endRFPollActivity.tv_sec -
         nfa_dm_cb.disc_cb.activation_info.startRFPollActivity.tv_sec);
    usecs_recorded =
        (nfa_dm_cb.disc_cb.activation_info.endRFPollActivity.tv_usec -
         nfa_dm_cb.disc_cb.activation_info.startRFPollActivity.tv_usec);
    nfa_dm_cb.disc_cb.activation_info.rf_poll_interval =
        ((secs_recorded)*1000 + usecs_recorded / 1000.0);

    NFA_TRACE_DEBUG1("Activation time in  msecs : %ld",
                     nfa_dm_cb.disc_cb.activation_info.rf_poll_interval);
  }
}
/*******************************************************************************
**
** Function         nfa_dm_rf_qos_record_poll_error
**
** Description      function to record time stamp error during poll
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_rf_qos_record_poll_error() {
  NFA_TRACE_DEBUG0("nfa_dm_rf_qos_record_poll_error ()");
  nfa_dm_cb.disc_cb.activation_info.rf_poll_record_flag =
      NFA_DM_RF_QOS_ACTIVITY_POLL_START;
  gettimeofday(&nfa_dm_cb.disc_cb.activation_info.startRFPollActivity, NULL);
}
/*******************************************************************************
**
** Function         nfa_dm_rf_qos_clear_poll_data
**
** Description      function to clear the QOS Poll data
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_rf_qos_clear_poll_data() {
  NFA_TRACE_DEBUG0("nfa_dm_rf_qos_clear_poll_data() - Enter ");
  nfa_dm_cb.disc_cb.activation_info.rf_poll_record_flag =
      NFA_DM_RF_QOS_ACTIVITY_POLL_STOP;
  nfa_dm_cb.disc_cb.activation_info.rf_poll_interval = 0;
}
/*******************************************************************************
**
** Function         nfa_dm_rf_qos_clear_listen_data
**
** Description      function to clear the QOS Listen data
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_rf_qos_clear_listen_data() {
  NFA_TRACE_DEBUG0("nfa_dm_rf_qos_clear_listen_data() - Enter ");
  if (nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle.in_use)
    nfa_sys_stop_timer(&nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle);

  if (nfa_dm_cb.disc_cb.activation_info.qos_tle.in_use)
    nfa_sys_stop_timer(&nfa_dm_cb.disc_cb.activation_info.qos_tle);

  nfa_dm_cb.disc_cb.activation_info.start_listen_timerflag = 0;
  nfa_dm_cb.disc_cb.activation_info.rf_event_timerflag = 0;
  nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_max_interval = 0;
  nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval = 0;
}

/*******************************************************************************
**
** Function         nfa_dm_rf_qos_listen_capture_onevent
**
** Description      Captures RF ON event in listen mode
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_rf_qos_listen_capture_onevent(void) {
  NFA_TRACE_DEBUG1("nfa_dm_rf_qos_listen_capture_onevent : %x start flag Enter",
                   nfa_dm_cb.disc_cb.activation_info.start_listen_timerflag);
  if (nfa_dm_cb.disc_cb.activation_info.start_listen_timerflag == 0) {
    nfa_dm_cb.disc_cb.activation_info.start_listen_timerflag = 1;
    nfa_dm_cb.disc_cb.activation_info.rf_event_timerflag = 0;
    NFA_TRACE_DEBUG0("nfa_dm_rf_qos_listen_capture_onevent -ON Event");
    nfa_sys_start_timer(&nfa_dm_cb.disc_cb.activation_info.qos_tle,
                        NFA_DM_LISTEN_DISC_QOS_TIMEOUT_EVT,
                        NFA_DM_RF_QOS_LISTEN_PATTERN_TIMEOUT);
  }
  if (nfa_dm_cb.disc_cb.activation_info.rf_event_timerflag == 0) {
    gettimeofday(&nfa_dm_cb.disc_cb.activation_info.startRFlistenTime, NULL);
    nfa_dm_cb.disc_cb.activation_info.rf_event_timerflag = 1;
  }
  if (nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle.in_use)
    nfa_sys_stop_timer(&nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle);

  nfa_sys_start_timer(&nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle,
                      NFA_DM_RF_EVENT_QOS_TIMEOUT_EVT,
                      NFA_DM_RF_QOS_EVENT_TIMEOUT);
}
/*******************************************************************************
**
** Function         nfa_dm_rf_qos_listen_capture_offevent
**
** Description      Captures RF OFF event in listen mode
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_rf_qos_listen_capture_offevent(void) {
  NFA_TRACE_DEBUG0("nfa_dm_rf_qos_listen_capture_offevent -Enter");

  if (nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle.in_use)
    nfa_sys_stop_timer(&nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle);

  nfa_sys_start_timer(&nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle,
                      NFA_DM_RF_EVENT_QOS_TIMEOUT_EVT,
                      NFA_DM_RF_QOS_EVENT_TIMEOUT);
}
/*******************************************************************************
**
** Function         nfa_dm_rf_qos_listen_event_timeout
**
** Description      records time stamp on timeout of RF ON/OFF events
**
** Returns          TRUE
**
*******************************************************************************/
bool nfa_dm_rf_qos_listen_event_timeout(tNFA_DM_MSG* p_data) {
  long secs_recorded;
  long usecs_recorded;
  long timecal;
  NFA_TRACE_DEBUG0("nfa_dm_rf_qos_listen_event_timeout -Enter");

  if (nfa_dm_cb.disc_cb.activation_info.rf_event_timerflag == 1) {
    nfa_dm_cb.disc_cb.activation_info.rf_event_timerflag = 0;
    nfa_sys_stop_timer(&nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle);

    gettimeofday(&nfa_dm_cb.disc_cb.activation_info.currentRFlistenTime, NULL);
    secs_recorded =
        (nfa_dm_cb.disc_cb.activation_info.currentRFlistenTime.tv_sec -
         nfa_dm_cb.disc_cb.activation_info.startRFlistenTime.tv_sec);
    usecs_recorded =
        (nfa_dm_cb.disc_cb.activation_info.currentRFlistenTime.tv_usec -
         nfa_dm_cb.disc_cb.activation_info.startRFlistenTime.tv_usec);
    timecal = ((secs_recorded)*1000 + usecs_recorded / 1000.0);

    NFA_TRACE_DEBUG1("timecal on timeout  : %ld", timecal);

    NFA_TRACE_DEBUG1(
        "rf_listen_inactive_interval in  msecs before cumalative : %ld",
        nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_max_interval);

    nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval =
        nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval +
        timecal;

    if (nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_max_interval <=
        (timecal)) {
      nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_max_interval =
          (timecal);
      NFA_TRACE_DEBUG1(
          "rf_listen_inactive_max_interval in  msecs : %ld",
          nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_max_interval);
    }

    NFA_TRACE_DEBUG1(
        "rf_listen_inactive_total_interval in  msecs After cumalative : %ld",
        nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval);
  }

  return 0;
}
/*******************************************************************************
**
** Function         nfa_dm_rf_qos_listen_interval_timeout
**
** Description      gets the time stamp on listen Mode pattern check timer
*expires
**
** Returns          TRUE (message buffer to be freed by caller)
**
*******************************************************************************/
bool nfa_dm_rf_qos_listen_interval_timeout(tNFA_DM_MSG* p_data) {
  NFA_TRACE_DEBUG0("nfa_dm_rf_qos_listen_interval_timeout -Enter");
  nfa_sys_stop_timer(&nfa_dm_cb.disc_cb.activation_info.qos_tle);
  nfa_dm_cb.disc_cb.activation_info.start_listen_timerflag = 0;

  if (nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle.in_use)
    nfa_sys_stop_timer(&nfa_dm_cb.disc_cb.activation_info.qos_rf_event_tle);

  if ((nfa_dm_cb.disc_cb.activation_info.rf_event_timerflag == 1) &&
      (nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval ==
       0)) {
    nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_max_interval =
        NFA_DM_RF_QOS_LISTEN_PATTERN_TIMEOUT;
    nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval =
        NFA_DM_RF_QOS_LISTEN_PATTERN_TIMEOUT;
  }

  NFA_TRACE_DEBUG1(
      "nfa_dm_rf_qos_listen_interval_timeout - MAX  msecs : %ld",
      nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_max_interval);
  NFA_TRACE_DEBUG1(
      "nfa_dm_rf_qos_listen_interval_timeout -TOTAL msecs  : %ld",
      nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval);

  if (!nfa_dm_cb.disc_cb.activation_info.rf_listen_active) {
    nfa_dm_cb.disc_cb.discovery_error_info.rf_error_code =
        NFA_NO_ACTIVATION_CODE;
    nfa_dm_cb.disc_cb.discovery_error_info.rf_error_info
        .rf_on_off_burst_period =
        nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval;
    NFA_TRACE_DEBUG1("nfa_dm_rf_qos_listen_interval_timeout - Error Code : %d",
                     nfa_dm_cb.disc_cb.discovery_error_info.rf_error_code);
    NFA_TRACE_DEBUG1(
        "nfa_dm_rf_qos_listen_interval_timeout -rf_on_off_burst_period  : %ld",
        nfa_dm_cb.disc_cb.activation_info.rf_listen_inactive_total_interval);
    // Todo report the errors
  }

  /* Clear the QOS listen data after reporting error */
  nfa_dm_rf_qos_clear_listen_data();

  return 0;
}

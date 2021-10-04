/******************************************************************************
 *
 *  Copyright (C) 2010-2014 Broadcom Corporation
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
 *  NFA interface to NFCEE - API functions
 *
 ******************************************************************************/
#include <android-base/stringprintf.h>
#include <base/logging.h>

#include "nfa_dm_int.h"
#include "nfa_ee_api.h"
#include "nfa_ee_int.h"
#include "nfc_int.h"

using android::base::StringPrintf;

extern bool nfc_debug_enabled;

/*****************************************************************************
**  APIs
*****************************************************************************/
/*******************************************************************************
**
** Function         NFA_EeDiscover
**
** Description      This function retrieves the NFCEE information from NFCC.
**                  The NFCEE information is reported in NFA_EE_DISCOVER_EVT.
**
**                  This function may be called when a system supports removable
**                  NFCEEs,
**
** Returns          NFA_STATUS_OK if information is retrieved successfully
**                  NFA_STATUS_FAILED If wrong state (retry later)
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeDiscover(tNFA_EE_CBACK* p_cback) {
  tNFA_EE_API_DISCOVER* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;

  DLOG_IF(INFO, nfc_debug_enabled) << __func__;

  if (nfa_ee_cb.em_state != NFA_EE_EM_STATE_INIT_DONE) {
    LOG(ERROR) << StringPrintf("NFA_EeDiscover bad em state: %d",
                               nfa_ee_cb.em_state);
    status = NFA_STATUS_FAILED;
  } else if ((nfa_ee_cb.p_ee_disc_cback != nullptr) || (p_cback == nullptr)) {
    LOG(ERROR) << StringPrintf("in progress or NULL callback function");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_DISCOVER*)GKI_getbuf(sizeof(tNFA_EE_API_DISCOVER));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_DISCOVER_EVT;
      p_msg->p_cback = p_cback;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeGetInfo
**
** Description      This function retrieves the NFCEE information from NFA.
**                  The actual number of NFCEE is returned in p_num_nfcee
**                  and NFCEE information is returned in p_info
**
** Returns          NFA_STATUS_OK if information is retrieved successfully
**                  NFA_STATUS_FAILED If wrong state (retry later)
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeGetInfo(uint8_t* p_num_nfcee, tNFA_EE_INFO* p_info) {
  int xx, ret = nfa_ee_cb.cur_ee;
  tNFA_EE_ECB* p_cb = nfa_ee_cb.ecb;
  uint8_t max_ret;
  uint8_t num_ret = 0;

  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("NFA_EeGetInfo em_state:%d cur_ee:%d", nfa_ee_cb.em_state,
                      nfa_ee_cb.cur_ee);
  /* validate parameters */
  if (p_info == nullptr || p_num_nfcee == nullptr) {
    LOG(ERROR) << StringPrintf("NFA_EeGetInfo bad parameter");
    return (NFA_STATUS_INVALID_PARAM);
  }
  max_ret = *p_num_nfcee;
  *p_num_nfcee = 0;
  if (nfa_ee_cb.em_state == NFA_EE_EM_STATE_INIT) {
    LOG(ERROR) << StringPrintf("NFA_EeGetInfo bad em state: %d",
                               nfa_ee_cb.em_state);
    return (NFA_STATUS_FAILED);
  }

  /* compose output */
  for (xx = 0; (xx < ret) && (num_ret < max_ret); xx++, p_cb++) {
    DLOG_IF(INFO, nfc_debug_enabled)
        << StringPrintf("xx:%d max_ret:%d, num_ret:%d ee_status:0x%x", xx,
                        max_ret, num_ret, p_cb->ee_status);
    if ((p_cb->ee_status & NFA_EE_STATUS_INT_MASK) ||
        (p_cb->ee_status == NFA_EE_STATUS_REMOVED)) {
      continue;
    }
    p_info->ee_handle = NFA_HANDLE_GROUP_EE | (tNFA_HANDLE)p_cb->nfcee_id;
    p_info->ee_status = p_cb->ee_status;
    p_info->num_interface = p_cb->num_interface;
    p_info->num_tlvs = p_cb->num_tlvs;
    memcpy(p_info->ee_interface, p_cb->ee_interface, p_cb->num_interface);
    memcpy(p_info->ee_tlv, p_cb->ee_tlv, p_cb->num_tlvs * sizeof(tNFA_EE_TLV));
    p_info->ee_power_supply_status = p_cb->ee_power_supply_status;
    p_info++;
    num_ret++;
  }
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("num_ret:%d", num_ret);
  *p_num_nfcee = num_ret;
  return (NFA_STATUS_OK);
}

/*******************************************************************************
**
** Function         NFA_EeRegister
**
** Description      This function registers a callback function to receive the
**                  events from NFA-EE module.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeRegister(tNFA_EE_CBACK* p_cback) {
  tNFA_EE_API_REGISTER* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;

  DLOG_IF(INFO, nfc_debug_enabled) << __func__;

  if (p_cback == nullptr) {
    LOG(ERROR) << StringPrintf("with NULL callback function");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_REGISTER*)GKI_getbuf(sizeof(tNFA_EE_API_REGISTER));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_REGISTER_EVT;
      p_msg->p_cback = p_cback;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeDeregister
**
** Description      This function de-registers the callback function
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeDeregister(tNFA_EE_CBACK* p_cback) {
  tNFA_EE_API_DEREGISTER* p_msg;
  tNFA_STATUS status = NFA_STATUS_INVALID_PARAM;
  int index = NFA_EE_MAX_CBACKS;
  int xx;

  for (xx = 0; xx < NFA_EE_MAX_CBACKS; xx++) {
    if (nfa_ee_cb.p_ee_cback[xx] == p_cback) {
      index = xx;
      status = NFA_STATUS_FAILED;
      break;
    }
  }

  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("%d, status:%d", index, status);
  if ((status != NFA_STATUS_INVALID_PARAM) &&
      (p_msg = (tNFA_EE_API_DEREGISTER*)GKI_getbuf(
           sizeof(tNFA_EE_API_DEREGISTER))) != nullptr) {
    p_msg->hdr.event = NFA_EE_API_DEREGISTER_EVT;
    p_msg->index = index;

    nfa_sys_sendmsg(p_msg);

    status = NFA_STATUS_OK;
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeModeSet
**
** Description      This function is called to activate
**                  (mode = NFA_EE_MD_ACTIVATE) or deactivate
**                  (mode = NFA_EE_MD_DEACTIVATE) the NFCEE identified by the
**                  given ee_handle. The result of this operation is reported
**                  with the NFA_EE_MODE_SET_EVT.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeModeSet(tNFA_HANDLE ee_handle, tNFA_EE_MD mode) {
  tNFA_EE_API_MODE_SET* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  tNFA_EE_ECB *p_cb, *p_found = nullptr;
  uint32_t xx;
  uint8_t nfcee_id = (ee_handle & 0xFF);

  p_cb = nfa_ee_cb.ecb;
  for (xx = 0; xx < nfa_ee_cb.cur_ee; xx++, p_cb++) {
    if (nfcee_id == p_cb->nfcee_id) {
      p_found = p_cb;
      break;
    }
  }
  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("handle:<0x%x>, mode:0x%02X", ee_handle, mode);

  if (p_found == nullptr) {
    LOG(ERROR) << StringPrintf("invalid NFCEE:0x%04x", ee_handle);
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_MODE_SET*)GKI_getbuf(sizeof(tNFA_EE_API_MODE_SET));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_MODE_SET_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->mode = mode;
      p_msg->p_cb = p_found;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeSetDefaultTechRouting
**
** Description      This function is called to add, change or remove the
**                  default routing based on RF technology in the listen mode
**                  routing table for the given ee_handle. The status of this
**                  operation is reported as the NFA_EE_SET_TECH_CFG_EVT.
**
** Note:            If RF discovery is started,
**                  NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT should
**                  happen before calling this function
**
** Note:            NFA_EeUpdateNow() should be called after last NFA-EE
**                  function to change the listen mode routing is called.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeSetDefaultTechRouting(
    tNFA_HANDLE ee_handle, tNFA_TECHNOLOGY_MASK technologies_switch_on,
    tNFA_TECHNOLOGY_MASK technologies_switch_off,
    tNFA_TECHNOLOGY_MASK technologies_battery_off,
    tNFA_TECHNOLOGY_MASK technologies_screen_lock,
    tNFA_TECHNOLOGY_MASK technologies_screen_off,
    tNFA_TECHNOLOGY_MASK technologies_screen_off_lock) {
  tNFA_EE_API_SET_TECH_CFG* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  tNFA_EE_ECB* p_cb;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf(
      ""
      "handle:<0x%x>technology_mask:<0x%x>/<0x%x>/<0x%x><0x%x><0x%x><0x%x>",
      ee_handle, technologies_switch_on, technologies_switch_off,
      technologies_battery_off, technologies_screen_lock,
      technologies_screen_off, technologies_screen_off_lock);
  p_cb = nfa_ee_find_ecb(nfcee_id);

  if (p_cb == nullptr) {
    LOG(ERROR) << StringPrintf("Bad ee_handle");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg =
        (tNFA_EE_API_SET_TECH_CFG*)GKI_getbuf(sizeof(tNFA_EE_API_SET_TECH_CFG));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_SET_TECH_CFG_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->p_cb = p_cb;
      p_msg->technologies_switch_on = technologies_switch_on;
      p_msg->technologies_switch_off = technologies_switch_off;
      p_msg->technologies_battery_off = technologies_battery_off;
      p_msg->technologies_screen_lock = technologies_screen_lock;
      p_msg->technologies_screen_off = technologies_screen_off;
      p_msg->technologies_screen_off_lock = technologies_screen_off_lock;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeClearDefaultTechRouting
**
** Description      This function is called to remove the default routing based
**                  on RF technology in the listen mode routing table for the
**                  given ee_handle. The status of this operation is reported
**                  as the NFA_EE_CLEAR_TECH_CFG_EVT.
**
** Note:            If RF discovery is started,
**                  NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT should
**                  happen before calling this function
**
** Note:            NFA_EeUpdateNow() should be called after last NFA-EE
**                  function to change the listen mode routing is called.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeClearDefaultTechRouting(
    tNFA_HANDLE ee_handle, tNFA_TECHNOLOGY_MASK clear_technology) {
  tNFA_EE_API_SET_TECH_CFG* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  tNFA_EE_ECB* p_cb;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf(
      "handle:<0x%x>clear technology_mask:<0x%x>", ee_handle, clear_technology);
  if (!clear_technology) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("nothing to clear");
    status = NFA_STATUS_OK;
    return status;
  }

  p_cb = nfa_ee_find_ecb(nfcee_id);

  if (p_cb == nullptr) {
    LOG(ERROR) << StringPrintf("Bad ee_handle");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_CLEAR_TECH_CFG*)GKI_getbuf(
        sizeof(tNFA_EE_API_CLEAR_TECH_CFG));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_CLEAR_TECH_CFG_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->p_cb = p_cb;
      p_msg->technologies_switch_on = clear_technology;
      p_msg->technologies_switch_off = clear_technology;
      p_msg->technologies_battery_off = clear_technology;
      p_msg->technologies_screen_lock = clear_technology;
      p_msg->technologies_screen_off = clear_technology;
      p_msg->technologies_screen_off_lock = clear_technology;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeSetDefaultProtoRouting
**
** Description      This function is called to add, change or remove the
**                  default routing based on Protocol in the listen mode routing
**                  table for the given ee_handle. The status of this
**                  operation is reported as the NFA_EE_SET_PROTO_CFG_EVT.
**
** Note:            If RF discovery is started,
**                  NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT should
**                  happen before calling this function
**
** Note:            NFA_EeUpdateNow() should be called after last NFA-EE
**                  function to change the listen mode routing is called.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeSetDefaultProtoRouting(
    tNFA_HANDLE ee_handle, tNFA_PROTOCOL_MASK protocols_switch_on,
    tNFA_PROTOCOL_MASK protocols_switch_off,
    tNFA_PROTOCOL_MASK protocols_battery_off,
    tNFA_PROTOCOL_MASK protocols_screen_lock,
    tNFA_PROTOCOL_MASK protocols_screen_off,
    tNFA_PROTOCOL_MASK protocols_screen_off_lock) {
  tNFA_EE_API_SET_PROTO_CFG* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  tNFA_EE_ECB* p_cb;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf(
      "handle:<0x%x>protocol_mask:<0x%x>/<0x%x>/<0x%x><0x%x><0x%x><0x%x>",
      ee_handle, protocols_switch_on, protocols_switch_off,
      protocols_battery_off, protocols_screen_lock, protocols_screen_off,
      protocols_screen_off_lock);
  p_cb = nfa_ee_find_ecb(nfcee_id);

  if (p_cb == nullptr) {
    LOG(ERROR) << StringPrintf("Bad ee_handle");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_SET_PROTO_CFG*)GKI_getbuf(
        sizeof(tNFA_EE_API_SET_PROTO_CFG));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_SET_PROTO_CFG_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->p_cb = p_cb;
      p_msg->protocols_switch_on = protocols_switch_on;
      p_msg->protocols_switch_off = protocols_switch_off;
      p_msg->protocols_battery_off = protocols_battery_off;
      p_msg->protocols_screen_lock = protocols_screen_lock;
      p_msg->protocols_screen_off = protocols_screen_off;
      p_msg->protocols_screen_off_lock = protocols_screen_off_lock;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeClearDefaultProtoRouting
**
** Description      This function is called to remove the default routing based
**                  on RF technology in the listen mode routing table for the
**                  given ee_handle. The status of this operation is reported
**                  as the NFA_EE_CLEAR_TECH_CFG_EVT.
**
** Note:            If RF discovery is started,
**                  NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT should
**                  happen before calling this function
**
** Note:            NFA_EeUpdateNow() should be called after last NFA-EE
**                  function to change the listen mode routing is called.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeClearDefaultProtoRouting(tNFA_HANDLE ee_handle,
                                           tNFA_PROTOCOL_MASK clear_protocol) {
  tNFA_EE_API_SET_PROTO_CFG* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  tNFA_EE_ECB* p_cb;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf(
      "handle:<0x%x>clear protocol_mask:<0x%x>", ee_handle, clear_protocol);
  if (!clear_protocol) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("nothing to clear");
    status = NFA_STATUS_OK;
    return status;
  }

  p_cb = nfa_ee_find_ecb(nfcee_id);

  if (p_cb == nullptr) {
    LOG(ERROR) << StringPrintf("Bad ee_handle");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_SET_PROTO_CFG*)GKI_getbuf(
        sizeof(tNFA_EE_API_SET_PROTO_CFG));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_CLEAR_PROTO_CFG_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->p_cb = p_cb;
      p_msg->protocols_switch_on = clear_protocol;
      p_msg->protocols_switch_off = clear_protocol;
      p_msg->protocols_battery_off = clear_protocol;
      p_msg->protocols_screen_lock = clear_protocol;
      p_msg->protocols_screen_off = clear_protocol;
      p_msg->protocols_screen_off_lock = clear_protocol;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeAddAidRouting
**
** Description      This function is called to add an AID entry in the
**                  listen mode routing table in NFCC. The status of this
**                  operation is reported as the NFA_EE_ADD_AID_EVT.
**
** Note:            If RF discovery is started,
**                  NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT should
**                  happen before calling this function
**
** Note:            NFA_EeUpdateNow() should be called after last NFA-EE
**                  function to change the listen mode routing is called.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeAddAidRouting(tNFA_HANDLE ee_handle, uint8_t aid_len,
                                uint8_t* p_aid, tNFA_EE_PWR_STATE power_state,
                                uint8_t aidInfo) {
  tNFA_EE_API_ADD_AID* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint16_t size = sizeof(tNFA_EE_API_ADD_AID) + aid_len;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  tNFA_EE_ECB* p_cb;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("handle:<0x%x>", ee_handle);
  p_cb = nfa_ee_find_ecb(nfcee_id);

  /* validate parameters - make sure the AID is in valid length range */
  if ((p_cb == nullptr) ||
      ((NFA_GetNCIVersion() == NCI_VERSION_2_0) && (aid_len != 0) &&
       (p_aid == nullptr)) ||
      ((NFA_GetNCIVersion() != NCI_VERSION_2_0) &&
       ((aid_len == 0) || (p_aid == nullptr) || (aid_len < NFA_MIN_AID_LEN))) ||
      (aid_len > NFA_MAX_AID_LEN)) {
    LOG(ERROR) << StringPrintf("Bad ee_handle or AID (len=%d)", aid_len);
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_ADD_AID*)GKI_getbuf(size);
    if (p_msg != nullptr) {
      if (p_aid != nullptr)
        DLOG_IF(INFO, nfc_debug_enabled)
            << StringPrintf("aid:<%02x%02x>", p_aid[0], p_aid[1]);
      p_msg->hdr.event = NFA_EE_API_ADD_AID_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->p_cb = p_cb;
      p_msg->aid_len = aid_len;
      p_msg->power_state = power_state;
      p_msg->p_aid = (uint8_t*)(p_msg + 1);
      p_msg->aidInfo = aidInfo;
      if (p_aid != nullptr) memcpy(p_msg->p_aid, p_aid, aid_len);

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeRemoveAidRouting
**
** Description      This function is called to remove the given AID entry from
**                  the listen mode routing table. If the entry configures VS,
**                  it is also removed. The status of this operation is reported
**                  as the NFA_EE_REMOVE_AID_EVT.
**
** Note:            If RF discovery is started,
**                  NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT should
**                  happen before calling this function
**
** Note:            NFA_EeUpdateNow() should be called after last NFA-EE
**                  function to change the listen mode routing is called.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeRemoveAidRouting(uint8_t aid_len, uint8_t* p_aid) {
  tNFA_EE_API_REMOVE_AID* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint16_t size = sizeof(tNFA_EE_API_REMOVE_AID) + aid_len;

  DLOG_IF(INFO, nfc_debug_enabled) << __func__;
  if (((NFA_GetNCIVersion() == NCI_VERSION_2_0) && (aid_len != 0) &&
       (p_aid == nullptr)) ||
      ((NFA_GetNCIVersion() != NCI_VERSION_2_0) &&
       ((aid_len == 0) || (p_aid == nullptr) || (aid_len < NFA_MIN_AID_LEN))) ||
      (aid_len > NFA_MAX_AID_LEN)) {
    LOG(ERROR) << StringPrintf("Bad AID");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_REMOVE_AID*)GKI_getbuf(size);
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_REMOVE_AID_EVT;
      p_msg->aid_len = aid_len;
      p_msg->p_aid = (uint8_t*)(p_msg + 1);
      memcpy(p_msg->p_aid, p_aid, aid_len);

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeAddSystemCodeRouting
**
** Description      This function is called to add an system code entry in the
**                  listen mode routing table in NFCC. The status of this
**                  operation is reported as the NFA_EE_ADD_SYSCODE_EVT.
**
** Note:            If RF discovery is started,
**                  NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT should
**                  happen before calling this function
**
** Note:            NFA_EeUpdateNow() should be called after last NFA-EE
**                  function to change the listen mode routing is called.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeAddSystemCodeRouting(uint16_t systemcode,
                                       tNFA_HANDLE ee_handle,
                                       tNFA_EE_PWR_STATE power_state) {
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("NFA_EeAddSystemCodeRouting(): handle:<0x%x>", ee_handle);
  tNFA_EE_ECB* p_cb = nfa_ee_find_ecb(nfcee_id);

  if (p_cb == nullptr || systemcode == 0) {
    LOG(ERROR) << StringPrintf("Bad ee_handle or System Code");
    status = NFA_STATUS_INVALID_PARAM;
  } else if ((NFA_GetNCIVersion() != NCI_VERSION_2_0) &&
             (nfc_cb.isScbrSupported == false)) {
    LOG(ERROR) << StringPrintf("Invalid NCI Version/SCBR not supported");
    status = NFA_STATUS_NOT_SUPPORTED;
  } else {
    tNFA_EE_API_ADD_SYSCODE* p_msg =
        (tNFA_EE_API_ADD_SYSCODE*)GKI_getbuf(sizeof(tNFA_EE_API_ADD_SYSCODE));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_ADD_SYSCODE_EVT;
      p_msg->power_state = power_state;
      p_msg->nfcee_id = nfcee_id;
      p_msg->p_cb = p_cb;
      // adjust endianness of syscode
      p_msg->syscode = (systemcode & 0x00FF) << 8 | (systemcode & 0xFF00) >> 8;
      nfa_sys_sendmsg(p_msg);
      status = NFA_STATUS_OK;
    }
  }
  return status;
}

/*******************************************************************************
**
** Function         NFA_EeRemoveSystemCodeRouting
**
** Description      This function is called to remove the given System Code
**                  based entry from the listen mode routing table. The status
**                  of this operation is reported as the
**                  NFA_EE_REMOVE_SYSCODE_EVT.
**
** Note:            If RF discovery is started,
**                  NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT should
**                  happen before calling this function
**
** Note:            NFA_EeUpdateNow() should be called after last NFA-EE
**                  function to change the listen mode routing is called.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeRemoveSystemCodeRouting(uint16_t systemcode) {
  tNFA_STATUS status = NFA_STATUS_FAILED;

  if (systemcode == 0) {
    LOG(ERROR) << "Bad ee_handle or System Code";
    status = NFA_STATUS_INVALID_PARAM;
  } else if ((NFA_GetNCIVersion() != NCI_VERSION_2_0) &&
             (nfc_cb.isScbrSupported == false)) {
    LOG(ERROR) << "Invalid NCI Version/SCBR Not supported";
    status = NFA_STATUS_NOT_SUPPORTED;
  } else {
    tNFA_EE_API_REMOVE_SYSCODE* p_msg = (tNFA_EE_API_REMOVE_SYSCODE*)GKI_getbuf(
        sizeof(tNFA_EE_API_REMOVE_SYSCODE));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_REMOVE_SYSCODE_EVT;
      p_msg->syscode = (systemcode & 0x00FF) << 8 | (systemcode & 0xFF00) >> 8;
      nfa_sys_sendmsg(p_msg);
      status = NFA_STATUS_OK;
    }
  }
  return status;
}

/*******************************************************************************
**
** Function         NFA_GetAidTableSize
**
** Description      This function is called to get the Maximum AID routing table
*size.
**
** Returns          AID routing table maximum size
**
*******************************************************************************/
uint16_t NFA_GetAidTableSize() { return nfa_ee_find_max_aid_cfg_len(); }

/*******************************************************************************
**
** Function         NFA_EeGetLmrtRemainingSize
**
** Description      This function is called to get remaining size of the
**                  Listen Mode Routing Table.
**                  The remaining size is reported in NFA_EE_REMAINING_SIZE_EVT
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**
*******************************************************************************/
tNFA_STATUS NFA_EeGetLmrtRemainingSize(void) {
  tNFA_EE_API_LMRT_SIZE* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;

  DLOG_IF(INFO, nfc_debug_enabled) << __func__;
  p_msg = (tNFA_EE_API_LMRT_SIZE*)GKI_getbuf(sizeof(tNFA_EE_API_LMRT_SIZE));
  if (p_msg != nullptr) {
    p_msg->event = NFA_EE_API_LMRT_SIZE_EVT;
    nfa_sys_sendmsg(p_msg);
    status = NFA_STATUS_OK;
  }

  return status;
}

/******************************************************************************
**
** Function         NFA_EeUpdateNow
**
** Description      This function is called to send the current listen mode
**                  routing table and VS configuration to the NFCC (without
**                  waiting for NFA_EE_ROUT_TIMEOUT_VAL).
**
**                  The status of this operation is
**                  reported with the NFA_EE_UPDATED_EVT.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_SEMANTIC_ERROR is update is currently in progress
**                  NFA_STATUS_FAILED otherwise
**
*******************************************************************************/
tNFA_STATUS NFA_EeUpdateNow(void) {
  NFC_HDR* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;

  DLOG_IF(INFO, nfc_debug_enabled) << __func__;
  if (nfa_ee_cb.ee_wait_evt & NFA_EE_WAIT_UPDATE_ALL) {
    LOG(ERROR) << StringPrintf("update in progress");
    status = NFA_STATUS_SEMANTIC_ERROR;
  } else {
    p_msg = (NFC_HDR*)GKI_getbuf(NFC_HDR_SIZE);
    if (p_msg != nullptr) {
      p_msg->event = NFA_EE_API_UPDATE_NOW_EVT;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeConnect
**
** Description      Open connection to an NFCEE attached to the NFCC
**
**                  The status of this operation is
**                  reported with the NFA_EE_CONNECT_EVT.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeConnect(tNFA_HANDLE ee_handle, uint8_t ee_interface,
                          tNFA_EE_CBACK* p_cback) {
  tNFA_EE_API_CONNECT* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  tNFA_EE_ECB* p_cb;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf(
      "handle:<0x%x> ee_interface:0x%x", ee_handle, ee_interface);
  p_cb = nfa_ee_find_ecb(nfcee_id);

  if ((p_cb == nullptr) || (p_cback == nullptr)) {
    LOG(ERROR) << StringPrintf("Bad ee_handle or NULL callback function");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_CONNECT*)GKI_getbuf(sizeof(tNFA_EE_API_CONNECT));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_CONNECT_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->p_cb = p_cb;
      p_msg->ee_interface = ee_interface;
      p_msg->p_cback = p_cback;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeSendData
**
** Description      Send data to the given NFCEE.
**                  This function shall be called after NFA_EE_CONNECT_EVT is
**                  reported and before NFA_EeDisconnect is called on the given
**                  ee_handle.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeSendData(tNFA_HANDLE ee_handle, uint16_t data_len,
                           uint8_t* p_data) {
  tNFA_EE_API_SEND_DATA* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  tNFA_EE_ECB* p_cb;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("handle:<0x%x>", ee_handle);

  p_cb = nfa_ee_find_ecb(nfcee_id);

  if ((p_cb == nullptr) || (p_cb->conn_st != NFA_EE_CONN_ST_CONN) ||
      (p_data == nullptr)) {
    LOG(ERROR) << StringPrintf("Bad ee_handle or NULL data");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_SEND_DATA*)GKI_getbuf(
        (uint16_t)(sizeof(tNFA_EE_API_SEND_DATA) + data_len));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_SEND_DATA_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->p_cb = p_cb;
      p_msg->data_len = data_len;
      p_msg->p_data = (uint8_t*)(p_msg + 1);
      memcpy(p_msg->p_data, p_data, data_len);

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EeDisconnect
**
** Description      Disconnect (if a connection is currently open) from an
**                  NFCEE interface. The result of this operation is reported
**                  with the NFA_EE_DISCONNECT_EVT.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EeDisconnect(tNFA_HANDLE ee_handle) {
  tNFA_EE_API_DISCONNECT* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  tNFA_EE_ECB* p_cb;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("handle:<0x%x>", ee_handle);
  p_cb = nfa_ee_find_ecb(nfcee_id);

  if ((p_cb == nullptr) || (p_cb->conn_st != NFA_EE_CONN_ST_CONN)) {
    LOG(ERROR) << StringPrintf("Bad ee_handle");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_DISCONNECT*)GKI_getbuf(sizeof(tNFA_EE_API_DISCONNECT));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_DISCONNECT_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->p_cb = p_cb;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_EePowerAndLinkCtrl
**
** Description      This Control Message is used by the DH to constrain the way
**                  the NFCC manages the power supply and communication links
**                  between the NFCC and its connected NFCEEs.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**                  NFA_STATUS_INVALID_PARAM If bad parameter
**
*******************************************************************************/
tNFA_STATUS NFA_EePowerAndLinkCtrl(tNFA_HANDLE ee_handle, uint8_t config) {
  tNFA_EE_API_PWR_AND_LINK_CTRL* p_msg;
  tNFA_STATUS status = NFA_STATUS_FAILED;
  uint8_t nfcee_id = (uint8_t)(ee_handle & 0xFF);
  tNFA_EE_ECB* p_cb;

  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("handle:<0x%x>, config:<0x%x>", ee_handle, config);
  p_cb = nfa_ee_find_ecb(nfcee_id);

  if ((p_cb == nullptr) || (p_cb->ee_status != NFA_EE_STATUS_ACTIVE)) {
    LOG(ERROR) << StringPrintf("Bad ee_handle");
    status = NFA_STATUS_INVALID_PARAM;
  } else {
    p_msg = (tNFA_EE_API_PWR_AND_LINK_CTRL*)GKI_getbuf(
        sizeof(tNFA_EE_API_PWR_AND_LINK_CTRL));
    if (p_msg != nullptr) {
      p_msg->hdr.event = NFA_EE_API_PWR_AND_LINK_CTRL_EVT;
      p_msg->nfcee_id = nfcee_id;
      p_msg->config = config;

      nfa_sys_sendmsg(p_msg);

      status = NFA_STATUS_OK;
    }
  }

  return status;
}

/*******************************************************************************
**
** Function         NFA_GetRoutingTableSize
**
** Description      This function is used to calculate the size of each entry
**
**                  index | meanings
**                  ------+---------------
**                    0   | max_size
**                    1   | total_proto
**                    2   | total_tech
**                    3   | total_aid
**                    4   | total_sys_code
**
** Returns          void
**
*******************************************************************************/
void NFA_GetRoutingTableSize(std::vector<int>& size) {
  int xx = 0;
  tNFA_EE_ECB* p_cb = &nfa_ee_cb.ecb[0];
  size[0] = nfc_cb.max_ce_table;
  for (xx = 0; xx < NFA_EE_NUM_ECBS; xx++, p_cb++) {
    size[1] += p_cb->size_mask_proto;
    size[2] += p_cb->size_mask_tech;
    size[3] += p_cb->size_aid;
    size[4] += p_cb->size_sys_code;
  }
}

/*******************************************************************************
**
** Function         NFA_GetRoutingTable
**
** Description      This function is used to parse the routing configuration
**                  which is stored in the cache and encoding the routing table
**                  to p_table
**
** Returns          void
**
*******************************************************************************/
void NFA_GetRoutingTable(std::vector<uint8_t>& p_table) {
  const uint8_t nfa_ee_tech_mask_list[3] = {
      NFA_TECHNOLOGY_MASK_A, NFA_TECHNOLOGY_MASK_B, NFA_TECHNOLOGY_MASK_F};

  const uint8_t nfa_ee_proto_mask_list[5] = {
      NFA_PROTOCOL_MASK_T1T, NFA_PROTOCOL_MASK_T2T, NFA_PROTOCOL_MASK_T3T,
      NFA_PROTOCOL_MASK_ISO_DEP, NFA_PROTOCOL_MASK_NFC_DEP};

  int xx = 0;
  tNFA_EE_ECB* p_cb = &nfa_ee_cb.ecb[0];

  for (xx = 0; xx < NFA_EE_NUM_ECBS; xx++, p_cb++) {
    if (p_cb->tech_switch_on | p_cb->tech_switch_off | p_cb->tech_battery_off) {
      for (int t_xx = 0; t_xx < 3; t_xx++) {
        uint8_t power_cfg = 0;
        if (p_cb->tech_switch_off & nfa_ee_tech_mask_list[t_xx])
          power_cfg |= NCI_ROUTE_PWR_STATE_SWITCH_OFF;
        if (p_cb->tech_battery_off & nfa_ee_tech_mask_list[t_xx])
          power_cfg |= NCI_ROUTE_PWR_STATE_BATT_OFF;
        if (p_cb->tech_switch_on & nfa_ee_tech_mask_list[t_xx]) {
          power_cfg |= NCI_ROUTE_PWR_STATE_ON;
          if (NFC_GetNCIVersion() == NCI_VERSION_2_0) {
            if (p_cb->tech_screen_lock & nfa_ee_tech_mask_list[t_xx])
              power_cfg |= NCI_ROUTE_PWR_STATE_SCREEN_ON_LOCK();
            if (p_cb->tech_screen_off & nfa_ee_tech_mask_list[t_xx])
              power_cfg |= NCI_ROUTE_PWR_STATE_SCREEN_OFF_UNLOCK();
            if (p_cb->tech_screen_off_lock & nfa_ee_tech_mask_list[t_xx])
              power_cfg |= NCI_ROUTE_PWR_STATE_SCREEN_OFF_LOCK();
          }
        }
        if (power_cfg) {
          p_table.push_back((uint8_t)'T');
          p_table.push_back(nfa_ee_tech_mask_list[t_xx]);
          p_table.push_back(p_cb->nfcee_id);
          p_table.push_back(power_cfg);
          p_table.push_back(0x00);
        }
      }
    }
    if (p_cb->proto_switch_on | p_cb->proto_switch_off |
        p_cb->proto_battery_off) {
      for (int p_xx = 0; p_xx < 5; p_xx++) {
        uint8_t power_cfg = 0;
        if (p_cb->proto_switch_off & nfa_ee_proto_mask_list[p_xx])
          power_cfg |= NCI_ROUTE_PWR_STATE_SWITCH_OFF;
        if (p_cb->proto_battery_off & nfa_ee_proto_mask_list[p_xx])
          power_cfg |= NCI_ROUTE_PWR_STATE_BATT_OFF;
        if (p_cb->proto_switch_on & nfa_ee_proto_mask_list[p_xx])
          power_cfg |= NCI_ROUTE_PWR_STATE_ON;

        if (power_cfg ||
            (p_cb->nfcee_id == NFC_DH_ID &&
             nfa_ee_proto_mask_list[p_xx] == NFA_PROTOCOL_MASK_NFC_DEP)) {
          uint8_t block_ctrl = 0;
          if (nfa_ee_proto_mask_list[p_xx] == NFA_PROTOCOL_MASK_ISO_DEP) {
            block_ctrl = nfa_ee_cb.route_block_control;
            if ((power_cfg & NCI_ROUTE_PWR_STATE_ON) &&
                (NFC_GetNCIVersion() == NCI_VERSION_2_0)) {
              if (p_cb->proto_screen_lock & nfa_ee_proto_mask_list[p_xx])
                power_cfg |= NCI_ROUTE_PWR_STATE_SCREEN_ON_LOCK();
              if (p_cb->proto_screen_off & nfa_ee_proto_mask_list[p_xx])
                power_cfg |= NCI_ROUTE_PWR_STATE_SCREEN_OFF_UNLOCK();
              if (p_cb->proto_screen_off_lock & nfa_ee_proto_mask_list[p_xx])
                power_cfg |= NCI_ROUTE_PWR_STATE_SCREEN_OFF_LOCK();
            }
          }
          if (p_cb->nfcee_id == NFC_DH_ID &&
              nfa_ee_proto_mask_list[p_xx] == NFA_PROTOCOL_MASK_NFC_DEP) {
            /* add to routing table if NFC_DEP interface is supported */
            if (nfc_cb.nci_interfaces & (1 << NCI_INTERFACE_NFC_DEP)) {
              p_table.push_back((uint8_t)'P');
              p_table.push_back(NFA_PROTOCOL_MASK_NFC_DEP);
              p_table.push_back(p_cb->nfcee_id);
              p_table.push_back(power_cfg);
              p_table.push_back(0x00);
            } else {
              continue;
            }
          } else {
            p_table.push_back((uint8_t)'P');
            p_table.push_back(nfa_ee_proto_mask_list[p_xx]);
            p_table.push_back(p_cb->nfcee_id);
            p_table.push_back(power_cfg);
            p_table.push_back(block_ctrl);
          }
        }
      }
    }
    if (p_cb->aid_entries) {
      int offset = 0;
      for (int a_xx = 0; a_xx < p_cb->aid_entries; a_xx++) {
        if (p_cb->aid_rt_info[a_xx] & NFA_EE_AE_ROUTE) {
          p_table.push_back((uint8_t)'A');
          p_table.push_back(p_cb->aid_len[a_xx] - 2);

          for (int i = 2; i < p_cb->aid_len[a_xx]; i++) {
            p_table.push_back(p_cb->aid_cfg[i + offset]);
          }

          p_table.push_back(p_cb->nfcee_id);
          p_table.push_back(p_cb->aid_pwr_cfg[a_xx]);
          p_table.push_back(nfa_ee_cb.route_block_control);

          uint8_t route_qual = 0;
          if (p_cb->aid_info[a_xx] & NCI_ROUTE_QUAL_LONG_SELECT)
            route_qual |= NCI_ROUTE_QUAL_LONG_SELECT;
          if (p_cb->aid_info[a_xx] & NCI_ROUTE_QUAL_SHORT_SELECT)
            route_qual |= NCI_ROUTE_QUAL_SHORT_SELECT;
          p_table.push_back(route_qual);
        }
        offset += p_cb->aid_len[a_xx];
      }
    }
    if (p_cb->sys_code_cfg_entries) {
      int offset = 0;
      for (int s_xx = 0; s_xx < p_cb->sys_code_cfg_entries; s_xx++) {
        if (p_cb->sys_code_rt_loc_vs_info[s_xx] & NFA_EE_AE_ROUTE) {
          if (nfa_ee_is_active(p_cb->sys_code_rt_loc[s_xx] |
                               NFA_HANDLE_GROUP_EE)) {
            p_table.push_back((uint8_t)'S');
            p_table.push_back(NFA_EE_SYSTEM_CODE_LEN);
            for (int i = 0; i < NFA_EE_SYSTEM_CODE_LEN; i++) {
              p_table.push_back(p_cb->sys_code_cfg[offset + i]);
            }
            p_table.push_back(p_cb->nfcee_id);
            p_table.push_back(p_cb->sys_code_pwr_cfg[s_xx]);
            p_table.push_back(nfa_ee_cb.route_block_control);
          }
        }
        offset += NFA_EE_SYSTEM_CODE_LEN;
      }
    }
  }
}

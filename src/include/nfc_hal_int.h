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
 *  this file contains the NCI transport internal definitions and functions.
 *
 ******************************************************************************/

#ifndef NFC_HAL_INT_H
#define NFC_HAL_INT_H

#include "gki.h"
#include "nci_defs.h"
#include "nfc_brcm_defs.h"
#include "nfc_hal_api.h"
#include "nfc_hal_int_api.h"
#include "nfc_hal_target.h"

/****************************************************************************
** NFC HAL TASK transport definitions
****************************************************************************/
/* NFC HAL Task event masks */
#define NFC_HAL_TASK_EVT_DATA_RDY EVENT_MASK(APPL_EVT_0)
#define NFC_HAL_TASK_EVT_INITIALIZE EVENT_MASK(APPL_EVT_5)
#define NFC_HAL_TASK_EVT_TERMINATE EVENT_MASK(APPL_EVT_6)
#define NFC_HAL_TASK_EVT_POWER_CYCLE EVENT_MASK(APPL_EVT_7)

#define NFC_HAL_TASK_EVT_MBOX (TASK_MBOX_0_EVT_MASK)

/* NFC HAL Task mailbox definitions */
#define NFC_HAL_TASK_MBOX (TASK_MBOX_0)

/* NFC HAL Task Timer events */
#ifndef NFC_HAL_QUICK_TIMER_EVT_MASK
#define NFC_HAL_QUICK_TIMER_EVT_MASK (TIMER_0_EVT_MASK)
#endif

#ifndef NFC_HAL_QUICK_TIMER_ID
#define NFC_HAL_QUICK_TIMER_ID (TIMER_0)
#endif

/* NFC HAL Task Timer types */
#define NFC_HAL_TTYPE_NCI_WAIT_RSP 0
#define NFC_HAL_TTYPE_POWER_CYCLE 1
#define NFC_HAL_TTYPE_NFCC_ENABLE 2

/* NFC HAL Task Wait Response flag */
/* wait response on an NCI command                  */
#define NFC_HAL_WAIT_RSP_CMD 0x10
/* wait response on an NCI vendor specific command  */
#define NFC_HAL_WAIT_RSP_VSC 0x20
/* wait response on a proprietary command           */
#define NFC_HAL_WAIT_RSP_PROP 0x40
/* not waiting for anything                         */
#define NFC_HAL_WAIT_RSP_NONE 0x00

typedef uint8_t tNFC_HAL_WAIT_RSP;

#if (NFC_HAL_HCI_INCLUDED == TRUE)

#define NFC_HAL_HCI_PIPE_INFO_SIZE 5

#define NFC_HAL_HCI_ANY_SET_PARAMETER 0x01
#define NFC_HAL_HCI_ANY_GET_PARAMETER 0x02
#define NFC_HAL_HCI_ADM_NOTIFY_ALL_PIPE_CLEARED 0x15

#define NFC_HAL_HCI_SESSION_IDENTITY_INDEX 0x01
#define NFC_HAL_HCI_WHITELIST_INDEX 0x03

#define NFC_HAL_HCI_ADMIN_PIPE 0x01
/* Host ID for UICC 0 */
#define NFC_HAL_HCI_HOST_ID_UICC0 0x02
/* Host ID for UICC 1 */
#define NFC_HAL_HCI_HOST_ID_UICC1 0x03
/* Host ID for UICC 2 */
#define NFC_HAL_HCI_HOST_ID_UICC2 0x04
#define NFC_HAL_HCI_COMMAND_TYPE 0x00
#define NFC_HAL_HCI_RESPONSE_TYPE 0x02

/* NFC HAL HCI responses */
#define NFC_HAL_HCI_ANY_OK 0x00

#endif

/* Flag defintions for tNFC_HAL_NVM */
/* No NVM available                     */
#define NFC_HAL_NVM_FLAGS_NO_NVM 0x01
/* FPM patch in NVM failed CRC check    */
#define NFC_HAL_NVM_FLAGS_LPM_BAD 0x02
/* LPM patch in NVM failed CRC check    */
#define NFC_HAL_NVM_FLAGS_FPM_BAD 0x04
/* Patch is present in NVM              */
#define NFC_HAL_NVM_FLAGS_PATCH_PRESENT 0x08

/* NFC HAL transport configuration */
typedef struct {
  bool shared_transport; /* TRUE if using shared HCI/NCI transport */
  uint8_t userial_baud;
  uint8_t userial_fc;
} tNFC_HAL_TRANS_CFG;

#ifdef TESTER
/* For Insight, ncit_cfg is runtime-configurable */
#define NFC_HAL_TRANS_CFG_QUALIFIER
#else
/* For all other platforms, ncit_cfg is constant */
#define NFC_HAL_TRANS_CFG_QUALIFIER const
#endif
extern NFC_HAL_TRANS_CFG_QUALIFIER tNFC_HAL_TRANS_CFG nfc_hal_trans_cfg;

/*****************************************************************************
* BT HCI definitions
*****************************************************************************/

/* Tranport message type */
#define HCIT_TYPE_COMMAND 0x01
#define HCIT_TYPE_EVENT 0x04
#define HCIT_TYPE_NFC 0x10

/* Vendor-Specific BT HCI definitions */
#define HCI_SUCCESS 0x00
#define HCI_GRP_VENDOR_SPECIFIC (0x3F << 10) /* 0xFC00 */
#define HCI_BRCM_WRITE_SLEEP_MODE (0x0027 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_GRP_HOST_CONT_BASEBAND_CMDS (0x03 << 10) /* 0x0C00 */
#define HCI_RESET (0x0003 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_COMMAND_COMPLETE_EVT 0x0E
#define HCI_BRCM_WRITE_SLEEP_MODE_LENGTH 12
#define HCI_BRCM_UPDATE_BAUD_RATE_UNENCODED_LENGTH 0x06
#define HCIE_PREAMBLE_SIZE 2
#define HCI_BRCM_PRE_SET_MEM (0x000C | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_BRCM_PRE_SET_MEM_LENGTH 10
#define HCI_BRCM_PRE_SET_MEM_TYPE 8

/****************************************************************************
** Internal constants and definitions
****************************************************************************/

/* errors during NCI packet reassembly process */
#define NFC_HAL_NCI_RAS_TOO_BIG 0x01
#define NFC_HAL_NCI_RAS_ERROR 0x02
typedef uint8_t tNFC_HAL_NCI_RAS;

typedef uint8_t tNFC_HAL_POWER_MODE;

#define NFC_HAL_ASSERT_NFC_WAKE 0x00   /* assert NFC_WAKE      */
#define NFC_HAL_DEASSERT_NFC_WAKE 0x01 /* deassert NFC_WAKE    */

#define NFC_HAL_BT_HCI_CMD_HDR_SIZE 3 /* opcode (2) +  length (1)    */
#define NFC_HAL_CMD_TOUT (2000)       /* timeout for NCI CMD (in ms) */

#define NFC_HAL_SAVED_HDR_SIZE (2)
#define NFC_HAL_SAVED_CMD_SIZE (2)

#ifndef NFC_HAL_DEBUG
#define NFC_HAL_DEBUG TRUE
#endif

#if (NFC_HAL_DEBUG == TRUE)
extern const char* const nfc_hal_init_state_str[];
#define NFC_HAL_SET_INIT_STATE(state)                                        \
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf(                          \
      "init state: %d->%d(%s)", nfc_hal_cb.dev_cb.initializing_state, state, \
      nfc_hal_init_state_str[state]);                                        \
  nfc_hal_cb.dev_cb.initializing_state = state;
#else
#define NFC_HAL_SET_INIT_STATE(state) \
  nfc_hal_cb.dev_cb.initializing_state = state;
#endif

typedef uint8_t tNFC_HAL_INIT_STATE;

typedef uint8_t tNFC_HAL_DM_CONFIG;

/* callback function prototype */
typedef struct {
  uint16_t opcode;
  uint16_t param_len;
  uint8_t* p_param_buf;
} tNFC_HAL_BTVSC_CPLT;

#if (NFC_HAL_HCI_INCLUDED == TRUE)

/* data type for NFC_HAL_HCI_RSP_NV_READ_EVT */
typedef struct {
  NFC_HDR hdr;
  uint8_t block;
  uint16_t size;
  tHAL_NFC_STATUS status;
} tNFC_HAL_HCI_RSP_NV_READ_EVT;

/* data type for NFC_HAL_HCI_RSP_NV_WRITE_EVT */
typedef struct {
  NFC_HDR hdr;
  tHAL_NFC_STATUS status;
} tNFC_HAL_HCI_RSP_NV_WRITE_EVT;

/* union of all event data types */
typedef union {
  NFC_HDR hdr;
  /* Internal events */
  tNFC_HAL_HCI_RSP_NV_READ_EVT nv_read;
  tNFC_HAL_HCI_RSP_NV_WRITE_EVT nv_write;
} tNFC_HAL_HCI_EVENT_DATA;

#endif
/*****************************************************************************
** Control block for NFC HAL
*****************************************************************************/

/* Patch RAM Download Control block */

/* PRM states */
enum {
  NFC_HAL_PRM_ST_IDLE

};
typedef uint8_t tNFC_HAL_PRM_STATE;

/* Maximum number of patches (currently 2: LPM and FPM) */
#define NFC_HAL_PRM_MAX_PATCH_COUNT 2
#define NFC_HAL_PRM_PATCH_MASK_ALL 0xFFFFFFFF
#define NFC_HAL_PRM_MAX_CHIP_VER_LEN 8

/* Structures for PRM Control Block */
typedef struct {
  uint8_t power_mode;
  uint16_t len;
} tNFC_HAL_PRM_PATCHDESC;

typedef struct {
  tNFC_HAL_PRM_STATE state; /* download state */
  uint32_t flags;           /* internal flags */
  uint16_t
      cur_patch_len_remaining; /* bytes remaining in patchfile to process     */
  const uint8_t*
      p_cur_patch_data;      /* pointer to patch currently being downloaded */
  uint16_t cur_patch_offset; /* offset of next byte to process              */
  uint32_t dest_ram;
  TIMER_LIST_ENT timer; /* Timer for patch download                    */
  void* p_param;        /* general purpose param for PRM               */
  uint8_t param_idx;    /* information related to general purpose param*/

  /* Secure Patch Download */
  uint32_t
      spd_patch_needed_mask; /* Mask of patches that need to be downloaded */
  uint8_t spd_patch_count;   /* Number of patches left to download */
  uint8_t spd_cur_patch_idx; /* Current patch being downloaded */

  tNFC_HAL_PRM_PATCHDESC spd_patch_desc[NFC_HAL_PRM_MAX_PATCH_COUNT];

  /* I2C-patch */
  uint8_t* p_spd_patch;             /* pointer to spd patch             */
  uint16_t spd_patch_len_remaining; /* patch length                     */
  uint16_t spd_patch_offset;        /* offset of next byte to process   */

  tNFC_HAL_PRM_FORMAT format;  /* format of patch ram              */
  tNFC_HAL_PRM_CBACK* p_cback; /* Callback for download status notifications */
  uint32_t patchram_delay;     /* the dealy after patch */
} tNFC_HAL_PRM_CB;

/* Information about current patch in NVM */
typedef struct {
  uint16_t project_id; /* Current project_id of patch in nvm       */
  uint16_t ver_major;  /* Current major version of patch in nvm    */
  uint16_t ver_minor;  /* Current minor version of patch in nvm    */
  uint16_t fpm_size;   /* Current size of FPM patch in nvm         */
  uint16_t lpm_size;   /* Current size of LPM patch in nvm         */
  uint8_t flags;       /* See NFC_HAL_NVM_FLAGS_* flag definitions */
  uint8_t nvm_type;    /* Current NVM Type - UICC/EEPROM           */
  uint8_t chip_ver[NFC_HAL_PRM_MAX_CHIP_VER_LEN]; /* patch chip version       */
} tNFC_HAL_NVM;

/* Patch for I2C fix */
typedef struct {
  uint8_t* p_patch;      /* patch for i2c fix                */
  uint32_t prei2c_delay; /* the dealy after preI2C patch */
  uint16_t len;          /* i2c patch length                 */
} tNFC_HAL_PRM_I2C_FIX_CB;

/* Control block for NCI transport */
typedef struct {
  uint8_t nci_ctrl_size; /* Max size for NCI messages */
  uint8_t rcv_state;     /* current rx state */
  uint16_t rcv_len; /* bytes remaining to be received in current rx state     */
  NFC_HDR* p_rcv_msg;  /* buffer to receive NCI message */
  NFC_HDR* p_frag_msg; /* fragmented NCI message; waiting for last fragment */
  NFC_HDR*
      p_pend_cmd; /* pending NCI message; waiting for NFCC state to be free */
  tNFC_HAL_NCI_RAS nci_ras; /* nci reassembly error status */
  TIMER_LIST_ENT
  nci_wait_rsp_timer; /* Timer for waiting for nci command response */
  tNFC_HAL_WAIT_RSP nci_wait_rsp; /* nci wait response flag */
  uint8_t
      last_hdr[NFC_HAL_SAVED_HDR_SIZE]; /* part of last NCI command header */
  uint8_t
      last_cmd[NFC_HAL_SAVED_CMD_SIZE]; /* part of last NCI command payload */
  void* p_vsc_cback; /* the callback function for last VSC command */
} tNFC_HAL_NCIT_CB;

/* Control block for device initialization */
typedef struct {
  tNFC_HAL_INIT_STATE initializing_state; /* state of initializing NFCC */

  uint32_t brcm_hw_id; /* BRCM NFCC HW ID                          */
  tNFC_HAL_DM_CONFIG next_dm_config; /* next config in post initialization */
  uint8_t next_startup_vsc; /* next start-up VSC offset in post init    */

  tNFC_HAL_POWER_MODE power_mode; /* NFCC power mode                          */
  uint8_t snooze_mode;            /* current snooze mode                      */
  uint8_t new_snooze_mode;        /* next snooze mode after receiving cmpl    */
  uint8_t nfc_wake_active_mode;   /* NFC_HAL_LP_ACTIVE_LOW/HIGH               */
  TIMER_LIST_ENT lp_timer;        /* timer for low power mode                 */

  tHAL_NFC_STATUS_CBACK*
      p_prop_cback; /* callback to notify complete of proprietary update */
} tNFC_HAL_DEV_CB;

#if (NFC_HAL_HCI_INCLUDED == TRUE)

/* data members for NFC_HAL-HCI */
typedef struct {
  TIMER_LIST_ENT
  hci_timer; /* Timer to avoid indefinitely waiting for response */
  uint8_t*
      p_hci_netwk_info_buf; /* Buffer for reading HCI Network information */
  uint8_t* p_hci_netwk_dh_info_buf; /* Buffer for reading HCI Network DH
                                       information */
  uint8_t hci_netwk_config_block;  /* Rsp awaiting for hci network configuration
                                      block */
  bool b_wait_hcp_conn_create_rsp; /* Waiting for hcp connection create response
                                      */
  bool clear_all_pipes_to_uicc1;   /* UICC1 was restarted for patch download */
  bool update_session_id; /* Next response from NFCC is to Get Session id cmd */
  bool hci_fw_workaround; /* HAL HCI Workaround need */
  bool hci_fw_validate_netwk_cmd; /* Flag to indicate if hci network ntf to
                                     validate */
  uint8_t hcp_conn_id;            /* NCI Connection id for HCP */
  uint8_t dh_session_id[1];       /* Byte 0 of DH Session ID */
} tNFC_HAL_HCI_CB;

#endif

#define NFC_HAL_FLAGS_NEED_DISABLE_VSC 0x01
typedef uint8_t tNFC_HAL_FLAGS;

typedef struct {
  tHAL_NFC_CBACK* p_stack_cback;     /* Callback for HAL event notification  */
  tHAL_NFC_DATA_CBACK* p_data_cback; /* Callback for data event notification  */

  TIMER_LIST_Q quick_timer_queue; /* timer list queue                 */
  TIMER_LIST_ENT timer;           /* timer for NCI transport task     */

  tNFC_HAL_NCIT_CB ncit_cb; /* NCI transport */
  tNFC_HAL_DEV_CB dev_cb;   /* device initialization */
  tNFC_HAL_NVM nvm_cb;      /* Information about current patch in NVM */

  /* Patchram control block */
  tNFC_HAL_PRM_CB prm;
  tNFC_HAL_PRM_I2C_FIX_CB prm_i2c;

#if (NFC_HAL_HCI_INCLUDED == TRUE)
  /* data members for NFC_HAL-HCI */
  tNFC_HAL_HCI_CB hci_cb;
#endif

  uint8_t pre_discover_done; /* TRUE, when the prediscover config is complete */
  tNFC_HAL_FLAGS hal_flags;
  uint8_t pre_set_mem_idx;

  uint8_t max_rf_credits; /* NFC Max RF data credits */
  uint8_t max_ee;         /* NFC Max number of NFCEE supported by NFCC */
} tNFC_HAL_CB;

/* Global NCI data */
extern tNFC_HAL_CB nfc_hal_cb;

extern uint8_t* p_nfc_hal_pre_discover_cfg;

/****************************************************************************
** Internal nfc functions
****************************************************************************/

#if (NFC_HAL_HCI_INCLUDED == TRUE)
/* nfc_hal_hci.c */
void nfc_hal_hci_enable(void);
void nfc_hal_hci_evt_hdlr(tNFC_HAL_HCI_EVENT_DATA* p_evt_data);

void nfc_hal_hci_handle_build_info(uint8_t chipverlen, uint8_t* p_chipverstr);
#else
#define nfc_hal_hci_enable() NFC_HAL_SET_INIT_STATE(NFC_HAL_INIT_STATE_IDLE);
#define nfc_hal_hci_handle_build_info(p, a)
#define nfc_hal_hci_evt_hdlr(p) ;
#endif

/* Define default NCI protocol trace function (if protocol tracing is enabled)
 */
#if (NFC_HAL_TRACE_PROTOCOL == TRUE)
#if !defined(DISP_NCI)
#define DISP_NCI (DispNci)
void DispNci(uint8_t* p, uint16_t len, bool is_recv);
#endif /* DISP_NCI */

/* For displaying vendor-specific HCI commands */
void DispHciCmd(NFC_HDR* p_buf);
void DispHciEvt(NFC_HDR* p_buf);
#endif /* NFC_HAL_TRACE_PROTOCOL */

#endif /* NFC_HAL_INT_H */

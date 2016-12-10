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
 *  This file contains source code for some utility functions to help parse
 *  and build NFC Data Exchange Format (NDEF) messages for Connection
 *  Handover
 *
 ******************************************************************************/

#include <string.h>
#include "ndef_utils.h"

/*******************************************************************************
**
** Static Local Functions
*/
static uint8_t *ndef_get_bt_oob_record (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                      char *p_id_str);

/*******************************************************************************
**
** Static data
*/

/* Handover Request Record Type */
static uint8_t hr_rec_type[HR_REC_TYPE_LEN] = { 0x48, 0x72 }; /* "Hr" */

/* Handover Select Record Type */
static uint8_t hs_rec_type[HS_REC_TYPE_LEN] = { 0x48, 0x73 }; /* "Hs" */

/* Handover Carrier recrod Type */
static uint8_t hc_rec_type[HC_REC_TYPE_LEN] = { 0x48, 0x63 }; /* "Hc" */

/* Collision Resolution Record Type */
static uint8_t cr_rec_type[CR_REC_TYPE_LEN] = { 0x63, 0x72 }; /* "cr" */

/* Alternative Carrier Record Type */
static uint8_t ac_rec_type[AC_REC_TYPE_LEN] = { 0x61, 0x63 }; /* "ac" */

/* Error Record Type */
static uint8_t err_rec_type[ERR_REC_TYPE_LEN] = { 0x65, 0x72, 0x72 }; /* "err" */

/* Bluetooth OOB Data Type */
static uint8_t *p_bt_oob_rec_type = (uint8_t *)"application/vnd.bluetooth.ep.oob";

/* Wifi WSC Data Type */
static uint8_t *p_wifi_wsc_rec_type = (uint8_t *)"application/vnd.wfa.wsc";

/*******************************************************************************
**
** Function         NDEF_MsgCreateWktHr
**
** Description      This function creates Handover Request Record with version.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgCreateWktHr (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                  uint8_t version )
{
    tNDEF_STATUS    status;

    NDEF_MsgInit (p_msg, max_size, p_cur_size);

    /* Add record with version */
    status = NDEF_MsgAddRec (p_msg, max_size, p_cur_size,
                             NDEF_TNF_WKT, hr_rec_type, HR_REC_TYPE_LEN,
                             NULL, 0, &version, 1);

    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgCreateWktHs
**
** Description      This function creates Handover Select Record with version.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgCreateWktHs (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                  uint8_t version )
{
    tNDEF_STATUS    status;

    NDEF_MsgInit (p_msg, max_size, p_cur_size);

    /* Add record with version */
    status = NDEF_MsgAddRec (p_msg, max_size, p_cur_size,
                             NDEF_TNF_WKT, hs_rec_type, HS_REC_TYPE_LEN,
                             NULL, 0, &version, 1);

    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAddWktHc
**
** Description      This function adds Handover Carrier Record.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAddWktHc (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                               char  *p_id_str, uint8_t ctf,
                               uint8_t carrier_type_len, uint8_t *p_carrier_type,
                               uint8_t carrier_data_len, uint8_t *p_carrier_data)
{
    tNDEF_STATUS    status;
    uint8_t         payload[256], *p, id_len;
    uint32_t        payload_len;

    if (carrier_type_len + carrier_data_len + 2 > 256)
    {
        return (NDEF_MSG_INSUFFICIENT_MEM);
    }

    p = payload;

    uint8_t_TO_STREAM (p, (ctf & 0x07));
    uint8_t_TO_STREAM (p, carrier_type_len);
    ARRAY_TO_STREAM (p, p_carrier_type, carrier_type_len);
    ARRAY_TO_STREAM (p, p_carrier_data, carrier_data_len);

    payload_len = (uint32_t)carrier_type_len + carrier_data_len + 2;

    id_len = (uint8_t)strlen (p_id_str);

    status = NDEF_MsgAddRec (p_msg, max_size, p_cur_size,
                             NDEF_TNF_WKT, hc_rec_type, HC_REC_TYPE_LEN,
                             (uint8_t*)p_id_str, id_len, payload, payload_len);
    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAddWktAc
**
** Description      This function adds Alternative Carrier Record.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAddWktAc (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                               uint8_t cps, char *p_carrier_data_ref_str,
                               uint8_t aux_data_ref_count, char *p_aux_data_ref_str[])
{
    tNDEF_STATUS    status;
    uint32_t        payload_len;
    uint8_t         ref_str_len, xx;
    uint8_t *p_rec, *p;

    /* get payload length first */

    /* CPS, length of carrier data ref, carrier data ref, Aux data reference count */
    payload_len = 3 + (uint8_t)strlen (p_carrier_data_ref_str);
    for (xx = 0; xx < aux_data_ref_count; xx++)
    {
        /* Aux Data Reference length (1 byte) */
        payload_len += 1 + (uint8_t)strlen (p_aux_data_ref_str[xx]);
    }

    /* reserve memory for payload */
    status = NDEF_MsgAddRec (p_msg, max_size, p_cur_size,
                             NDEF_TNF_WKT, ac_rec_type, AC_REC_TYPE_LEN,
                             NULL, 0, NULL, payload_len);

    if (status == NDEF_OK)
    {
        /* get AC record added at the end */
        p_rec = NDEF_MsgGetLastRecInMsg (p_msg);

        /* get start pointer of reserved payload */
        p = NDEF_RecGetPayload (p_rec, &payload_len);

        /* Add Carrier Power State */
        uint8_t_TO_BE_STREAM (p, cps);

        /* Carrier Data Reference length */
        ref_str_len = (uint8_t)strlen (p_carrier_data_ref_str);

        uint8_t_TO_BE_STREAM (p, ref_str_len);

        /* Carrier Data Reference */
        ARRAY_TO_BE_STREAM (p, p_carrier_data_ref_str, ref_str_len);

        /* Aux Data Reference Count */
        uint8_t_TO_BE_STREAM (p, aux_data_ref_count);

        for (xx = 0; xx < aux_data_ref_count; xx++)
        {
            /* Aux Data Reference length (1 byte) */
            ref_str_len = (uint8_t)strlen (p_aux_data_ref_str[xx]);

            uint8_t_TO_BE_STREAM (p, ref_str_len);

            /* Aux Data Reference */
            ARRAY_TO_BE_STREAM (p, p_aux_data_ref_str[xx], ref_str_len);
        }
    }

    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAddWktCr
**
** Description      This function adds Collision Resolution Record.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAddWktCr (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                               uint16_t random_number )
{
    tNDEF_STATUS    status;
    uint8_t         data[2], *p;

    p = data;
    uint16_t_TO_BE_STREAM (p, random_number);

    status = NDEF_MsgAddRec (p_msg, max_size, p_cur_size,
                             NDEF_TNF_WKT, cr_rec_type, CR_REC_TYPE_LEN,
                             NULL, 0, data, 2);
    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAddWktErr
**
** Description      This function adds Error Record.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAddWktErr (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                uint8_t error_reason, uint32_t error_data )
{
    tNDEF_STATUS    status;
    uint8_t         payload[5], *p;
    uint32_t        payload_len;

    p = payload;

    uint8_t_TO_BE_STREAM (p, error_reason);

    if (error_reason == 0x02)
    {
        uint32_t_TO_BE_STREAM (p, error_data);
        payload_len = 5;
    }
    else
    {
        uint8_t_TO_BE_STREAM (p, error_data);
        payload_len = 2;
    }

    status = NDEF_MsgAddRec (p_msg, max_size, p_cur_size,
                             NDEF_TNF_WKT, err_rec_type, ERR_REC_TYPE_LEN,
                             NULL, 0, payload, payload_len);
    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAddMediaBtOob
**
** Description      This function adds BT OOB Record.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAddMediaBtOob (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                    char *p_id_str, BD_ADDR bd_addr)
{
    tNDEF_STATUS    status;
    uint8_t         payload[BD_ADDR_LEN + 2];
    uint8_t        *p;
    uint8_t         payload_len, id_len;

    p = payload;

    /* length including itself */
    uint16_t_TO_STREAM (p, BD_ADDR_LEN + 2);

    /* BD Addr */
    BDADDR_TO_STREAM (p, bd_addr);

    payload_len = BD_ADDR_LEN + 2;
    id_len = (uint8_t)strlen (p_id_str);

    status = NDEF_MsgAddRec (p_msg, max_size, p_cur_size,
                             NDEF_TNF_MEDIA, p_bt_oob_rec_type, BT_OOB_REC_TYPE_LEN,
                             (uint8_t*)p_id_str, id_len, payload, payload_len);
    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAppendMediaBtOobCod
**
** Description      This function appends COD EIR data at the end of BT OOB Record.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAppendMediaBtOobCod (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                          char *p_id_str, DEV_CLASS cod)
{
    tNDEF_STATUS    status;
    uint8_t        *p_rec;
    uint8_t         eir_data[BT_OOB_COD_SIZE + 2];
    uint8_t        *p;
    uint8_t         eir_data_len;
    uint32_t        oob_data_len;

    /* find record by Payload ID */
    p_rec = ndef_get_bt_oob_record (p_msg, max_size, p_cur_size, p_id_str);

    if (!p_rec)
        return (NDEF_REC_NOT_FOUND);

    /* create EIR data format for COD */
    p = eir_data;
    uint8_t_TO_STREAM (p, BT_OOB_COD_SIZE + 1);
    uint8_t_TO_STREAM (p, BT_EIR_OOB_COD_TYPE);
    DEVCLASS_TO_STREAM (p, cod);
    eir_data_len = BT_OOB_COD_SIZE + 2;

    /* append EIR data at the end of record */
    status = NDEF_MsgAppendPayload(p_msg, max_size, p_cur_size,
                                   p_rec, eir_data, eir_data_len);

    /* update BT OOB data length, if success */
    if (status == NDEF_OK)
    {
        /* payload length is the same as BT OOB data length */
        p = NDEF_RecGetPayload (p_rec, &oob_data_len);
        uint16_t_TO_STREAM (p, oob_data_len);
    }

    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAppendMediaBtOobName
**
** Description      This function appends Bluetooth Local Name EIR data
**                  at the end of BT OOB Record.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAppendMediaBtOobName (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                           char *p_id_str, bool    is_complete,
                                           uint8_t name_len, uint8_t *p_name)
{
    tNDEF_STATUS    status;
    uint8_t        *p_rec;
    uint8_t         eir_data[256];
    uint8_t        *p;
    uint8_t         eir_data_len;
    uint32_t        oob_data_len;

    /* find record by Payload ID */
    p_rec = ndef_get_bt_oob_record (p_msg, max_size, p_cur_size, p_id_str);

    if (!p_rec)
        return (NDEF_REC_NOT_FOUND);

    /* create EIR data format for COD */
    p = eir_data;
    uint8_t_TO_STREAM (p, name_len + 1);

    if (is_complete)
    {
        uint8_t_TO_STREAM (p, BT_EIR_COMPLETE_LOCAL_NAME_TYPE);
    }
    else
    {
        uint8_t_TO_STREAM (p, BT_EIR_SHORTENED_LOCAL_NAME_TYPE);
    }

    ARRAY_TO_STREAM (p, p_name, name_len);
    eir_data_len = name_len + 2;

    /* append EIR data at the end of record */
    status = NDEF_MsgAppendPayload(p_msg, max_size, p_cur_size,
                                   p_rec, eir_data, eir_data_len);

    /* update BT OOB data length, if success */
    if (status == NDEF_OK)
    {
        /* payload length is the same as BT OOB data length */
        p = NDEF_RecGetPayload (p_rec, &oob_data_len);
        uint16_t_TO_STREAM (p, oob_data_len);
    }

    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAppendMediaBtOobHashCRandR
**
** Description      This function appends Hash C and Rand R at the end of BT OOB Record.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAppendMediaBtOobHashCRandR (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                                 char *p_id_str, uint8_t *p_hash_c, uint8_t *p_rand_r)
{
    tNDEF_STATUS    status;
    uint8_t        *p_rec;
    uint8_t         eir_data[BT_OOB_HASH_C_SIZE + BT_OOB_RAND_R_SIZE + 4];
    uint8_t        *p;
    uint8_t         eir_data_len;
    uint32_t        oob_data_len;

    /* find record by Payload ID */
    p_rec = ndef_get_bt_oob_record (p_msg, max_size, p_cur_size, p_id_str);

    if (!p_rec)
        return (NDEF_REC_NOT_FOUND);

    /* create EIR data format */
    p = eir_data;

    uint8_t_TO_STREAM   (p, BT_OOB_HASH_C_SIZE + 1);
    uint8_t_TO_STREAM   (p, BT_EIR_OOB_SSP_HASH_C_TYPE);
    ARRAY16_TO_STREAM (p, p_hash_c);

    uint8_t_TO_STREAM   (p, BT_OOB_RAND_R_SIZE + 1);
    uint8_t_TO_STREAM   (p, BT_EIR_OOB_SSP_RAND_R_TYPE);
    ARRAY16_TO_STREAM (p, p_rand_r);

    eir_data_len = BT_OOB_HASH_C_SIZE + BT_OOB_RAND_R_SIZE + 4;

    /* append EIR data at the end of record */
    status = NDEF_MsgAppendPayload(p_msg, max_size, p_cur_size,
                                   p_rec, eir_data, eir_data_len);

    /* update BT OOB data length, if success */
    if (status == NDEF_OK)
    {
        /* payload length is the same as BT OOB data length */
        p = NDEF_RecGetPayload (p_rec, &oob_data_len);
        uint16_t_TO_STREAM (p, oob_data_len);
    }

    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAppendMediaBtOobEirData
**
** Description      This function appends EIR Data at the end of BT OOB Record.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAppendMediaBtOobEirData (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                              char *p_id_str,
                                              uint8_t eir_type, uint8_t data_len, uint8_t *p_data)
{
    tNDEF_STATUS    status;
    uint8_t        *p_rec;
    uint8_t         eir_data[256];
    uint8_t        *p;
    uint8_t         eir_data_len;
    uint32_t        oob_data_len;

    /* find record by Payload ID */
    p_rec = ndef_get_bt_oob_record (p_msg, max_size, p_cur_size, p_id_str);

    if (!p_rec)
        return (NDEF_REC_NOT_FOUND);

    /* create EIR data format */
    p = eir_data;
    uint8_t_TO_STREAM (p, data_len + 1);
    uint8_t_TO_STREAM (p, eir_type);
    ARRAY_TO_STREAM (p, p_data, data_len);
    eir_data_len = data_len + 2;

    /* append EIR data at the end of record */
    status = NDEF_MsgAppendPayload(p_msg, max_size, p_cur_size,
                                   p_rec, eir_data, eir_data_len);

    /* update BT OOB data length, if success */
    if (status == NDEF_OK)
    {
        /* payload length is the same as BT OOB data length */
        p = NDEF_RecGetPayload (p_rec, &oob_data_len);
        uint16_t_TO_STREAM (p, oob_data_len);
    }

    return (status);
}

/*******************************************************************************
**
** Function         NDEF_MsgAddMediaWifiWsc
**
** Description      This function adds Wifi Wsc Record header.
**
** Returns          NDEF_OK if all OK
**
*******************************************************************************/
tNDEF_STATUS NDEF_MsgAddMediaWifiWsc (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                    char *p_id_str, uint8_t *p_payload, uint32_t payload_len)
{
    tNDEF_STATUS    status;
    uint8_t         id_len = 0;

    if (p_id_str)
        id_len = (uint8_t)strlen (p_id_str);

    status = NDEF_MsgAddRec (p_msg, max_size, p_cur_size,
                             NDEF_TNF_MEDIA, p_wifi_wsc_rec_type, WIFI_WSC_REC_TYPE_LEN,
                             (uint8_t*)p_id_str, id_len, p_payload, payload_len);
    return (status);
}

/*******************************************************************************
**
**              Static Local Functions
**
*******************************************************************************/
/*******************************************************************************
**
** Function         ndef_get_bt_oob_record
**
** Description      This function returns BT OOB record which has matched payload ID
**
** Returns          pointer of record if found, otherwise NULL
**
*******************************************************************************/
static uint8_t *ndef_get_bt_oob_record (uint8_t *p_msg, uint32_t max_size, uint32_t *p_cur_size,
                                      char *p_id_str)
{
    uint8_t  *p_rec, *p_type;
    uint8_t id_len, tnf, type_len;

    /* find record by Payload ID */
    id_len = (uint8_t)strlen (p_id_str);
    p_rec = NDEF_MsgGetFirstRecById (p_msg, (uint8_t*)p_id_str, id_len);

    if (!p_rec)
        return (NULL);

    p_type = NDEF_RecGetType (p_rec, &tnf, &type_len);

    /* check type if this is BT OOB record */
    if ((!p_rec)
      ||(tnf != NDEF_TNF_MEDIA)
      ||(type_len != BT_OOB_REC_TYPE_LEN)
      ||(memcmp (p_type, p_bt_oob_rec_type, BT_OOB_REC_TYPE_LEN)))
    {
        return (NULL);
    }

    return (p_rec);
}


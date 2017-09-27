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
#ifndef BT_TRACE_H
#define BT_TRACE_H
#include <cutils/log.h>

/******************************************************************************
**
** Trace configurable parameters
**
******************************************************************************/

/* Enables or disables verbose trace information. */
#ifndef BT_TRACE_VERBOSE
#define BT_TRACE_VERBOSE FALSE
#endif

/* Enables or disables protocol trace information. */
#ifndef BT_TRACE_PROTOCOL
#define BT_TRACE_PROTOCOL TRUE /* Android requires TRUE */
#endif

/******************************************************************************
**
** Trace Levels
**
** The following values may be used for different levels:
**      BT_TRACE_LEVEL_NONE    0        * No trace messages to be generated
**      BT_TRACE_LEVEL_ERROR   1        * Error condition trace messages
**      BT_TRACE_LEVEL_WARNING 2        * Warning condition trace messages
**      BT_TRACE_LEVEL_API     3        * API traces
**      BT_TRACE_LEVEL_EVENT   4        * Debug messages for events
**      BT_TRACE_LEVEL_DEBUG   5        * Debug messages (general)
******************************************************************************/

/* Core Stack default trace levels */
#ifndef HCI_INITIAL_TRACE_LEVEL
#define HCI_INITIAL_TRACE_LEVEL BT_TRACE_LEVEL_DEBUG
#endif

#ifndef LLCP_INITIAL_TRACE_LEVEL
#define LLCP_INITIAL_TRACE_LEVEL BT_TRACE_LEVEL_DEBUG
#endif

#ifndef APPL_INITIAL_TRACE_LEVEL
#define APPL_INITIAL_TRACE_LEVEL BT_TRACE_LEVEL_DEBUG
#endif

#ifndef NFC_INITIAL_TRACE_LEVEL
#define NFC_INITIAL_TRACE_LEVEL BT_TRACE_LEVEL_DEBUG
#endif

#ifndef SMP_INITIAL_TRACE_LEVEL
#define SMP_INITIAL_TRACE_LEVEL BT_TRACE_LEVEL_DEBUG
#endif

#endif /* BT_TRACE_H */

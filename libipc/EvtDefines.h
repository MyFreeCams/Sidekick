/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <list>
#include <string>
// used for the spy program various grid displays.
// convert the clientlist or message list to array of std strings.
typedef std::list<std::string> StringList;
typedef std::list<StringList> StringListList;


#ifndef MFC_IPC_SHMEMFILE_NAME
#define MFC_IPC_SHMEMFILE_NAME "MFCSharedMemory.mem"
#endif


#ifndef SHARED_MEMNAME
#define SHARED_MEMNAME					"MFCSharedMemory.mem"
#endif

#ifndef CONTAINER_NAME
#define CONTAINER_NAME					"MyBoostMsgList"
#endif

#ifndef CLIENT_CONTAINER_NAME
#define CLIENT_CONTAINER_NAME           "MyClientList"
#endif

#ifndef MFC_SEMA_NAME
#define MFC_SEMA_NAME					"fcsSEMA"
#endif

#ifndef MFC_SHMEM_VERSION
#define MFC_SHMEM_VERSION               "fcsVersion"
#endif

// these are all defined in the cmake file, but give them a default
// incase they get deleted.
#ifndef MFC_SHMEM_POOL_SIZE
#define MFC_SHMEM_POOL_SIZE 1000000
#endif

#ifndef MFC_IPC_RECORD_ACTIVE_INTERVAL_SECS
#define MFC_IPC_RECORD_ACTIVE_INTERVAL_SECS 5
#endif

#ifndef MFC_IPC_INACTIVE_TIMEOUT_SECS
#define MFC_IPC_INACTIVE_TIMEOUT_SECS 600
#endif

#ifndef MFC_IPC_HEARTBEAT_INTERVAL_SECS
#define MFC_IPC_HEARTBEAT_INTERVAL_SECS 120
#endif

#ifndef MFC_IPC_MAINTENANCE_INTERVAL_SECS
#define MFC_IPC_MAINTENANCE_INTERVAL_SECS 90
#endif

#ifndef MFC_IPC_ROUTER_SLEEP_SECS
#define MFC_IPC_ROUTER_SLEEP_SECS 60
#endif

#ifndef MFC_IPC_EVENT_DELETE_AGE_SECS
#define MFC_IPC_EVENT_DELETE_AGE_SECS 0
#endif


#ifndef MFC_MAX_EVENT_QUE_SIZE
#define MFC_MAX_EVENT_QUE_SIZE 200
#endif

#ifndef MFC_MAX_PROCESS_QUE_SIZE
#define MFC_MAX_PROCESS_QUE_SIZE 20
#endif

#ifndef MFC_SHMEM_VERSION
#define MFC_SHMEM_VERSION "1.0.0.0"
#endif

#ifndef MFC_SHMEM_POOL_SIZE
#define MFC_SHMEM_POOL_SIZE 524287
#endif

#ifndef MFC_SHMEM_EVENT_SUBSCRIBE_MAX
#define MFC_SHMEM_EVENT_SUBSCRIBE_MAX 20
#endif


#define MFC_MAINTENANCE_MUTEX           "fcsMaintenanceMutex"

// size of the message payload
#ifndef EVT_PAYLOAD_SIZE
#define EVT_PAYLOAD_SIZE						1024
#endif

// size of client id fields.
#ifndef ADDR_BUF_SIZE
#define ADDR_BUF_SIZE					32
#endif

#ifndef TOPIC_BUF_SIZE
#define TOPIC_BUF_SIZE                  32
#endif

#ifndef MAX_CLIENTS
#define MAX_CLIENTS                     10
#endif

#ifndef SIZE_OF_TIME_T
#define SIZE_OF_TIME_T 12
#endif

#ifndef READ_BY_BUFFER_SIZE
#define READ_BY_BUFFER_SIZE              ( (ADDR_BUF_SIZE + SIZE_OF_TIME_T) * MAX_CLIENTS)
#endif

#ifndef MAINT_TO
#define MAINT_TO "System"
#endif

#ifndef MAINT_FROM
#define MAINT_FROM "Maint"
#endif

#ifndef MAINT_TOPIC
#define MAINT_TOPIC "Maintenance"
#endif

#ifndef MAINT_STATUS
#define MAINT_STATUS "Maint Status"
#endif


#ifndef SHMEM_STATUS_TOPIC
#define SHMEM_STATUS_TOPIC "Shmem Status"
#endif

#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif

#ifndef SYSTEM_EVT_LIFESPAN
// Expected format for string is "[-]h[h][:mm][:ss][.fff]".
// A negative duration will be created if the first character in string is a '-',
// all other '-' will be treated as delimiters. Accepted delimiters are "-:,.".
#define SYSTEM_EVT_LIFESPAN "23:00:00"
#endif

typedef enum {
  EVT_NOT_IN_USE = 0
  , EVT_GENERIC
  , EVT_TIMED
  , EVT_POST_IT
  ,MSG_TYPE_PING
  ,MSG_TYPE_START
  ,MSG_TYPE_SHUTDOWN
  ,MSG_TYPE_DOLOGIN
  ,MSG_TYPE_LOGIN_DENY
  ,MSG_TYPE_LOGIN_AUTH
  ,MSG_TYPE_SET_MSK
  ,MSG_TYPE_DOCREDENTIALS
  ,MSG_TYPE_LOG
  , EVT_SYSTEM = 9000
  , EVT_MAINTENANCE_START
  , EVT_MAINTENANCE_END
  , EVT_MAINTENANCE_STATUS
  , EVT_HEARTBEAT
} EVT_TYPE;

namespace MFCIPC
{
typedef enum {
    EVT_READ_ONCE = 0
    , EVT_READ_HOURLY
    , EVT_READ_DAILY
    , EVT_READ_LOGIN

} EVT_READ_FREQ;

std::string getMessageTypeName(int n);



#define LOG_EVT(msg, evt) \
_TRACE("EVENT: %s Key: %3d Topic: %-10s From: %-10s To: %-10s Type: %-10s Topic: %-10s Read: %1s  Expired: %1s ReadBy: %s expire: %-8s Payload %s"\
, msg\
, evt.getKey()\
, evt.getTopic()\
, evt.getFrom()\
, evt.getTo()\
, getMessageTypeName(evt.getType()).c_str()\
, evt.getTopic()\
, evt.isRead() ? "Y" : "N"\
, evt.isExpired() ? "Y" : "N"\
, evt.getReadBy().c_str() \
, boost::posix_time::to_simple_string(evt.getExpirationDate()).c_str()\
, evt.getPayload()\
)

#define LOG_PROCESS(msg, evt) \
_TRACE("PROCESS: %s Key: %d ID: %s Checkin %s Update: %s Register: %s MutexName: %s"\
, msg\
, evt.getKey()\
, evt.getID()\
, boost::posix_time::to_simple_string(evt.getLastCheckinTime()).c_str()\
, boost::posix_time::to_simple_string(evt.getLastUpdateTime()).c_str()\
, boost::posix_time::to_simple_string(evt.getRegisterTime()).c_str()\
, evt.getMutexName()\
)

}

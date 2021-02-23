/*
 * Copyright (c) 2013-2020 MFCXY, Inc. <mfcxy@mfcxy.com>
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

#include "ShmemDefines.h"
#include "ShmemVector.h"
#include "IPCEvent.h"
#include "ShmemSignal.h"

class CShmemManager;
class CSimpleEventHandler;
class CRouter;
class CIPCEventList;
class CProcessRecord;

namespace MFCIPC
{




//---------------------------------------------------------------------
// CShmemManager
//
// shared memory manager.
class CShmemManager
{
    friend class CRouter;
public:
    CShmemManager();
    virtual ~CShmemManager();

    // removes all shared mem objects.  Really just for development.
    static bool reset();

    boost::interprocess::managed_shared_memory * getSegment() { return m_pSegment; }

    static CShmemManager &getInstance();

    ShmemVector<CIPCEvent> &getEventsVectors() { return m_events; }
    ShmemVector<CProcessRecord>&getProcessVectors() { return m_processes; }
    ShmemSignal<ShmemMutex> &getMaintenanceMutex() { return m_mutMaintenance; }
    ShmemStringContainer &getVersionString() { return m_shmemStringVersion; }

    void logAllEvents(const char *);
    // the over all memory block.
    boost::interprocess::managed_shared_memory*	m_pSegment;

protected:
    // Initialize the shared memory objects.
    virtual bool init();

private:

    // Init the shared memory block
    bool initShmemSegment(bool bRemoveFirst);

private:



  ShmemVector<CIPCEvent>m_events;                // shared memory events
  ShmemVector<CProcessRecord>m_processes;     // shared memory processes.

  ShmemSignal<ShmemMutex>m_mutMaintenance;    // shared memory maintenance mujtex
  ShmemStringContainer m_shmemStringVersion;  // version string.

};


}


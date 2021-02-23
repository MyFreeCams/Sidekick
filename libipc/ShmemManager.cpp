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

#include "fcslib_string.h"

#include "EvtDefines.h"
#include "IPCUtil.h"
#include "ShmemManager.h"

namespace MFCIPC
{
CShmemManager g_memMan;


//---------------------------------------------------------------------
//
//
CShmemManager::CShmemManager()
: m_events(CONTAINER_NAME, MFC_MAX_EVENT_QUE_SIZE)
, m_processes(CLIENT_CONTAINER_NAME, MFC_MAX_PROCESS_QUE_SIZE)
, m_mutMaintenance(MFC_MAINTENANCE_MUTEX)
, m_shmemStringVersion(MFC_SHMEM_VERSION,MFC_SHMEM_VERSION)
{

}

CShmemManager::~CShmemManager()
{
  //  delete m_pSegment;
    m_pSegment = nullptr;
}

//-----------------------------------------------------------------
// reset
//
// remove the shared memory block.
bool CShmemManager::reset()
{
  //  assert(! g_memMan.m_pSegment);
    boost::interprocess::shared_memory_object::remove(MFC_IPC_SHMEMFILE_NAME);
    boost::interprocess::named_mutex::remove("MFCStartup");
    return true;
}

//-------------------------------------------------------------------
// getInstance
//
// ge the boost shared memory manager
CShmemManager & CShmemManager::getInstance()
{
    if (nullptr == g_memMan.m_pSegment)
    {
        g_memMan.init();
    }
    return g_memMan;
}


boost::interprocess::managed_shared_memory * getShmemManagerInstance()
{
  return CShmemManager::getInstance().getSegment();

}


//-----------------------------------------------------------------
// initShmemSegment
//
// initializse the boost segment manager.
bool CShmemManager::initShmemSegment(bool bRemoveFirst)
{
    bool bRv = true;
    if (bRemoveFirst)
    {
        boost::interprocess::shared_memory_object::remove(MFC_IPC_SHMEMFILE_NAME);
    }

    try
    {
        uint64_t nSize = MFC_SHMEM_POOL_SIZE;
        m_pSegment = new boost::interprocess::managed_shared_memory(boost::interprocess::open_or_create
                , MFC_IPC_SHMEMFILE_NAME
                , nSize);

        assert(m_pSegment);
        if (nullptr == m_pSegment)
        {
            // this should never happen because a failure to create the shared mem block should
            // throw an exception
            bRv = false;
            _TRACE("Failed to create boost shared memory manager.");
        }
    }
    catch (boost::interprocess::interprocess_exception &e)
    {
        // MFCCefLogin Helper has a security issue on Mac!!!
        std::string sError = e.what();
        _TRACE("Error creating shared memory file: %s", sError.c_str());
        m_pSegment = nullptr;
        bRv = false;
    }
    return bRv;
}

//-----------------------------------------------------------------
//
//
bool CShmemManager::init()  // todo: remove parameter
{
    struct mutex_remove
    {
        mutex_remove() { boost::interprocess::named_mutex::remove("MFCStartup"); }
        ~mutex_remove(){ boost::interprocess::named_mutex::remove("MFCStartup"); }
    } remover;

    boost::interprocess::named_mutex mutexStartup(boost::interprocess::open_or_create,"MFCStartup");
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutexStartup);

    bool bRv = false;
    std::string s = SHARED_MEMNAME;

    bool bRemove = false;

    // create or open the shared memory shared memory.
    if (initShmemSegment(bRemove))
    {

        bRv = true;
    }

    //_TRACE("CMemoryManager returned %d", bRv);`
    return bRv;
}

void CShmemManager::logAllEvents(const char *pMsg)
{
  m_events.log(pMsg);
}



}

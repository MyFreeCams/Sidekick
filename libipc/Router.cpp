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

#ifdef _WIN32
#include <stdio.h>
#endif
#include <list>

#include "../libfcs/Log.h"
#include "../libfcs/fcslib_string.h"

// local project files
#include "mfc_ipc.h"

#include "ShmemManager.h"
#include "ShmemVector.h"
#include "ShmemSimpleType.h"
#include "ShmemContainer.h"
#include "ShmemSignal.h"

#include "Router.h"


// boost
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <nlohmann/json.hpp>

using njson = nlohmann::json;
// boost
#include <boost/thread/thread.hpp>

namespace MFCIPC
{
CRouter * CRouter::m_pMsgEngineInstance = nullptr;
std::string CRouter::m_sID;

boost::thread* m_pThread;
static std::string m_sID;
/*
ShmemVector<CIPCEvent>m_events(CONTAINER_NAME, MFC_MAX_EVENT_QUE_SIZE);                // shared memory events
ShmemVector<CProcessRecord>m_processes(CLIENT_CONTAINER_NAME, MFC_MAX_PROCESS_QUE_SIZE);     // shared memory processes.

ShmemSignal<ShmemMutex>m_mutMaintenance(MFC_MAINTENANCE_MUTEX);    // shared memory maintenance mujtex
ShmemStringContainer m_shmemStringVersion(MFC_SHMEM_VERSION,MFC_SHMEM_VERSION);  // version string.
*/

boost::recursive_mutex m_mutexFlags;        // mutex for flag access. (local)*/

//---------------------------------------------------------------------
//
//
CRouter::CRouter()
//: m_psShmemMem(std::make_unique<CShmemManager>())
/*, m_pThread(nullptr)
, m_events(CONTAINER_NAME, MFC_MAX_EVENT_QUE_SIZE)
, m_processes(CLIENT_CONTAINER_NAME, MFC_MAX_PROCESS_QUE_SIZE)
, m_mutMaintenance(MFC_MAINTENANCE_MUTEX)
, m_shmemStringVersion(MFC_SHMEM_VERSION,MFC_SHMEM_VERSION)
 */
{

}

CRouter::~CRouter()
{
    delete m_pThread;
    m_pThread = nullptr;
}

//---------------------------------------------------------------------
// getInstance
//
// get the singleton msg que engine
CRouter *CRouter::getInstance()
{
    if (nullptr == CRouter::m_pMsgEngineInstance)
    {
        CRouter::m_pMsgEngineInstance = new CRouter();
        CShmemManager &mman = CShmemManager::getInstance();
        mman.init(); // todo: necessary?

    }
    return CRouter::m_pMsgEngineInstance;
}

void CRouter::setRouterID(const char *p)
{
    m_sID = p;
}

const std::string &CRouter::getID()
{
    return m_sID;
}

//---------------------------------------------------------------------
// registerEvtHandler
//
void CRouter::registerEvtHandler(CIPCEventHandlerBase *pClient)
{
    m_lstMyEvtHandlers.push_back(pClient);
}

//---------------------------------------------------------------------
// deregisterEvtHandler
//
// todo: is this necessary?  the handler will timeout.
void CRouter::deregisterEvtHandler(CIPCEventHandlerBase *pClient)
{
    m_lstMyEvtHandlers.remove(pClient);

    std::string sID = pClient->getID();
    for(CIPCEventHandlerBase *pC : m_lstMyEvtHandlers)
    {
        std::string s = pC->getID();
        if (sID == s && ! s.empty())
        {
            assert(! "remove didn't remove?");
        }

    }

}


//---------------------------------------------------------------------
// reset
//
// delete and remove the shared memory block.
bool CRouter::reset()
{
    return CShmemManager::reset();
}

//----------------------------------------------------------------------
// getShmemBlockVersion
//
// get the shared mem block version.
std::string CRouter::getShmemBlockVersion()
{
  CShmemManager &man = CShmemManager::getInstance();
   std::string s = man.getVersionString().get();
    return s;

}

//-----------------------------------------------------------------------
// getAllEvents
//
// I know better, give me all the records from the shared memory
// vector.
void CRouter::getAllEvents(CIPCEventList &el)
{
   CShmemManager &man = CShmemManager::getInstance();
   man.getEventsVectors().for_each_all([&el](CIPCEvent &e){
        el.push_back(e);
    });

}

//---------------------------------------------------------------------
//
//
int CRouter::getEventsAsString(bool bAll,StringListList &lstRows)
{
    std::function<void(CIPCEvent &)> func = [&lstRows](CIPCEvent &sc)
    {
        StringList lstCol;
        lstCol.push_back(std::string(sc.getTopic()));
        lstCol.push_back(std::string(sc.getTo()));
        lstCol.push_back(sc.getFrom());
        std::string sTN = getMessageTypeName(sc.getType());
        lstCol.push_back(sTN);
        lstCol.push_back(sc.isInUse() ? "Yes" : "No");
        lstCol.push_back(sc.isRead() ? "Yes" : "No");
        time_t t = boost::posix_time::to_time_t(sc.getCreateDate());
        char buffer[32];
        // Format: Mo, 15.06.2009 20:20:00
        std::tm * ptm = std::localtime(&t);
        std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);
        std::string sT = buffer; //boost::posix_time::to_simple_string(sc.getCreateDate());
        
        lstCol.push_back(sT);
        time_t tExp = boost::posix_time::to_time_t(sc.getExpirationDate());
        std::tm * ptmExp = std::localtime(&tExp);
        std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", ptmExp);
        std::string sTExt = buffer;
        
        lstCol.push_back(sTExt);
        lstCol.push_back(sc.getReadBy());
        lstCol.push_back(sc.getPayload());

        lstRows.push_back(lstCol);
    };

    if (bAll)
    {
      CShmemManager &man = CShmemManager::getInstance();
      man.getEventsVectors().for_each_all(func);
    }
    else
    {
      CShmemManager &man = CShmemManager::getInstance();
      man.getEventsVectors().for_each(func);
    }
    return static_cast<int>(lstRows.size());
}

//---------------------------------------------------------------------
//
//
int CRouter::getProcessesAsString(bool bAll, StringListList &lstRows)
{
    std::function<void(CProcessRecord &)> func = [&lstRows](CProcessRecord &sc)
    {
        StringList lstCol;
        lstCol.push_back(sc.getID());
        try
        {
        lstCol.push_back(boost::posix_time::to_simple_string(sc.getRegisterTime()));
        } catch(...)
        {
            lstCol.push_back("tmRegister is bad");
        }

        lstCol.push_back(boost::posix_time::to_simple_string(sc.getLastCheckinTime()));
        lstCol.push_back(boost::posix_time::to_simple_string(sc.getLastUpdateTime()));
        lstCol.push_back(boost::posix_time::to_simple_string(sc.getLastMaintenanceTime()));
        lstCol.push_back(sc.isActve() ? "Yes" : "No");
        lstCol.push_back(std::string(sc.getMutexName()));
        char buf[10] = { '\0' };
        snprintf(buf,9,"%d",sc.getMaintenanceBid());
        lstCol.push_back(buf);
        lstRows.push_back(lstCol);
    };

    if (bAll)
    {

      CShmemManager &man = CShmemManager::getInstance();
      man.getProcessVectors().for_each_all(func);
    }
    else
    {
      CShmemManager &man = CShmemManager::getInstance();
      man.getProcessVectors().for_each(func);
    }
    return static_cast<int>(lstRows.size());
}

//---------------------------------------------------------------------
// sendEvent
//
// add event to shared memory pool
void CRouter::sendEvent(const char *pTopic, const char *pTo, const char *pFrom, int nType, boost::posix_time::ptime timeExpiration, const char *pFormat, ...)
{

    char buffer[EVT_PAYLOAD_SIZE] = { '\0' };

    va_list args;
    va_start(args, pFormat);
    vsnprintf(buffer,EVT_PAYLOAD_SIZE - 1, pFormat, args);
    va_end(args);
    CIPCEvent evt(pTopic, pTo, pFrom, nType, buffer,timeExpiration);
    sendEvent(evt);
}

//---------------------------------------------------------------------
// sendEvent
//
// add event to shared memory pool
void CRouter::sendEvent(const char *pTopic, const char *pTo, const char *pFrom, int nType, const char *pFormat, ...)
{
    boost::posix_time::ptime timeExpiration = IPCUtil::NowPlusSeconds(360);
    char buffer[EVT_PAYLOAD_SIZE] = { '\0' };

    va_list args;
    va_start(args, pFormat);
    vsnprintf(buffer,EVT_PAYLOAD_SIZE - 1, pFormat, args);
    va_end(args);
    CIPCEvent evt(pTopic, pTo, pFrom, nType, buffer,timeExpiration);
    sendEvent(evt);
}


void CRouter::sendEvent(CIPCEvent &evt, bool bTrigger /* true */)
{

    CShmemManager &man = CShmemManager::getInstance();
    man.getEventsVectors().update_add(evt);
    if (bTrigger)
        triggerProcesses();

}

//---------------------------------------------------------------------
// triggerProcesses
//
// trigger all processes that are waiting for shared memory updates.
void CRouter::triggerProcesses()
{
  CShmemManager &man = CShmemManager::getInstance();
  man.getProcessVectors().for_each([](CProcessRecord &r){
        std::string sID = r.getMutexName();
        _TRACE("posting semaphore %s", sID.c_str());
        ShmemSignal <ShmemSemaphore> sema(sID.c_str());
        sema.post();
    });
}

//---------------------------------------------------------------------
// recordPing
//
// record client ping in shared memory
void CRouter::recordPing()
{

    CProcessRecord r(this->getID().c_str());
    getProcessRecord(getID(), r);
    r.setLastCheckinTime(IPCUtil::Now());
    CShmemManager &man = CShmemManager::getInstance();
    man.getProcessVectors().update(r);


 }

//---------------------------------------------------------------------
// udpate_event
//
// update the matching shared memory event record.
void CRouter::update_event(CIPCEvent &sc)
{
    // if the key is zero, update event will not
    // add or update the event.  It's probably
    // a heartbeat or internal event that isn't
    // stored in the shared mem pool
    if (sc.m_nKey > 0)
    {
      CShmemManager &man = CShmemManager::getInstance();
      man.getEventsVectors().update(sc);
    }
}

//---------------------------------------------------------------------
// remove_event
//
// marked shared memory event record unused.
void CRouter::remove_event(CIPCEvent &sc)
{
        CShmemManager &man = CShmemManager::getInstance();
        man.getEventsVectors().remove(sc.getKey(),[](CIPCEvent &evt){
        evt.m_nKey = 0;
        evt.m_nType = EVT_NOT_IN_USE;
        evt.m_bIsInUse = false;

        evt.clear();
        evt.m_nKey = 0;
        evt.m_nType = 0;
        evt.m_bHasBeenRead = false;

        memset(evt.m_szTopic,'\0', TOPIC_BUF_SIZE);
        memset(evt.m_szTo,'\0', ADDR_BUF_SIZE);
        memset(evt.m_szFrom,'\0',ADDR_BUF_SIZE);
        memset(evt.m_szBuffer,'\0',EVT_PAYLOAD_SIZE);
        evt.m_tmCreateDate = IPCUtil::Now();
        evt.m_tmExpirationDate = IPCUtil::Now();
        evt.m_tmReadDate = boost::posix_time::min_date_time;
        evt.m_tmUpdateTime = boost::posix_time::min_date_time;
        evt.m_bIsInUse = false;

    });
}

//---------------------------------------------------------------------------
// remove_list
//
// mark all events in list as unused
void CRouter::remove_events(std::list<CIPCEvent>&el)
{
        CShmemManager &man = CShmemManager::getInstance();
        man.getEventsVectors().for_each([&el](CIPCEvent &evt){
        for(auto &e : el)
        {
            if (e.getKey() && e.getKey() == evt.getKey())
            {
                evt.m_nKey = 0;
                evt.m_nType = EVT_NOT_IN_USE;
                evt.m_bIsInUse = false;

                evt.clear();
                evt.m_nKey = 0;
                evt.m_nType = 0;
                evt.m_bHasBeenRead = false;

                memset(evt.m_szTopic,'\0', TOPIC_BUF_SIZE);
                memset(evt.m_szTo,'\0', ADDR_BUF_SIZE);
                memset(evt.m_szFrom,'\0',ADDR_BUF_SIZE);
                memset(evt.m_szBuffer,'\0',EVT_PAYLOAD_SIZE);
                evt.m_tmCreateDate = IPCUtil::Now();
                evt.m_tmExpirationDate = IPCUtil::Now();
                evt.m_tmReadDate = boost::posix_time::min_date_time;
                evt.m_tmUpdateTime = boost::posix_time::min_date_time;
                evt.m_bIsInUse = false;
                break;
            }
        }
        _TRACE("Event was removed");
    });
}

//---------------------------------------------------------------------
// processEventsForLocalClients
//
// send local clients any pending events.
bool CRouter::processEventsForLocalClients()
{
    bool bRv = false;
    _TRACE("ProcessEvents for Local clients\r\n\r\n");
    CShmemManager &man = CShmemManager::getInstance();
    man.getEventsVectors().log("all events");
    for(auto &pEvtHandler : m_lstMyEvtHandlers)
    {
        CIPCEventList el;
        std::string s = pEvtHandler->getID();
        el.addPendingEvents(s.c_str());
        el.appendFilters(pEvtHandler->getSubscriptions());
        el.fetch();


        el.log("Events to be sent to local handlers.");


        _TRACE("Processing %d events for Event Handler %s",el.size(),pEvtHandler->getID().c_str());
        if (! el.empty())
        {
            pEvtHandler->postIncomingEvent(el);
            bRv = true;
        }
        if (isExit())
            break;

      }


    return bRv;
    
}

//---------------------------------------------------------------------
// performMaintenance
//
// perform maintenance function unless another process is already doing it.
void CRouter::performMaintenance()
{
    CShmemManager &man = CShmemManager::getInstance();
  ShmemSignal<ShmemMutex> &mutMaint = man.getMaintenanceMutex();
    assert(mutMaint.get());

    if (mutMaint->try_lock())  // if it's already locked, then another process is doing maintenance.
    {
        
        // fetch the maintenance events so we can reuse them.
        CIPCEventList lst;
        try
        {
            lst.addEventType(EVT_MAINTENANCE_START);
            lst.addEventType(EVT_MAINTENANCE_END);
            lst.addEventType(EVT_MAINTENANCE_STATUS);
            lst.fetch();


            int nCnt = 0;


            //---------------------------------------------------------
            // record that this process performed maintenance.
            CProcessRecord r = getProcessRecord();
            r.setLastMaintenanceTime(IPCUtil::Now());
            // todo: add update method to CProcessRecord
            CShmemManager &man = CShmemManager::getInstance();
            man.getProcessVectors().update(r);

            _TRACE("Starting Maintenance");
            CMaintenanceEvent evtMaintStart(MAINT_STATUS
                    , EVT_MAINTENANCE_START
                    , "12:00:00"
                    , "Maintenance started By %d at %s"
                    , getpid()
                    , boost::posix_time::to_simple_string(IPCUtil::Now()).c_str()
                    );


            for(auto &e : lst)
            {
              if (e.getType() == EVT_MAINTENANCE_START)
                evtMaintStart = e;
            }
            evtMaintStart.setPayload("Maintenance started By %d at %s"
                , getpid()
                , boost::posix_time::to_simple_string(IPCUtil::Now()).c_str());
            evtMaintStart.isRead(false);
            evtMaintStart.clearIsRead();
            evtMaintStart.setExpirationDate("24:00:00");
            evtMaintStart.update(false);

            //
            // find all messages that are expired.
            //
            CIPCEventList fbk;
            fbk.addIsExpired(true);
            fbk.fetch();

            fbk.log("These records are going to be removed");
            // send to handlers.
            for(auto &eh : m_lstMyEvtHandlers)
            {
                eh->postRemoveEvent(fbk);
            }
            
            // actually remove the events.
            for(auto &evt : fbk)
            {
                // todo:: does this trigger?
                remove_event(evt);
                nCnt++;
            }
            //
            // send maintenance end record.
            njson sendMaintEnd =
                    {
                            { "process", getpid() },
                            { "time",  boost::posix_time::to_simple_string(IPCUtil::Now()).c_str()  },
                            { "recordsProcessed", nCnt }
                    };


            CMaintenanceEvent evtMaintFinish(MAINT_STATUS
                , EVT_MAINTENANCE_END
                , "12:00:00"
                , sendMaintEnd.dump().c_str()
            );
            // find the maintenance end event.
            for(auto &e : lst)
            {
              if (e.getType() == EVT_MAINTENANCE_END)
                evtMaintFinish = e;
            }
            evtMaintFinish.setPayload(sendMaintEnd.dump().c_str());
            evtMaintFinish.isRead(false);
            evtMaintFinish.clearIsRead();
            evtMaintFinish.setExpirationDate("24:00:00");
            evtMaintFinish.update(false);

             mutMaint->unlock();

        } catch (...) {
          mutMaint->unlock();
            _TRACE("Exception caught!");
        }

        CShmemManager &man = CShmemManager::getInstance();
        njson sendResponse =
                {
                        { "size", CShmemManager::getInstance().getSegment()->get_size() },
                        { "free",  CShmemManager::getInstance().getSegment()->get_free_memory()             },
                        { "EventSize", man.getEventsVectors().getQueSize() }
                };

        CMaintenanceEvent evtShmem(SHMEM_STATUS_TOPIC
            , EVT_MAINTENANCE_STATUS
            , "24:00:00"
            ,sendResponse.dump().c_str()
        );

        for(auto &e : lst)
        {
          if (e.getType() == EVT_MAINTENANCE_STATUS)
            evtShmem = e;
        }
        evtShmem.setPayload(sendResponse.dump().c_str());
        evtShmem.isRead(false);
        evtShmem.clearIsRead();
        evtShmem.setExpirationDate("24:00:00");
        evtShmem.update(false);
        _TRACE("Maintenance completed.");
    }
    else
    {
        _TRACE("Maintenance Skipped couldn't lock mutex");
    }
}

//---------------------------------------------------------------------
// performProcessCheck
//
// look for any newly attached or detached processes
void CRouter::performProcessCheck(std::list<uint64_t>&lstActiveProcesses)
{
    if (! lstActiveProcesses.empty())
    {
        std::list<uint64_t> lst;
         CShmemManager &man = CShmemManager::getInstance();
         man.getProcessVectors().for_each([&](CProcessRecord &r)
         {
             bool bFnd = false;
             for (auto k : lstActiveProcesses)
             {
                 if (k == r.getKey())
                 {
                     bFnd = true;
                     if (r.isActve())
                     {
                         lst.push_back(r.getKey());
                     }
                     else
                     {
                         for (auto eh : m_lstMyEvtHandlers)
                         {
                             eh->postDetachProcess(r);
                         }
                     }

                 }
             }

             if (!bFnd)
             {
                 for (auto eh : m_lstMyEvtHandlers)
                 {
                     eh->postAttachProcess(r);
                 }
             }

         });

        lstActiveProcesses.clear();
        for (auto k : lst)
            lstActiveProcesses.push_back(k);

    }
}


//---------------------------------------------------------------------
// getProcessRecord
//
// get the process record.
bool CRouter::getProcessRecord(std::string sID, CProcessRecord &r)
{
    bool bFnd = false;
      CShmemManager &man = CShmemManager::getInstance();
      man.getProcessVectors().for_each([&sID,&r, &bFnd](CProcessRecord &sc){
       if (sID == sc.getID())
       {
           r = sc;
           LOG_PROCESS("Found!",r);
           bFnd = true;
       }

    });
    return bFnd;
}

//---------------------------------------------------------------------
// getProcessRecord
//
// get my process record.
CProcessRecord CRouter::getProcessRecord()
{
    CProcessRecord r;
    std::string s = getID();
    bool b = getProcessRecord(s,r);
    assert(b);
    return r;
}

//--------------------------------------------------------------------
// getRegisterTime
//
// time that the router was started.
boost::posix_time::ptime CRouter::getRegisterTime()
{
    CProcessRecord r = getProcessRecord();
    return r.getRegisterTime();
}

//---------------------------------------------------------------------
// isMaintenanceProcess
//
// return true if this process should perform maintenance
bool CRouter::isMaintenaceProcess()
{
    int nMaxBid = 0;
    std::string sProcess;
    CShmemManager &man = CShmemManager::getInstance();
    man.getProcessVectors().for_each([&nMaxBid,&sProcess](CProcessRecord &pr){
        if (pr.isActve())
        {
            if (pr.getMaintenanceBid() > nMaxBid )
            {
                nMaxBid = pr.getMaintenanceBid();
                sProcess = pr.getID();
            }
        }
    });

    _TRACE("Highest maintenance bid is %d", nMaxBid);
    if (sProcess == getID())
    {
        _TRACE("Process %s is the maintenance process",getID().c_str());
        return true;
    }
    return false;

}

//---------------------------------------------------------------------
// TriggerRouterMutex
//
// Wake the router thread.
void CRouter::TriggerRouterMutex()
{

    CProcessRecord r(getID().c_str());
    getProcessRecord(getID(), r);
    std::string sMutexName = r.getMutexName();
    ShmemSignal <ShmemSemaphore> sema(sMutexName.c_str());
    sema.post();
}

//-------------------------------------------------------------------
// TriggerRouterMaintenance
//
// force router maintenance.
void CRouter::TriggerRouterMaintenance()
{
    performMaintenance();
}

//-------------------------------------------------------------------
// TriggerProcessClientEvents
//
// force router to process local client.
void CRouter::TriggerProcessClientEvents()
{
    processEventsForLocalClients();
}

//---------------------------------------------------------------------
// Process
//
// Main processing loop for the back ground thread.
void CRouter::Process()
{

    boost::posix_time::ptime tmNow, tmLastPing, tmLastHeartbeat, tmLastMaintenance;
    tmNow = tmLastPing = tmLastHeartbeat =  tmLastMaintenance = boost::posix_time::min_date_time;
  
    
    CProcessRecord r(getID().c_str());
    getProcessRecord(getID(), r);
    std::string sMutexName = r.getMutexName();
    m_sSemaName = sMutexName;
    ShmemSignal <ShmemSemaphore> sema(sMutexName.c_str());

    std::list<uint64_t>lstActiveProcesses;
    CShmemManager &man = CShmemManager::getInstance();
    man.getProcessVectors().for_each([&lstActiveProcesses](CProcessRecord &r){
        lstActiveProcesses.push_back(r.getKey());
    });

    while (! isExit())
    {

        isReady(true);

        bool b = sema.timed_wait(MFC_IPC_ROUTER_SLEEP_SECS);

        
        if (!isExit())
        {
            tmNow = IPCUtil::Now();


            boost::posix_time::time_duration tmDiff = tmNow - tmLastPing;
            //
            // record that we are still alive and send heart beat to
            // our clients
            //
            if (tmDiff.total_seconds() >= MFC_IPC_RECORD_ACTIVE_INTERVAL_SECS)
            {
                recordPing();
                tmLastPing = IPCUtil::Now();
            }


            // check for newly attached or detached processes.
            performProcessCheck(lstActiveProcesses);
 

            tmDiff = tmNow - tmLastMaintenance;
            //
            // send any pending messages to local clients.
            //
            if (tmDiff.total_seconds() >= MFC_IPC_MAINTENANCE_INTERVAL_SECS && !isManualMaintenance())
            {
                if (isExit())
                    continue;

                _TRACE("Maintenance timer.  It's been %d seconds", tmDiff.total_seconds());
                if (isMaintenaceProcess())
                {
                    //
                    // Perform maintenance
                    //
                    performMaintenance();

                }
                else
                {
                    _TRACE("This process is NOT the maintenance!");
                }
                tmLastMaintenance = IPCUtil::Now();
            }

            tmDiff = tmNow - tmLastHeartbeat;

            if (isExit())
                continue;

            if (! isManualProcessEvents())
            {
                if (! processEventsForLocalClients())
                {

                    if (!isManualMaintenance())
                    {
                        _TRACE("Events were processed, call maintenance");
                        performMaintenance();
                        tmLastMaintenance = IPCUtil::Now();
                    }


                }
            }

        }

    }
    isReady(false);
    printf("process thread ending");
}

//---------------------------------------------------------------------
// startProcessThread
//
// static callback for thread create.
void *CRouter::startProcessThread(void* pCtx)
{
    auto pThis = (CRouter*)pCtx;
    pThis->Process();
    return nullptr;
}



//---------------------------------------------------------------------
// start
//
// start the background thread.
bool CRouter::start(int nMaintenanceBid)
{

    assert(strlen(getID().c_str()) > 0);
    
    CProcessRecord r(getID().c_str());
    getProcessRecord(getID(), r);
    r.setLastCheckinTime(IPCUtil::Now());
    r.setRegisterTime(IPCUtil::Now());
    r.setLastUpdateTime(IPCUtil::Now());
    r.setMaintenanceBid(nMaintenanceBid);
    
    std::string sID = r.getMutexName();
    if (sID.empty())
    {
        sID = r.createMutexName();
        r.setMutexName(sID);
    }
    assert(! sID.empty());
    r.isInUse(true);

    CShmemManager &man = CShmemManager::getInstance();
    man.getProcessVectors().update_add(r);


    //pthread_create(&m_thread, nullptr, CRouter::startProcessThread, this);
    m_pThread = new boost::thread(CRouter::startProcessThread, this);
    isStarted(true);
    assert(isStarted());
    return true;

}

void CRouter::stop()
{
    isExit(true);

    CProcessRecord r = getProcessRecord();
    getProcessRecord(getID(), r);
    std::string sMutexName = r.getMutexName();
    ShmemSignal <ShmemSemaphore> sema(sMutexName.c_str());
    sema.post();
    assert(m_pThread);
    m_pThread->join();
    isReady(false);
    isStarted(false);
}

//---------------------------------------------------------------------
// isReady
//
// true if the router is ready.  Some of the unit tests hit
// the router very quickly and it's not quite ready to process
// maintenance or messages yet.
bool CRouter::isReady()
{
    boost::lock_guard lock(m_mutexFlags);
    return m_bIsReady;
}

void CRouter::isReady(bool b)
{
    boost::lock_guard lock(m_mutexFlags);
    m_bIsReady = b;
}

bool CRouter::isExit()
{
    boost::lock_guard lock(m_mutexFlags);
    return m_bExit;
}

void CRouter::isExit(bool b)
{
    boost::lock_guard lock(m_mutexFlags);
    m_bExit = b;
}

bool CRouter::isStarted()
{
    boost::lock_guard lock(m_mutexFlags);
    return m_bIsStarted;
}

void CRouter::isStarted(bool b)
{
    boost::lock_guard lock(m_mutexFlags);
    m_bIsStarted = b;
}

std::string CRouter::getVersion()
{
  CShmemManager &man = CShmemManager::getInstance();
  std::string s = man.getVersionString().get();
  return s;
}

#ifdef WIN32_XXXXX
void proxy_blog(int nLevel, const char* pszMsg)
{
    // blog(nLevel, "%s", pszMsg);
}
#endif
}
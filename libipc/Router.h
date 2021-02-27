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

#ifndef ROUTER_H_
#define ROUTER_H_

// local files
#include "mfc_ipc.h"

namespace MFCIPC
{

class CSimpleEventHandler;
class CIPCEventList;
class CProcessRecord;
class CIPCEventHandlerBase;

//---------------------------------------------------------------------
// CRouter
//
// Router handles processing messages for current process.  Singleton
// object.
class CRouter
{
protected:
    CRouter();
    virtual ~CRouter();

public:
    // singleton, no compiler generated assignments.
    CRouter(CRouter&) = delete;
    CRouter(const CRouter&) = delete;
    const CRouter& operator=(const CRouter&) = delete;

    // singleton Router Factory
    static CRouter* getInstance();


    //*****************************************************************
    // startup and shutdown functions
    // start the background thread.
    virtual bool start(int nMaintenanceBid = 5);
    virtual void stop();

    static bool reset();
    // get the block version string
    std::string getShmemBlockVersion();


    //*****************************************************************
    // Message IO functions
    //*****************************************************************

    // send a message

    //*****************************************************************
    // CLient functions.
    //*****************************************************************
    void registerEvtHandler(CIPCEventHandlerBase*);
    void deregisterEvtHandler(CIPCEventHandlerBase*);

    //---------------------------------------------------------------------
    // isStarted
    //
    // router started flag.
    bool isStarted() ;
    void isStarted(bool b);

    //---------------------------------------------------------------------
    // isExit
    //
    // exit flag.
    bool isExit() ;
    void isExit(bool b) ;

    //---------------------------------------------------------------------
    // getMaintenanceBid
    //
    // access member for maintenance bid.
    int getMaintenanceBid() const { return m_nMaintenanceBid; }
    void getMaintenanceBid(int n) { m_nMaintenanceBid = n; }

    //---------------------------------------------------------------------
    // getEvetnAsString
    //
    // get the list of events as a list of std::strings.
    int getEventsAsString(bool bAll, StringListList& lstRows);
    int getProcessesAsString(bool bAll, StringListList& lstRows);
    void getAllEvents(CIPCEventList &);

    //---------------------------------------------------------------------
    // sendEvent
    //
    // add an event to the shared memory pool
    void sendEvent(const char* pTopic, const char* pTo, const char* pFrom,  int nType, boost::posix_time::ptime timeExpiration, const char* pFormat, ...);
    void sendEvent(const char* pTopic, const char* pTo, const char* pFrom,  int nType, const char* pFormat, ...);
    void sendEvent(CIPCEvent& msg, bool bTrigger = true);

    //---------------------------------------------------------------------
    // update_event
    //
    // update existing event.
    void update_event(CIPCEvent& sc);

    //---------------------------------------------------------------------
    // remove_event
    //
    // mark event records as unused.
    void remove_event(CIPCEvent& sc);
    void remove_events(std::list<CIPCEvent>&);

    //---------------------------------------------------------------------
    // ID
    //
    // access member for router id, which is really the process id.
    static void setRouterID(const char* p);
    const std::string& getID();

    //---------------------------------------------------------------------
    // debugging function used to manually trigger operations.
    void triggerProcesses();

    void TriggerRouterMutex(); // debug only
    void TriggerRouterMaintenance(); // debug only
    void TriggerProcessClientEvents(); // debug only

    bool isManualMaintenance() { return m_bManMaintenance; }
    void isManualMaintenance(bool b) { m_bManMaintenance = b;}

    bool isManualProcessEvents() { return m_bProcessLocal;; }
    void isManualProcessEvents(bool b) { m_bProcessLocal = b; }


    //---------------------------------------------------------------------
    // getProcessRecord
    //
    // get this events process record.
    bool getProcessRecord(std::string sID, CProcessRecord& r);
    CProcessRecord getProcessRecord();

    //--------------------------------------------------------------------
    // getRegisterTime
    //
    // time that the router was started.
    boost::posix_time::ptime getRegisterTime();

    //---------------------------------------------------------------------
    // getVersion
    //
    // get shared memory version.
    std::string getVersion();

    //---------------------------------------------------------------------
    // isReady
    //
    // true if router is ready.
    bool isReady() ;
    void isReady(bool b);

    //ShmemVector<CIPCEvent>&getEventsVectors();
    //ShmemVector<CProcessRecord>&getProcessVectors() { return m_processes; }

protected:
    //boost::recursive_mutex& getReadyMutex() { return m_mutexFlags;}

    //---------------------------------------------------------------------
    // processEventsForLocalClients
    //
    // process events for all local event handlers.
    bool processEventsForLocalClients();

    //---------------------------------------------------------------------
    // performMaintenance
    //
    // perform maintenance fuinctions.
    void performMaintenance();

    //---------------------------------------------------------------------
    // performNewProcessCheck
    //
    // look for any new processes that have been added
    void performProcessCheck(std::list<uint64_t>&);

    static void* startProcessThread(void* pCtx);

    //---------------------------------------------------------------------
    // Process
    //
    // main thread loop.
    void Process();

    void recordPing();
    bool isMaintenaceProcess();

private:
    std::list<CIPCEventHandlerBase*>m_lstMyEvtHandlers;
    static std::string m_sID;

    std::string m_sSemaName;                    // name for the shared memory semaphore used to trigger.

    static CRouter* m_pMsgEngineInstance;       // our instance
    int m_nMaintenanceBid;                      // maintenanc ebid.

    bool m_bProcessLocal = false;
    bool m_bManMaintenance = false;
    bool m_bIsReady = false;
    bool m_bExit = false;
    bool m_bIsStarted = false;
};

typedef boost::shared_ptr<CRouter> CMsgQueEngine_ptr;

}

#endif  // ROUTER_H_

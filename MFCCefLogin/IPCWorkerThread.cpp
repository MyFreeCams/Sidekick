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
#include <string>
#include <cstdlib> //std::system

#ifdef _WIN32
#include <malloc.h>
#endif

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <list>

// cef include files
// #include "include/cef_app.h"

// boost includes
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>


// solution includes
#include <libcef_fcs/cefLogin_app.h>
#include <libcef_fcs/cefEventHandler.h>
#include <libPlugins/IPCShared.h>
#include <libPlugins/Portable.h>
#include <libfcs/Log.h>
#include <libfcs/fcslib_string.h>

// ipclib includes
#include <mfc_ipc.h>
#ifdef _USE_OLD_MEMMANAGER
#include "IPCSemaphore.h"
#endif

// project includes
#include "IPCWorkerThread.h"

#define MYADDRESS ADDR_FCSLOGIN
#define PARTNER_ADDRESS ADDR_OBS_BROADCAST_Plugin

#ifdef _USE_OLD_MEMMANAGER
CSemaphore g_semaClient;
#endif

CIPCWorkerEventHandler::CIPCWorkerEventHandler(const char *p, MFC_LOGIN_APP &app)
    : _myBase(p)
    , m_App(app)
    , m_spMutexFlags(new boost::mutex)
    , m_spMutexApp(new boost::mutex)
{

}

CIPCWorkerEventHandler::~CIPCWorkerEventHandler()
{}

//---------------------------------------------------------------------
// onAttachPRocess
//
//
void CIPCWorkerEventHandler::onAttachProcess(MFCIPC::CProcessRecord&)
{

}

//---------------------------------------------------------------------
// OnDetachProcess
//
//
void CIPCWorkerEventHandler::onDetachProcess(MFCIPC::CProcessRecord& r)
{
    _TRACE("Process detach! %s",r.getID());
    closeAllBrowsers();

}

//---------------------------------------------------------------------
// onIncoming
//
// handle incoming events
void CIPCWorkerEventHandler::onIncoming(MFCIPC::CIPCEvent& msg)
{
       CefRefPtr<CefBrowser> browser = this->getBrowser();
       MFCIPC::CRouter *pR = MFCIPC::CRouter::getInstance();

        switch (msg.getType())
         {
            case MSG_TYPE_PING:
                {
                _TRACE("Ping Message  Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(), msg.getFrom(), msg.getPayload());
                break;
                }
            case MSG_TYPE_START:
                //_TRACE("MSG_TYPE_START Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(), msg.getFrom(), msg.getPayload());
                break;

            // shutdown message is sent by broadcast plugin when it shuts down. 
            case MSG_TYPE_SHUTDOWN:
            {
                _TRACE("MSG_TYPE_SHUTDOWN Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(),msg.getFrom(), msg.getPayload());
                //pR->sendEvent("Shutdown",ADDR_OBS_BROADCAST_Plugin, MYADDRESS, MSG_TYPE_SHUTDOWN, stdprintf("%u",_getpid()).c_str());
                this->closeAllBrowsers();
                
                break;
            }

            default:
                _TRACE("unhandled message Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(),msg.getFrom(), msg.getPayload());
                break;
            
         } // end switch

}

//---------------------------------------------------------------------
// getBrowser
//
// helper function to get the cef browser object.
CefRefPtr<CefBrowser> CIPCWorkerEventHandler::getBrowser()
{
    boost::lock_guard lock(*m_spMutexApp);
    CefRefPtr<cefEventHandler> pEvtHandler = m_App.getHandler();
    CefRefPtr<CefBrowser> browser = pEvtHandler->getBrowser();
    return browser;
}


//---------------------------------------------------------------------
// closeAllBrowsers
//
// close all browser windows.
void CIPCWorkerEventHandler::closeAllBrowsers()
{
    boost::lock_guard lock(*m_spMutexApp);
    m_App.getHandler()->CloseAllBrowsers(false);
}

#ifdef _USE_OLD_MEMMANAGER
//---------------------------------------------------------------------
// CIPCWorkerThread
//
// thread to handle all communication with broadcast plugin.
//
CIPCWorkerThread::CIPCWorkerThread(MFC_LOGIN_APP &app)
    : m_pThread(NULL)
    , m_spMutexFlags(new boost::mutex)
    , m_spMutexApp(new boost::mutex)
    , m_spSemaClient(new CSemaphore)
    , m_App(app)
    , m_bStop(false)
{

}

CIPCWorkerThread::~CIPCWorkerThread()
{
}

bool CIPCWorkerThread::init()
{
    CRouter::setRouterID("MFCCefLogin");
    //
    // start router with a low maintenance bid
    // the lower the maintenance bid, the less likely
    // it is for this instance to do messagse maintenance. 
    return CRouter::getInstance()->start(5);
//   return getSharedMemManager().init(false);
}



//---------------------------------------------------------------------
// getBrowser
//
// helper function to get the cef browser object.
CefRefPtr<CefBrowser> CIPCWorkerThread::getBrowser()
{
    boost::lock_guard lock(*m_spMutexApp);
    CefRefPtr<cefEventHandler> pEvtHandler = m_App.getHandler();
    CefRefPtr<CefBrowser> browser = pEvtHandler->getBrowser();
    return browser;
}

//---------------------------------------------------------------------------
// CIPCWorkerThread::Process()
//
// this code executes in the CEF Login app browser exe.  Not the renderer exe
int CIPCWorkerThread::Process()
{
    int nSleepInterval = 100;
#ifdef _DEBUG
    int nPingCnt = 0;
    int nSendCnt = 0;
    int nRecCnt = 0;
#endif
    // send a startup msg to broadcast plugin with our PID
    //getSharedMemManager().sendMessage(ADDR_OBS_BROADCAST_Plugin, MYADDRESS, MSG_TYPE_START, stdprintf("%u", _getpid()).c_str());
    CRouter *pR = CRouter::getInstance();
    pR->sendEvent("startup",ADDR_OBS_BROADCAST_Plugin,MYADDRESS, MSG_TYPE_START,"%u",_getpid());

    CSimpleEventHandler myHandler(MYADDRESS);
    myHandler.setIncomingFunc([&pR,this](CIPCEvent &msg){
        CefRefPtr<CefBrowser> browser = this->getBrowser();

        switch (msg.getType())
         {
            case MSG_TYPE_PING:
                {
                _TRACE("Ping Message  Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(), msg.getFrom(), msg.getPayload());
                break;
                }
            case MSG_TYPE_START:
                //_TRACE("MSG_TYPE_START Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(), msg.getFrom(), msg.getPayload());
                break;

            case MSG_TYPE_SHUTDOWN:
            {
                _TRACE("MSG_TYPE_SHUTDOWN Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(),msg.getFrom(), msg.getPayload());
                    
                //getSharedMemManager().sendMessage( MFC_Shared_Mem::CSharedMemMsg( ADDR_OBS_BROADCAST_Plugin, MYADDRESS, MSG_TYPE_SHUTDOWN, stdprintf("%u",_getpid()).c_str()));
                pR->sendEvent("Shutdown",ADDR_OBS_BROADCAST_Plugin, MYADDRESS, MSG_TYPE_SHUTDOWN, stdprintf("%u",_getpid()).c_str());
                this->setStopFlag(true);
                g_semaClient.post();
                this->closeAllBrowsers();
                
                break;
            }
            case MSG_TYPE_DOLOGIN:
            {
                _TRACE("MSG_TYPE_DOLOGIN Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(),msg.getFrom(), msg.getPayload());
                // we should not get this message
                //assert(!"Miss directed message, this should go from cef renderer directly to the broadcast plugin");
                break;
            }
            case MSG_TYPE_LOGIN_DENY:
            {
                /*
                _TRACE("MSG_TYPE_LOGIN_DENY Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(),
                        msg.getFrom(), msg.getPayload());
                // todo:  Wrap CefProcessMessage.
                // message are caught by the renderer process in CMFCCefApp
                CefRefPtr<CefProcessMessage> cefmsg = CefProcessMessage::Create("login-deny");
                CefRefPtr<CefListValue> args = cefmsg->GetArgumentList();
                args->SetString(0, "Login Denied");
                std::vector< int64_t > vIds;
                browser->GetFrameIdentifiers(vIds);
                CefRefPtr<CefFrame> frame = browser->GetFrame(vIds[0]);
                frame->SendProcessMessage(PID_RENDERER, cefmsg);
                */
                break;
            }

            case MSG_TYPE_LOGIN_AUTH:
            {
                _TRACE("MSG_TYPE_LOGIN_AUTH Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(),msg.getFrom(), msg.getPayload());
                /*// todo:  Wrap CefProcessMessage.
                CefRefPtr<CefProcessMessage> cefmsg = CefProcessMessage::Create("login-approved");
                CefRefPtr<CefListValue> args = cefmsg->GetArgumentList();
                args->SetString(0, "Login Authorized");
                CefRefPtr<CefBrowser> browser = getBrowser();
                vector< int64_t > vIds;
                browser->GetFrameIdentifiers(vIds);
                CefRefPtr<CefFrame> frame = browser->GetFrame(vIds[0]);
                frame->SendProcessMessage(PID_RENDERER, cefmsg);
                */
                break;
            }
            default:
                _TRACE("unhandled message Type: %d To: %s From: %s Msg: %s", (int) msg.getType(), msg.getTo(),msg.getFrom(), msg.getPayload());
                break;
            
         } // end switch
     }); // end lambda

    //CEvent start(MSG_TYPE_START,ADDR_OBS_BROADCAST_Plugin,MYADDRESS,EVT_GENERIC,boost::posix_time::seconds(360),"%u",_getpid());
    //start.update();

    boost::posix_time::ptime tPing = boost::posix_time::second_clock::local_time();
    while (!isStopFlag())
    {
        int nCnt = 0;
        _TRACE("***start timer");
        nSleepInterval = 600;
        boost::posix_time::ptime t1 =
                boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(nSleepInterval);
        g_semaClient.timed_wait(nSleepInterval);

    }

    return 0;
}

//---------------------------------------------------------------------
// Run
//
// call back method for when the thread starts.
int Run(void *p)
{
    CIPCWorkerThread *pThis = (CIPCWorkerThread *) p;
    return pThis->Process();
}

//---------------------------------------------------------------------
// Start
//
// create the worker thread and run it.
bool CIPCWorkerThread::Start()
{
    m_pThread = new std::thread(Run, this);
//    Process();
    return true;
}

//---------------------------------------------------------------------
// Stop
//
// stop the worker thread but don't wait for it to exit
bool CIPCWorkerThread::Stop()
{
    _TRACE("Stopping worker thread");
    setStopFlag(true);
    g_semaClient.post();
    //getSharedMemManager().getClientMutex()->unlock();
    return true;
}

//---------------------------------------------------------------------
// End
//
// Stop the worker thread and wait for it to exit
bool CIPCWorkerThread::End()
{
    _TRACE("Ending worker thread");
    setStopFlag(true);
    g_semaClient.post();
    //clientMutex.unlock();
    m_pThread->join();

    return true;
}

void CIPCWorkerThread::closeAllBrowsers()
{
    boost::lock_guard lock(*m_spMutexApp);
    m_App.getHandler()->CloseAllBrowsers(false);
}

bool CIPCWorkerThread::isStopFlag()
{
      boost::lock_guard lock(*m_spMutexFlags);
      bool b = m_bStop;
      return b;
}

void CIPCWorkerThread::setStopFlag(bool b)
{
    boost::lock_guard lock(*m_spMutexFlags);
    m_bStop = b;
}


#endif

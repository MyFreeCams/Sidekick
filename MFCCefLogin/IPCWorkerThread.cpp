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

#include "boost/date_time/posix_time/posix_time.hpp"

// solution includes
#include <libcef_fcs/cefLogin_app.h>
#include <libcef_fcs/cefEventHandler.h>
#include <libPlugins/IPCShared.h>
#include <libPlugins/Portable.h>
#include <libfcs/Log.h>
#include <libfcs/fcslib_string.h>

// project includes
#include "IPCWorkerThread.h"

using namespace MFC_Shared_Mem;

//---------------------------------------------------------------------
// CIPCWorkerThread
//
// thread to handle all communication with broadcast plugin.
//
CIPCWorkerThread::CIPCWorkerThread(MFC_LOGIN_APP &app)
    : m_pThread(NULL), m_App(app), m_bStop(false)
{}

CIPCWorkerThread::~CIPCWorkerThread()
{}

bool CIPCWorkerThread::init()
{
   return getSharedMemManager().init(false);
}

#define MYADDRESS ADDR_FCSLOGIN
#define PARTNER_ADDRESS ADDR_OBS_BROADCAST_Plugin

//---------------------------------------------------------------------
// getBrowser
//
// helper function to get the cef browser object.
CefRefPtr<CefBrowser> CIPCWorkerThread::getBrowser()
{
    CefRefPtr<cefEventHandler> pEvtHandler = getCefApp().getHandler();
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
    getSharedMemManager().sendMessage(ADDR_OBS_BROADCAST_Plugin, MYADDRESS, MSG_TYPE_START, stdprintf("%u", _getpid()).c_str());

    boost::posix_time::ptime tPing = boost::posix_time::second_clock::local_time();
    while (!isStopFlag())
    {
        int nCnt = 0;
        // get all messages that are addressed to this process
        // these messages were probably sent by the broadcast plugin.
        CSharedMemMsg msg;
        while (getSharedMemManager().getNextMessage(&msg, ADDR_FCSLOGIN))
        {
            nCnt++;
            switch (msg.getID())
            {
            case MSG_TYPE_PING:
                _TRACE("Ping Message  Type: %d To: %s From: %s Msg: %s", (int) msg.getID(), msg.getTo(), msg.getFrom(), msg.getMessage());
                break;

            case MSG_TYPE_START:
                //_TRACE("MSG_TYPE_START Type: %d To: %s From: %s Msg: %s", (int) msg.getID(), msg.getTo(), msg.getFrom(), msg.getMessage());
                break;

            case MSG_TYPE_SHUTDOWN:
            {
                _TRACE("MSG_TYPE_SHUTDOWN Type: %d To: %s From: %s Msg: %s", (int) msg.getID(), msg.getTo(),msg.getFrom(), msg.getMessage());
                //getSharedMemManager().sendMessage( MFC_Shared_Mem::CSharedMemMsg( ADDR_OBS_BROADCAST_Plugin, MYADDRESS, MSG_TYPE_SHUTDOWN, stdprintf("%u",_getpid()).c_str()));
                getCefApp().getHandler()->CloseAllBrowsers(false);
                break;
            }
            case MSG_TYPE_DOLOGIN:
            {
                _TRACE("MSG_TYPE_DOLOGIN Type: %d To: %s From: %s Msg: %s", (int) msg.getID(), msg.getTo(),msg.getFrom(), msg.getMessage());
                // we should not get this message
                //assert(!"Miss directed message, this should go from cef renderer directly to the broadcast plugin");
                break;
            }
            case MSG_TYPE_LOGIN_DENY:
            {
                _TRACE("MSG_TYPE_LOGIN_DENY Type: %d To: %s From: %s Msg: %s", (int) msg.getID(), msg.getTo(),
                        msg.getFrom(), msg.getMessage());
                // todo:  Wrap CefProcessMessage.
                // message are caught by the renderer process in CMFCCefApp
                CefRefPtr<CefProcessMessage> cefmsg = CefProcessMessage::Create("login-deny");
                CefRefPtr<CefListValue> args = cefmsg->GetArgumentList();
                args->SetString(0, "Login Denied");
                CefRefPtr<CefBrowser> browser = getBrowser();
                vector< int64_t > vIds;
                browser->GetFrameIdentifiers(vIds);
                CefRefPtr<CefFrame> frame = browser->GetFrame(vIds[0]);
                frame->SendProcessMessage(PID_RENDERER, cefmsg);
                break;
            }

            case MSG_TYPE_LOGIN_AUTH:
            {
                _TRACE("MSG_TYPE_LOGIN_AUTH Type: %d To: %s From: %s Msg: %s", (int) msg.getID(), msg.getTo(),msg.getFrom(), msg.getMessage());
                // todo:  Wrap CefProcessMessage.
                CefRefPtr<CefProcessMessage> cefmsg = CefProcessMessage::Create("login-approved");
                CefRefPtr<CefListValue> args = cefmsg->GetArgumentList();
                args->SetString(0, "Login Authorized");
                CefRefPtr<CefBrowser> browser = getBrowser();
                vector< int64_t > vIds;
                browser->GetFrameIdentifiers(vIds);
                CefRefPtr<CefFrame> frame = browser->GetFrame(vIds[0]);
                frame->SendProcessMessage(PID_RENDERER, cefmsg);
                break;
            }
            default:
                _TRACE("unhandled message Type: %d To: %s From: %s Msg: %s", (int) msg.getID(), msg.getTo(),msg.getFrom(), msg.getMessage());
                break;
            }
        }
#ifdef _DEBUG_UNIT_TEST
        boost::posix_time::ptime tNow = boost::posix_time::second_clock::local_time();
        boost::posix_time::time_duration diff = tNow - tPing;
        static int nPingTime = 200;
        if (diff.total_seconds() > nPingTime)
        {
            nSendCnt++;
            getSharedMemManager().sendMessage(PARTNER_ADDRESS, MYADDRESS, MSG_TYPE_PING, "Ping %d fcsLogin",
                                              nPingCnt++);
            nSendCnt = getSharedMemManager().dump(PARTNER_ADDRESS);
            nSendCnt = 0;
            _TRACE("Total Sent: %d Total Recieved %d", nSendCnt, nRecCnt);
            tPing = boost::posix_time::second_clock::local_time();
        }
#endif
        _TRACE("***start timer");
        nSleepInterval = 600;
        boost::posix_time::ptime t1 =
                boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(nSleepInterval);
        bool bResult = getSharedMemManager().getClientMutex()->timed_lock(t1);
        _TRACE("*** End Timer bResult=%s", (bResult ? "true" : "false"));
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
    getSharedMemManager().getClientMutex()->unlock();
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
    getSharedMemManager().getClientMutex()->unlock();
    m_pThread->join();

    return true;
}

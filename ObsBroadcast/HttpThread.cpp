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
#include <ctime>
#include <list>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/timeb.h>
#include <pthread.h>
#endif

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <curl/curl.h>

// obs includes
#include <libobs/obs-module.h>
#include <libobs/util/threading.h>

// MFC includes
#include <libfcs/Log.h>
#include <libfcs/MfcJson.h>

// solution includes
#include <libPlugins/HttpRequest.h>
#include <libPlugins/IPCShared.h>
#include <libPlugins/MFCConfigConstants.h>
#include <libPlugins/MFCPluginAPI.h>
#include <libPlugins/ObsUtil.h>
//#include <libPlugins/PluginParameterBlock.h>
#include <libPlugins/Portable.h>

// project includes
#include "HttpThread.h"
#include "ObsBroadcast.h"

using std::string;

// https://stackoverflow.com/questions/27314485/use-of-deleted-function-error-with-stdatomic-int
std::atomic< uint32_t >         CHttpThread::sm_dwThreadCmd = { THREADCMD_NONE };
pthread_mutex_t                 CHttpThread::sm_mutexTimed;
MFC_Shared_Mem::CMessageManager CHttpThread::sm_mem;

extern CBroadcastCtx g_ctx; // part of MFCLibPlugins.lib::MfcPluginAPI.obj

//---------------------------------------------------------------------------
// setStop
//
// access member for stop flag.
void CHttpThread::setCmd(uint32_t dwCmd)
{
    sm_dwThreadCmd = dwCmd;
}

uint32_t CHttpThread::getCmd(void)
{
    return sm_dwThreadCmd;
}


//---------------------------------------------------------------------------
// stopBroadcasterCallback
//
// call back from libcurl that will allows us to terminate an http request
// if stop flag is set.
//
// curl_off_t is an int64
int stopBroadcasterCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t uLow)
{
    UNUSED_PARAMETER(clientp);
    UNUSED_PARAMETER(dltotal);
    UNUSED_PARAMETER(dlnow);
    UNUSED_PARAMETER(ultotal);
    UNUSED_PARAMETER(uLow);
    if (CHttpThread::getCmd() == THREADCMD_SHUTDOWN)
    {
        _TRACE("Terminating http request!");
        return 1;
    }
    return 0;
}


//---------------------------------------------------------------------------
// CHttpThread
//
// Http thread.
CHttpThread::CHttpThread()
{
    // create exit timed mutex
    pthread_mutexattr_t attrTimed;
    pthread_mutexattr_init(&attrTimed);
    pthread_mutexattr_settype(&attrTimed, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&sm_mutexTimed, &attrTimed);
    setCmd(THREADCMD_NONE);
}


CHttpThread::~CHttpThread()
{
    pthread_mutex_destroy(&sm_mutexTimed);
}


//---------------------------------------------------------------------------
// startProcessThread
//
// Call back function from thread create.
void* CHttpThread::startProcessThread(void* pCtx)
{
    CHttpThread* pThis = (CHttpThread*)pCtx;
    pThis->Process();
    return nullptr;
}


//---------------------------------------------------------------------------
// start
//
// start worker thread.
//---------------------------------------------------------------------------
bool CHttpThread::Start()
{
    // hold mutex locked unless work is needed
    pthread_mutex_lock(&sm_mutexTimed);

    bool bInitSharedMem = sm_mem.init(true);
    if (bInitSharedMem)
    {
        assert(sm_mem.getBroadcastPluginMutex());
        sm_mem.getBroadcastPluginMutex()->lock();
        setCmd(THREADCMD_NONE);
        pthread_create(&m_thread, nullptr, CHttpThread::startProcessThread, this);
        return true;
    }
    else _TRACE("Failed to init shared memory!");

    return false;
}


//---------------------------------------------------------------------------
// Stop
//
// Stop the thread.
bool CHttpThread::Stop(int nTimeout)
{
    UNUSED_PARAMETER(nTimeout);
    setCmd(THREADCMD_SHUTDOWN);
    // clear the mutex and this will wake the thread
    sm_mem.getBroadcastPluginMutex()->unlock();
    // pthread_mutex_unlock(&sm_mutexTimed);
    pthread_join(m_thread, nullptr);
    return true;
}


#ifdef __APPLE__
// apple doesn't define pthread_mutex_timedlock
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *timeout)
{
    int nResult;
    struct timeb currsystime;
    const int64_t nNanoToSec = 1000000;
    _TRACE("Start pthread_mutex_timedlock");
    bool bDone = false;
    do
    {
        nResult = pthread_mutex_trylock(mutex);
        if (EBUSY == nResult)
        {
            timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = nNanoToSec;
            nanosleep(&ts, &ts);
            ftime(&currsystime);
            if (currsystime.time >= timeout->tv_sec)
                bDone = true;
        }
        else
        {
            _TRACE("Break");
            break;
        }
    } while (nResult != 0 && !bDone);
    _TRACE("End pthread_mutex_timedlock");

    return nResult;
}
#endif


//---------------------------------------------------------------------------
// Process
//
// process rtn for the thread call back.
//---------------------------------------------------------------------------
void CHttpThread::Process()
{
    bool bConnected = false, bDone = false, bStarted = false;
    boost::posix_time::ptime nWakeTm, nLastPingTm;
    CMFCPluginAPI api(stopBroadcasterCallback);
    size_t nErrCx = 0, nPingCx = 0;
    int nErr = 0, nSleepTimer = 0;
    CBroadcastCtx ctx;
    uint32_t dwCmd = 0u;

    // to start off, set sleep timeout to now so it will trigger a polling action right away.
    // Set initial ping time to now as well.
    nWakeTm     = boost::posix_time::second_clock::universal_time();
    nLastPingTm = boost::posix_time::second_clock::local_time() - boost::posix_time::seconds((CHttpThread::PING_MSG_INTERVAL - 5));

    while (!bDone)
    {
        // Collect current ctx data from main thread
        ctx = g_ctx;

        if (boost::posix_time::second_clock::universal_time() >= nWakeTm)
        {
            bConnected = false;
            if (ctx.agentPolling)
            {
                // Only send heartbeat to agentSvc.php when we attached to an mfc WebRTC backend
                if (ctx.isMfc)  // && !ctx.isCustom)
                {
                    int nInterval = ctx.isLoggedIn ? 15 : 8;
                    if ((nErr = api.SendHeartBeat()) == 0)
                    {
                        bConnected = true;
                        ctx = g_ctx;
                        nSleepTimer = nInterval;
                        nErrCx = 0;
                    }
                    //
                    // Launch CEF Login window, we need auth credentials before we can login.
                    //
                    else if (nErr == ERR_NEED_LOGIN)
                    {
                        // Send message to SK log aboput login required, pause until we read a msg
                        // of MSG_TYPE_DOCREDENTIALS or MSG_TYPE_SET_MSK and we can resume heartbeat calls
                        if (g_ctx.activeState != SkNoCredentials)
                        {
                            g_ctx.activeState = SkNoCredentials;
                            _MESG("state => SkNoCredentials, stopping agent polling");
                            g_ctx.stopPolling();

#if MFC_BROWSER_LOGIN
                            // TODO: Honor an option/config setting to only load login window when manually
                            // started by user, otherwise we launch it automatically here.
                            if (!CObsUtil::ExecMFCLogin())
                                _MESG("SendHeartbeat() response ERR_NEED_LOGIN, but ExecMFCLogin() FAILED to start process.");
#endif
                        }
                    }
                    //
                    // set sleep timer to either normal interval seconds (if we have less than 5 errors) or
                    // triple the normnal interval if we have 5 or more since the last successful heartbeat
                    //
                    else nSleepTimer = (++nErrCx < 5 ? nInterval : (nInterval * 3));
                }
                else nSleepTimer = 1;   // set sleep timer to 1 second when we aren't a webRtc stream
            }
            else nSleepTimer = 1;       // set sleep timer to 1 second when we are paused

            // Set the sleep timer forward by +nSleepTimer seconds before we process another heartbeat api check
            nWakeTm = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds( nSleepTimer );
        }

        boost::posix_time::time_duration diffTm = boost::posix_time::second_clock::local_time() - nLastPingTm;
        if (diffTm.total_seconds() > CHttpThread::PING_MSG_INTERVAL)
        {
            if (!m_workerPids.empty())
                sm_mem.sendMessage(ADDR_FCSLOGIN, ADDR_OBS_BROADCAST_Plugin, MSG_TYPE_PING, "Ping %d obsBroadcast", nPingCx++);

            nLastPingTm = boost::posix_time::second_clock::local_time();
        }

        // Sleep for at most 1 second (if we aren't woken up by acquisition of lock) before we timeout and
        // check for threadcmd events and loop starting block over and potentially sending a heartbeat API
        //
        boost::posix_time::ptime t1 = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds( 1 );
        sm_mem.getBroadcastPluginMutex()->timed_lock( t1 );

        // If we woke up from a thread cmd being set, collect threadCmd and take action,
        // otherwise return to top of loop and check for heatbeat/ping times before sleeping
        if ((dwCmd = getCmd()) != THREADCMD_NONE)
        {
            // Reset the wakeup time in case we are changing something that relies on not being asleep
            nWakeTm = boost::posix_time::second_clock::universal_time();

            switch (dwCmd)
            {
            case THREADCMD_SHUTDOWN:
                g_ctx.stopPolling();
                if ( ! bDone )
                {
                    sm_mem.sendMessage(ADDR_FCSLOGIN, ADDR_OBS_BROADCAST_Plugin, MSG_TYPE_SHUTDOWN, "Shutdown!");
                    bDone = true;
                }
                break;

            case THREADCMD_PAUSE:
                if ( g_ctx.agentPolling )
                {
                    _TRACE("HttpThread polling PAUSED.");
                    g_ctx.stopPolling();
                }
                break;

            case THREADCMD_RESUME:
                if ( ! g_ctx.agentPolling )
                {
                    _TRACE("HttpThread polling UNPAUSED.");
                    g_ctx.startPolling();
                }
                break;

            case THREADCMD_STREAMSTART:
                if ( ! bStarted )
                {
                    _TRACE("Stream Started, widening poll interval.");
                    bStarted = true;
                }
                break;

            case THREADCMD_STREAMSTOP:
                if ( bStarted )
                {
                    _TRACE("Stream Stopped, narrowing poll interval.");
                    bStarted = false;

                    // also reset the wake time to be next loop since it could be upto 6 seconds away still
                    nWakeTm = boost::posix_time::second_clock::universal_time();
                }
                break;

            case THREADCMD_PROFILE:
                // < recursive_mutex > lk = g_ctx.sharedLock();
                //
                // Send profile change event to any shared memory segment subscribers
                // that may be interested in it (along with profile data, server, etc)
                //
                g_ctx.startPolling();
                break;

            default:
                _TRACE("UNHANDLED ThreadCmd value: %u -- dropping!", dwCmd);
                break;
            }
            setCmd(THREADCMD_NONE);
        }
        // Check for any shared mem messages for us before looping
        if (!bDone)
            readSharedMsg(ctx);
    }
}


void CHttpThread::readSharedMsg(CBroadcastCtx& ctx)
{
    MFC_Shared_Mem::CSharedMemMsg msg;

    while (sm_mem.getNextMessage(&msg, ADDR_OBS_BROADCAST_Plugin))
    {
        string sMsg( msg.getMessage() );
        string sFrom( msg.getFrom() );
        string sTo( msg.getTo() );

        switch (msg.getID())
        {
        case MSG_TYPE_PING:
            _TRACE("MSG_TYPE_PING  To:%s From:%s Type:%d Msg: %s\n", sTo.c_str(), msg.getFrom(), msg.getID(), sMsg.c_str());
            break;

        case MSG_TYPE_LOG:
            _TRACE("MSG_TYPE_LOG To:%s From:%s Type:%d Msg:\r\n%s\n", sTo.c_str(), msg.getFrom(), msg.getID(), sMsg.c_str());
            break;

        case MSG_TYPE_START:
        {
            int nPid = atoi( sMsg.c_str() );
            if (nPid > 0)
            {
                m_workerPids.insert(nPid);
                // _TRACE("MSG_TYPE_START  To:%s From:%s Type:%d Worker process id %d (%zu total workers running)",
                //        sTo.c_str(),
                //        msg.getFrom(),
                //        msg.getID(),
                //        nPid,
                //        m_workerPids.size());
            }
            break;
        }

        case MSG_TYPE_SHUTDOWN:
        {
            if (sFrom == ADDR_CEF_JSEXTENSION || sFrom == ADDR_FCSLOGIN)
            {
                int nPid = atoi( sMsg.c_str() );
                if (nPid > 0)
                {
                    m_workerPids.erase(nPid);
                    // _TRACE("MSG_TYPE_SHUTDOWN  To:%s From:%s Type:%d process id %d (%zu total running)\n", sTo.c_str(), sFrom.c_str(), msg.getID(), nPid, m_workerPids.size());
                }
                else _TRACE("MSG_TYPE_SHUTDOWN  To:%s From:%s Type:%d: Invalid process id: '%s'\n",
                            sTo.c_str(), sFrom.c_str(), msg.getID(), sMsg.c_str() );
            }
            else _TRACE("MSG_TYPE_SHUTDOWN  To:%s From:%s Type:%d: Invalid From field; expected JSEXtension or FCSLOGIN; pid: '%s'\n",
                        sTo.c_str(), sFrom.c_str(), msg.getID(), sMsg.c_str() );
            break;
        }

        case MSG_TYPE_DOLOGIN:
            _TRACE("MSG_TYPE_DOLOGIN  To:%s From:%s Type:%d Msg: %s\n", sTo.c_str(), msg.getFrom(), msg.getID(), sMsg.c_str());
            break;

        case MSG_TYPE_SET_MSK:
            _TRACE("MSG_TYPE_SET_MSK  To:%s From:%s Type:%d Msg: %s\n", sTo.c_str(), msg.getFrom(), msg.getID(), sMsg.c_str());
            if (sMsg.length() > 0)
            {
                ctx.cfg.set("ctx", sMsg);
                ctx.cfg.writeProfileConfig();
                // Send ctx data back to main thread after we updated it
                g_ctx = ctx;
            }
            break;

        case MSG_TYPE_DOCREDENTIALS:
            //_MESG("MSG_TYPE_DOCREDENTIALS  To:%s From:%s Type:%d Msg: %s\n", sTo.c_str(), msg.getFrom(), msg.getID(), sMsg.c_str());
            if (ctx.cfg.Deserialize(sMsg))
            {
                MfcJsonObj js;
                ctx.cfg.Serialize(js);
                ctx.cfg.writeProfileConfig();

                // Send ctx data back to main thread after we updated it
                g_ctx = ctx;
                g_ctx.agentPolling = true;

            }
            else
            {
                _TRACE("failed to read data from do credentials msg: %s", sMsg.c_str());

                // collect original ctx data again, since we may have corrupted or
                // invalidated our local 'ctx' from cfg.Deserialize() call that just failed.
                ctx = g_ctx;
            }
            break;

        default:
            _TRACE("Unknown Message Type: To:%s From:%s Type:%d Msg: %s\n", sTo.c_str(), msg.getFrom(), msg.getID(), sMsg.c_str());
            break;
        }
    }
}

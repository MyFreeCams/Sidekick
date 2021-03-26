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

#include "ObsBroadcast.h"

#include <libPlugins/MFCConfigConstants.h>

#include <util/config-file.h>
#include <util/platform.h>
#include <util/threading.h>

#ifndef _WIN32
#include <sys/time.h>
#else
#include <libfcs/UtilCommon.h>  // gettimeofday()
#endif

#include <libfcs/Log.h>

using std::string;


CBroadcastCtx::CBroadcastCtx(bool isSharedCtx)
    : tmPollingStamp(0)
    , tmStreamStart(0)
    , tmStreamStop(0)
    , agentPolling(false)
    , cfg(isSharedCtx)
    , m_pConsole(nullptr)
{
    clear(false);
}

CBroadcastCtx::CBroadcastCtx()
    : tmPollingStamp(0)
    , tmStreamStart(0)
    , tmStreamStop(0)
    , agentPolling(false)
    , cfg(false)
    , m_pConsole(nullptr)
{
    clear(false);
}

CBroadcastCtx::CBroadcastCtx(const CBroadcastCtx& other)
    : m_pConsole(nullptr)
{
    copyFrom(other);
}


CBroadcastCtx& CBroadcastCtx::operator=(const CBroadcastCtx& other)
{
    return copyFrom(other);
}


CBroadcastCtx& CBroadcastCtx::copyFrom(const CBroadcastCtx& other)
{
    // Lock mutex of ourself or other if either are a sharedCtx.
    auto otherLock  = other.sharedLock();
    auto ourLock    = this->sharedLock();

    uint32_t nCurSid = (uint32_t)this->cfg.getInt("sid");
    uint32_t nCurUid = (uint32_t)this->cfg.getInt("uid");
    string sCurProfileName = this->profileName;

    // Copy the member properties that should be copied regardless
    // of if we are the shared instance or not -- which is just
    // sidekick model config, or 'cfg' prop. Shared instances are
    // expected to be the ones who have correct status for
    // agentPolling and the various timestamps, where non-shared
    // instances are most likely not attached to the main thread or
    // are polling agentSvc php, since they are instantiated just
    // for loading and moving the SidekickModelConfig data over to
    // the shared ctx.
    this->cfg = other.cfg;
    bool profileChanged = (sCurProfileName != this->profileName);

    onUpdateSid(nCurSid, (uint32_t)other.cfg.getInt("sid"), profileChanged, this->cfg.isShared());
    onUpdateUid(nCurUid, (uint32_t)other.cfg.getInt("uid"), profileChanged, this->cfg.isShared());

    validateActiveState(this->cfg.isShared());

    // Copy the shared ctx data over only if they are a shared ctx and we are not.
    if (other.cfg.isShared() && !this->cfg.isShared())
    {
        this->tmPollingStamp = other.tmPollingStamp;
        this->tmStreamStart = other.tmStreamStart;
        this->tmStreamStop = other.tmStreamStop;
        if (this->agentPolling != other.agentPolling)
        {
            int nB = 2;
        }

        this->agentPolling = other.agentPolling;
        this->isStreaming = other.isStreaming;
        this->profileName = other.profileName;
        this->serverName = other.serverName;
        this->isLoggedIn = other.isLoggedIn;
        this->isLinked = other.isLinked;
        this->isWebRTC = other.isWebRTC;
        this->isCustom = other.isCustom;
        this->isRTMP = other.isRTMP;
        this->isMfc = other.isMfc;
    }

    return *this;
}


void CBroadcastCtx::updateState(SidekickActiveState oldState, SidekickActiveState newState, bool triggerHooks)
{
    int nChange = -1;

    if (this->cfg.isShared())
    {
        if (newState != SkUninitialized)
        {
            if (oldState == SkUninitialized)
                nChange = 1;
            else if (oldState != newState)
                nChange = 2;
        }
        else if (oldState != SkUninitialized)
        {
            nChange = 0;
        }

        if (nChange != -1 && triggerHooks)
        {
            auto lk = eventLock();
            SIDEKICK_UI_EV ev;
            gettimeofday(&ev.tvStamp, NULL);
            ev.ev = SkModelState;
            ev.dwArg1 = (uint32_t)nChange;
            ev.dwArg2 = oldState;
            ev.dwArg3 = newState;
            ev.dwArg4 = 0;
            sm_eventQueue.push_front( ev );
        }
    }

    this->activeState = newState;
}


void CBroadcastCtx::onUpdateSid(uint32_t nCurSid, uint32_t nNewSid, bool profileChanged, bool triggerHooks)
{
    int nChange = -1;

    if (this->cfg.isShared())
    {
        if (nNewSid > 0)
        {
            if (nCurSid == 0)
            {
                // modelweb login detected - cursid 0, new sid valid session
                // _MESG("*** SessionId connect, from %u >> %u !!", nCurSid, nNewSid);
                this->isLoggedIn = true;
                nChange = 1;
            }
            else if (nCurSid != nNewSid)
            {
                // modelweb relog detected - cur sid valid session, new sid different valid session
                // _MESG("*** SessionId reconnect, from %u >> %u !!", nCurSid, nNewSid);
                this->isLoggedIn = true;
                nChange = 2;
            }
            else
            {
                // logged in with same sid as before, so just make sure we preserve the isLoggedIn state
                this->isLoggedIn = true;
            }
        }
        else
        {
            if (nCurSid > 0)
            {
                // modelweb logoff detected - from valid session => 0
                //_MESG("*** SessionId disconnect, from %u >> %u !!", nCurSid, nNewSid);
                nChange = 0;
            }
            this->isLoggedIn = false;
        }

        if (nChange != -1 && triggerHooks)
        {
            auto lk = eventLock();

            SIDEKICK_UI_EV ev;
            gettimeofday(&ev.tvStamp, nullptr);
            ev.ev = SkSessionId;
            ev.dwArg1 = (uint32_t)nChange;
            ev.dwArg2 = nCurSid;
            ev.dwArg3 = nNewSid;
            ev.dwArg4 = profileChanged ? 1 : 0;

            sm_eventQueue.push_front(ev);
            //_MESG("updateSid: change(%d); sid %u => %u, event added, queue sz now: %zu", nChange, nCurSid, nNewSid, sm_eventQueue.size());
        }

        this->cfg.set("sid", (int)nNewSid);
    }
}


void CBroadcastCtx::onUpdateUid(uint32_t nCurUid, uint32_t nNewUid, bool profileChanged, bool triggerHooks)
{
    int nChange = -1;

    if (this->cfg.isShared())
    {
        if (nNewUid > 0)
        {
            if (nCurUid == 0)
            {
                // modelweb login detected - cur uid 0, new uid valid session
                this->isLinked = true;
                nChange = 1;
            }
            else if (nCurUid != nNewUid)
            {
                // modelweb relog detected - cur Uid valid session, new uid different valid session
                this->isLinked = true;
                nChange = 2;
            }
            else
            {
                // still linked with same uid as before, so just preserve isLinked state
                this->isLinked = true;
            }
        }
        else
        {
            this->isLinked = false;
            if (nCurUid > 0)
            {
                // unlink/logout from valid uid => 0
                nChange = 0;
            }
        }

        // we only issue an update event for users if the user id is changing from
        // a non-zero value currently to a different new value (zero or otherwise).
        // This prevents a duplicate event from being issued for showLinkStatus() on
        // startup when the user link status is displayed as part of a profile change.
        if (nChange != -1 && triggerHooks)
        {
            auto lk = eventLock();

            SIDEKICK_UI_EV ev;
            gettimeofday(&ev.tvStamp, nullptr);
            ev.ev = SkUserId;
            ev.dwArg1 = (uint32_t)nChange;
            ev.dwArg2 = nCurUid;
            ev.dwArg3 = nNewUid;
            ev.dwArg4 = profileChanged ? 1 : 0;

            sm_eventQueue.push_front(ev);
        }

        this->cfg.set("uid", (int)nNewUid);
    }
}


void CBroadcastCtx::onUpdateStreamkey(const string& sCurKey, const string& sNewKey, bool triggerHooks)
{
    int nChange = -1;

    if (this->cfg.isShared())
    {
        if (   this->activeState != SkStreamStarting
            && this->activeState != SkStreamStarted
            && this->activeState != SkStreamStopping)
        {
            if (!sNewKey.empty())
            {
                if (sCurKey.empty())
                    nChange = 1;
                else if (sNewKey != sCurKey)
                    nChange = 2;
            }

            if (nChange != -1 && triggerHooks)
            {
                auto lk = eventLock();

                SIDEKICK_UI_EV ev;
                gettimeofday(&ev.tvStamp, nullptr);
                ev.ev = SkStreamKey;
                ev.dwArg1 = (uint32_t)nChange;
                ev.dwArg2 = FCVIDEO_TX_IDLE;
                ev.dwArg3 = 0;
                ev.dwArg4 = 0;
                ev.sArg1 = sCurKey;
                ev.sArg2 = sNewKey;

                sm_eventQueue.push_front(ev);
                Log::Mesg("[DBG] STREAMKEY UPDATED ***** change(%d); %s [%zu] => [%zu] %s", nChange, sCurKey.c_str(), sCurKey.size(), sNewKey.size(), sNewKey.c_str());
            }
        }

        this->cfg.set("streamkey", sNewKey);
    }
}


void CBroadcastCtx::onUpdateServerUrl(const string& sCurUrl, const string& sNewUrl, bool triggerHooks)
{
    int nChange = -1;

    if (this->cfg.isShared())
    {
        if (   this->activeState != SkStreamStarting
            && this->activeState != SkStreamStarted
            && this->activeState != SkStreamStopping)
        {
            if (!sNewUrl.empty())
            {
                if (sCurUrl.empty())
                    nChange = 1;
                else if (sNewUrl != sCurUrl)
                    nChange = 2;
            }

            if (nChange != -1 && triggerHooks)
            {
                auto lk = eventLock();

                SIDEKICK_UI_EV ev;
                gettimeofday(&ev.tvStamp, nullptr);
                ev.ev = SkServerUrl;
                ev.dwArg1 = (uint32_t)nChange;
                ev.dwArg2 = 0;
                ev.dwArg3 = 0;
                ev.dwArg4 = 0;
                ev.sArg1 = sCurUrl;
                ev.sArg2 = sNewUrl;

                sm_eventQueue.push_front(ev);
                //Log::Mesg("[DBG] ServerUrl UPDATED **** change(%d); %s => %s", nChange, sCurUrl.c_str(), sNewUrl.c_str());
            }
        }

        this->cfg.set("streamurl", sNewUrl);
    }
}


bool CBroadcastCtx::DeserializeCfg(MfcJsonObj& js, bool triggerHooks)
{
    bool retVal = false;

    uint32_t nNewSid = 0;
    js.objectGetInt("sid", nNewSid);
    onUpdateSid(this->cfg.getInt("sid"), nNewSid, false, triggerHooks);

    uint32_t nCurUid = (uint32_t)this->cfg.getInt("uid");
    uint32_t nNewUid = 0;
    js.objectGetInt("uid", nNewUid);
    if (nCurUid != nNewUid)
        onUpdateUid(nCurUid, nNewUid, false, triggerHooks);

    // read current key value from obs settings data directly (in case it was changed in the UI)
    string sCurKey, sData;
    CObsUtil::getCurrentSetting("key", sCurKey);
    if (js.objectGetString("streamkey", sData) && !sData.empty())
        onUpdateStreamkey(sCurKey, sData, triggerHooks);

    string sCurUrl;
    CObsUtil::getCurrentSetting("server", sCurUrl);
    if (sCurUrl.empty())
    {
        string sUrl = MFC_DEFAULT_BROADCAST_URL;
        if (js.objectGetString("region", sData) && !sData.empty())
            sUrl = string("rtmp://publish-") + sData + string(".myfreecams.com/NxServer");
        
        Log::Mesg("[DBG] DeserializeCfg setting empty serverurl to %s (triggerHooks: %s)", sUrl.c_str(), triggerHooks ? "true" : "false");
        onUpdateServerUrl(sCurUrl, sUrl, triggerHooks);
        this->cfg.set("streamurl", sData);
    }

    js.Serialize(sData);
    retVal = this->cfg.Deserialize(sData);

    // After syncing isLoggedIn and uid, validate active state to make sure activeState is current
    validateActiveState(triggerHooks);

    return retVal;
}


void CBroadcastCtx::validateActiveState(bool triggerHooks)
{
    if (this->isMfc)
    {
        if (this->isLoggedIn)
        {
            // if we are logged in and our state is not one of the
            // streaming states (starting, started, stopping, stopped)
            // then we move it to the default, stopped.
            if (this->activeState < SkStreamStarting)
            {
                updateState(this->activeState, SkStreamStopped, triggerHooks);
            }
        }
        // if we aren't logged in, but our uid is still valid, set
        // state to 'waiting on modelweb session'
        else if (this->cfg.getInt("uid") > 0)
        {
            updateState(this->activeState, SkNoModelwebSession, triggerHooks);
        }
        // Otherwise (uid is 0) we set our state to 'invalid credentials'
        else updateState(this->activeState, SkInvalidCredentials, triggerHooks);
    }
    else updateState(this->activeState, SkUnknownProfile, triggerHooks);
}


bool CBroadcastCtx::importProfileConfig(void)
{
    bool retVal;

    uint32_t nCurSid = this->cfg.getInt("sid");
    uint32_t nCurUid = this->cfg.getInt("uid");
    string sCurKey, sCurUrl;    // = this->cfg.getString("ctx");  this->cfg.getString("streamurl");
    CObsUtil::getCurrentSetting("key", sCurKey);
    CObsUtil::getCurrentSetting("server", sCurUrl);
    string sCurProfile = this->profileName;

    retVal = this->cfg.readProfileConfig();

    uint32_t nNewSid = this->cfg.getInt("sid");
    uint32_t nNewUid = this->cfg.getInt("uid");
    string sNewKey = this->cfg.getString("ctx");
    string sNewUrl = this->cfg.getString("streamurl");

    bool profileChanged = (sCurProfile != this->profileName);

    // onUpdateSid() will make sure isLoggedIn is the correct value
    onUpdateSid(nCurSid, nNewSid, profileChanged, false);
    onUpdateUid(nCurUid, nNewUid, profileChanged, false);
    onUpdateStreamkey(sCurKey, sNewKey, false);

    if (sCurUrl != sNewUrl)
    {
        Log::Mesg("[DBG] readPluginCOnfig setting url from %s => %s", sCurUrl.c_str(), sNewUrl.c_str());
        onUpdateServerUrl(sCurUrl, sNewUrl, false);
    }

    // After syncing isLoggedIn and uid, validate active state to
    // make sure activeState is current as well
    validateActiveState(false);

    return retVal;
}


void CBroadcastCtx::setConsole(void* pConsole)
{
    m_pConsole = pConsole;
}


void* CBroadcastCtx::getConsole(void)
{
    return m_pConsole;
}


void CBroadcastCtx::stopPolling(void)
{
    if (this->cfg.isShared())
    {
        auto lk = sharedLock();
        if (this->agentPolling)
        {
            this->tmPollingStamp = time(nullptr);
            this->agentPolling = false;
        }
    }
}


void CBroadcastCtx::startPolling(void)
{
    if (this->cfg.isShared())
    {
        auto lk = sharedLock();
        if (!this->agentPolling)
        {
            this->tmPollingStamp = time(nullptr);
            this->agentPolling = true;
        }
    }
}


void CBroadcastCtx::onStartStreaming(void)
{
    if (this->cfg.isShared())
    {
        auto lk = sharedLock();
        if (!this->isStreaming)
        {
            this->tmStreamStart = time(nullptr);
            this->isStreaming = true;

            if (this->isLoggedIn)
                updateState(this->activeState, SkStreamStarted, true);
        }
    }
}


void CBroadcastCtx::onStopStreaming(void)
{
    if (this->cfg.isShared())
    {
        auto lk = sharedLock();
        if (this->isStreaming)
        {
            this->tmStreamStop = time(nullptr);
            this->isStreaming = false;
        }
    }

    updateState(activeState, SkStreamStopped, true);
}


void CBroadcastCtx::clear(bool triggerHooks)
{
    auto lk = sharedLock();

    string sCurProfile(this->profileName);
    uint32_t nCurSid = 0, nCurUid = 0;
    uint32_t nNewSid = 0, nNewUid = 0;

    this->isMfc = false;
    this->isRTMP = false;
    this->isCustom = false;
    this->isWebRTC = false;
    this->isLinked = false;
    this->isLoggedIn = false;
    this->isStreaming = false;
    this->profileName.clear();
    this->serverName.clear();

    if (triggerHooks)
    {
        nCurSid = this->cfg.getInt("sid");
        nCurUid = this->cfg.getInt("uid");
        //sCurKey = this->cfg.getString("ctx");
    }

    this->cfg.clear();

    if (triggerHooks)
    {
        nNewSid = this->cfg.getInt("sid");
        nNewUid = this->cfg.getInt("uid");
    }

    // onUpdateSid() will make sure isLoggedIn is the correct value
    bool profileChanged = (sCurProfile != this->profileName);
    onUpdateSid(nCurSid, nNewSid, profileChanged, triggerHooks);
    onUpdateUid(nCurUid, nNewUid, profileChanged, triggerHooks);

    stopEdgeSock();

    // After syncing isLoggedIn and uid, validate active state to
    // make sure activeState is current as well
    validateActiveState(triggerHooks);
}


string CBroadcastCtx::streamName(void)
{
    return string("ext_x_") + std::to_string(this->cfg.getInt("uid"));
}


// Wrapper fcor SidekickModelConfig::sharedLock() for our cfg instance,
// or when updating any of our other member properties if we are g_ctx
std::unique_lock<std::recursive_mutex> CBroadcastCtx::sharedLock(void) const
{
    return this->cfg.sharedLock();
}


void CBroadcastCtx::sendEvent(SidekickEventType evType, uint32_t dwArg1, uint32_t dwArg2,
                              uint32_t dwArg3, uint32_t dwArg4, const char* pszArg1, const char* pszArg2)
{
    SIDEKICK_UI_EV ev;
    auto lk = eventLock();

    gettimeofday(&ev.tvStamp, NULL);
    ev.ev = evType;
    ev.dwArg1 = dwArg1;
    ev.dwArg2 = dwArg2;
    ev.dwArg3 = dwArg3;
    ev.dwArg4 = dwArg4;
    ev.sArg1 = pszArg1 ? pszArg1 : "";
    ev.sArg2 = pszArg2 ? pszArg2 : "";

    sm_eventQueue.push_front( ev );
}


std::unique_lock<std::recursive_mutex> CBroadcastCtx::eventLock(void)
{
    std::unique_lock<std::recursive_mutex> lk(sm_eventLock, std::try_to_lock);
    return lk;
}


const char* CBroadcastCtx::MapSidekickState(SidekickActiveState state)
{
    switch (state)
    {
        case SkUninitialized:       return "SkUninitialized";
        case SkUnknownProfile:      return "SkUnknownProfile";
        case SkNoCredentials:       return "SkNoCredentials";
        case SkInvalidCredentials:  return "SkInvalidCredentials";
        case SkNoModelwebSession:   return "SkNoModelwebSession";
        case SkStreamStarting:      return "SkStreamStarting";
        case SkStreamStarted:       return "SkStreamStarted";
        case SkStreamStopping:      return "SkStreamStopping";
        case SkStreamStopped:       return "SkStreamStopped";
    }

    return "SkInvalid_State_Error";
}


bool CBroadcastCtx::startEdgeSock(const string& sUser, uint32_t modelId, const string& sToken, const string& sUrl)
{
    bool retVal = false;

    if (sm_edgeSock == nullptr)
    {
        sm_edgeSock = new EdgeChatSock(sUser, modelId, sToken, sUrl);
        retVal = true;
    }

    return retVal;
}

bool CBroadcastCtx::startEdgeSock(const string& sUser, uint32_t modelId, const string& sToken)
{
    bool retVal = false;

    if (sm_edgeSock == nullptr)
    {
        const string sUrl = string("wss://") + EdgeChatSock::FcsServer() + string(".myfreecams.com/fcsl");
        retVal = startEdgeSock(sUser, modelId, sToken, sUrl);
    }

    return retVal;
}

bool CBroadcastCtx::stopEdgeSock(void)
{
    bool retVal = false;

    if (sm_edgeSock != nullptr)
    {
        retVal = sm_edgeSock->stop();
        delete sm_edgeSock;
        sm_edgeSock = nullptr;
    }

    return retVal;
}

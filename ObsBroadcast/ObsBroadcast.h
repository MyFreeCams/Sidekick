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

#ifndef OBS_BROADCAST_H_
#define OBS_BROADCAST_H_

#ifndef MFC_LOG_LEVEL
#define MFC_LOG_LEVEL ILog::LogLevel::DBG
#endif

#ifndef MFC_LOG_OUTPUT_MASK
#define MFC_LOG_OUTPUT_MASK 10
#endif

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>

#include <libPlugins/EdgeChatSock.h>
#include <libPlugins/ObsUtil.h>
#include <libPlugins/SidekickModelConfig.h>
#include <libfcs/MfcJson.h>

#include "SidekickTypes.h"


class CBroadcastCtx
{
public:
    explicit CBroadcastCtx(bool isSharedCtx);
    CBroadcastCtx();
    CBroadcastCtx(const CBroadcastCtx& other);

    virtual ~CBroadcastCtx() = default;

    CBroadcastCtx& operator=(const CBroadcastCtx& other);
    CBroadcastCtx& copyFrom(const CBroadcastCtx& other);

    void updateState(SidekickActiveState oldState, SidekickActiveState newState, bool triggerHooks);

    void onUpdateSid(uint32_t nCurSid, uint32_t nNewSid, bool profileChanged, bool triggerHooks);
    void onUpdateUid(uint32_t nCurUid, uint32_t nNewUid, bool profileChanged, bool triggerHooks);
    void onUpdateStreamkey(const std::string& sCurKey, const std::string& sNewKey, bool triggerHooks);
    void onUpdateServerUrl(const std::string& sCurUrl, const std::string& sNewUrl, bool triggerHooks);

    bool DeserializeCfg(MfcJsonObj& js, bool triggerHooks);
    void validateActiveState(bool triggerHooks);
    bool readPluginConfig(bool triggerHooks);

    void setConsole(void* pConsole);
    void* getConsole(void);

    void stopPolling(void);
    void startPolling(void);

    void onStartStreaming(void);
    void onStopStreaming(void);

    void clear(bool triggerHooks);
    std::string streamName(void);

    // Wrapper fcor SidekickModelConfig::sharedLock() for our cfg instance,
    // or when updating any of our other member properties if we are g_ctx
    std::unique_lock<std::recursive_mutex> sharedLock(void) const;

    static std::unique_lock<std::recursive_mutex> eventLock(void);

    static void sendEvent(SidekickEventType evType, uint32_t dwArg1 = 0, uint32_t dwArg2 = 0, uint32_t dwArg3 = 0,
                          uint32_t dwArg4 = 0, const char* pszArg1 = nullptr, const char* pszArg2 = nullptr);

    static const char* MapSidekickState(SidekickActiveState state);

    static bool startEdgeSock(const std::string& sUser, uint32_t modelId, const std::string& sToken, const std::string& sUrl);
    static bool startEdgeSock(const std::string& sUser, uint32_t modelId, const std::string& sToken);

    std::string profileName;    // name of current profile, even if it's not an MFC profile
    std::string serverName;     // name of current server, even if not an MFC server

    bool isMfc;                 // set to true if the service is MyFreeCams RTMP or MyFreeCams WebRTC
    bool isRTMP;                // is true when service type is 'MyFreeCams' (non-webrtc, assumed RTMP)
    bool isWebRTC;              // is true when service type is 'MyFreeCams WebRTC'
    bool isLinked;              // is true if user id is non-zero
    bool isLoggedIn;            // is true if session id is non-zero
    bool isStreaming;           // true when current profile is actively streaming

    time_t tmPollingStamp;      // time the agent polling last started (agentPolling = true)
                                // or stopped (agentPolling = false), or 0 if never started.

    time_t tmStreamStart;       // time streaming began, even if it has since stopped.
    time_t tmStreamStop;        // if not currently streaming, time streaming was stopped, otherwise 0

    bool agentPolling;          // flag if agent polling to sidekick.myfreecams.com/agentSvc.php is active

    atomic<SidekickActiveState> activeState;

    // If isMfc is true, then SidekickModelConfig is loaded with the profile's current config
    // If isMfc is false, smc is not loaded and will be empty/default values.
    // The data in SidekickModelConfig is relative to the currently active profile only.
    SidekickModelConfig cfg;

    static std::deque<SIDEKICK_UI_EV> sm_eventQueue;
    static EdgeChatSock* sm_edgeSock;

private:
    // only set to non-null for g_ctx instance.
    void* m_pConsole;

    static std::recursive_mutex sm_eventLock;
};

#endif  // OBS_BROADCAST_H_

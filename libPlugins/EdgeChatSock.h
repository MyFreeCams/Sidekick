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

#ifndef EDGE_CHAT_SOCK_H_
#define EDGE_CHAT_SOCK_H_

#ifdef _WIN32
#pragma warning (disable: 4100)
#pragma warning (disable: 4189)
#endif

#include <cstdint>
#include <memory>
#include <regex>
#include <string>
#include <vector>

// solution
#include <libfcs/FcMsg.h>
#include <libfcs/MfcTimer.h>
#include <ObsBroadcast/SidekickTypes.h>
#include <websocket-client/FcsWebsocket.h>

#ifndef DEFAULT_EDGECHAT_SERVER
#define DEFAULT_EDGECHAT_SERVER     "https://video502.myfreecams.com:8080/"
#endif

typedef SidekickActiveState ModelState;


class EdgeChatSock : public FcsWebsocket::FcsListener
{
public:
    EdgeChatSock();
    EdgeChatSock(const std::string& sUser, uint32_t dwModelId, const std::string& sToken, const std::string& sUrl);

    ~EdgeChatSock() override;

    void onTimerEvent(size_t pulseCx);
    bool start(const std::string& sUser, const std::string& sToken, const std::string& sUrl);
    bool stop();

    //
    // FcsWebsocket::FcsListener implementation.
    //
    void onConnected(void) override;
    void onDisconnected(void) override;
    void onMsg(std::string& sMsg) override;
    bool preDisconnect(bool wait) override;

    void onStateChange(ModelState oldState, ModelState newState);

    //
    // FCS chatserver handlers for FCTYPE_LOGIN, FCTYPE_AGENT, and FCTYPE_SESSIONSTATE
    //
    void onLogin(FcMsg& msg, MfcJsonObj& js);
    void onSessionState(FcMsg& msg, MfcJsonObj& js);
    uint32_t onAgent(FcMsg& msg, MfcJsonObj& js, MfcJsonObj& jsResp);
    uint32_t onAgentQuery(FcMsg& msg, MfcJsonObj& jsData, MfcJsonObj& jsResp);
    void onAgentNotify(MfcJsonObj& jsData);
    void onAgentUpdate(MfcJsonObj& jsData);
    void onAgentJoin(uint32_t dwOp, MfcJsonObj& jsData);

    // Randomly selects an active FCS chatserver from serverconfig.js
    static std::string FcsServer();

    static uint32_t addErrMsg(MfcJsonObj& js, uint32_t dwErr)
    {
        js.objectAdd("_err", dwErr);
        return dwErr;
    }
    static uint32_t addErrMsg(MfcJsonObj& js, uint32_t dwErr, const std::string& sMsg)
    {
        if ( !sMsg.empty() )
            js.objectAdd("_msg", sMsg);
        js.objectAdd("_err", dwErr);
        return dwErr;
    }

    static bool changeCurrentProfile(const string& sProfile);

    // sub-handlers for FCCHAN_QUERY ops, one for processing replies to requests we made,
    // the other for processing requests being made of us
    uint32_t onAgentQuery_reply(FcMsg& msg, MfcJsonObj& jsData, MfcJsonObj& jsResp, int64_t nReqId);
    uint32_t onAgentQuery_request(FcMsg& msg, MfcJsonObj& jsData, MfcJsonObj& jsResp, int64_t nReqId);

    // sends a FCTYPE_AGENT msg of FCCHAN_UPDATE with current status and system info if logged in
    void sendUpdate(void);

    void sendVirtualCameraState(bool virtualCameraEnabled);

    std::unique_ptr<FcsWebsocket>   m_edgeClient;

    std::string     m_username;     // username (not used)
    std::string     m_authToken;    // authToken used to join agent channel of model
    std::string     m_serverUrl;    // edgechat websocket server url we connect to
    uint32_t        m_sessionId;    // our chat server sessionId for edgechat websocket client
    size_t          m_updatesSent;  // counter for how many FCTYPE_AGENT messages of FCCHAN_UPDATE have been sent

    std::string     m_partialFrame; // if we had part of a frame from previous onMsg, store contents here until next onMsg
                                    // is called (if partial), or process again if |m_partialFrame| has at least one completed
                                    // frame in it

    // track state data from chat server (model id we are an agent for,
    // current state of model on chat server, collection of other agents
    // or modelsoft clients connected to agent channel for model)
    uint32_t        m_modelId;
    ModelState      m_modelState;

    size_t          m_retryConnect;
    bool            m_edgeConnected;
    bool            m_edgeLoggedIn;
    bool            m_virtualCameraActive;

    MfcTimer        m_sincePing;
};

#endif  // EDGE_CHAT_SOCK_H_

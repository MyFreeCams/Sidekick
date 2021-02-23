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

#ifdef WIN32
#pragma warning (disable: 4003)
#pragma warning (disable: 4244)
#pragma warning (disable: 4245)
#pragma warning (disable: 4840)
#pragma warning (disable: 4005)
#endif

#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <random>
#include <thread>

// obs
#include <obs-module.h>
#include <obs-frontend-api.h>

// solution
#include <libPlugins/HttpRequest.h>
#include <libPlugins/ObsUtil.h>
#include <libPlugins/ObsServicesJson.h>
#include <libfcs/Log.h>
#include <libfcs/MfcJson.h>
#include <libfcs/fcs.h>
#include <libfcs/FcMsg.h>
#include <ObsBroadcast/ObsBroadcast.h>

// project
#include "EdgeChatSock.h"
#include "CollectSystemInfo.h"

#include <nlohmann/json.hpp>

using njson = nlohmann::json;
using std::string;

std::unique_ptr<FcsWebsocket> proxy_createFcsWebsocket(void);

extern CBroadcastCtx g_ctx;         // part of MFCLibPlugins.lib::MfcPluginAPI.obj

#define obs_debug(format, ...) blog(400, format, ##__VA_ARGS__)
#define obs_info(format, ...)  blog(300, format, ##__VA_ARGS__)
#define obs_warn(format, ...)  blog(200, format, ##__VA_ARGS__)
#define obs_error(format, ...) blog(100, format, ##__VA_ARGS__)


EdgeChatSock::EdgeChatSock()
    : m_edgeClient(nullptr)
    , m_sessionId(0)
    , m_updatesSent(0)
    , m_modelId(0)
    , m_modelState(SkUninitialized)
    , m_retryConnect(false)
    , m_edgeConnected(false)
    , m_edgeLoggedIn(false)
    , m_virtualCameraActive(false)
{}


EdgeChatSock::EdgeChatSock(const string& sUser, uint32_t dwModelId, const string& sToken, const string& sUrl)
    : m_edgeClient(nullptr)
    , m_sessionId(0)
    , m_updatesSent(0)
    , m_modelId(dwModelId)
    , m_modelState(SkUninitialized)
    , m_retryConnect(false)
    , m_edgeConnected(false)
    , m_edgeLoggedIn(false)
    , m_virtualCameraActive(false)
{
    if ( ! start(sUser, sToken, sUrl) )
    {
        _MESG(  "unable to start EdgeChatSock to %s",
                sUrl.empty() ? DEFAULT_EDGECHAT_SERVER : sUrl.c_str());
    }
}


EdgeChatSock::~EdgeChatSock()
{
    obs_info(__FUNCTION__);
    stop();
}


bool EdgeChatSock::start(const string& sUser, const string& sToken, const string& sUrl)
{
    bool retVal = false;

    stop();  // Stop just in case
    m_retryConnect = 0;

    if ((m_edgeClient = proxy_createFcsWebsocket()) != nullptr)
    {
        m_username    = sUser;
        m_authToken   = sToken;
        m_serverUrl   = sUrl.empty() ? DEFAULT_EDGECHAT_SERVER : sUrl;

        // Connect to server
        obs_info("[DBG Edge] Connecting to EdgeChat on %s...", m_serverUrl.c_str());

        if ( m_edgeClient->connect(m_username, m_authToken, m_serverUrl, this) )
        {
            obs_info("[DBG Edge] EdgeChatSock connected to %s!", m_serverUrl.c_str());
            retVal = true;
        }
        else obs_error("[ERR Edge] Error connecting to %s", m_serverUrl.c_str());
    }
    else obs_error("[ERR Edge] Error creating FcsWebsocket");

    return retVal;
}


void EdgeChatSock::onTimerEvent(size_t pulseCx)
{
    // we're called every ~250ms, so retryCount >= 12 = >= 3 seconds
    if (!m_edgeConnected)
    {
        if (++m_retryConnect >= 12)
        {
            // start will reset m_retryConnect as well as call stop for us
            start(m_username, m_authToken, m_serverUrl);
        }
    }
    else if (m_edgeClient)
    {
        // Every 5 seconds (approx) when connected
        if ((pulseCx % 3) == 0)
        {
            if (m_sincePing.Stop() > 5)
            {
                m_edgeClient->send( FcMsg::textMsg(true, FCTYPE_NULL, 0, 0, 0, 0, 0, nullptr) );
                m_sincePing.Start();
            }
        }
    }
}


bool EdgeChatSock::stop()
{
    bool retVal = false;

    if (m_edgeClient)
    {
        retVal = m_edgeClient->disconnect(true);
        //delete m_edgeClient;
        m_edgeClient = nullptr;
    }
    return retVal;
}


void EdgeChatSock::onConnected(void)
{
    //obs_debug("EdgeChatSock::onConnected");
    m_edgeConnected = true;
    m_edgeLoggedIn  = false;
    m_retryConnect  = 0;
    m_sessionId     = 0;
    m_modelState    = g_ctx.activeState;

    // send version/login banner
    if (m_edgeClient->send( stdprintf("fcsws_%d", DEFAULT_WEBSOCK_VERSION) ) )
    {
        //if ( m_edgeClient->send( FcMsg::textMsg(false, FCTYPE_LOGIN, 0, 0, DEFAULT_LOGIN_VERSION, 0, stdprintf("%d/guest:guest", PLATFORM_MFC) ) ) )
        if ( m_edgeClient->send( FcMsg::textMsg(false, FCTYPE_LOGIN, 0, 0, DEFAULT_LOGIN_VERSION, 0, "guest:guest" ) ) )
        {
            // now wait for login response msg...
        }
        else _MESG("EdgeChatSock::onConnected() unable to send login msg");
    }
    else _MESG("EdgeChatSock::onConnected() unable to fcsws_ version banner");
}


void EdgeChatSock::onDisconnected(void)
{
    obs_debug("EdgeChatSock::onDisconnected");
    m_edgeConnected         = false;
    m_edgeLoggedIn          = false;
    m_virtualCameraActive   = false;
    m_sessionId             = 0;
    m_retryConnect          = 0;
    m_modelState            = g_ctx.activeState;
}


void EdgeChatSock::onStateChange(ModelState oldState, ModelState newState)
{
    if (m_modelState != newState)
    {
        m_modelState = newState;
        sendUpdate();
    }
}


void EdgeChatSock::sendVirtualCameraState(bool virtualCameraActive)
{
    m_virtualCameraActive = virtualCameraActive;
    sendUpdate();
}


void EdgeChatSock::sendUpdate(void)
{
    MfcJsonObj js;

    if (m_edgeLoggedIn)
    {
        js.objectAdd("op", FCCHAN_UPDATE);
        js.objectAdd("model", m_modelId);

        MfcJsonPtr pHost = MfcJsonObj::newType(JSON_T_OBJECT);
        js.objectAdd("agent_host", pHost);
        collectSystemInfo(*pHost);
        pHost->objectAdd("activeState", (int64_t)m_modelState);
        pHost->objectAdd("virtualCameraActive", m_virtualCameraActive);

        m_edgeClient->send( FcMsg::textMsg(true, FCTYPE_AGENT, m_sessionId, 0, 0, 0, js) );
        m_updatesSent++;
    }
}


bool EdgeChatSock::preDisconnect(bool wait)
{
    bool retVal = wait;
    string sMsg, sData;

    if (m_edgeConnected)
    {
        MfcJsonObj js;
        js.objectAdd("op", FCCHAN_PART);
        js.objectAdd("model", m_modelId);

        // preDisconnect() is called from within FcsWebSocketImpl::disconnect()'s try block,
        // which is why we are not catching any exceptions here for the send() call.
        if ( ! m_edgeClient->send( FcMsg::textMsg(true, FCTYPE_AGENT, m_sessionId, 0, 0, 0, js) ) )
        {
            obs_error("FcsWebsocketImpl::disconnect() unable to send logoff msg");
            retVal = false;
        }
    }

    return retVal;
}


void EdgeChatSock::onMsg(string& sMsg)
{
    uint32_t dwResp = FCRESPONSE_UNKNOWN;
    MfcJsonObj js(JSON_T_NONE), jsResp;
    FcMsg msg;

    if (msg.readFromText(m_partialFrame, sMsg, &js))
    {
        switch (msg.dwType)
        {
        case FCTYPE_LOGIN:
            onLogin(msg, js);
            break;
        case FCTYPE_SESSIONSTATE:
            onSessionState(msg, js);
            break;
        case FCTYPE_AGENT:
            if ((dwResp = onAgent(msg, js, jsResp)) != FCRESPONSE_QUEUED)
            {
                // Send response messge back to chat server for this agent msg,
                // most likely a FCCHAN_QUERY op from modelweb or another agent
                //_MESG("AGENTDBG: sending response FCTYPE_AGENT: %s", jsResp.prettySerialize().c_str());

                m_edgeClient->send( FcMsg::textMsg(true, FCTYPE_AGENT, m_sessionId, msg.dwFrom, msg.dwArg1, msg.dwArg2, jsResp) );
            }
            break;
        default:
            break;
        }
#if 0
        obs_info("[DBG Edge] onMsg: %s (%u,%u) {%u,%u} msgLen %u:  %s",
                 FcMsg::MapFcType(msg.dwType), msg.dwFrom, msg.dwTo, msg.dwArg1,
                 msg.dwArg2, msg.dwMsgLen, msg.payload_str(js));
#endif
    }
    else obs_error("[ERR Edge] FcMsg::readFromText() failed to parse FCS msg: %s", sMsg.c_str());
}


void EdgeChatSock::onLogin(FcMsg& msg, MfcJsonObj& jsData)
{
    MfcJsonObj js;

    if (msg.dwArg1 == FCRESPONSE_SUCCESS)
    {
        //obs_info("[DBG Edge] EdgeChatSock Logged in OK as Guest; session id is %u", msg.dwTo);
        m_sessionId     = msg.dwTo;
        m_edgeLoggedIn  = true;
        m_modelState    = g_ctx.activeState;
        m_updatesSent   = 0;

        // reset our keep alive ping timer
        m_sincePing.Start();

        // join agent channel for model
        js.objectAdd("op", FCCHAN_JOIN);
        js.objectAdd("model", m_modelId);
        //js.objectAdd("tag", "sidekick-obs");
        js.objectAdd("ctxenc", m_authToken);

        MfcJsonPtr pHost = MfcJsonObj::newType(JSON_T_OBJECT);
        js.objectAdd("agent_host", pHost);
        collectSystemInfo(*pHost);
        pHost->objectAdd("activeState", (int64_t)m_modelState);
        pHost->objectAdd("virtualCameraActive", m_virtualCameraActive);

        if ( ! m_edgeClient->send( FcMsg::textMsg(true, FCTYPE_AGENT, 0, 0, 0, 0, js) ) )
            obs_error("FcsWebsocketImpl::disconnect() unable to send logoff msg");
    }
    else _MESG("EdgeChatSock login failed: %s", jsData.prettySerialize().c_str());
}


bool EdgeChatSock::changeCurrentProfile(const string& sProfile)
{
    char*   pszProfile  = obs_frontend_get_current_profile();
    char**  ppNames     = obs_frontend_get_profiles();
    char**  ppName      = ppNames;
    bool    retVal      = false;

    if (strcmp(pszProfile, sProfile.c_str()) != 0)
    {
        while (!retVal && ppName && *ppName)
        {
            if (pszProfile && strcmp(sProfile.c_str(), *ppName) == 0)
            {
                obs_frontend_set_current_profile(sProfile.c_str());
                retVal = true;
            }
            ppName++;
        }
        if ( ! retVal )
            obs_error("unable to find profile in setprofile query request: %s", sProfile.c_str());
    }
    else
    {
        retVal = true;  // we already are setr to that profile, so return success
    }
    bfree(pszProfile);
    bfree(ppNames);

    return retVal;
}


uint32_t EdgeChatSock::onAgentQuery_request(FcMsg& msg, MfcJsonObj& jsData, MfcJsonObj& jsResp, int64_t nReqId)
{
    string sCmd, sVal, sKey;
    MfcJsonPtr pQuery;
    CObsServicesJson services;
    uint32_t dwResp = FCRESPONSE_ERROR;

    jsResp.objectAdd("reply", true);

    // a modelweb client or other bot sending us a direct query request
    // (stop, start, change profile, etc)
    if (jsData.objectGetObject("query", &pQuery))
    {
        if (pQuery->objectGetString("cmd", sCmd))
        {
            pQuery->objectGetString("val", sVal);

            if (sCmd == "setprofile")
            {
                if ( ! sVal.empty() )
                {
                    if (    g_ctx.activeState != SkStreamStarting
                        &&  g_ctx.activeState != SkStreamStarted
                        &&  g_ctx.activeState != SkStreamStopping)
                    {
                        if (changeCurrentProfile(sVal))
                        {
                            // we changed to new profile matching sVal
                            dwResp = addErrMsg(jsResp, FCRESPONSE_SUCCESS);
                        }
                        else dwResp = addErrMsg(jsResp, FCRESPONSE_ERROR, "unable to find profile");
                    }
                    else dwResp = addErrMsg(jsResp, FCRESPONSE_ERROR, "must be stopped to set profile");
                }
                else dwResp = addErrMsg(jsResp, FCRESPONSE_INVALIDARG, "missing profile");
            }
            else if (sCmd == "start")
            {
                if (g_ctx.activeState == SkStreamStopped)
                {
                    string sNewKey = g_ctx.cfg.getString("ctx");
                    string sEncKey;

                    if (sNewKey.size() > 10 && sNewKey.at(10) == '/')
                        sEncKey = MfcJsonObj::encodeURIComponent(sNewKey);
                    else
                        sEncKey = sNewKey;

                    stdprintf(sKey, "%s?sk=%s", g_ctx.streamName().c_str(), sEncKey.c_str());

                    services.updateProfileSettings(sKey, "(null)");
                    CObsUtil::setCurrentSetting("key", sNewKey.c_str());

                    obs_frontend_streaming_start();  // start public stream

                    dwResp = addErrMsg(jsResp, FCRESPONSE_SUCCESS);
                }
                else if (   g_ctx.activeState == SkStreamStarted
                         || g_ctx.activeState == SkStreamStarting
                         || g_ctx.activeState == SkStreamStopping)
                {
                    dwResp = addErrMsg(jsResp, FCRESPONSE_ERROR, "must be fully stopped before starting");
                }
                else dwResp = addErrMsg(jsResp, FCRESPONSE_ERROR, "invalid profile or non-MFC servers, cannot start");
            }
            else if (sCmd == "stop")
            {
                if (g_ctx.activeState == SkStreamStarted || g_ctx.activeState == SkStreamStarting)
                {
                    obs_frontend_streaming_stop();  // stop stream
                    dwResp = addErrMsg(jsResp, FCRESPONSE_SUCCESS);
                }
                else dwResp = addErrMsg(jsResp, FCRESPONSE_ERROR, "must be started/starting to stop");
            }
            else dwResp = addErrMsg(jsResp, FCRESPONSE_NOTFOUND, "cmd not recognized");
        }
        else dwResp = addErrMsg(jsResp, FCRESPONSE_INVALIDARG, "missing cmd property in query");
    }
    else dwResp = addErrMsg(jsResp, FCRESPONSE_INVALIDARG, "missing query object in req");

    return dwResp;
}


uint32_t EdgeChatSock::onAgentQuery_reply(FcMsg& msg, MfcJsonObj& jsData, MfcJsonObj& jsResp, int64_t nReqId)
{
    //string sCmd, sVal, sKey;
    //MfcJsonPtr pQuery;

    // TODO
    // ...

    //_MESG("AGENTDBG: FCCHAN_QUERY reply handler for reqId: %ld (not implemented).   obj: %s", nReqId, jsData.prettySerialize().c_str());

    // Returning FCRESPONSE_QUEUED instructs caller to NOT send jsResp
    // back to edgechat, since we recongize that this is not a request
    // that needs any reply -- it's a response itself to some previous
    // request we made.
    //
    return FCRESPONSE_QUEUED;
}


uint32_t EdgeChatSock::onAgentQuery(FcMsg& msg, MfcJsonObj& jsData, MfcJsonObj& jsResp)
{
    uint32_t dwResp = FCRESPONSE_ERROR, dwModel = 0, dwErr = FCRESPONSE_UNKNOWN;
    int64_t nReqId = -1;
    bool reply = false;

    if (jsData.objectGetInt("_reqid", nReqId))
        jsResp.objectAdd("_reqid", nReqId);

    if (jsData.objectGetInt("model", dwModel))
        jsResp.objectAdd("model", dwModel);

    jsResp.objectAdd("op", FCCHAN_QUERY);
    jsResp.objectAdd("from", m_sessionId);
    jsResp.objectAdd("to", msg.dwFrom);

    if (msg.dwFrom > 0)
    {
        if (jsData.objectGetBool("reply", reply))
        {
            if ( ! reply )  dwResp = onAgentQuery_request(msg, jsData, jsResp, nReqId);
            else            dwResp = onAgentQuery_reply(msg, jsData, jsResp, nReqId);
        }
        else dwResp = addErrMsg(jsResp, FCRESPONSE_INVALIDARG, "missing reply property in FCCHAN_QUERY req");
    }
    else
    {
        // return FCRESPONSE_QUEUED to prevent looping message back to server
        if (jsData.objectGetInt("_err", dwErr) && dwErr == FCRESPONSE_SUCCESS)
        {
            dwResp = addErrMsg(jsResp, FCRESPONSE_QUEUED, "server accepted our FCCHAN_QUERY reply");
        }
        else
        {
            _MESG("server rejected FCCHAN_QUERY msg we sent: %s", FcMsg::textMsg(false, msg).c_str());
            dwResp = addErrMsg(jsResp, FCRESPONSE_QUEUED, "server rejected FCCHAN_QUERY msg");
        }
    }

    return dwResp;
}


uint32_t EdgeChatSock::onAgent(FcMsg& msg, MfcJsonObj& jsData, MfcJsonObj& jsResp)
{
    // if dwResp remains FCRESPONSE_QUEUED, no response is sent to chatserver.
    // this is mainly for agent msgs of FCCHAN_QUERY that are relayued from other agents
    // or modelweb clients and need a response sent to chat server to relay back.
    uint32_t dwResp = FCRESPONSE_QUEUED;
    uint32_t dwOp = FCCHAN_NOOPT;

    //_MESG("[DBG Edge] Agent msg TODO: %s", FcMsg::textMsg(false, msg).c_str());
    if (jsData.objectGetInt("op", dwOp))
    {
        if (dwOp == FCCHAN_QUERY)
        {
            dwResp = onAgentQuery(msg, jsData, jsResp);
        }
        else if (dwOp == FCCHAN_NOTIFY)
        {
            // a bot/agent sending notification to us/channel
            onAgentNotify(jsData);
        }
        else if (dwOp == FCCHAN_PART || dwOp == FCCHAN_JOIN)
        {
            // another bot/agent joined or left metachannel
            onAgentJoin(dwOp, jsData);
        }
        else if (dwOp == FCCHAN_UPDATE)
        {
            // another bot/agent sending updated info about their status
            onAgentUpdate(jsData);
        }
    }

    return dwResp;
}


void EdgeChatSock::onAgentNotify(MfcJsonObj& jsData)
{
    //_MESG("AGENTDBG: FCCHAN_NOTIFY received; %s", jsData.prettySerialize().c_str());
}


void EdgeChatSock::onAgentJoin(uint32_t dwOp, MfcJsonObj& jsData)
{
    uint32_t dwModel = 0, dwFrom = 0;

    if (jsData.objectGetInt("model", dwModel))
    {
        if (jsData.objectGetInt("from", dwFrom))
        {
            if (dwOp == FCCHAN_JOIN)
            {
                _MESG("Agent %u joined model %u's metachannel.", dwFrom, dwModel);
                // TODO: Add to local cache of current agents
                // ...
            }
            else if (dwOp == FCCHAN_PART)
            {
                _MESG("Agent %u left model %u's metachannel.", dwFrom, dwModel);
                // TODO: Remove from local cache of current agents
                // ...
            }
            else obs_error("invalid op for onAgentJoin, should be FCCHAN_JOIN or FCCHAN_PART, not %u", dwOp);
        }
        else obs_error("missing 'from' in jsdata, unable to process join/part");
    }
    else obs_error("missing model in jsdata, unable to process join/part");
}


void EdgeChatSock::onAgentUpdate(MfcJsonObj& jsData)
{
    //_MESG("AGENTDBG: FCCHAN_UPDATE received; %s", jsData.prettySerialize().c_str());
}


void EdgeChatSock::onSessionState(FcMsg& msg, MfcJsonObj& jsData)
{
    //_MESG("AGENTDBG: session state TODO; %s", FcMsg::textMsg(false, msg).c_str());
}


// Produces random integers uniformly distributed on the interval [min, max]
int randomInt(int min, int max)
{
    std::random_device rd;                              // obtain a random number from hardware
    std::mt19937 gen(rd());                             // seed the generator
    std::uniform_int_distribution<> distr(min, max);    // define the range
    return distr(gen);
}


// static
std::string EdgeChatSock::FcsServer()
{
    unsigned int dwLen = 0;
    const string serverConfigUrl("https://assets.mfcimg.com/_js/serverconfig.js");
    string server = "";

    try
    {
        CCurlHttpRequest httpreq;
        uint8_t* pResponse = httpreq.Get(serverConfigUrl, &dwLen);
        if (pResponse != nullptr)
        {
            njson res = njson::parse((char*)pResponse, nullptr, false);
            free(pResponse);
            pResponse = nullptr;
            if (res.is_discarded())
            {
                _MESG("Error parsing serverconfig");
                return "xchat100";
            }
            njson wsServers = res["websocket_servers"];
            std::vector<string> servers;
            for (const auto& it: wsServers.items())
            {
                servers.push_back(it.key());
            }

            server = servers[randomInt(0, (int)servers.size() - 1)];
        }
    }
    catch (const std::exception& e)
    {
        _MESG("Error fetching chat server: %s", e.what());
        server = "xchat100";
    }
    catch (const string& e)
    {
        _MESG("Error fetching chat server: %s", e.c_str());
        server = "xchat100";
    }
    catch (...)
    {
        _MESG("Error fetching chat server");
        server = "xchat100";
    }

    return server;
}

#if 0
void EdgeChatSock::collectAgentHostData(MfcJsonPtr pHost)
{
    string sHostname, sOpertingSystem, sOsVer, sSidekickVer;

#ifdef _WIN32
    DWORD dwSz = 0;
    char szBuf[MAX_COMPUTERNAME_LENGTH + 1];
    if (GetComputerNameA(szBuf, &dwSz))
    {
        sHostname = szBuf;
    }
    OSVERSIONINFOA osver;
#else
    char szBuf[HOST_NAME_MAX];
    gethostname(szBuf, HOST_NAME_MAX);
    sHostname = szBuf;
    sOpertingSystem = "unknown";
    sHostname = "unknown";
    sOsVer = "0.0";
#endif
    pHost->objectAdd("hostname", sHostname);
    pHost->objectAdd("os", sOperatingSystem);
    pHost->objectAdd("os_ver", sOsVer);
    pHost->objectAdd("sidekick_ver", sSidekickVer);
}
#endif

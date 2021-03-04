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

#ifndef MFC_AGENT_EDGESOCK
#define MFC_AGENT_EDGESOCK 1
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif
// Windows Header Files
#include <windows.h>
#include <io.h>
#include <process.h>
#endif // _WIN32

#include <deque>
#include <list>
#include <mutex>
#include <string>
#include <fcntl.h>
#include <stdio.h>

#include <libobs/obs.h>

#include <libfcs/MfcJson.h>
#include <libfcs/Log.h>
#include <ObsBroadcast/ObsBroadcast.h>

#include "Portable.h"
#include "MFCConfigConstants.h"
#include "SysParam.h"
#include "ObsUtil.h"

#include "HttpRequest.h"
#include "MFCPluginAPI.h"

#include "build_version.h"

using std::string;

CBroadcastCtx g_ctx(true);

std::deque<SIDEKICK_UI_EV>  CBroadcastCtx::sm_eventQueue;
std::recursive_mutex        CBroadcastCtx::sm_eventLock;
EdgeChatSock*               CBroadcastCtx::sm_edgeSock = nullptr;


CMFCPluginAPI::CMFCPluginAPI(PROGRESS_CALLBACK pfnProgress)
{
    m_pfnProgress   = pfnProgress;
    m_sHost         = MFC_API_SVR;
    m_sFileHost     = MFC_FILE_SVR;

#ifdef _WIN64
    m_sPlatform     = PLATFORM_WIN64;
#elif _WIN32
    m_sPlatform     = PLATFORM_WIN32;
#else
    m_sPlatform     = PLATFORM_MAC;
#endif
}




// helper function to handle errors returned from rest api
int CMFCPluginAPI::HandleError(int nErr)
{
    int retVal = nErr;

    if (ERR_BAD_PARAMETER == nErr)
    {
        // ignore this error - return success code
        _TRACE("[IGNORING HTTP RESPONSE CODE: ERR_BAD_PARAMETER / %d]", nErr);
        retVal = S_OK;
    }

    return retVal;
}


int CMFCPluginAPI::SendHeartBeat(void)
{
    int nErr = ERR_NO_RESPONSE;
    CCurlHttpRequest httpreq;
    uint8_t* pResponse = nullptr;
    unsigned int dwLen = 0;
    MfcJsonObj jo, js;

    auto lk = g_ctx.sharedLock();
    string sURL = "https://sidekick.mfc.dev/agentSvc.php"; //MFC_AGENT_SVC_URL;
    string sPayload, tokenKey, sErr, sKey;
    time_t nNow = time(nullptr), tokenTm = 0;

    uint32_t nUid = g_ctx.cfg.getInt("uid");
    if (nUid == 0)
        return ERR_NEED_LOGIN;

    if (    g_ctx.cfg.getString("tok", tokenKey)
        &&  g_ctx.cfg.getTime("tok_tm", tokenTm)
        &&  (nNow - tokenTm) < 300)
    {
        // continue existing session with fcs service using tok that is less than 5m old
        sKey = "tok";
    }
    else if (g_ctx.cfg.getString("ctx", tokenKey) && tokenKey.size() > 3)
    {
        sKey = "sk";
    }

    if (tokenKey.empty() || nUid < 100)
    {
        _MESG("Cannot send heartbeat, tokenKey '%s' and userId %u, require login first", tokenKey.c_str(), nUid);
        return ERR_NEED_LOGIN;
    }

    js.objectAdd("uid",             nUid);
    js.objectAdd(sKey,              tokenKey);
    js.objectAdd("plugin_version",  SIDEKICK_VERSION_STR);
    js.objectAdd("plugin_state",    g_ctx.activeState);
    js.objectAdd("pid",             (int)getpid());
    js.objectAdd("ver_obs",         obs_get_version_string() );
    js.objectAdd("ver_branch",      SIDEKICK_VERSION_GITBRANCH);
    js.objectAdd("ver_commit",      SIDEKICK_VERSION_GITCOMMIT);
    js.objectAdd("ver_buildtm",     SIDEKICK_VERSION_BUILDTM);

    string serviceType;
    if (g_ctx.isWebRTC)
    {
        serviceType = "mfc_webrtc";
    }
    else if (g_ctx.isRTMP && g_ctx.isMfc)
    {
        serviceType = "mfc_rtmp";
    }
    else serviceType = "custom";

    js.objectAdd("serviceType",  serviceType);

    dwLen = 0;
    js.Serialize(sPayload);

    if ((pResponse = httpreq.Post(sURL, &dwLen, sPayload, m_pfnProgress)) != nullptr && dwLen > 0)
    {
        if (jo.Deserialize(pResponse, dwLen))
        {
            jo.objectGetString("_msg", sErr);
            if (jo.objectGetInt("_err", nErr))
            {
                if (nErr == S_OK)
                {
                    SidekickActiveState prevState = g_ctx.activeState;
                    int prevUid = g_ctx.cfg.getInt("uid");
                    nErr = EFAULT;

                    if (g_ctx.DeserializeCfg(jo, true))
                    {
                        g_ctx.cfg.set("tok_tm", nNow);
                        g_ctx.cfg.writeProfileConfig();
                        nErr = S_OK;

                        // Debug log if we detect state or uid changes as a result of the new plugin config data
                        if (g_ctx.activeState != prevState || g_ctx.cfg.getInt("uid") != prevUid)
                        {
                            blog(   100,
                                    "[svcAgent Heartbeat] uid: %u => %u skState: %s (%u => %u)",
                                    prevUid,
                                    g_ctx.cfg.getInt("uid"),
                                    CBroadcastCtx::MapSidekickState(g_ctx.activeState),
                                    (unsigned int)prevState,
                                    (unsigned int)g_ctx.activeState);
                        }
#if 0
                        if (newServerUrl != origServerUrl)
                        {
                            blog(100, "[svcAgent] SERVER URL CHANGE: %s => %s", origServerUrl.c_str(), newServerUrl.c_str());
                            CBroadcastCtx::sendEvent(SkServerUrl, 0, 0, 0, 0, origServerUrl.c_str(), newServerUrl.c_str());
                        }
#endif

#if MFC_AGENT_EDGESOCK
                        MfcJsonPtr pEdge = NULL;
                        if (jo.objectGetObject("edgechat", &pEdge))
                        {
                            //string sUrl, sToken, sUser;
                            //if (    pEdge->objectGetString("url",   sUrl)
                            //    &&  pEdge->objectGetString("user",  sUser)
                            //    &&  pEdge->objectGetString("tok",   sToken))
                            string sToken, sUser;
                            if (    pEdge->objectGetString("user",  sUser)
                                &&  pEdge->objectGetString("tok",   sToken))
                            {
                                uint32_t dwModel;
                                if (jo.objectGetInt("uid", dwModel) && dwModel > USER_ID_START)
                                {
                                    // if edgechat socket isnt started, start it up with edgechat url/token data provided
                                    //sUrl = "wss://video502.myfreecams.com:443/fcsl";
                                    //sUrl = "wss://xchat100.myfreecams.com/fcsl";
                                    //CBroadcastCtx::startEdgeSock(sUser, dwModel, sToken, sUrl);
                                    CBroadcastCtx::startEdgeSock(sUser, dwModel, sToken);
                                }
                                else _MESG("DBG: no model uid in json obj, unable to start edgesock: %s", jo.Serialize().c_str());
                            }
                            else _MESG("DBG: no edgechat url, user, or tok provided, unable to start edgesock: %s",
                                       pEdge->Serialize().c_str());
                        }
#endif
                    }
                    else stdprintf(sErr, "Unable to re-encode tkx from sidekick svc resp (%u bytes): %s",
                                   dwLen, string((const char*)pResponse, (size_t)dwLen).c_str());
                }
                else if (nErr == EPERM)
                {
                    uint32_t nCurSid = g_ctx.cfg.getInt("sid");
                    uint32_t nCurUid = g_ctx.cfg.getInt("uid");

                    // Sidekick agent svc response had error, make sure to correct g_ctx state if it thinks we are linked
                    _MESG("[PROFILEDBG] error processing response from SendHeartBeat, clear/writing config...");
                    g_ctx.clear(true);
                    g_ctx.cfg.writeProfileConfig();
                    g_ctx.cfg.readProfileConfig();

                    CBroadcastCtx::sendEvent(SkUnlink, nCurSid, nCurUid);
                 }
            }
            else stdprintf(sErr, "sidekick agent svc response missing _err prop");
        }
        else stdprintf(sErr, "unable to deserialize sidekick service response of (%u bytes): %s",
                       dwLen, dwLen > 0 ? string((const char*)pResponse, (size_t)dwLen).c_str() : "<empty>");

        free(pResponse);
        pResponse = nullptr;
    }
    else
    {
        stdprintf(sErr, "HTTP Err/Empty/Shutting-down; sidekick svc http-err:%d, resp-len:%u", httpreq.getResult(), dwLen);
        nErr = ERR_NO_RESPONSE;
    }

    if (nErr != S_OK && sErr.empty())
        stdprintf(sErr, "Unknown error, _msg empty / _err == %u", nErr);

    uint32_t dwSid = 0;
    jo.objectGetInt("sid", dwSid);

    if ( ! sErr.empty() && nErr != S_OK)
    {
        _MESG("SidekickSvcErr (%u): %s", nErr, sErr.c_str());
        setLastHttpError(sErr);
    }

    if ((nErr = HandleError(nErr)) != S_OK)
        _TRACE("Heartbeat failed, returned _err:%d", nErr);

    return nErr;
}




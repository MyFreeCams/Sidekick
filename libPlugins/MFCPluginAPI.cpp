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


// report the plugin startup and fetch parameters from the server.
int CMFCPluginAPI::StartupReport(int nPluginType)
{
    unsigned int dwLen = 0;
    uint8_t* pResponse = nullptr;
    MfcJsonObj json;

    //auto lk = g_ctx.sharedLock();
    //if ( ! g_ctx.readPluginConfig())
    //    _MESG("FAILED to readPluginConfig()....");

    int nUserId         = g_ctx.cfg.getInt("uid");
    string sModelUser   = g_ctx.cfg.getString("username");
    string sModelPwd    = g_ctx.cfg.getString("vidctx");
    string sStreamKey   = g_ctx.cfg.getString("ctx");

    if (sStreamKey.empty())
    {
        _TRACE("cant run startup report, no modelStreamingKey to provide API for auth");
        return ERR_BAD_PARAMETER;
    }

    json.objectAdd("modelUserID",       nUserId);
    json.objectAdd("modelStreamingKey", sStreamKey);
    json.objectAdd("pluginType",        nPluginType);

    string s = json.prettySerialize();

    string sURL = m_sHost;
    sURL += STARTUP_API;
    _TRACE("StartupReport URL %s", sURL.c_str());

    string sPayload = json.Serialize();
    _MESG("Startup Payload:\r\n%s\r\n", sPayload.c_str());
    json.clear();

    int nErr = ERR_NO_RESPONSE;
    CCurlHttpRequest httpreq;
    if ((pResponse = httpreq.Post(sURL, &dwLen, sPayload, m_pfnProgress)) != nullptr)
    {
        MfcJsonObj jo;
        if (jo.Deserialize(pResponse, dwLen))
        {
            jo.objectGetInt(HTTP_RESP_ERR, nErr);
            _TRACE("Return: %s", pResponse);

            string sErr = "";
            if (jo.objectGetString(HTTP_RESP_ERRMSG, sErr))
            {
                _TRACE("Failure errmsg from response \"%s\"", sErr.c_str());
                _TRACE("parameters: %s",s.c_str());
                setLastHttpError(sErr);
            }

            if (0 == nErr)
            {
                string sResult;
                if (jo.objectGetString(HTTP_RESP_RESULT, sResult))
                {
                    if ( !g_ctx.cfg.Deserialize(sResult) )
                        _MESG("failed to deserialize configuration block from API response object");
                }
                else _TRACE("unable to find resp object inside json results: ", jo.prettySerialize().c_str());
            }
        }
        else setLastHttpError("Unable to deserialize json response");

        free(pResponse);
        pResponse = nullptr;
    }
    else setLastHttpError("No HTTP Response");

    nErr = HandleError(nErr);
    if (nErr != S_OK)
        _TRACE("Startup Report failed returning %d; %s", nErr, m_sLastError.c_str());

    return nErr;
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


int CMFCPluginAPI::SendSystemReport(void)
{
    string sBinPath = CObsUtil::FindPluginDirectory();

    int nErr = ERR_NO_RESPONSE;
    uint8_t* pResponse = nullptr;
    unsigned int dwLen = 0;

    // collect the system data
    CSysParamList parms;
    parms.CollectData(sBinPath);

    auto lk = g_ctx.sharedLock();
    g_ctx.readPluginConfig(false);
    MfcJsonObj json;

    json.objectAdd("modelUserID", g_ctx.cfg.getInt("uid"));
    string sMSK = g_ctx.cfg.getString("ctx");
    json.objectAdd("modelStreamingKey", sMSK);

    // add the parameters as an array.
    parms.ToJson(json);

    string sURL = m_sHost;
    sURL += SYSREPORT_API;

    string sPayload = json.Serialize();
    _TRACE("Sending System Report URL: %s\r\nPayload: \r\n%s", sURL.c_str(), sPayload.c_str());

    CCurlHttpRequest httpreq;
    pResponse = httpreq.Post(sURL, &dwLen, sPayload, m_pfnProgress);
    if (pResponse != nullptr)
    {
        _TRACE("response: %s", (char*)pResponse);
        MfcJsonObj jo;
        jo.Deserialize(pResponse, dwLen);

        bool bParse = jo.objectGetInt(HTTP_RESP_ERR, nErr);
        string sErr = "";
        if (jo.objectGetString(HTTP_RESP_ERRMSG, sErr))
        {
            _TRACE("Failure errmsg from response \"%s\"", sErr.c_str());
            setLastHttpError(sErr);
        }

        if (bParse && 0 == nErr)
        {
            _TRACE("Successful call to %s", SYSREPORT_API);
        }
        else if (!bParse)
        {
            // this really shouldn't happen.
            string sBuf;
            for (unsigned int i = 0; i < dwLen; i++)
                sBuf += (char)pResponse[i];
            _TRACE("Parsing error %s", sBuf.c_str());
        }
        else
        {
            _TRACE("Error returned from %s %d", STARTUP_API, nErr);
        }
        free(pResponse);
        pResponse = nullptr;
    }
    else
    {
        setLastHttpError("No HTTP Response");
        _TRACE("Http Error %d", httpreq.getResult());
        // either no response, we can't get out or there is a stop request in progress.
        nErr = ERR_NO_RESPONSE;
    }

    nErr = HandleError(nErr);
    if (nErr != S_OK)
        _TRACE("SendSystem Report failed returning %d", nErr);

    return nErr;
}


int CMFCPluginAPI::SendHeartBeat(void)
{
    int nErr = ERR_NO_RESPONSE;
    CCurlHttpRequest httpreq;
    uint8_t* pResponse = nullptr;
    unsigned int dwLen = 0;
    MfcJsonObj jo, js;

    auto lk = g_ctx.sharedLock();
    string sURL = MFC_AGENT_SVC_URL;
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
                        g_ctx.cfg.writePluginConfig();
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
                    g_ctx.clear(true);
                    g_ctx.cfg.writePluginConfig();
                    g_ctx.cfg.readPluginConfig();

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


// send the plugin shutdown report
int CMFCPluginAPI::ShutdownReport(int nPluginType)
{
    //string sModelID = ppb.getModelID();
    int nErr = ERR_NO_RESPONSE;
    uint8_t* pResponse = nullptr;
    unsigned int dwLen = 0;

    auto lk = g_ctx.sharedLock();
    MfcJsonObj json;

    json.objectAdd("modelUserID", g_ctx.cfg.getInt("uid"));
    json.objectAdd("pluginType", nPluginType);
    json.objectAdd("modelStreamingKey", g_ctx.cfg.getString("ctx"));

#ifdef _DEBUG
    string s = json.prettySerialize();
#endif

    string sURL = m_sHost + SHUTDOWN_API;
    _TRACE("Shutdown URL: %s", sURL.c_str());
    string sPayload = json.Serialize();

    CCurlHttpRequest httpreq;
    pResponse = httpreq.Post(sURL, &dwLen, sPayload, m_pfnProgress);
    if (pResponse != nullptr)
    {
        MfcJsonObj jo;
        jo.Deserialize(pResponse, dwLen);

        bool bParse = jo.objectGetInt(HTTP_RESP_ERR, nErr);
        string sErr = "";
        if (jo.objectGetString(HTTP_RESP_ERRMSG, sErr))
        {
            _TRACE("Failure errmsg from response \"%s\"", sErr.c_str());
            setLastHttpError(sErr);
        }

        if (bParse && 0 == nErr)
        {
            _TRACE("Successful call to %s", STARTUP_API);
        }
        else if (!bParse)
        {
            string sBuf;
            for (unsigned int i = 0; i < dwLen; i++)
                sBuf += (char*)pResponse[i];
            _TRACE("Parsing error %s", sBuf.c_str());
        }
        else
        {
            _TRACE("Error returned from %s %d", STARTUP_API, nErr);
        }
        free(pResponse);
        pResponse = nullptr;
    }
    else
    {
        setLastHttpError("No HTTP Response");
        _TRACE("Http Error %d", httpreq.getResult());
        // either no response, we can't get out, or there is a stop request in progress.
        nErr = ERR_NO_RESPONSE;
    }

    nErr = HandleError(nErr);
    if (nErr)
        _TRACE("Shutdown failed returning %d", nErr);

    return nErr;
}


int CMFCPluginAPI::getManifestFile(const string& sVersion, string& sFile)
{
    int nErr = ERR_NO_RESPONSE;
    string sManifestFile = stdprintf("%s/%s/%s/%s", m_sFileHost.c_str(), sVersion.c_str(),
                                     m_sPlatform.c_str(), MANIFEST_FILE);
    CCurlHttpRequest httpreq;

#define _LOCAL_MANIFEST_
#ifdef _LOCAL_MANIFEST_
    string sRes;
    if (stdGetFileContents(sManifestFile, sRes) > 0)
    {
        sFile = sRes;
        nErr = 0;
    }
#else
    // fetch the manifest file from the server.
    unsigned int dwLen = 0;
    uint8_t* pResponse = httpreq.Get(sManifestFile.c_str(), &dwLen, "", m_pfnProgress);
    if (pResponse != nullptr)
    {
        string sRes = (char*)pResponse;
        size_t nOffset = sRes.find("<html>", 0);
        if (nOffset > 0 && nOffset < 5)
        {
            // html tag with first 5 chars, this is not a manifest file.
            _TRACE("Error downloading file %s", sRes.c_str());
            setLastHttpError(sRes);
            nErr = ERR_FILE_ERROR;
        }
        else
        {
            *psFile = (char*)pResponse;
            nErr = 0;
        }
        free(pResponse);
        pResponse = nullptr;
    }
#endif
    else
    {
        setLastHttpError("No HTTP Response");
        _TRACE("Http Error %d", httpreq.getResult());
        // either no response, we can't get out, or there is a stop request in progress.
        nErr = ERR_NO_RESPONSE;
    }

    return HandleError(nErr);
}


// fetch a file from the server.
int CMFCPluginAPI::getUpdateFile(const string& sVersion, const string& sTargetFile, uint8_t* pFileContents,
                                 unsigned int nSize, unsigned int* pFileSize)
{
    string sFile = stdprintf("%s/%s/%s/%s", m_sFileHost.c_str(), sVersion.c_str(),
                             m_sPlatform.c_str(), sTargetFile.c_str());
    int nErr = ERR_NO_RESPONSE;
    uint8_t* pchFile = nullptr;
    unsigned int dwFileLen = 0;
    CCurlHttpRequest httpreq;

#ifdef _LOCAL_MANIFEST_
#ifdef _WIN32
    UNREFERENCED_PARAMETER( nSize );
    DBG_UNREFERENCED_LOCAL_VARIABLE( dwFileLen );
    DBG_UNREFERENCED_LOCAL_VARIABLE( pchFile );
#else
    UNUSED_PARAMETER( nSize );
    UNUSED_PARAMETER( dwFileLen );
    UNUSED_PARAMETER( pchFile );
#endif
    string sRes;
    if (stdGetFileContents(sFile, sRes) > 0)
    {
        memcpy(pFileContents, sRes.c_str(), sRes.size());
        *pFileSize = (unsigned int)sRes.size();
        nErr = 0;
    }
#else  // _LOCAL_MANIFEST_
    _TRACE("Downloading file %s", m_sFileHost.c_str());
    if ((pchFile = httpreq.Get(sFile.c_str(), &dwFileLen, "", m_pfnProgress)) != nullptr)
    {
        if (dwFileLen > nSize)
        {
            _TRACE("File size exceeded %d %d", dwFileLen, nSize);
            setLastHttpError("File size exceeded!");
            nErr = 2;
        }
        else
        {
            string sRes = (char*)pchFile;
            size_t nOffset = sRes.find("<html>", 0);
            if (nOffset > 0 && nOffset < 5)
            {
                // html tag with first 5 chars, this is probably not a binary file.
                _TRACE("Error downloading file %s", sRes.c_str());
                setLastHttpError(sRes);
                nErr = ERR_FILE_ERROR;
            }
            else
            {
                _TRACE("Successful download of file: %s", sFile.c_str());
                nErr = 0;
                memcpy(pFileContents, pchFile, dwFileLen);
                *pFileSize = dwFileLen;
            }
        }
        free(pchFile);
        pchFile = nullptr;
    }
    else
    {
        setLastHttpError("No HTTP Response");
        _TRACE("Http Error %d", httpreq.getResult());
        // either no response, we can't get out or there is a stop request in progress.
        nErr = ERR_NO_RESPONSE;
    }
#endif

    return HandleError(nErr);
}


// get the version of the plugin assigned to the model.
int CMFCPluginAPI::getPluginVersionForModel(string& sVersion)
{
    sVersion = "default";

    return S_OK;
#if 0
    const string sModelID = pBlock->getModelID();
    int nErr = 0;
    uint8_t* pResponse;

    unsigned int dwLen = 0;
    string sUrl = m_sHost;
    sUrl += MODELVERSION_API;

    string sPayload; // = json.Serialize();

    *psVersion = DEFAULT_VERSION;
    MfcJsonObj json;
    json.objectAdd(string(JSON_MODEL_USR_ID), "0");
    json.objectAdd(string(JSON_PLUGIN_TYPE), pBlock->getVersion());
    string sStreamKey = pBlock->getMSK();
    json.objectAdd(string(JSON_MODEL_STRM_KEY), sStreamKey);

#ifdef _DEBUG
    string s = json.prettySerialize();
#endif

    sPayload = json.Serialize();
    CCurlHttpRequest httpreq;
    pResponse = httpreq.Post(sUrl, &dwLen, sPayload, m_pfnProgress);
    if (pResponse != nullptr)
    {
        MfcJsonObj js;
        js.Deserialize(pResponse, dwLen);

        bool bParse = js.objectGetInt(HTTP_RESP_ERR, nErr);
        string sErr = "";
        if (js.objectGetString(HTTP_RESP_ERRMSG, sErr))
        {
            _TRACE("Failure errmsg from response \"%s\"", sErr.c_str());
            setLastHttpError(sErr);
        }

        if (bParse && 0 == nErr)
        {
            if (js.objectGetString(HTTP_RESP_RESULT, *psVersion))
            {
                _TRACE("Successful call to %s Model version: %s", MODELVERSION_API, psVersion->c_str());
                std::transform(psVersion->begin(), psVersion->end(), psVersion->begin(), ::tolower);
            }
            else
            {
                *psVersion = DEFAULT_VERSION;
            }
        }
        else if (!bParse)
        {
            string sBuf;
            for (unsigned int i = 0; i < dwLen; i++)
                sBuf += (char)pResponse[i];
            _TRACE("Parsing error %s", sBuf.c_str());
            *psVersion = DEFAULT_VERSION;
        }
        else
        {
            _TRACE("Error returned from %s %d", MODELVERSION_API, nErr);
            // fall back to defeault
            nErr = 0;
        }
        free(pResponse);
        pResponse = nullptr;
    }
    else
    {
        setLastHttpError("No HTTP Response");
        _TRACE("Http Error %d", httpreq.getResult());
        // either no response, we can't get out or there is a stop request in progress.
        nErr = ERR_NO_RESPONSE;
        *psVersion = DEFAULT_VERSION;
    }
    pBlock->setVersion(*psVersion);

    // this function always succeeds. As a fall back it will at a minimum return "default" as the version.
    return nErr;
#endif
}

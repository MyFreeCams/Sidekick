/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
 *
 * Created by Kiran on 10/12/20
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

#include "MfcOauthApi.h"

#include <libfcs/Log.h>

#include <nlohmann/json.hpp>
#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>

#include <memory>
#include <sstream>
#include <thread>

#ifndef MFC_OAUTH_API_URL
#define MFC_OAUTH_API_URL "https://dev-mfc8.myfreecams.com/api"
#endif

#ifndef MFC_USER_AUTH_URL
#define MFC_USER_AUTH_URL "https://rp-sf1.mfc.dev"
#endif

#ifndef MFC_API_CLIENT_ID
#define MFC_API_CLIENT_ID "splitcam"
#endif

using njson = nlohmann::json;


MfcOauthApi::MfcOauthApi()
    : m_sClientId(MFC_API_CLIENT_ID)
    , m_sApiBaseUrl(MFC_OAUTH_API_URL)
    , m_sAuthBaseUrl(MFC_USER_AUTH_URL)
    , m_sCreateUrl(m_sApiBaseUrl + "/oauth/device/create")
    , m_sQueryUrlRoot(m_sApiBaseUrl + "/oauth/device/query")
    , m_sQueryUrl(m_sQueryUrlRoot)
    , m_sAgentServiceUrl(m_sApiBaseUrl + "/agent_service")
    , m_sLogoutUrl(m_sAuthBaseUrl + "/link/" + m_sClientId + "/logout")
    , m_sLinkCode("")
    , m_nRefId(0)
    , sid_(0)
    , uid_(0)
    , room_(0)
    , fps_(0)
    , camserv_(0)
    , camscore_(0.0f)
{
    RestClient::init();
    m_sAgentServiceUrl = "https://rp-sf1.mfc.dev/api/agent_service"; // TODO: DELETE
}


MfcOauthApi::~MfcOauthApi()
{
    RestClient::disable();
}


MfcOauthApi::MfcOauthApi(const MfcOauthApi& other)
{
    copyFrom(other);
}


const MfcOauthApi& MfcOauthApi::operator=(const MfcOauthApi& other)
{
    return copyFrom(other);
}


MfcOauthApi& MfcOauthApi::copyFrom(const MfcOauthApi& other)
{
    // Lock mutex of ourself/other
    std::unique_lock<std::recursive_mutex> otherLock  = other.sharedLock();
    std::unique_lock<std::recursive_mutex> ourLock    = sharedLock();

    m_sClientId         = other.clientId();

    m_sApiBaseUrl       = other.apiBaseUrl();
    m_sAuthBaseUrl      = other.authBaseUrl();
    m_sCreateUrl        = other.createUrl();
    m_sAgentServiceUrl  = other.agentServiceUrl();
    m_sLogoutUrl        = other.logoutUrl();

    m_nRefId            = other.refId();
    m_sLinkCode         = other.linkCode();
    m_sLinkUrl          = other.linkUrl();

    m_sQueryUrlRoot     = other.queryUrlRoot();
    m_sQueryUrl         = other.queryUrl();
    m_sQueryResponse    = other.queryResponse();

    sid_                = other.sid();
    uid_                = other.uid();
    room_               = other.room();
    fps_                = other.fps();
    camserv_            = other.camserv();
    camscore_           = other.camscore();
    tok_                = other.tok();
    videoserver_        = other.videoserver();
    username_           = other.username();
    pwd_                = other.pwd();
    streamkey_          = other.streamkey();
    ctx_                = other.ctx();
    vidctx_             = other.vidctx();
    region_             = other.region();
    prot_               = other.prot();
    codec_              = other.codec();

    return *this;
}


bool MfcOauthApi::NewLinkCode()
{
    const std::string data = "clientId=" + m_sClientId;

    auto conn = std::make_unique<RestClient::Connection>("");
    conn->SetTimeout(5);
    conn->FollowRedirects(true);
    conn->AppendHeader("Content-Type", "application/x-www-form-urlencoded");
    RestClient::Response r = conn->post(m_sCreateUrl, data);

    if (r.code != 200)
    {
        _MESG("Error: POST %s to %s\nstatus: %d", data.c_str(), m_sCreateUrl.c_str(), r.code);
        return false;
    }

    auto res = njson::parse(r.body);
    if (!res["result"]["success"].get<bool>())
    {
        _MESG("Error: POST %s to %s\nbody: %s", data.c_str(), m_sCreateUrl.c_str(), res.dump().c_str());
        return false;
    }

    m_sApiBaseUrl   = res["result"]["apiUrls"]["base"].get<std::string>();
    m_sAuthBaseUrl  = res["result"]["mfcAuthUrls"]["base"].get<std::string>();

    m_sApiBaseUrl   = MFC_OAUTH_API_URL;  // TODO: DELETE
    m_sAuthBaseUrl  = MFC_USER_AUTH_URL;  // TODO: DELETE

    m_sQueryUrlRoot = m_sApiBaseUrl + res["result"]["apiUrls"]["query"].get<std::string>();

    m_sLinkUrl      = m_sAuthBaseUrl + res["result"]["mfcAuthUrls"]["link"].get<std::string>();
    m_sLogoutUrl    = m_sAuthBaseUrl + res["result"]["mfcAuthUrls"]["logout"].get<std::string>();

    m_nRefId        = res["result"]["session"]["refId"].get<int>();
    m_sLinkCode     = res["result"]["session"]["linkCode"].get<std::string>();

    return true;
}


bool MfcOauthApi::CheckIfLinked()
{
    if (m_sLinkCode.empty())
    {
        _MESG("Error: missing link code. |NewLinkCode()| must return true before executing |CheckIfLinked()|");
        return false;
    }

    std::stringstream ss;
    ss << m_sQueryUrlRoot << "?clientId=" << m_sClientId << "&linkCode=" << m_sLinkCode << "&refId=" << m_nRefId;
    m_sQueryUrl = ss.str();

    RestClient::Response r = RestClient::get(m_sQueryUrl);
    if (r.code == 422)
    {
        // Model is not authenticated yet.
        return false;
    }
    else if (r.code != 200)
    {
        _MESG("Error: GET %s\nstatus: %d", m_sQueryUrl.c_str(), r.code);
        return false;
    }

    auto res = njson::parse(r.body);
    if (res["err"].get<int>() != 0)
    {
        _MESG("Error: GET %s\nbody: %s", m_sQueryUrl.c_str(), res.dump().c_str());
        return false;
    }

    uid_        = res["result"]["userId"].get<int>();
    username_   = res["result"]["username"].get<std::string>();
    tok_        = res["result"]["userAuth"]["tok"].get<std::string>();
    streamkey_  = res["result"]["userAuth"]["streamKey"].get<std::string>();

    return true;
}


bool MfcOauthApi::FetchStreamingCredentials()
{
    njson data =
    {
        { "userId", uid_ },
        { "tok",    tok_ }
    };

    auto conn = std::make_unique<RestClient::Connection>("");
    conn->SetTimeout(5);
    conn->FollowRedirects(true);
    conn->AppendHeader("Content-Type", "application/json");
    RestClient::Response r = conn->post(m_sAgentServiceUrl, data.dump());

    if (r.code != 200)
    {
        _MESG("Error: POST %s to %s\nstatus: %d", data.dump().c_str(), m_sAgentServiceUrl.c_str(), r.code);
        return false;
    }

    auto res = njson::parse(r.body);
    if (res["data"]["_err"].get<int>() != 0)
    {
        _MESG("Error: POST %s to %s\nbody: %s", data.dump().c_str(), m_sAgentServiceUrl.c_str(), res.dump().c_str());
        return false;
    }

    m_sQueryResponse = r.body;

    int sid                 = res["data"]["sid"].get<int>();
    int uid                 = res["data"]["uid"].get<int>();
    int room                = res["data"]["room"].get<int>();
    int fps                 = res["data"]["fps"].get<int>();
    int camserv             = res["data"]["camserv"].get<int>();
    float camscore          = res["data"]["camscore"].get<float>();

    std::string tok         = res["data"]["tok"].get<std::string>();
    std::string videoserver = res["data"]["videoserver"].get<std::string>();
    std::string username    = res["data"]["username"].get<std::string>();
    std::string pwd         = res["data"]["pwd"].get<std::string>();
    std::string streamkey   = res["data"]["streamkey"].get<std::string>();
    std::string ctx         = res["data"]["ctx"].get<std::string>();
    std::string vidctx      = res["data"]["vidctx"].get<std::string>();
    std::string region      = res["data"]["region"].get<std::string>();
    std::string prot        = res["data"]["prot"].get<std::string>();
    std::string codec       = res["data"]["codec"].get<std::string>();

    if (videoserver != videoserver_ || camserv != camserv_ || region != region_ || codec != codec_
        || sid != sid_  || username != username_ || pwd != pwd_ || room != room_ || prot != prot_
        || streamkey != streamkey_ || ctx != ctx_ || vidctx != vidctx_ || fps != fps_)
    {
        // TODO: Notify application that streaming credentials have changed
    }

    sid_            = sid;
    uid_            = uid;
    room_           = room;
    fps_            = fps;
    camserv_        = camserv;
    camscore_       = camscore;
    tok_            = tok;
    videoserver_    = videoserver;
    username_       = username;
    pwd_            = pwd;
    streamkey_      = streamkey;
    ctx_            = ctx;
    vidctx_         = vidctx;
    region_         = region;
    prot_           = prot;
    codec_          = codec;

    return true;
}


std::unique_lock<std::recursive_mutex> MfcOauthApi::sharedLock() const
{
    std::unique_lock<std::recursive_mutex> lk(m_csMutex, std::defer_lock);
    lk.lock();
    return lk;
}

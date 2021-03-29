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

#pragma once

#ifndef MFC_OAUTH_API_H_
#define MFC_OAUTH_API_H_

#include <mutex>
#include <string>


/// Securely retrieve credentials needed to broadcast to MyFreeCams
/// using an external RTMP/WebRTC broadcaster.
class MfcOauthApi
{
public:
    MfcOauthApi();
    virtual ~MfcOauthApi();

    MfcOauthApi(const MfcOauthApi& other);
    const MfcOauthApi& operator=(const MfcOauthApi& other);

    MfcOauthApi& copyFrom(const MfcOauthApi& other);

    /// Generate new |linkCode()| and |linkUrl()|
    bool NewLinkCode();

    /// Returns true if the model has successfully authenticated with MyFreeCams.
    /// Execute a callback where |CheckIfLinked()| is called a single time
    /// and the result is returned. Iterate until result is true. The model's
    /// user ID |uid| and RTMP |streamKey| are populated on completion.
    bool CheckIfLinked();

    /// Fetch the credentials needed to broadcast to MyFreeCams using WebRTC.
    /// Periodically call this method (every 6-10 seconds) while the model is broadcasting.
    bool FetchStreamingCredentials();

    /// |NewLinkCode()| must successfully execute in order to populate
    /// |linkCode|, |refId|, and |queryUrl|.
    int refId() const { return m_nRefId; }
    std::string linkCode() const { return m_sLinkCode; }
    std::string linkUrl() const { return m_sLinkUrl; }
    std::string queryUrl() const { return m_sQueryUrl; }
    std::string logoutUrl() const { return m_sLogoutUrl; }

    /// |CheckIfLinked()| must return true prior to accessing these variables.
    std::string tok() const { return tok_; }
    std::string streamKey() const { return streamkey_; }
    std::string username() const { return username_; }
    int uid() const { return uid_; }

    /// |FetchStreamingCredentials()| populates these variables.
    int sid() const { return sid_; }
    int room() const { return room_; }
    int fps() const { return fps_; }
    float camscore() const { return camscore_; }
    int camserv() const { return camserv_; }
    std::string videoserver() const { return videoserver_; }
    std::string pwd() const { return pwd_; }
    std::string streamkey() const { return streamkey_; }
    std::string ctx() const { return ctx_; }
    std::string vidctx() const { return vidctx_; }
    std::string region() const { return region_; }
    std::string prot() const { return prot_; }
    std::string codec() const { return codec_; }

    std::string clientId() const { return m_sClientId; }

    std::string apiBaseUrl() const { return m_sApiBaseUrl; }
    std::string authBaseUrl() const { return m_sAuthBaseUrl; }
    std::string createUrl() const { return m_sCreateUrl; }
    std::string queryUrlRoot() const { return m_sQueryUrlRoot; }
    std::string queryResponse() const { return m_sQueryResponse; }
    std::string agentServiceUrl() const { return m_sAgentServiceUrl; }

    std::unique_lock<std::recursive_mutex> sharedLock() const;

protected:
    mutable std::recursive_mutex m_csMutex;

private:
    std::string m_sClientId;

    std::string m_sApiBaseUrl;
    std::string m_sAuthBaseUrl;
    std::string m_sCreateUrl;

    std::string m_sQueryUrlRoot;
    std::string m_sQueryUrl;
    std::string m_sQueryResponse;

    std::string m_sAgentServiceUrl;
    std::string m_sLogoutUrl;

    std::string m_sLinkCode;
    std::string m_sLinkUrl;
    int         m_nRefId;

    int         sid_;
    int         uid_;
    int         room_;
    int         fps_;
    int         camserv_;
    float       camscore_;
    std::string tok_;
    std::string videoserver_;
    std::string username_;
    std::string pwd_;
    std::string streamkey_;
    std::string ctx_;
    std::string vidctx_;
    std::string region_;
    std::string prot_;
    std::string codec_;
};

#endif  // MFC_OAUTH_API_H_

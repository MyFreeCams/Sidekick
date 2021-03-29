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

#ifndef MFC_OAUTH_API_CTX_H_
#define MFC_OAUTH_API_CTX_H_

#include "MfcOauthApi.h"

#include <QObject>

#include <memory>
#include <mutex>
#include <string>


class MfcOauthApiCtx
{
public:
    MfcOauthApiCtx();
    ~MfcOauthApiCtx() = default;

    MfcOauthApiCtx(const MfcOauthApiCtx& other);
    MfcOauthApiCtx& operator=(const MfcOauthApiCtx& other);

    MfcOauthApiCtx& copyFrom(const MfcOauthApiCtx& other);

    bool Link();
    void Unlink();

    bool FetchCredentials();

    bool IsLinked() const { return m_bIsLinked; }
    void SetLinked(bool isLinked) { m_bIsLinked = isLinked; }

    bool HaveCredentials() const { return m_bHaveCredentials; }
    void SetHaveCredentials(bool haveCredentials) { m_bHaveCredentials = haveCredentials; }

    std::string linkCode() const { return m_sLinkCode; }
    std::string linkUrl() const { return m_sLinkUrl; }
    std::string logoutUrl() const { return m_sLogoutUrl; }
    std::string queryResponse() const { return m_sQueryResponse; }

    int sid() const { return m_nSid; }
    int uid() const { return m_nUid; }
    int room() const { return m_nRoomId; }
    int fps() const { return m_nFps; }
    float camscore() const { return m_fCamScore; }

    std::string videoserver() const { return m_sVideoServer; }
    std::string username() const { return m_sUsername; }
    std::string pwd() const { return m_sPassword; }
    std::string streamkey() const  { return m_sStreamKey; }
    std::string vidctx() const { return m_sVidCtx; }
    std::string region() const { return m_sRegion; }
    std::string prot() const { return m_sProtocol; }
    std::string codec() const { return m_sVideoCodec; }

    // Wrapper for |MfcOauthApi::sharedLock()| for our |api| instance,
    // or when updating any of our other member properties
    std::unique_lock<std::recursive_mutex> sharedLock() const;
    //std::unique_lock<std::recursive_mutex> eventLock();

    MfcOauthApi api;

    //static std::recursive_mutex sm_eventLock;

    bool m_bIsLinked;
    bool m_bHaveCredentials;

    std::string m_sLinkCode;
    std::string m_sLinkUrl;
    std::string m_sLogoutUrl;
    std::string m_sQueryResponse;

    int m_nSid;
    int m_nUid;
    int m_nRoomId;
    int m_nFps;
    int m_nCamserv;
    float m_fCamScore;
    std::string m_sTok;
    std::string m_sVideoServer;
    std::string m_sUsername;
    std::string m_sPassword;
    std::string m_sStreamKey;
    std::string m_sCtx;
    std::string m_sVidCtx;
    std::string m_sRegion;
    std::string m_sProtocol;
    std::string m_sVideoCodec;
};


class CheckIfLinked : public QObject
{
    Q_OBJECT

public:
    explicit CheckIfLinked(MfcOauthApiCtx* apiCtx);
    ~CheckIfLinked() override = default;

public slots:
    void start();

private slots:
    void check();

signals:
    void finished();

private:
    MfcOauthApiCtx* m_pApiCtx;
    int sinceLast;
    int nextCheck;
};

#endif  // MFC_OAUTH_API_CTX_H_


// Example:
/*
 * #include "MfcOauthApiCtx.h"
 *
 * #include <QDesktopServices>
 * #include <QString>
 *
 * class MfcOauthApiExample
 * {
 * public:
 *     MfcOauthApiExample()
 *         : m_pApiCtx(std::make_unique<MfcOauthApiCtx>())
 *         , m_pCheck(std::make_unique<CheckIfLinked>(m_pApiCtx.get()))
 *     {
 *         QObject::connect(m_pCheck.get(), SIGNAL(finished()), this, SLOT(onLinked()));
 *     }
 *
 *     ~MfcOauthApiExample() = default;
 *
 *     void OnLink()
 *     {
 *         auto lk = m_pApiCtx->sharedLock();
 *
 *         m_pApiCtx->Link();
 *         QString qstr = QString::fromUtf8(m_pApiCtx->linkUrl().c_str());
 *         QDesktopServices::openUrl(QUrl(qstr));
 *
 *         g_check.start();
 *     }
 *
 *     void MfcOauthApiExample::onLinked()
 *     {
 *         {
 *             auto lk = m_pApiCtx->sharedLock();
 *
 *             if (!m_pApiCtx->FetchCredentials())
 *                 return;
 *
 *             //  The following variables are now populated:
 *             //
 *             //  m_pApiCtx->api.queryResponse()
 *             //  m_pApiCtx->api.codec()
 *             //  m_pApiCtx->api.prot()
 *             //  m_pApiCtx->api.region()
 *             //  m_pApiCtx->api.username()
 *             //  m_pApiCtx->api.pwd()
 *             //  m_pApiCtx->api.camscore()
 *             //  m_pApiCtx->api.sid()
 *             //  m_pApiCtx->api.uid()
 *             //  m_pApiCtx->api.room()
 *             //  m_pApiCtx->api.streamkey()
 *             //  m_pApiCtx->api.ctx()
 *             //  m_pApiCtx->api.vidctx()
 *             //  m_pApiCtx->api.videoserver()
 *         }
 *     }
 *
 *     bool AuthenticateModel()
 *     {
 *         auto lk = m_pApiCtx->sharedLock();
 *
 *         if (!m_pApiCtx->IsLinked())
 *             OnLink();
 *
 *         if (!m_pApiCtx->HaveCredentials() && !m_pApiCtx->FetchCredentials())
 *             return false;
 *
 *         m_sVideoCodec   = m_pApiCtx->codec();
 *         m_sProtocol     = m_pApiCtx->prot();
 *         m_sUsername     = m_pApiCtx->username();
 *         m_sPassword     = m_pApiCtx->pwd();
 *         m_fCamScore     = m_pApiCtx->camscore();
 *         m_nSid          = m_pApiCtx->sid();
 *         m_nUid          = m_pApiCtx->uid();
 *         m_nRoomId       = m_pApiCtx->room();
 *         m_sStreamKey    = m_pApiCtx->ctx();
 *         m_sVidCtx       = m_pApiCtx->vidctx();
 *         m_sVideoServer  = m_pApiCtx->videoserver();
 *
 *         return true;
 *     }
 *
 * private:
 *     std::unique_ptr<MfcOauthApiCtx> std::string m_pApiCtx;
 *     std::unique_ptr<CheckIfLinked> m_pCheck;
 *
 *     std::string m_sVideoCodec;
 *     std::string m_sProtocol;
 *     std::string m_sUsername;
 *     std::string m_sPassword;
 *     float m_fCamScore = 0.0f;
 *     int m_nSid = 0;
 *     int m_nUid = 0;
 *     int m_nRoomId = 0;
 *     std::string m_sStreamKey;
 *     std::string m_sVidCtx;
 *     std::string m_sVideoServer;
 * };
 */

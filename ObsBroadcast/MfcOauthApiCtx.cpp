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

#include "MfcOauthApiCtx.h"

#include <QTime>
#include <QTimer>


CheckIfLinked::CheckIfLinked(MfcOauthApiCtx* apiCtx)
    : QObject()
    , m_pApiCtx(apiCtx)
    , sinceLast(0)
    , nextCheck(0)
{}

void CheckIfLinked::start()
{
    QTimer::singleShot(0, this, SLOT(check()));
}

void CheckIfLinked::check()
{
    QTime t;
    t.start();

    while (t.elapsed() < 150)
    {
        if (sinceLast >= nextCheck)
        {
            auto lk = m_pApiCtx->sharedLock();
            if (m_pApiCtx->api.CheckIfLinked())
            {
                emit finished();
                return;
            }
            sinceLast = 0;
            nextCheck = 1200;
        }
    }
    sinceLast += 150;
    QTimer::singleShot(0, this, SLOT(check()));
}


MfcOauthApiCtx::MfcOauthApiCtx()
    : m_nSid(0)
    , m_nUid(0)
    , m_nRoomId(0)
    , m_nFps(0)
    , m_nCamserv(0)
    , m_fCamScore(0.0f)
{}


MfcOauthApiCtx::MfcOauthApiCtx(const MfcOauthApiCtx& other)
{
    copyFrom(other);
}


MfcOauthApiCtx& MfcOauthApiCtx::operator=(const MfcOauthApiCtx& other)
{
    return copyFrom(other);
}


MfcOauthApiCtx& MfcOauthApiCtx::copyFrom(const MfcOauthApiCtx& other)
{
    // Lock mutex of ourself or other if either are a sharedCtx.
    std::unique_lock<std::recursive_mutex> otherLock  = other.sharedLock();
    std::unique_lock<std::recursive_mutex> ourLock    = sharedLock();

    m_bIsLinked         = other.IsLinked();
    m_bHaveCredentials  = other.HaveCredentials();

    m_sLinkCode         = other.linkCode();
    m_sLinkUrl          = other.linkUrl();
    m_sLogoutUrl        = other.logoutUrl();
    m_sQueryResponse    = other.queryResponse();

    m_nSid              = other.sid();
    m_nUid              = other.uid();
    m_nRoomId           = other.room();
    m_nFps              = other.fps();
    m_fCamScore         = other.camscore();

    m_sVideoServer      = other.videoserver();
    m_sUsername         = other.username();
    m_sPassword         = other.pwd();
    m_sStreamKey        = other.streamkey();
    m_sVidCtx           = other.vidctx();
    m_sRegion           = other.region();
    m_sProtocol         = other.prot();
    m_sVideoCodec       = other.codec();

    api = other.api;

    return *this;
}


bool MfcOauthApiCtx::Link()
{
    if (!api.NewLinkCode())
        return false;

    auto lk = sharedLock();

    m_sLinkCode  = api.linkCode();
    m_sLinkUrl   = api.linkUrl();
    m_sLogoutUrl = api.logoutUrl();

    return true;
}


bool MfcOauthApiCtx::FetchCredentials()
{
    auto lk = sharedLock();

    if (!api.FetchStreamingCredentials())
    {
        m_bHaveCredentials = false;
        return false;
    }

    m_bIsLinked = true;
    m_bHaveCredentials = true;

    m_sQueryResponse = api.queryResponse();

    m_sVideoCodec   = api.codec();
    m_sProtocol     = api.prot();
    m_sRegion       = api.region();
    m_sUsername     = api.username();
    m_sPassword     = api.pwd();
    m_fCamScore     = api.camscore();
    m_nSid          = api.sid();
    m_nUid          = api.uid();
    m_nRoomId       = api.room();
    m_sStreamKey    = api.streamkey();
    m_sCtx          = api.ctx();
    m_sVidCtx       = api.vidctx();
    m_sVideoServer  = api.videoserver();

    return true;
}


void MfcOauthApiCtx::Unlink()
{
    auto lk = sharedLock();
    m_bIsLinked = false;
    m_bHaveCredentials = false;
}


std::unique_lock<std::recursive_mutex> MfcOauthApiCtx::sharedLock() const
{
    return api.sharedLock();
}


#if 0
std::unique_lock<std::recursive_mutex> MfcOauthApiCtx::eventLock()
{
    std::unique_lock<std::recursive_mutex> lk(sm_eventLock, std::try_to_lock);
    return lk;
}
#endif

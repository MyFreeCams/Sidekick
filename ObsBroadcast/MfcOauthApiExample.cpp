/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
 *
 * Created by Kiran on 12/1/20
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

#include "MfcOauthApiExample.h"

MfcOauthApiExample::MfcOauthApiExample()
    : m_pApiCtx(std::make_unique<MfcOauthApiCtx>())
    , m_pCheck(std::make_unique<CheckIfLinked>(m_pApiCtx.get()))
{
    QObject::connect(m_pCheck.get(), SIGNAL(finished()), this, SLOT(onLinked()));
}


void MfcOauthApiExample::OnLink(CefRefPtr<CefBrowser> browser)
 {
    std::string linkUrl;
    {
        auto lk = m_pApiCtx->sharedLock();
         if (!m_pApiCtx->Link())
            return;
        linkUrl = m_pApiCtx->linkUrl();
    }

    browser->LoadURL(linkUrl);

    m_pCheck->start();
}


void MfcOauthApiExample::onLinked()
{
    {
        auto lk = m_pApiCtx->sharedLock();

        if (!m_pApiCtx->FetchCredentials())
            return;

        /*
            The following variables are now populated:

            m_pApiCtx->api.queryResponse()
            m_pApiCtx->api.codec()
            m_pApiCtx->api.prot()
            m_pApiCtx->api.region()
            m_pApiCtx->api.username()
            m_pApiCtx->api.pwd()
            m_pApiCtx->api.camscore()
            m_pApiCtx->api.sid()
            m_pApiCtx->api.uid()
            m_pApiCtx->api.room()
            m_pApiCtx->api.streamkey()
            m_pApiCtx->api.ctx()
            m_pApiCtx->api.vidctx()
            m_pApiCtx->api.videoserver()
        */
    }
}


bool MfcOauthApiExample::AuthenticateModel()
{
    auto lk = m_pApiCtx->sharedLock();

    if (!m_pApiCtx->IsLinked())
        OnLink();

    if (!m_pApiCtx->HaveCredentials() && !m_pApiCtx->FetchCredentials())
        return false;

    m_sVideoCodec   = m_pApiCtx->codec();
    m_sProtocol     = m_pApiCtx->prot();
    m_sUsername     = m_pApiCtx->username();
    m_sPassword     = m_pApiCtx->pwd();
    m_fCamScore     = m_pApiCtx->camscore();
    m_nSid          = m_pApiCtx->sid();
    m_nUid          = m_pApiCtx->uid();
    m_nRoomId       = m_pApiCtx->room();
    m_sStreamKey    = m_pApiCtx->ctx();
    m_sVidCtx       = m_pApiCtx->vidctx();
    m_sVideoServer  = m_pApiCtx->videoserver();

    return true;
}

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

#pragma once

#ifndef MFC_OAUTH_API_EXAMPLE_H_
#define MFC_OAUTH_API_EXAMPLE_H_

#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_version.h>

#include "MfcOauthApiCtx.h"

class MfcOauthApiExample
{
public:
    MfcOauthApiExample();
    virtual ~MfcOauthApiExample() = default;

    void OnLink(CefRefPtr<CefBrowser> browser);
    void OnLinked();
    bool AuthenticateModel();

private:
    std::unique_ptr<MfcOauthApiCtx> std::string m_pApiCtx;
    std::unique_ptr<CheckIfLinked> m_pCheck;

    std::string m_sVideoCodec;
    std::string m_sProtocol;
    std::string m_sUsername;
    std::string m_sPassword;
    float m_fCamScore = 0.0f;
    int m_nSid = 0;
    int m_nUid = 0;
    int m_nRoomId = 0;
    std::string m_sStreamKey;
    std::string m_sVidCtx;
    std::string m_sVideoServer;
};

#endif  // MFC_OAUTH_API_EXAMPLE_H_

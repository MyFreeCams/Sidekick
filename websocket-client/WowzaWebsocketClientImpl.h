/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
 * Copyright (c) 2020 CoSMo - Dr Alex Gouaillard <contact@cosmosoftware.io>
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

#include "WebsocketClient.h"

#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include <string>
#include <thread>

typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;
typedef Client::connection_ptr ConPtr;


class WowzaWebsocketClientImpl : public WebsocketClient
{
public:
    WowzaWebsocketClientImpl();
    ~WowzaWebsocketClientImpl() override = default;

    ///
    /// WebsocketClient implementation.
    ///
    bool connect(Listener*          pListener,
                 const std::string& sUrl,
                 const std::string& sStreamName,
                 const std::string& sStreamKey,
                 const std::string& sPassword,
                 const std::string& sVidCtx,
                 int                nSid,
                 int                nUid,
                 int                nRoomId,
                 int                nWidth,
                 int                nHeight,
                 int                nFrameRate,
                 float              fCamScore) override;
    bool sendSdp(const std::string& sSdp, const std::string& /*sVideoCodec*/) override;
    bool trickle(const std::string& sCandidate,
                 const std::string& /*sMid*/, int /*nIndex*/, bool /*bIsLast*/) override;
    bool disconnect(bool bWait) override;

private:
    Client      m_client;
    ConPtr      m_pConnection;
    Listener*   m_pListener;
    std::thread m_thread;

    std::string m_sStreamName;
    std::string m_sPassword;
    int         m_nSid;
    int         m_nRoomId;
    int         m_nModelId;
    int         m_nWidth;
    int         m_nHeight;
    int         m_nFrameRate;
    float       m_fCamScore;

    bool        m_bAuthenticated;
    bool        m_bAnswerReceived;
    bool        m_bBroadcastStarted;
    bool        m_bUserClosedConnection;
};

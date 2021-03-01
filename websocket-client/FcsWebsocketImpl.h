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

#pragma once

#ifdef _WIN32
#pragma warning(disable : 4244)
#endif

#include "FcsWebsocket.h"

//#include <libobs/obs.h>

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

typedef websocketpp::client< websocketpp::config::asio_tls_client > Client;

class FcsWebsocketImpl : public FcsWebsocket
{
public:
    FcsWebsocketImpl();
    ~FcsWebsocketImpl() override;

    bool connect(   const std::string&  username,
                    const std::string&  token,
                    const std::string&  url,
                    FcsListener*        listener) override;

    bool disconnect(bool wait) override;
    bool send(const std::string& sMsg) override;

    bool isConnected = false;

private:
    FcsListener*                _listener;
    size_t                      _frameNum;
    std::string                 _username;
    std::string                 _token;
    std::string                 _url;
    int                         _uid;

    Client                      m_client;
    Client::connection_ptr      m_pConnection;
    std::thread                 m_thread;
};


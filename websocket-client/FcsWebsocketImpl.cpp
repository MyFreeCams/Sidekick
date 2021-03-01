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

#include "FcsWebsocketImpl.h"

#include <libobs/obs.h>

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <sstream>

#define obs_debug(format, ...) blog(400, format, ##__VA_ARGS__)
#define obs_info(format,  ...) blog(300, format, ##__VA_ARGS__)
#define obs_warn(format,  ...) blog(200, format, ##__VA_ARGS__)
#define obs_error(format, ...) blog(100, format, ##__VA_ARGS__)

using njson = nlohmann::json;
using std::string;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;


FcsWebsocketImpl::FcsWebsocketImpl()
    : _listener(NULL)
    , _frameNum(0)
    , _uid(0)
    , m_pConnection(NULL)
{
    // Set logging to be pretty verbose (everything except message payloads)
    m_client.set_access_channels(websocketpp::log::alevel::all);
    m_client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    m_client.clear_access_channels(websocketpp::log::alevel::frame_header);
    m_client.clear_access_channels(websocketpp::log::alevel::control);
    m_client.set_error_channels(websocketpp::log::elevel::all);

    // Initialize ASIO
    m_client.init_asio();
}


FcsWebsocketImpl::~FcsWebsocketImpl()
{
    // Disconnect just in case
    obs_debug("FcsWebsocketImpl::~FcsWebsocketImpl()");
    disconnect(false);
}


bool FcsWebsocketImpl::connect( const string&   username,
                                const string&   token,
                                const string&   url,
                                FcsListener*    listener    )
{
    bool retVal = true;

    _uid        = 0;
    _frameNum   = 0;
    _url        = url + "/fcsl";        // LEGACY Websockets format, add /fcsl dirname isntead of subprotocol
    _token      = token;
    _username   = username;
    _listener   = listener;

    try
    {
        m_client.set_tls_init_handler([&](websocketpp::connection_hdl /* unused */)
        {
            auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);

            try
            {
                ctx->set_options(   asio::ssl::context::default_workarounds
                                 |  asio::ssl::context::no_sslv2
                                 |  asio::ssl::context::no_sslv3
                                 |  asio::ssl::context::single_dh_use       );
            }
            catch (std::exception& e)
            {
                obs_error("TLS Exception: %s", e.what());
            }
            return ctx;
        });

        websocketpp::lib::error_code ec;
        m_pConnection = m_client.get_connection(_url, ec);
        if (ec)
        {
            obs_error("Error establishing TLS connection: %s", ec.message().c_str());
            return false;
        }
        if (!m_pConnection)
        {
            obs_warn("** NO CONNECTION **");
            return false;
        }

        // LEGACY Websockets Format, no fcsl subprotocol
        //m_pConnection->add_subprotocol("fcsl");

        m_pConnection->set_message_handler([=](websocketpp::connection_hdl /* unused */, message_ptr frame)
        {
            _frameNum++;
            string sMsg(frame->get_payload());
            listener->onMsg(sMsg);
        });

        m_pConnection->set_open_handler([=](websocketpp::connection_hdl /* unused */)
        {
            _frameNum = 0;
            try
            {
                listener->onConnected();
            }
            catch (const websocketpp::exception& e)
            {
                obs_error("Error in onConnected handdler: %s", e.what());
            }
        });

        m_pConnection->set_close_handler([=](...)
        {
            obs_debug("set_close_handler");

            if (m_thread.joinable())
                m_thread.detach();
            m_pConnection = NULL;

            listener->onDisconnected();
        });

        m_pConnection->set_fail_handler([=](...)
        {
            obs_debug("** set_fail_handler called **");
            listener->onDisconnected();
        });

        m_pConnection->set_interrupt_handler([=](...)
        {
            obs_debug("** set_interrupt_handler called **");
            listener->onDisconnected();
        });

        // Request connection (no network messages exchanged until event loop starts running)
        auto pCon = m_client.connect(m_pConnection);

        isConnected = true;

        // Async
        m_thread = std::thread([&]()
        {
            m_client.set_access_channels(websocketpp::log::alevel::all);
            m_client.clear_access_channels(websocketpp::log::alevel::frame_payload);
            m_client.clear_access_channels(websocketpp::log::alevel::frame_header);
            m_client.clear_access_channels(websocketpp::log::alevel::control);
            m_client.set_error_channels(websocketpp::log::elevel::all);

            // Start ASIO io_service run loop (single connection will be made to the server)
            m_client.run(); // exits when this connection is closed
        });
    }
    catch (const websocketpp::exception& e)
    {
        obs_error("FcsWebsocketImpl::connect exception: %s", e.what());
        isConnected = false;
        retVal = false;
    }

    return retVal;
}


bool FcsWebsocketImpl::send(const string& sMsg)
{
    bool retVal = false;
    uint8_t* pchMsg = nullptr;

    if (m_pConnection)
    {
        size_t nMsgSz = sMsg.size() + 2;
        pchMsg = new uint8_t[nMsgSz];
        memset(pchMsg, 0, nMsgSz);
        memcpy(pchMsg, sMsg.c_str(), nMsgSz - 2);
        pchMsg[nMsgSz - 2] = '\n';

        //string sMsg2 = sMsg + "\n";
        //if ( ! m_pConnection->send(sMsg2) )

        if ( ! m_pConnection->send(pchMsg, nMsgSz) )
        {
            // suppress ping msgs
            //if (sMsg != "0 0 0 0 0 ")
            //    obs_info("[DBG Edge] TX: %s", sMsg.c_str());
            retVal = true;
        }
        else obs_error("[DBG Edge] send() failed, dropping tx: %s", sMsg.c_str());
    }
    else obs_error("[DBG Edge] send() skipped, m_pConnection is null, dropping tx: %s", sMsg.c_str());

    delete[] pchMsg;

    return retVal;
}


bool FcsWebsocketImpl::disconnect(bool wait)
{
    websocketpp::lib::error_code ec;
    long long sleepTm = 1;

    if (isConnected && m_pConnection)
    {
        try
        {
            if (_listener && _listener->preDisconnect(wait))
            {
                // Give our messge some time to be sent if connection::send() didn't err
                std::this_thread::sleep_for( std::chrono::seconds(sleepTm) );
            }
            else sleepTm = 0;

            // close down connection and clear callback handlers
            m_client.close(m_pConnection, websocketpp::close::status::normal, "", ec);
            m_client.stop();
            m_client.set_open_handler([](...) {});
            m_client.set_close_handler([](...) {});
            m_client.set_fail_handler([](...) {});

            // shut down socket's IO thread, if its still spun up
            if (m_thread.joinable())
                wait ? m_thread.join() : m_thread.detach();

            isConnected = false;
        }
        catch (const websocketpp::exception& e)
        {
            obs_error("FcsWebsocketImpl::disconnect exception: %s", e.what());
        }
    }

    return (sleepTm > 0);
}

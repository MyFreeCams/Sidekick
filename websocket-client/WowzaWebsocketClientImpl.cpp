/*
 * Copyright (c) 2013-2020 MFCXY, Inc. <mfcxy@mfcxy.com>
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

#include "WowzaWebsocketClientImpl.h"

#include <libobs/obs.h>

#include <libPlugins/Portable.h>
#include <libPlugins/HttpRequest.h>
#include <libfcs/MfcJson.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iterator>
#include <random>
#include <sstream>

#ifdef _WIN32
char* __progname = "websocketclient";
#else
const char* __progname = "websocketclient";
#endif

#define obs_debug(format, ...) blog(400, format, ##__VA_ARGS__)
#define obs_info(format,  ...) blog(300, format, ##__VA_ARGS__)
#define obs_warn(format,  ...) blog(200, format, ##__VA_ARGS__)
#define obs_error(format, ...) blog(100, format, ##__VA_ARGS__)

using njson = nlohmann::json;
using std::string;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;


WowzaWebsocketClientImpl::WowzaWebsocketClientImpl()
    : m_pConnection(nullptr)
    , m_pListener(nullptr)
    , m_nSid(0)
    , m_nRoomId(0)
    , m_nModelId(0)
    , m_nWidth(0)
    , m_nHeight(0)
    , m_nFrameRate(0)
    , m_fCamScore(0.0f)
    , m_bAuthenticated(false)
    , m_bAnswerReceived(false)
    , m_bBroadcastStarted(false)
    , m_bUserClosedConnection(false)
{
    // Set logging to be pretty verbose (everything except message payloads)
    m_client.set_access_channels(websocketpp::log::alevel::all);
    m_client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    m_client.clear_access_channels(websocketpp::log::alevel::frame_header);
    m_client.set_error_channels(websocketpp::log::elevel::all);

    // Initialize ASIO
    m_client.init_asio();
}


bool WowzaWebsocketClientImpl::connect(Listener*       pListener,
                                       const string&   sUrl,
                                       const string&   sStreamName,
                                       const string&   sStreamKey,
                                       const string&   sPassword,
                                       const string&   sVidCtx,
                                       int             nSid,
                                       int             nUid,
                                       int             nRoomId,
                                       int             nWidth,
                                       int             nHeight,
                                       int             nFrameRate,
                                       float           fCamScore)
{
    obs_debug("WowzaWebsocketClientImpl::connect");

    bool retVal     = true;
    m_pListener     = pListener;
    m_sStreamName   = sStreamName;
    m_sPassword     = sPassword;
    m_nSid          = nSid;
    m_nRoomId       = nRoomId;
    m_nModelId      = nUid;
    m_nWidth        = nWidth;
    m_nHeight       = nHeight;
    m_nFrameRate    = nFrameRate;
    m_fCamScore     = fCamScore;

    string sDecodedKey = sStreamKey;
    if (sDecodedKey.size() > 10 && sDecodedKey.at(10) == '%')
        MfcJsonObj::decodeURIComponent(sDecodedKey);

    obs_info("stream key (decoded): %s\n", sDecodedKey.c_str());

    try
    {
        m_client.set_tls_init_handler([&](websocketpp::connection_hdl /*con*/)
        {
            auto ctx = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);
            try
            {
                ctx->set_options(asio::ssl::context::default_workarounds |
                                 asio::ssl::context::no_sslv2 |
                                 asio::ssl::context::no_sslv3 |
                                 asio::ssl::context::single_dh_use);
            }
            catch (const std::exception& e)
            {
                obs_error("TLS Exception: %s", e.what());
            }
            return ctx;
        });

        websocketpp::lib::error_code ec;
        m_pConnection = m_client.get_connection(sUrl, ec);
        if (ec)
        {
            obs_error("Error creating websocket connection pointer: %s", ec.message().c_str());
            return false;
        }

        m_pConnection->set_message_handler([=](websocketpp::connection_hdl /*con*/, message_ptr frame)
        {
            const char* x = frame->get_payload().c_str();
            auto msg = njson::parse(frame->get_payload());

            if (msg.find("status") == msg.end())
                return;

            int status = msg["status"].get<int>();
            if (status == 200)
            {
                njson iceCandidates;
                string sdp;

                if (msg.find("sdp") != msg.end())
                    sdp = msg["sdp"]["sdp"].get<string>();

                if (msg.find("iceCandidates") != msg.end())
                    iceCandidates = msg["iceCandidates"];

                string command = msg["command"].get<string>();
                if (command == "auth")
                {
                    obs_info("** AUTHENTICATED **");
                    m_bAuthenticated = true;
                    m_pListener->onConnected();
                }
                else if (command == "sendOffer")
                {
                    if (!sdp.empty())
                    {
                        m_bAnswerReceived = true;
                        m_pListener->onAnswer(sdp);
                    }

                    if (!iceCandidates.empty())
                    {
                        for (const auto& iceCandidate : iceCandidates)
                        {
                            string candidate = iceCandidate["candidate"].get<string>();
                            string sdpMid = iceCandidate["sdpMid"].get<string>();
                            int sdpMLineIndex = iceCandidate["sdpMLineIndex"].get<int>();
                            m_pListener->onRemoteIceCandidate(candidate, sdpMid, sdpMLineIndex);
                        }
                        m_pListener->onRemoteIceCandidate("", "", -1);
                    }

                    njson sendResponse = {{ "command", "updateState" }, { "state", 0 }};
                    m_pConnection->send(sendResponse.dump());

                    if (m_bAuthenticated && m_bAnswerReceived && !m_bBroadcastStarted)
                    {
                        m_bBroadcastStarted = true;
                        m_pListener->onReadyToStartBroadcast();
                    }
                }
                else if (command == "updateState")
                {
                    obs_info("updateState command received");
                }
                else
                {
                    obs_info("Unknown command: %s", x);
                }
            }
            else
            {
                obs_info("RECEIVED MESSAGE:\n%s", x);
                if (status > 499)
                {
                    obs_warn("INVALID LOGIN CREDENTIALS");
                    m_pListener->onAuthFailure();
                }
                else
                {
                    m_pListener->onConnectError();
                }
            }
        });

        m_pConnection->set_open_handler([=](websocketpp::connection_hdl /*con*/)
        {
            double fAspect = (double)m_nWidth / (double)m_nHeight;
            m_nModelId = 0;

            njson login =
            {
                {   "command", "auth"   },
                {
                    "userData",
                    {
                        {   "streamName",   m_sStreamName   },
                        {   "password",     m_sPassword     },
                        {   "sessionId",    m_nSid          },
                        {   "roomId",       m_nRoomId       },
                        {   "modelId",      m_nModelId      },
                        {   "camscore",     m_fCamScore     },
                        {   "sk",           sDecodedKey     },
                        {   "vidctx",       sVidCtx         },
                        {   "mode",         "BROADCAST"     },
                        {   "state",        0               }
                    }
                },
                {
                    "streamTracksInfo",
                    {
                        {
                            "audio",
                            {
                                {
                                    "settings",
                                    {
                                        {   "autoGainControl",  false   },
                                        {   "echoCancellation", false   },
                                        {   "noiseSuppression", false   },
                                        {   "sampleRate",       48000   },
                                        {   "sampleSize",       16      },
                                        {   "channelCount",     2       },
                                        {   "volume",           1       }
                                    }
                                },
                                {
                                    "constraints",
                                    {
                                        {   "echoCancellation", false   },
                                        {   "channelCount",     2       }
                                    }
                                }
                            }
                        },
                        {
                            "video",
                            {
                                {
                                    "settings",
                                    {
                                        {   "width",        m_nWidth    },
                                        {   "height",       m_nHeight   },
                                        {   "aspectRatio",  fAspect     },
                                        {   "frameRate",    nFrameRate  },
                                        {   "resizeMode",   "none"      }
                                    }
                                },
                                {
                                    "constraints",
                                    {
                                        {   "width",        m_nWidth    },
                                        {   "height",       m_nHeight   },
                                        {   "aspectRatio",  fAspect     }
                                    }
                                }
                            }
                        }
                    }
                }
            };

            obs_info("Sending authentication request...");
            m_pConnection->send(login.dump());
        });

        m_pConnection->set_close_handler([=](...)
        {
            obs_info("** set_close_handler called **");
            if (m_thread.joinable())
                m_thread.detach();
            m_pConnection = nullptr;
            if (m_bUserClosedConnection)
                m_pListener->onDisconnected();
        });

        m_pConnection->set_fail_handler([=](...)
        {
            obs_info("** set_fail_handler called **");
            m_pListener->onConnectError();
        });

        m_pConnection->set_interrupt_handler([=](...)
        {
            obs_info("** set_interrupt_handler called **");
            m_pListener->onDisconnected();
        });

        // Request connection (no network messages exchanged until event loop starts running)
        m_client.connect(m_pConnection);

        // Async
        m_thread = std::thread([&]()
        {
            obs_info("** Starting ASIO io_service run loop **");
            // Start ASIO io_service run loop (single connection will be made to the server)
            m_client.run(); // exits when this connection is closed
        });
    }
    catch (const websocketpp::exception& e)
    {
        obs_error("WowzaWebsocketClientImpl::connect exception: %s", e.what());
        retVal = false;
    }

    return retVal;
}


bool WowzaWebsocketClientImpl::sendSdp(const string& sSdp, const string& /*sVideoCodec*/)
{
    bool retVal = true;
    m_nModelId  = 0;

    njson offer =
    {
        {   "direction", "publish"      },
        {   "command",   "sendOffer"    },
        {
            "streamInfo",
            {
                {   "applicationName", "NxServer"      },
                {   "streamName",      m_sStreamName   },
                {   "sessionId",       "[empty]"       }
            }
        },
        {
            "sdp",
            {
                {   "type", "offer" },
                {   "sdp",  sSdp    }
            }
        },
        {
            "userData",
            {
                {   "streamName",   m_sStreamName   },
                {   "password",     m_sPassword     },
                {   "sessionId",    m_nSid          },
                {   "roomId",       m_nRoomId       },
                {   "modelId",      m_nModelId      },
                {   "camscore",     m_fCamScore     },
                {   "mode",         "BROADCAST"     },
                {   "state",        0               }  // HACKHACK added for test
            }
        }
    };

    try
    {
        obs_info("Sending offer...");
        if (m_pConnection->send(offer.dump()))
            retVal = false;
    }
    catch (const websocketpp::exception& e)
    {
        obs_error("Error sending offer: %s", e.what());
        retVal = false;
    }

    return retVal;
}


bool WowzaWebsocketClientImpl::trickle(const string& sCandidate,
                                       const string& /*sMid*/, int /*nIndex*/, bool /*bIsLast*/)
{
    obs_debug("Trickle candidate: %s", sCandidate.c_str());
    return true;
}


bool WowzaWebsocketClientImpl::disconnect(bool bWait)
{
    obs_info("WowzaWebsocketClientImpl::disconnect");

    m_bUserClosedConnection = true;
    if (!m_pConnection)
        return true;

    try
    {
        if (bWait)
        {
            obs_info("Sending cam off message");
            njson close = {{ "command", "updateState" }, { "state", 90 }};
            if (m_pConnection->send(close.dump()))
                return false;
            // Wait for unpublish message to send
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        if (!m_pConnection)
            return true;

        if (m_pConnection->get_state() == websocketpp::session::state::open)
            m_client.close(m_pConnection, websocketpp::close::status::normal, string("disconnect"));

        m_client.stop();
        m_client.set_open_handler([](...) {});
        m_client.set_close_handler([](...) {});
        m_client.set_fail_handler([](...) {});

        if (m_thread.joinable())
            bWait ? m_thread.join() : m_thread.detach();
    }
    catch (const websocketpp::exception& e)
    {
        obs_error("WowzaWebsocketClientImpl::disconnect exception: %s", e.what());
        return false;
    }

    return true;
}

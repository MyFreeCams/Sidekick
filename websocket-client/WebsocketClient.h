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

#ifdef _MSC_VER
#ifdef WEBSOCKETCLIENT_EXPORTS
#define WEBSOCKETCLIENT_API __declspec(dllexport)
#else
#define WEBSOCKETCLIENT_API __declspec(dllimport)
#endif
#else
#ifndef  __APPLE__
#define WEBSOCKETCLIENT_API
#else
#define WEBSOCKETCLIENT_API __attribute__((visibility("default")))
#endif
#endif

#include <memory>
#include <string>


class WEBSOCKETCLIENT_API WebsocketClient
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;

        virtual void onConnected() = 0;
        virtual void onAnswer(const std::string& sSdp) = 0;
        virtual void onRemoteIceCandidate(const std::string& sCandidate, const std::string& sMid, int nIndex) = 0;
        virtual void onReadyToStartBroadcast() = 0;
        virtual void onAuthFailure() = 0;
        virtual void onConnectError() = 0;
        virtual void onDisconnected() = 0;
    };

    virtual ~WebsocketClient() = default;

    virtual bool connect(Listener*          pListener,
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
                         float              fCamScore) = 0;

    virtual bool sendSdp(const std::string& sSdp, const std::string& sVideoCodec) = 0;
    virtual bool trickle(const std::string& sCandidate, const std::string& sMid, int nIndex, bool bIsLast) = 0;
    virtual bool disconnect(bool bWait) = 0;
};


WEBSOCKETCLIENT_API std::unique_ptr<WebsocketClient> CreateWebsocketClient();


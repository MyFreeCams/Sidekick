/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
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
#define FCS_WEBSOCKET_API __declspec(dllexport)
#else
#define FCS_WEBSOCKET_API __declspec(dllimport)
#endif
#else
#ifndef  __APPLE__
#define FCS_WEBSOCKET_API
#else
#define FCS_WEBSOCKET_API __attribute__((visibility("default")))
#endif
#endif

#include <memory>
#include <string>


class FCS_WEBSOCKET_API FcsWebsocket
{
public:
    virtual ~FcsWebsocket() = default;

    virtual bool send(const std::string& sMsg) = 0;
    virtual bool disconnect(bool wait) = 0;

    class FcsListener
    {
    public:
        virtual ~FcsListener() = default;

        // Called when websocket connection is established
        virtual void onConnected(void) = 0;

        // Called when websocket connection is terminated
        virtual void onDisconnected(void) = 0;

        // Called when any message frame is received from socket
        virtual void onMsg(std::string& sMsg) = 0;

        // Called at the beginning of disconnect() by base case, giving
        // listener a chance to send any logout messages to server
        // before socket is torn down.
        virtual bool preDisconnect(bool wait) = 0;
    };

public:
    virtual bool connect(   const std::string &username,
                            const std::string &token,
                            const std::string &url,
                            FcsListener* listener) = 0;
};


FCS_WEBSOCKET_API std::unique_ptr<FcsWebsocket> createFcsWebsocket(void);

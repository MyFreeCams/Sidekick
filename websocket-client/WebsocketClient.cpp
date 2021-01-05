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

#include "WebsocketClient.h"
#include "WowzaWebsocketClientImpl.h"

#include <libobs/obs-module.h>
#include <openssl/opensslv.h>

#include <memory>

OBS_DECLARE_MODULE()

bool obs_module_load()
{
    OPENSSL_init_ssl(0, nullptr);
    //-------------------------------------------------------------------------------------
    // This should be OpenSSL and not BoringSSL, or TLS negotiation in ASIO sockets
    // for websocketpp will crash due to ABI incompatabilities.
    //
    //blog(   100,
    //        "(websocket-client lib) SSL header version 0x%X, OpenSSL_version(): %s \r\n",
    //        OPENSSL_VERSION_NUMBER,
    //        OpenSSL_version(0));
    //---
    return true;
}


WEBSOCKETCLIENT_API std::unique_ptr<WebsocketClient> CreateWebsocketClient()
{
    return std::make_unique<WowzaWebsocketClientImpl>();
}

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

#ifndef MFC_EDGE_INGEST_H_
#define MFC_EDGE_INGEST_H_

#include <string>
#include <tuple>


class MFCEdgeIngest
{
public:
    struct SiteIpPort
    {
        std::string site;
        std::string tcpIp;
        int port = 0;
    };

    /**
     * RTMP publishing URL (automatically selects nearest ingest).
     */
    static std::string RtmpPublishUrl();

    /**
     * RTMP publishing URL in |region|.
     * @param region - TUK: Seattle, ORD: Chicago, AMS: Amsterdam, BUH: Bucharest, SYD: Sydney, SAO: Sao Paulo, TYO: Tokyo
     */
    static std::string RtmpPublishUrl(const std::string& region);

    /**
     * Nearest WebRTC edge ingest site, ip address, and port for |videoServer|.
     * @param videoServer - Example: "video343"
     */
    static SiteIpPort WebrtcTcpIpPort(const std::string& videoServer);

    /**
     * WebRTC ingest |ip| and |port| for |region| for broadcasting to |videoServer|.
     * @param videoServer - Example: "video343"
     * @param region - TUK: Seattle, ORD: Chicago, AMS: Amsterdam, BUH: Bucharest, SYD: Sydney, SAO: Sao Paulo, TYO: Tokyo
     */
    static SiteIpPort WebrtcTcpIpPort(const std::string& videoServer, const std::string& region);

private:
    /**
     * WebRTC ingest |ip| address for nearest |site|.
     */
    static bool WebrtcTcpIp(std::string& site, std::string& ip);

    /**
     * WebRTC ingest |ip| address for |region|.
     * @param region - TUK: Seattle, ORD: Chicago, AMS: Amsterdam, BUH: Bucharest, SYD: Sydney, SAO: Sao Paulo, TYO: Tokyo
     * @param ip - TCP ice candidate IP address override
     */
    static bool WebrtcTcpIp(const std::string& region, std::string& ip);

    /**
     * WebRTC ingest |port| number for |videoServer|.
     * @param videoServer - Example: "video343"
     */
    static int WebrtcTcpPort(const std::string& videoServer);
};

#endif  // MFC_EDGE_INGEST_H_

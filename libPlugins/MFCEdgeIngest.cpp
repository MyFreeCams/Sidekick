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

#include "MFCEdgeIngest.h"
#include "HttpRequest.h"

#include <libfcs/Log.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <regex>

#define MFC_EDGE_DETAILS_URL "https://modelweb.mfcimg.com/edge/details.json"
#define MFC_EDGE_RTP_INFO_URL "https://rtp-edgeingest.myfreecams.com/edge/info.json"

#ifndef MFC_DEFAULT_BROADCAST_URL
#define MFC_DEFAULT_BROADCAST_URL "rtmp://publish.myfreecams.com/NxServer"
#endif

using njson = nlohmann::json;
using std::string;


static string ToLower(const string& s)
{
    string sCopy(s);
    std::transform(sCopy.begin(), sCopy.end(), sCopy.begin(),
                   [](const unsigned char& c){ return std::tolower(c); });
    return sCopy;
}


// static
string MFCEdgeIngest::RtmpPublishUrl()
{
    return string(MFC_DEFAULT_BROADCAST_URL);
}


// static
string MFCEdgeIngest::RtmpPublishUrl(const string& region)
{
    if (region.empty())
        return MFCEdgeIngest::RtmpPublishUrl();
    return string("rtmp://publish-") + ToLower(region) + string(".myfreecams.com/NxServer");
}


// static
MFCEdgeIngest::SiteIpPort MFCEdgeIngest::WebrtcTcpIpPort(const std::string& videoServer)
{
    string ip, site;
    MFCEdgeIngest::SiteIpPort siteIpPort;
    if (!MFCEdgeIngest::WebrtcTcpIp(site, ip))
        return siteIpPort;

    int port = WebrtcTcpPort(videoServer);
    if (port > 0)
    {
        siteIpPort.site = site;
        siteIpPort.tcpIp = ip;
        siteIpPort.port = port;
    }
    return siteIpPort;
}


// static
MFCEdgeIngest::SiteIpPort MFCEdgeIngest::WebrtcTcpIpPort(const string& videoServer, const string& region)
{
    string ip;
    MFCEdgeIngest::SiteIpPort siteIpPort;
    if (!MFCEdgeIngest::WebrtcTcpIp(region, ip))
        return siteIpPort;

    int port = WebrtcTcpPort(videoServer);
    if (port > 0)
    {
        siteIpPort.site = ToLower(region);
        siteIpPort.tcpIp = ip;
        siteIpPort.port = port;
    }
    return siteIpPort;
}


// static
bool MFCEdgeIngest::WebrtcTcpIp(std::string& site, std::string& ip)
{
    try
    {
        const string url = MFC_EDGE_RTP_INFO_URL;
        unsigned int dwLen = 0;
        CCurlHttpRequest httpreq;
        uint8_t* pResponse = httpreq.Get(url, &dwLen, "", nullptr);
        if (!pResponse)
            return false;

        const auto res = njson::parse((char*)pResponse);
        ip = res["ip"].get<string>();
        site = ToLower(res["site"].get<string>());

        free(pResponse);
        pResponse = nullptr;
        return true;
    }
    catch (const std::exception& e)
    {
        _MESG("Error fetching edge IP: %s", e.what());
        return false;
    }
}


// static
bool MFCEdgeIngest::WebrtcTcpIp(const string& region, std::string& ip)
{
    try
    {
        const string url = string("https://rtp-edgeingest-") + ToLower(region) + ".myfreecams.com/edge/info.json";
        unsigned int dwLen = 0;
        CCurlHttpRequest httpreq;
        uint8_t* pResponse = httpreq.Get(url, &dwLen, "", nullptr);
        if (!pResponse)
            return false;

        const auto res = njson::parse((char*)pResponse);
        ip = res["ip"].get<string>();

        free(pResponse);
        pResponse = nullptr;
        return true;
    }
    catch (const std::exception& e)
    {
        _MESG("Error fetching edge IP: %s", e.what());
        return false;
    }
}


// static
int MFCEdgeIngest::WebrtcTcpPort(const string& videoServer)
{
    std::smatch match;
    std::regex re("video([0-9]{3,4})");
    if (!std::regex_search(videoServer, match, re))
        return 0;

    int port = std::stoi(match[1].str()) + 2000;
    return port;
}

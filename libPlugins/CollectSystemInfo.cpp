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

#ifndef DO_SERVICE_JSON_COPY
#define DO_SERVICE_JSON_COPY
#endif

#ifdef _WIN32
#include "targetver.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif
#include <windows.h>
#include <libfcs/UtilCommon.h>
#endif // _WIN32

#include <string>
#include <regex>

#ifdef __APPLE__
#include <cstdio>
#include <cstdarg>
#include <ctime>
#endif

#include <iostream>
#include <string>

#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>

#include <ObsBroadcast/ObsBroadcast.h>
#include "build_version.h"

#include "CollectSystemInfo.h"

#include <util/platform.h>

extern CBroadcastCtx g_ctx; // part of MFCLibPlugins.lib::MfcPluginAPI.obj

using std::string;
using std::wstring;

#ifdef _WIN32


LONG GetDWORDRegKey(HKEY hKey, const wstring& strValueName, DWORD& nValue, DWORD nDefaultValue)
{
    nValue = nDefaultValue;
    DWORD dwBufferSize(sizeof(DWORD));
    DWORD nResult(0);

    LONG nError = ::RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, reinterpret_cast<LPBYTE>(&nResult), &dwBufferSize);
    if (ERROR_SUCCESS == nError)
        nValue = nResult;

    return nError;
}


LONG GetStringRegKey(HKEY hKey, const wstring& strValueName, wstring& strValue, const wstring& strDefaultValue)
{
    strValue = strDefaultValue;
    WCHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError;
    nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (ERROR_SUCCESS == nError)
        strValue = szBuffer;
    return nError;
}


bool Is64BitWindows()
{
#if defined(_WIN64)
    return true;  // 64-bit programs run only on Win64
#else
    // 32-bit programs run on both 32-bit and 64-bit Windows
    bool f64 = FALSE;
    return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#endif
}


bool collectSystemInfo(MfcJsonObj& js)
{
    wstring keyProductName, keyCurrentBuild, keyCurrentVersion, keyPath;
    string sProduct, sBuild, sVersion;
    DWORD keyMajVer, keyMinVer;
    char szBuf[2048];
    HKEY hKey;

    if (Is64BitWindows())
        keyPath = L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows NT\\CurrentVersion\\";
    else
        keyPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\";

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        GetStringRegKey(hKey, L"ProductName",                   keyProductName,      L"err");
        GetStringRegKey(hKey, L"CurrentBuild",                  keyCurrentBuild,     L"err");
        GetDWORDRegKey(hKey,  L"CurrentMajorVersionNumber",     keyMajVer,           0);
        GetDWORDRegKey(hKey,  L"CurrentMinorVersionNumber",     keyMinVer,           0);

        // If these are 0, then we are not Windows 10 so another query is necessary
        if (keyMajVer == 0 && keyMinVer == 0)
            GetStringRegKey(hKey, L"CurrentVersion", keyCurrentVersion, L"err");
        else
            keyCurrentVersion = to_wstring(keyMajVer) + L"." + to_wstring(keyMinVer);

        os_wcs_to_utf8(keyProductName.c_str(), keyProductName.size(), szBuf, sizeof(szBuf));
        sProduct = szBuf;

        os_wcs_to_utf8(keyCurrentBuild.c_str(), keyCurrentBuild.size(), szBuf, sizeof(szBuf));
        sBuild = szBuf;

        os_wcs_to_utf8(keyCurrentVersion.c_str(), keyCurrentVersion.size(), szBuf, sizeof(szBuf));
        sVersion = szBuf;

        js.objectAdd("osn", sProduct);
        js.objectAdd("osb", sBuild);
        js.objectAdd("osv", sVersion);
        //js.objectAdd("x64", Is64BitWindows());
    }

    string sObsVer( obs_get_version_string() );
    js.objectAdd("skv", SIDEKICK_VERSION_STR);
    js.objectAdd("appver", stdprintf("OBS %s", sObsVer.c_str()));

    WCHAR wszBuf[128];
    DWORD dwSz = sizeof(wszBuf);
    if (GetComputerNameW(wszBuf, &dwSz))
    {
        os_wcs_to_utf8(wszBuf, wcslen(wszBuf), szBuf, sizeof(szBuf));
        js.objectAdd("nm", szBuf);
    }

    char **ppNames = obs_frontend_get_profiles();
    char **ppName = ppNames;

    int64_t nCurrent = -1, n = 0;
    MfcJsonPtr pList = new MfcJsonObj(JSON_T_ARRAY);
    const char* pszProfile = obs_frontend_get_current_profile();
    while (ppName && *ppName)
    {
        if (pszProfile && strcmp(pszProfile, *ppName) == 0)
            nCurrent = n;

        pList->arrayAdd(*ppName);
        ppName++;
        n++;
    }

    bfree((void*)pszProfile);
    bfree(ppNames);

    js.objectAdd("profiles", pList);
    if (nCurrent >= 0 && nCurrent < n)
        js.objectAdd("curprofile", nCurrent);

    js.objectAdd("streamtype", FCVIDEO_TX_IDLE);

    string sTransport("Unknown");

    if (g_ctx.isMfc)
    {
        if      (g_ctx.isRTMP)      sTransport = "RTMP";
        else if (g_ctx.isWebRTC)    sTransport = "WebRTC";
    }
    else sTransport = "Non-MFC";
    js.objectAdd("transport", sTransport);

    return true;
}

#endif  // _WIN32

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

#ifndef __OBS_UTIL_H__
#define __OBS_UTIL_H__

#include <string>
#include <utility>
#include <vector>


class CObsUtil
{
public:
    // helpers to get a value from the obs profile or return a default value if not found
    static const std::string getConfigOrDefault(const char* pSection, const char* pVariable, const char* pDefault);
    static int getConfigOrDefault(const char* pSection, const char* pVariable, int nDefault);
    static bool getConfigOrDefault(const char* pSection, const char* pVariable, bool bDefault);
    static time_t getConfigOrDefault(const char* pSection, const char* pVariable, time_t nDefault);
    static size_t getConfigItems(const std::string& sBlock, std::vector<std::pair<std::string,std::string>>& vItems);

    // helper to remove a value from obs profile section
    static bool removeConfig(const std::string& sBlock, const std::string& sVar);

    // threadsafe helpers to set a value into the obs profile.
    static bool setConfig(const char* pSection, const char* pVariable, const char* value);
    static bool setConfig(const char* pSection, const char* pVariable, const std::string& value);
    static bool setConfig(const char* pSection, const char* pVariable, int value);
    static bool setConfig(const char* pSection, const char* pVariable, bool value);

    // sets or gets settings from stream settings of current profile (url, key, service type)
    static bool setCurrentSetting(const char* pszName, const char* pszValue);
    static bool getCurrentSetting(const char* pszName, std::string& sVal);

    static bool isValidConfig(void);
    static std::string FindPluginDirectory(void);
    static std::string AppendPath(const std::string& sPath, const std::string& sFile);

    static const std::string getFilenameCachePath(const std::string&);
    static const std::string getFilenamePluginPath(const std::string&);

    static const std::string getProfilePath();
    static const std::string getLogPath();

    static std::string getInstallPath(void);

    static bool connect_browser();
    static void disconnect_browser();
    static bool openBrowserPanel(const char* pCaption, const char* pURL,
                                 int nWidth, int nHeight);
    static bool closeBrowserPanel(const char* pCaption);
    static void clearBrowserPanel();

    static unsigned long ExecProcess(const char*, const char*);
    static bool ExecMFCLogin();
    static bool TerminateMFCLogin();

    static std::string expandTokens(const std::string& s);

    static int strcmpi(const char* p, const char* p2);

#ifdef UNUSED_CODE
    static char* getDataPath(const char* p);
    static void AddModulePath(const char*, const char*);
#endif
};

#endif  // __OBS_UTIL_H__

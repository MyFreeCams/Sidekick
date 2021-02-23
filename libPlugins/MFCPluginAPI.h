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

#ifndef __MFC_PLUGIN_API_H__
#define __MFC_PLUGIN_API_H__


class CMFCPluginAPI
{
public:
    CMFCPluginAPI(PROGRESS_CALLBACK pfnProgress);
    ~CMFCPluginAPI() = default;

    // fetch the model's plugin version.
    int getPluginVersionForModel(std::string& sVersion);

    // download the manifest file for the model's version.
    int getManifestFile(const std::string& sVersion, std::string& sFile);
    int getUpdateFile(const std::string& sVersion, const std::string& sTargetFile, uint8_t* pFileContents, unsigned int nSize, unsigned int* pFileSize);

    int ShutdownReport(int nPluginType);
    int StartupReport(int nPluginType);

    int SendSystemReport(void);
    int SendHeartBeat(void);

    void setLastHttpError(const std::string& s) { m_sLastError = s; }
    std::string& getLastHttpError() { return m_sLastError; }

    int HandleError(int nErr);

private:
    std::string m_sHost;
    std::string m_sFileHost;
    std::string m_sPlatform;
    std::string m_sLastError;
    PROGRESS_CALLBACK m_pfnProgress;
};

#endif  // __MFC_PLUGIN_API_H__

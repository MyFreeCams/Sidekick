#pragma once

#ifndef _MFC_UPDATER_API_H__
#define _MFC_UPDATER_API_H__

#include "libfcs/MfcJson.h"

class CMFCUpdaterAPI
{
public:
    CMFCUpdaterAPI(PROGRESS_CALLBACK pfnProgress);
    ~CMFCUpdaterAPI() = default;

    // fetch the model's plugin version.
    int getPluginVersionForModel(std::string& sVersion);

    // download the manifest file for the model's version.
    int getManifestFile(const std::string& sVersion, std::string& sFile);

    int getUpdateFile(const std::string& sVersion, const std::string& sTargetFile, BYTE* pFileContents, DWORD nSize, DWORD* pFileSize);

    void setLastHttpError(const std::string& s) { m_sLastError = s; }

    std::string& getLastHttpError() { return m_sLastError;  }

    int HandleError(int nErr);

private:
    std::string m_sHost;
    std::string m_sFileHost;
    std::string m_sPlatform;
    std::string m_sLastError;
    PROGRESS_CALLBACK m_pfnProgress;
};

#endif  // _MFC_UPDATER_API_H__

#pragma once

#ifndef __MANIFEST_FILE_H__
#define __MANIFEST_FILE_H__

#include <string>
#ifdef _WIN32
#include <wtypes.h>
#endif

class CManifestFile
{
public:
    CManifestFile() = default;
    CManifestFile(const char *pFile, const char *pCheckSum, const char *pURL)
    {
        m_sFilename = pFile;
        m_sCheckSum = pCheckSum;
        m_sURL      = pURL;
    }
    CManifestFile(const CManifestFile &src)
    {
        operator=(src);
    }
    const CManifestFile &operator=(const CManifestFile &src)
    {
        m_sFilename     = src.m_sFilename;
        m_sCheckSum     = src.m_sCheckSum;
        m_sURL          = src.m_sURL;
        m_sDestination  = src.m_sDestination;
        return *this;
    }

    virtual ~CManifestFile() = default;

    // virtual do update method.
    virtual bool writeToCache(BYTE *, DWORD);

    // access members.
    const std::string getTargetFilename() { return m_sFilename; }

#ifdef UNUSED_MANIFEST_CODE
    std::string getPluginPathFilename()
    {
        std::string sFile = getTargetFilename();
        std::string sPath = CObsUtil::FindPluginDirectory();
        std::string::reverse_iterator itr = sPath.rbegin();
        if (itr != sPath.rend())
        {
            if (*itr != '\\' && *itr != '/')
                sPath += "/";
        }
        sPath += sFile;
        return sPath;
    }

    std::string getCachePathFilename()
    {
        std::string sFile = getTargetFilename();
        std::string strFile = CObsUtil::getDataPath(sFile.c_str());
        return strFile;
    }
#endif

    // manifest line.
    void setManifestLine(const std::string &sLine)
    {
        m_sManifestLine = sLine;
    }

    const std::string &getManifestLine()
    {
        return m_sManifestLine;
    }

    void setTargetFilename(const char *p) { m_sFilename = p; }
    void setTargetFilename(const std::string &s) { m_sFilename = s; }

    std::string &getMD5CheckSum() { return m_sCheckSum; }
    void setMD5CheckSum(const char *p) { m_sCheckSum = p; }
    void setMD5CheckSum(const std::string &s) { m_sCheckSum = s; }

    std::string &getMD5CalcCheckSum() { return m_sCalcChecksum; }
    void setMD5CalcCheckSum(const char *p) { m_sCalcChecksum = p; }

    std::string &getFileURL() { return m_sURL;  }
    void setFileURL(const char *p) { m_sURL = p; }

    void setOpCode(const char *p) { m_sOpCode = p; }
    void setOpCode(const std::string &s) { m_sOpCode = s; }
    std::string getOpCode() { return m_sOpCode; }

    void setDestination(const std::string &s) { m_sDestination =s; }
    std::string getDestination() { return m_sDestination; }

    bool isUpdateNeeded();

    bool appendCachedManifestLine(const std::string &sLine);

protected:
    bool calcCheckSum(const std::string &sFilename, std::string *);

private:
    std::string m_sFilename;
    std::string m_sCheckSum;
    std::string m_sCalcChecksum;
    std::string m_sURL;
    std::string m_sOpCode;
    std::string m_sManifestLine;
    std::string m_sDestination;
};

#endif  // __MANIFEST_FILE_H__

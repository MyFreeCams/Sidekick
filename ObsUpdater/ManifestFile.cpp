#include "ObsUpdater.h"

#ifdef _WIN32
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <errno.h>

#include <stdlib.h>
#include <fcntl.h>

#include <string>
#include <map>
#include <regex>
#include <cctype>

#ifdef _WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#include <wincrypt.h>
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

// obs includes
#include "util/platform.h"
#include <obs-module.h>

// MFC Includes
#include <libfcs/Log.h>
#include <libfcs/md5.h>
#include <libfcs/fcslib_string.h>
#include <libPlugins/MFCConfigConstants.h>

// solution includes
#include <libPlugins/Portable.h>
#include <libPlugins/ObsUtil.h>

// project includes
#include "ManifestFile.h"


//---------------------------------------------------------------------------
// doUpdate
//
// copy update files to the cache.
bool CManifestFile::writeToCache(BYTE *pchFile, DWORD dwFileLen)
{
    std::string sTarget = getTargetFilename();
    std::string strFile = CObsUtil::getFilenameCachePath(sTarget);// getCachePathFilename();

    _TRACE("writing cache file: %s", strFile.c_str());
    bool bRv = true;

#ifdef _WIN32
    int nFd = 0;
    if (_sopen_s(&nFd, strFile.c_str(), _O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC, _SH_DENYRW, _S_IWRITE) == 0)
    {
        int nOffset = 0, n;
        while (nOffset < (int)dwFileLen)
        {
            n = _write(nFd, pchFile + nOffset, dwFileLen - nOffset);
            if (n < 0)
            {
                _ASSERT(!"Failed to update file");
                _TRACE("file updated %s", strFile.c_str());
                _close(nFd);

            }
            nOffset += n;
        }
        _close(nFd);
        _TRACE("Cache updated");
        // Add this line to the manifest file in the cache.
        appendCachedManifestLine(getManifestLine());
        bRv = true;
    }
    else
    {
        std::string sErr = getWin32Error(GetLastError());
        _ASSERT(!sErr.c_str());
        _TRACE("Failed saving %s to disk: %s (%d)", strFile.c_str(), sErr.c_str());
        //free(pszText);
        bRv = false;
    }
#else
    FILE *f;
    if ((f = fopen(strFile.c_str(), "w")) != NULL)
    {
        size_t n = fwrite(pchFile, 1, dwFileLen, f);
        if (n != dwFileLen)
        {
            std::string sBuf;
            stderror(sBuf, errno);
            _ASSERT(!"Failed to update file");
            _TRACE("file update failed %s %d %s", strFile.c_str(), errno, sBuf.c_str());
            bRv = false;
        }
        fclose(f);
        _TRACE("Cache updated");
        // Add this line to the manifest file in the cache.
        appendCachedManifestLine(getManifestLine());
    }
    else
    {
        std::string sErr;
        stderror(sErr, errno);
        _ASSERT(!sErr.c_str());
        _MESG("Failed saving %s to disk: %s (%d)", strFile.c_str(), sErr.c_str());
        //free(pszText);
        bRv = false;
    }
#endif
    return bRv;
}


//---------------------------------------------------------------------------
// isUpdateNeeded
//
// is an update needed? does the target file exist and does the check
// sum match?
//---------------------------------------------------------------------------
bool CManifestFile::isUpdateNeeded()
{
    bool bRv = false;
    std::string sTarget = getTargetFilename();

    std::string strFile = CObsUtil::getFilenamePluginPath(sTarget); // getPluginPathFilename();

    _TRACE("Check if file %s needs update", strFile.c_str());

    if (os_file_exists(strFile.c_str()))
    {
        std::string sCalc;
        if (calcCheckSum(strFile, &sCalc))
        {
            _TRACE("Local file: %s",sCalc.c_str());
            _TRACE("Remote file %s",m_sCheckSum.c_str());

            if (sCalc.length() == m_sCheckSum.length())
            {
                const char *pCalc = sCalc.c_str();
                const char *pFile = m_sCheckSum.c_str();
                // bytewise compare, compensate for case.
                while (*pCalc && *pFile && !bRv)
                {
                    if (toupper(*pCalc) != toupper(*pFile))
                    {
                        _TRACE("Failed checksum doing update");
                        bRv = true;
                    }
                    else
                    {
                        pCalc++;
                        pFile++;
                    }
                }
            }
            else
            {
                // They don't match lengths, can't be the same sequence
                bRv = true;
                _TRACE("MD5 calc strings don't match.  Doing update");
            }
        }
        else
        {
            // probably can't open the file?   We really shouldn't be here.
            // update any way
            bRv = true;
            _ASSERT(!"Calc Checksum failed");
            std::string s;
            stderror(s, errno);
            _TRACE("Calc checksum failed %s", s.c_str());
        }
    }
    else
    {
        _TRACE("Target file does not exist.  Updating: %s", strFile.c_str());
        bRv = true;
    }
    return bRv;
}


//--------------------------------------------------------------------------
// calcCheckSum
//
// calculate the md5 checksum of the target file
//--------------------------------------------------------------------------
bool CManifestFile::calcCheckSum(const std::string &filename, std::string *pCheckSum)
{
    bool bRv = true;
    unsigned char c[MD5_DIGEST_LENGTH];

    _TRACE("Running checksum on file: %s", filename.c_str());

    int i;
    FILE *inFile = fopen(filename.c_str(), "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1025];

    if (inFile == NULL)
    {
        std::string sErr = stderror(errno);
        _ASSERT(!"calcCheckSum failed to open file");
        _TRACE("calcCheckSum %s can't be opened %s.\n", filename.c_str(), sErr.c_str());
        return false;
    }

    MD5_Init(&mdContext);
    while ((bytes = (int) fread(data, 1, 1024, inFile)) != 0)
    {
        MD5_Update(&mdContext, data, bytes);
    }

    MD5_Final(c, &mdContext);
    std::string ss;
    for (i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        char buf[10] = {'\0'};
        sprintf(buf, "%02x", c[i]);
        ss += buf;

    }
    fclose(inFile);
    *pCheckSum = m_sCalcChecksum = ss;
    _TRACE("Checksum for file is %s", ss.c_str());

    return bRv;
}


//---------------------------------------------------------------------------
// appendCacheManifestline
//
// append a line to the cached manifest file.
bool CManifestFile::appendCachedManifestLine(const std::string &sLine)
{
    std::string strFile( CObsUtil::getFilenameCachePath( MANIFEST_FILE ) ); // CObsUtil::getDataPath(MANIFEST_FILE);
    bool bRv = true;
    _TRACE("cache manifest: %s", strFile.c_str());
    _TRACE("Writing line %s", sLine.c_str());

    FILE *f;

    if ((f = fopen(strFile.c_str(), "a")) != NULL)
    {
        fprintf(f, "%s\n", sLine.c_str());
        fclose(f);
    }
    else
    {
        std::string sErr;
#ifdef _WIN32
        sErr = getWin32Error(GetLastError());
#else
        stderror(sErr, errno);
#endif
        _ASSERT(!sErr.c_str());
        _MESG("Failed saving %s to disk: %s (%d)", strFile.c_str(), sErr.c_str());
        //free(pszText);
        bRv = false;
    }

    return bRv;
}

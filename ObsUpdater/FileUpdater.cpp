// FileUpdater : Performs the file updates from the manifest file.
//
#include "ObsUpdater.h"

#include <stdio.h>
#include <fcntl.h>
#include <list>

#ifdef _WIN32
#include <io.h>
#include <string>
#include <algorithm>
#include <functional>
#else
#include <errno.h>
#include <pthread.h>
#endif

// obs includes
#include <util/platform.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>
#include <util/threading.h>

// MFC Includes
#include <libfcs/Log.h>
#include <libfcs/fcslib_string.h>

// Solutions includes
#include <libPlugins/Portable.h>
#include <curl/curl.h>
#include <libPlugins/PluginParameterBlock.h>
#include <libPlugins/MFCConfigConstants.h>
#include <libPlugins/HttpRequest.h>
#include <libPlugins/MFCPluginAPI.h>
#include <libPlugins/SysParam.h>
#include <libPlugins/ObsUtil.h>

// project includes
#include "MFCUpdaterAPI.h"
#include "FileUpdater.h"
#include "ManifestFile.h"
#include <ObsBroadcast/ObsBroadcast.h>

#define FILE_BUF_SIZE 1024000000

pthread_mutex_t g_mutexStop;

bool g_bStop = false;

//---------------------------------------------------------------------------
// setStop
//
// access member for stop flag.
void setStop(bool b)
{
    try
    {
        pthread_mutex_lock(&g_mutexStop);
        g_bStop = b;
    }
    catch (...)
    {
        assert(!"unknown exception caught");
    }
    pthread_mutex_unlock(&g_mutexStop);
}

bool getStop()
{
    bool b = false;
    try
    {
        pthread_mutex_lock(&g_mutexStop);
        b = g_bStop;
    }
    catch (...)
    {
        assert(!"unknown exception caught");
    }
    pthread_mutex_unlock(&g_mutexStop);
    return b;
}

//---------------------------------------------------------------------------
// StopUpdaterCallback
//
// call back from libcurl that will allows us to terminate an http request
// if stop flag is set.
// curl_off_t is an int64
int StopUpdaterCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    UNUSED_PARAMETER(clientp);
    UNUSED_PARAMETER(dltotal);
    UNUSED_PARAMETER(dlnow);
    UNUSED_PARAMETER(ultotal);
    UNUSED_PARAMETER(ulnow);
    if (getStop())
        return 1;
    return 0;
}

//----------------------------------------------------------------------------
// MyThreadFunction
//
// call back function for new thread.  Pass control back to the FileUpdater
// object.
#ifdef _WIN32XXX
DWORD WINAPI MyThreadFunction(LPVOID pThis)
#else
void *MyThreadFunction(void *pThis)
#endif
{
    CFileUpdater *pT = (CFileUpdater *) pThis;
    pT->Process();
    return NULL;
}

//---------------------------------------------------------------------------
// CFileUpdater
//
// constructor
CFileUpdater::CFileUpdater()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_mutexStop, &attr);
}

//---------------------------------------------------------------------------
// Stop
//
// Stop the thread.
bool CFileUpdater::Stop()
{
    setStop(true);
    pthread_join(m_hHandle, NULL);
    return true;
}

//---------------------------------------------------------------------------
// doUpdate
//
// perform the update called for in the manifest.
//
// creaet the thread, and wait for the thread to perform the updates.
//---------------------------------------------------------------------------
bool CFileUpdater::doUpdate(DWORD nTimeout)
{
    UNUSED_PARAMETER(nTimeout);
    int rc = pthread_create(&m_hHandle, NULL, MyThreadFunction, (void *) this);
    if (rc)
    {
        _ASSERT(!"Unable to create thread");
        _TRACE("Error: unable to create thread %d", rc);
    }
    return true;
}

//---------------------------------------------------------------------------
// Process
//
// process rtn for the thread call back.
//---------------------------------------------------------------------------
DWORD CFileUpdater::Process()
 {
    DWORD dwFileContents = FILE_BUF_SIZE;
    std::unique_ptr<BYTE> pchFileContents(new BYTE[FILE_BUF_SIZE + 1]);
    memset(pchFileContents.get(), '\0', FILE_BUF_SIZE + 1);

    std::string sBuf;
    std::string sURL;
    CMFCPluginAPI api(StopUpdaterCallback);
    int nErr = -1;

    // step 1: get current manifest file.

    if (!getStop())
    {
        nErr = api.getPluginVersionForModel(sBuf);
        // either we got through to fcsagents and got a version for hte model
        // or we got back "default"
        nErr = 0; // we always get an answer efvent if the answer is "default"
    }

    // step 2: Update the model's broadcast plugin.

    // fetch the manifest file for hte model's version
    if (0 == nErr && !getStop())
    {
        _TRACE("Model version is %s", sBuf.c_str());
        std::string sManiFile;
        nErr = api.getManifestFile(sBuf, sManiFile);
        if (0 == nErr && !getStop())
        {
            _TRACE("Got Manifest file %s", sManiFile.c_str());
            // parse the manifest into a list.  Each row is a file.
            strVec vec;
            stdsplit(sManiFile, '\n', vec);
            // for each line/file
            for (unsigned long i = 0; i < vec.size() && !getStop(); i++)
            {
                std::string sLine = vec[i];
                // remove the /r
                size_t nEOL = sLine.find('\r');
                if (std::string::npos != nEOL)
                {
                    sLine[nEOL] = '\0';
                }
                // remove spaces.
                stdLeftTrim(sLine);
                // skip the line if it's a comment
                if (sLine[0] != '#')
                {
                    strVec arrUpdateFile;
                    size_t flds = stdsplit(sLine, ',', arrUpdateFile);
                    if (flds >= 4)
                    {
                        // new format with destination token at the end
                        CManifestFile upd;
                        upd.setOpCode(arrUpdateFile[0]);
                        upd.setMD5CheckSum(arrUpdateFile[1]);
                        upd.setTargetFilename(arrUpdateFile[2]);
                        upd.setDestination(arrUpdateFile[3]);
                        upd.setManifestLine(sLine);
                        // line parsed correctly.
                        if (!getStop() && upd.isUpdateNeeded())
                        {
                            // an update is needed.
                            std::string sFile;
                            std::string sTargetFileName = upd.getTargetFilename();
                            std::string sTargetFile = sTargetFileName;
                            memset(pchFileContents.get(), '\0', FILE_BUF_SIZE + 1);
                            DWORD dwFileSize = 0;
                            // fetch the update file from the MFC api.
                            nErr = api.getUpdateFile(sBuf, sTargetFile, pchFileContents.get(), dwFileContents, &dwFileSize);
                            if (0 == nErr)
                            {
                                // copy the file to the cache.
                                if (upd.writeToCache(pchFileContents.get(), dwFileSize))
                                {
                                    _TRACE("cache updated with file %s", sTargetFile.c_str());
                                }
                                else
                                {
                                    std::string sErr;
#ifdef _WIN32
                                    sErr = getWin32Error(GetLastError());
#else
                                    stderror(sErr, errno);
#endif
                                    _TRACE("Failed saving %s to disk: %s", sTargetFile.c_str(), sErr.c_str());
                                }
                            }
                            else
                            {
                                // _ASSERT(!"Error downloading file");
                                _TRACE("Failed downloading %s, Error (%d)", sTargetFile.c_str(), nErr);
                            }
                        }
                        else
                        {
                            _TRACE("No update needed %d", upd.getTargetFilename().c_str());
                        }
                    }
                    else
                    {
                        _TRACE("Bad manifest line %s", sLine.c_str());
                    }
                }
            } // end for
        }
        else
        {
            _TRACE("Failed to download manifest file error %d", nErr);
        }
    } // endif get model plugin version

    /*
    if (!getStop()) // && ppb.isUpdaterRunSysReport())
    {
        // step 3: Send System Report
        nErr = api.SendSystemReport();
        if (nErr != 0)
        {
            _ASSERT(!"Failed to send system report");
            _TRACE("Failed to send System Report Error %d", nErr);
        }
    }
    */

#ifdef _DOSHUTDOWN_REP
    // step 3: Send Shutodwn Report
    nErr = api.ShutdownReport(nPluginType, smc);
    if (nErr != 0)
    {
        _TRACE("Failed to send System Report Error %d", nErr);
    }
#endif
    return 1;
}

//---------------------------------------------------------------------------
// ExecAndWait
//
// Spawn a exe and wait for it to exit.
bool ExecAndWait(std::string &cmd)
{
#ifdef _WIN32
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char pCmd[_MAX_PATH + 1] = { '\0' };
    strcpy(pCmd, cmd.c_str());

    bool b = CreateProcessA(NULL,   // No module name (use command line)
        pCmd,           // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure
    );

    if (b)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        return true;
    }
    std::string sErr = getWin32Error(GetLastError());
    _TRACE("Error creating process %s", sErr.c_str());
    _ASSERT(!"Failed to create process");
    return false;
#else
    bool bRv = false;
    system(cmd.c_str());
    /*
    switch( fork() )
    {
        case -1 : // Error
            // Handle the error
            _ASSERT(! "Failed to do fork!");
            bRv = false;
            break;

        case 0 :
            execlp("path/to/binary","binary name", arg1, arg2, .., NULL);
            break;

        default :
            // Do what you want
            break;
    }
    */
    return bRv;
#endif
}

// useful debugging tool.  Uncomment to NOT delete the cache files.
// #define KEEP_CACHE_FILES

void CFileUpdater::ProcessCachedFiles()
{
    std::string sManifestFile( CObsUtil::getFilenameCachePath( MANIFEST_FILE ) ); // getDataPath(MANIFEST_FILE);
    _TRACE("Cached manifest file %s", sManifestFile.c_str());
    std::string sFileContents;
    size_t size = stdGetFileContents(sManifestFile, sFileContents);
    if (size > 0)
    {
        // delete manifest file.
#ifndef KEEP_CACHE_FILES
        os_unlink(sManifestFile.c_str());
#endif
        // cut it up into individual lines.
        strVec vec;
        stdsplit(sFileContents, '\n', vec);
        for (unsigned long i = 0; i < vec.size(); i++)
        {
            std::string sLine = vec[i];
            // remove the /r
            size_t nEOL = sLine.find('\r');
            if (std::string::npos != nEOL)
            {
                sLine[nEOL] = '\0';
            }
            // remove trailing spaces.
            stdLeftTrim(sLine);
            if (sLine[0] != '#')
            {
                // chop up the line:
                strVec arr;
                size_t size = stdsplit(sLine, ',', arr);
                if (size >= 3)
                {
                    // offset 2 is the filename
                    std::string s = arr[2];
                    // default to the plugin bin directory
                    // Most files won't need a destination
                    std::string sDestPath = "${MFC_PLATFORM}";
                    std::string sUpdateFile;
                     // last value on the line is the destination path.
                    if (size > 3)
                    {
                        sDestPath = arr[3];
                    }
                    else
                    {
                        // no destination given.  Do we still need this code?
                        sUpdateFile = CObsUtil::getFilenameCachePath(s); // CObsUtil::getDataPath(s.c_str());
                        sDestPath = CObsUtil::expandTokens(sDestPath);
                    }
                    sUpdateFile = CObsUtil::getFilenameCachePath(s);
                    // append the file name
                    std::string sDestination = CObsUtil::AppendPath(sDestPath, s.c_str());
                    std::string sBackup = sDestination;

                    // replace extension with .bak
                    size_t nE = sBackup.rfind('.', sBackup.length());
                    if (nE == std::string::npos)
                        sBackup += ".bak";
                    else
                        sBackup.replace(nE + 1, 3, "bak");

                    // parse out the opcode.
                    std::string sOpCopde = arr[0];
                    if (sOpCopde == "D" || sOpCopde == "d")
                    {
                        // do copy
                        _TRACE("sOpcode d found, doing copy");
                        _TRACE("source file %s", sUpdateFile.c_str());
                        _TRACE("Destination file %s", sDestination.c_str());
                        // only do the copy if the source file exists.
                        int nErr = os_safe_replace(sDestination.c_str(), sUpdateFile.c_str(), sBackup.c_str());
                        if (nErr != 0)
                        {
#ifdef _WIN32
                            std::string sErr = getWin32Error(GetLastError());
#else
                            std::string sErr = stderror(errno);
#endif
                            _TRACE("Upgrade failed: %s %s\n", sErr.c_str(), arr[2].c_str());
                        }
                        else
                        {
                            // clean up
#ifndef KEEP_CACHE_FILES
                            os_unlink(sBackup.c_str());
#endif
                            _TRACE("Upgrade successful");
                        }
                    }
                    else if (sOpCopde == "E" || sOpCopde == "e")
                    {
                        _TRACE("Source code E found doing execute");
                        os_unlink(sDestination.c_str());
                        int nErr = os_safe_replace(sDestination.c_str(), sUpdateFile.c_str(), sBackup.c_str());
                        if (nErr != 0)
                        {
                            std::string sErr = stderror(errno);
                            _TRACE("Upgrade failed: %s %s\n", sErr.c_str(), arr[2].c_str());
                        }
                        // execute
                        ExecAndWait(sDestination);
                        os_unlink(sUpdateFile.c_str());
                    }
                    else if (sOpCopde == "R" || sOpCopde == "r")
                    {
#ifdef _WIN32
                        _TRACE("Opcode R found doing register");
                        int nErr = os_safe_replace(sDestination.c_str(), sUpdateFile.c_str(), sBackup.c_str());
                        if (nErr != 0)
                        {
                            std::string sErr = stderror(errno);
                            _TRACE("Upgrade Failed: %s %s\n", sErr.c_str(), arr[2].c_str());
                        }
                        std::string sCmd = "regsvr32 ";
                        sCmd += sDestination;
                        ExecAndWait(sCmd);
                        os_unlink(sUpdateFile.c_str());
#else
                        _TRACE("Mac ignoring opcode R");
#endif
                    }
                    else
                    {
                        _ASSERT(!"Error unknown opcode in manifest file");
                    }
                } // end if size is 3
                else
                {
                    _TRACE("Bad Manifest line %s", sLine.c_str());
                }
            }
        } // end for each line.
    } // if size > 0
    else
    {
        _TRACE("No cached manifest file found");
    }
}

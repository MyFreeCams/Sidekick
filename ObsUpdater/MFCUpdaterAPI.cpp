#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <io.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <list>
#include <string>

// mfc includes
#include <libfcs/MfcJson.h>
#include <libfcs/Log.h>

// Solution Includes
#include <curl/curl.h>
#include <libPlugins/Portable.h>
#include <libPlugins/MFCConfigConstants.h>
//#include <SysParam.h>
//#include "PluginParameterBlock.h"
#include <libPlugins/ObsUtil.h>
//#include "ObsBroadcast/ObsBroadcast.h"

// project includes
#include <libPlugins/HttpRequest.h>
#include "MFCUpdaterAPI.h"

//CBroadcastCtx g_ctx(true);

//deque< SIDEKICK_UI_EV >   CBroadcastCtx::sm_eventQueue;
//recursive_mutex           CBroadcastCtx::sm_eventLock;
//bool                      CBroadcastCtx::suppressUpdates = false;


//---------------------------------------------------------------------------
// CMFCPluginAPI
//
//
CMFCUpdaterAPI::CMFCUpdaterAPI(PROGRESS_CALLBACK pfnProgress)
{
    m_pfnProgress   = pfnProgress;
    m_sHost         = MFC_API_SVR;
    m_sFileHost     = MFC_FILE_SVR;

#ifdef _WIN64
    m_sPlatform     = PLATFORM_WIN64;
#elif _WIN32
    m_sPlatform     = PLATFORM_WIN32;
#else
    m_sPlatform     = PLATFORM_MAC;
#endif
}


//-------------------------------------------------------------------------------
// HandleError
//
// helper function to handle errors returned from rest api
int CMFCUpdaterAPI::HandleError(int nErr)
{
    int retVal = nErr;

    if (ERR_BAD_PARAMETER == nErr)
    {
        // ignore this error - return success code
        _TRACE("[IGNORING HTTP RESPONSE CODE: ERR_BAD_PARAMETER / %d]", nErr);
        retVal = S_OK;
    }

    return retVal;
}


//---------------------------------------------------------------------------
// getManifestFile
//
// fetch the manifest file from the server.
int CMFCUpdaterAPI::getManifestFile(const std::string& sVersion, std::string& sFile)
{
    int nErr = ERR_NO_RESPONSE;
    std::string sManifestFile = stdprintf("%s/%s/%s/%s", m_sFileHost.c_str(), sVersion.c_str(),
                                          m_sPlatform.c_str(), MANIFEST_FILE);
    // on test machine: localhost\manifest = C:\Users\Todd\source\repos\MFC-OBSPlugins\Site
    CCurlHttpRequest httpreq;

#ifdef _LOCAL_MANIFEST_
    std::string sRes;
    if (stdGetFileContents(sManifestFile, sRes) > 0)
    {
        sFile = sRes;
        nErr = 0;
    }
#else
    BYTE* pResponse = NULL;
    unsigned int dwLen = 0;
    if ((pResponse = httpreq.Get(sManifestFile.c_str(), &dwLen, "", m_pfnProgress)) != NULL)
    {
        std::string sRes = (char *) pResponse;
        size_t nOffset = sRes.find("<html>", 0);
        if (nOffset > 0 && nOffset < 5)
        {
            // html tag with first 5 chars, this is not a manifest file.
            _TRACE("Error downloading file %s", sRes.c_str());
            setLastHttpError(sRes);
            nErr = ERR_FILE_ERROR;
        }
        else
        {
            sFile = (char *) pResponse;
            nErr = 0;;
        }
        free(pResponse);
        pResponse = NULL;
    }
#endif
    else
    {
        setLastHttpError("No HTTP Response");
        _TRACE("Http Error %d", httpreq.getResult());
        // either no response, we can't get out or there is a stop request in progress.
        nErr = ERR_NO_RESPONSE;
    }

    return HandleError(nErr);
}


//---------------------------------------------------------------------------
// getUpdateFile
//
// fetch a file from the server.
int CMFCUpdaterAPI::getUpdateFile(const std::string& sVersion, const std::string& sTargetFile, BYTE* pFileContents, DWORD nSize, DWORD* pFileSize)
{
    int nErr = ERR_NO_RESPONSE;
    BYTE* pchFile = NULL;
    CCurlHttpRequest httpreq;
    std::string sFile = stdprintf("%s/%s/%s/%s", m_sFileHost.c_str(), sVersion.c_str(), m_sPlatform.c_str(), sTargetFile.c_str());
    unsigned int dwFileLen = 0;
    _TRACE("Downloading file %s", m_sFileHost.c_str());

#ifdef _LOCAL_MANIFEST_
    UNREFERENCED_PARAMETER( nSize );
    DBG_UNREFERENCED_LOCAL_VARIABLE( dwFileLen );
    DBG_UNREFERENCED_LOCAL_VARIABLE( pchFile );

    std::string sRes;
    if (stdGetFileContents(sFile, sRes) > 0)
    {
        memcpy(pFileContents, sRes.c_str(), sRes.size());
        *pFileSize = (DWORD)sRes.size();
        nErr = 0;
    }
#else
    if ((pchFile = httpreq.Get(sFile.c_str(), &dwFileLen, "", m_pfnProgress)) != NULL)
    {
        if (dwFileLen > nSize)
        {
            _TRACE("File size exceeded %d %d", dwFileLen, nSize);
            setLastHttpError("File size exceeded!");
            nErr = 2;
        }
        else
        {
            std::string sRes = (char *) pchFile;
            size_t nOffset = sRes.find("<html>", 0);
            if (nOffset > 0 && nOffset < 5)
            {
                // html tag with first 5 chars, this is probably not a binary file.
                _TRACE("Error downloading file %s", sRes.c_str());
                setLastHttpError(sRes);
                nErr = ERR_FILE_ERROR;
            }
            else
            {
                _TRACE("Successful download of file: %s", sFile.c_str());
                nErr = 0;
                memcpy(pFileContents, pchFile, dwFileLen);
                *pFileSize = dwFileLen;
            }
        }
        free(pchFile);
        pchFile = NULL;
    }
    else
    {
        setLastHttpError("No HTTP Response");
        _TRACE("Http Error %d", httpreq.getResult());
        // either no response, we can't get out or there is a stop request in progress.
        nErr = ERR_NO_RESPONSE;
    }
#endif

    return HandleError(nErr);
}


//---------------------------------------------------------------------------
// getPluginVersionForModel
//
// get the version of the plugin assigned to the model.
int CMFCUpdaterAPI::getPluginVersionForModel(std::string& sVersion)
{
    sVersion = "default";
    return S_OK;
/*
    const std::string sModelID = pBlock->getModelID();
    int nErr = 0;
    BYTE *pResponse;

    unsigned int dwLen = 0;
    std::string sUrl = m_sHost;
    sUrl += MODELVERSION_API;

    std::string sPayload; // = json.Serialize();

    *psVersion = DEFAULT_VERSION;
    MfcJsonObj json;
    json.objectAdd(std::string(JSON_MODEL_USR_ID), "0");
    json.objectAdd(std::string(JSON_PLUGIN_TYPE), pBlock->getVersion());
    std::string sMSK = pBlock->getMSK();
    json.objectAdd(std::string(JSON_MODEL_STRM_KEY), sMSK);

#ifdef _DEBUG
    std::string s = json.prettySerialize();
#endif

    sPayload = json.Serialize();
    CCurlHttpRequest httpreq;
    pResponse = httpreq.Post(sUrl, &dwLen, sPayload, m_pfnProgress);
    if (pResponse != NULL)
    {
        MfcJsonObj js;
        js.Deserialize(pResponse, dwLen);

        bool bParse = js.objectGetInt(HTTP_RESP_ERR, nErr);
        std::string sErr = "";
        if (js.objectGetString(HTTP_RESP_ERRMSG, sErr))
        {
            _TRACE("Failure errmsg from response \"%s\"", sErr.c_str());
            setLastHttpError(sErr);
        }

        if (bParse && 0 == nErr)
        {
            if (js.objectGetString(HTTP_RESP_RESULT, *psVersion))
            {
                _TRACE("Successful call to %s Model version: %s", MODELVERSION_API, psVersion->c_str());
                std::transform(psVersion->begin(), psVersion->end(), psVersion->begin(), ::tolower);
            }
            else
            {
                *psVersion = DEFAULT_VERSION;
            }
        }
        else if (!bParse)
        {
            string sBuf;
            for (DWORD i = 0; i < dwLen; i++)
                sBuf += (char) pResponse[i];
            _TRACE("Parsing error %s", sBuf.c_str());
            *psVersion = DEFAULT_VERSION;
        }
        else
        {
            _TRACE("Error returned from %s %d", MODELVERSION_API, nErr);
            // fall back to defeault
            nErr = 0;
        }
        free(pResponse);
        pResonse = NULL;
    }
    else
    {
        setLastHttpError("No HTTP Response");
        _TRACE("Http Error %d", httpreq.getResult());
        // either no response, we can't get out or there is a stop request in progress.
        nErr = ERR_NO_RESPONSE;
        *psVersion = DEFAULT_VERSION;
    }
    pBlock->setVersion(*psVersion);

    // this function always succeeds.  As a fall back it will at a minimum return "default" as the version.
    return nErr;
*/
}

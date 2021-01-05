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

// System Includes
#include <string>
#include <iostream>
#include <fstream>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>

// mfc includes
#include "libfcs/fcslib_string.h"
#include "libfcs/Log.h"
#include "libfcs/MfcJson.h"

// solution includes
#include "MFCConfigConstants.h"
#include  <libPlugins/ObsServicesJson.h>

// project includes
#include "ObsUtil.h"
#include "PluginParameterBlock.h"

using std::string;

//---------------------------------------------------------------------------
// CPluginParameterBlock
//
// various parameters used to stream or connect with the restapi
CPluginParameterBlock::CPluginParameterBlock()
{
    sidekickTokenTm = 0;
}

/*
//---------------------------------------------------------------------------
// CPluginParameterBlock
//
// copy constructor
CPluginParameterBlock::CPluginParameterBlock(const CPluginParameterBlock &src)
{
    operator=(src);
}
*/

//---------------------------------------------------------------------------
// operator=
//
// copy operator
const CPluginParameterBlock &CPluginParameterBlock::operator=(const CPluginParameterBlock &src)
{
    m_nHeartBeatInterval = src.m_nHeartBeatInterval;
    m_nDeadCount = src.m_nDeadCount;
    m_bSendLogs = src.m_bSendLogs;
    m_bUpdateUpdater = src.m_bUpdateUpdater;
    m_bUpdaterRunSysReport = src.m_bUpdaterRunSysReport;
    m_sBroadCastURL = src.m_sBroadCastURL;
    m_sSessionTicket = src.m_sSessionTicket;
    m_sModelID = src.m_sModelID;
    m_sStreamingKey = src.m_sStreamingKey;
    m_sBinPath = src.m_sBinPath;
    m_sVersion = src.m_sVersion;
    m_bAllowConnection = src.m_bAllowConnection;
    m_sServicesFilename = src.m_sServicesFilename;
    m_bDirty = src.m_bDirty;

    //webrtc parameters
    m_sModelPassword = src.m_sModelPassword;
    m_sModelUserName = src.m_sModelUserName;
    m_nSessionID = src.m_nSessionID;
    m_nHeight = src.m_nHeight;
    m_nWidth = src.m_nWidth;
    m_nFrameRate = src.m_nFrameRate;
    m_sProtocol = src.m_sProtocol;
    m_sCodec = src.m_sCodec;

    sidekickToken = src.sidekickToken;
    sidekickTokenTm = src.sidekickTokenTm;

    return *this;
}

/*
//---------------------------------------------------------------------------
// operator==
//
// comparision operator
bool CPluginParameterBlock::operator==(const CPluginParameterBlock &src)
{
    if (!m_nHeartBeatInterval == src.m_nHeartBeatInterval)
        return false;

    if (!m_nDeadCount == src.m_nDeadCount)
        return false;

    if (!m_bSendLogs == src.m_bSendLogs)
        return false;

    if (!m_bUpdateUpdater == src.m_bUpdateUpdater)
        return false;

    if (!m_bUpdaterRunSysReport == src.m_bUpdaterRunSysReport)
        return false;

    if (!(m_sBroadCastURL == src.m_sBroadCastURL))
        return false;

    if (!(m_sSessionTicket == src.m_sSessionTicket))
        return false;

    if (!(m_sModelID == src.m_sModelID))
        return false;

    if (!(m_sStreamingKey == src.m_sStreamingKey))
        return false;

    if (!(m_sBinPath == src.m_sBinPath))
        return false;

    if (!(m_sVersion == src.m_sVersion))
        return false;

    if (!m_bAllowConnection == src.m_bAllowConnection)
        return false;

    if (!(m_sServicesFilename == src.m_sServicesFilename))
        return false;

    if (m_nCamScore != src.m_nCamScore)
        return false;
    // if (!atof(m_sCamScore.c_str()) == atof(src.m_sCamScore.c_str()))
    //     return false;

    if (m_nSessionID != src.m_nSessionID)
    return false;
    // if (!atoi(m_sSessionID.c_str()) == atoi(src.m_sSessionID.c_str()))
    //    return false;

    if (m_nHeight != src.m_nHeight)
        return false;

    if (m_nWidth != src.m_nWidth)
        return false;

    if (m_nFrameRate != src.m_nFrameRate)
        return false;

    if (!(m_sProtocol == src.m_sProtocol))
        return false;

    if (!(m_sCodec == src.m_sCodec))
        return false;

    if (! (m_sModelPassword == src.m_sModelPassword))
        return false;

    if (!(m_sModelUserName == src.m_sModelUserName))
        return false;

    return true;
}
*/

//---------------------------------------------------------------------------
// loadDefaults
//
// Load the parameters from the obs profiles.
void CPluginParameterBlock::LoadDefaults()
{
    if (isDirty())
    {
        _TRACE("loading into dirty block");
    }

#ifdef UNUSED_CODE
    setVersion( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_VERSION, "default") );
    isSendLogs(CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_SEND_LOGS, false));
    isUpdateUpdater(CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_UPD_UPDATER, false));
    isUpdaterRunSysReport(CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_UPD_SYSREP, false));
    setAllowConnection(CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_ALLOW_CONNECT, true));

    setBinPath( CObsUtil::FindPluginDirectory() );

    setHeartBeatInterval(
            CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_HEARTBEAT_INTERVAL, DEFAULT_HEARTBEAT_INTERVAL));
    setDeadCount( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_DEADCNT, DEFAULT_DEAD_COUNT) );

    // Visual studio sometimes has stack issues with return objects being passed directly to a function.

    setBroadcastURL( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_URL, MFC_DEFAULT_BROADCAST_URL) );
    setModelID( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_MODELID, "") );

    // note we do NOT store model password.

    setModelStreamKey( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_KEY, "") );

    // new parameters for webrtc
    //setCamScore( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_CAM_SCORE, "") );
    setCamScore(0);

    int n       = CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_SESSION_ID, 0);
    int nn      = getSessionID();
    int nSid    = (n != 0 ? n : nn);

    _MESG("sid currently: %d, sid default: %d, using %d", nn, n, nSid);
    setSessionID(nSid);

    setProtocol( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_PROTOCOL, MFC_DEFAULT_WEBRTC_PROTOCOL) );
    setCodec( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_CODEC, MFC_DEFAULT_WEBRTC_CODEC) );
    setHeight(CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_HEIGHT, MFC_DEFAULT_WEBRTC_HEIGHT));
    setWidth(CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_WIDTH, MFC_DEFAULT_WEBRTC_WIDTH));
    setFrameRate(CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_FRAME_RATE, MFC_DEFAULT_WEBRTC_FRAMERATE));
    setToken(CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_TOKEN, ""));
    setTokenStamp(CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_TOKENSTAMP, 0));
    setModelUserName( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_MODEL_USER_NAME, "") );
    setModelPwd( CObsUtil::getConfigOrDefault(CONFIG_SECTION, CONFIG_MODEL_PWD, "") );

    // in version 23++ the streaming key is stored in profile path/service.json
    string sProfilePath( CObsUtil::getProfilePath() ), sKey, sSvr;

    sProfilePath += "/service.json";
    std::ifstream fs(sProfilePath);
    if (fs.is_open())
    {
        njson j;
        fs >> j;
        if (j.find("settings") != j.end())
        {
            njson jSettings = j["settings"];
            sKey = jSettings["key"].get<string>();
            sSvr = jSettings["server"].get<string>();
        }
        else _TRACE("Settings not found in service.json");

        setModelStreamKey(sKey);
        setBroadcastURL(sSvr);
        fs.close();
    }
    else
    {
        string sErr;
#ifdef _WIN32
        sErr = getWin32Error(GetLastError());
#else
        stderror(sErr, errno);
#endif
        _TRACE("Failed to load profile %s; err: %s", sProfilePath.c_str(), sErr.c_str());
    }
#endif
}

bool CPluginParameterBlock::refresh()
{
#ifdef UNUSED_CODE
    CObsServicesJson services;
    string sKey, sURL;

    if (services.refreshProfileSettings(sKey, sURL))
    {
        setBroadcastURL(sURL);
        setModelStreamKey(sKey);
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_URL, getBroadcastURL().c_str());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_KEY, getModelStreamKey().c_str());
        if (services.load(getServicesFilename()))
        {
            strVec arrNames;
            strVec arrURL;
            arrNames.push_back(MFC_SERVICES_JSON_PRIMARY_SERVER_NAME);
            string s = getBroadcastURL();
            arrURL.push_back(s);
            services.setURLList(arrNames, arrURL);
            services.save();
            return true;
        }
    }
#endif
    return false;
}

bool CPluginParameterBlock::Deserialize(MfcJsonObj& result)
{
    bool bV = false;
    int n = 0;
    string sR;

    if (isDirty())
        _TRACE("Deserialize a dirty block!");

    if (result.objectGetString(STARTUP_SESSION_TICKET, sR))
    {
        setSessionTIcket(sR);
        _TRACE("Session ticket set to %s", sR.c_str());

    }
    if (result.objectGetString(STARTUP_SESSION_TKX, sR))
    {
        setSessionTKX(sR);
        _TRACE("Session ticket set to %s", sR.c_str());
    }

    sR = "";
    n = 0;
    if (n > MFC_MAX_SLEEP_INTERVAL)
    {
        _TRACE("Heart beat interval of %d exceeded max", n, MFC_MAX_SLEEP_INTERVAL);
        n = MFC_MAX_SLEEP_INTERVAL;
    }

    if (result.objectGetInt(STARTUP_HEARTBEAT_INTERVAL, n))
    {
        setHeartBeatInterval(n);
        _TRACE("Heart beat interval set to %d", n);

    }
    else if (result.objectGetString(STARTUP_HEARTBEAT_INTERVAL, sR))
    {
        n = atoi(sR.c_str());
        setHeartBeatInterval(n);
        _TRACE("Heart beat interval (string value) set to %d", n);
    }

    sR = "";
    if (result.objectGetString(STARTUP_BROADCAST_URL, sR))
    {
        setBroadcastURL(sR);
        _TRACE("Broadcast URL set to %s", sR.c_str());
    }

    n = 0;
    if (result.objectGetInt(STARTUP_DEADCNT, n))   // || result.objectGetInt(STARTUP_DEADCNT2, n))
    {
        if (n > MFC_MAX_DEADCOUNT)
        {
            _TRACE("Dead count interval of %s exceeds max %s", n, MFC_MAX_DEADCOUNT);
            n = MFC_MAX_DEADCOUNT;
        }
        setDeadCount(n);
        _TRACE("Dead count set to %d", n);
    }

    bV = true;
    if (result.objectGetBool(STARTUP_ALLOWCONNECTION, bV))
    {
        setAllowConnection(bV);
        _TRACE("Allow connection set to %d", bV);
    }
    else if (result.objectGetString(STARTUP_ALLOWCONNECTION, sR))
    {
        std::transform(sR.begin(), sR.end(), sR.begin(), ::tolower);
        if (sR == "true")
        {
            bV = true;
        }
        else
        {
            bV = false;
        }

        setAllowConnection(bV);
        _TRACE("Allow connection set to %d", bV);
    }

    if (result.objectGetBool(STARTUP_SENDLOGS, bV))
    {
        setSendLogs(bV);
        _TRACE("Send logs set to %d", bV);
    }

    if (result.objectGetBool(STARTUP_UPD_UPDATER, bV))
    {
        setUpdateUpdater(bV);
        _TRACE("Update Updater set to %d", bV);
    }

    if (result.objectGetBool(STARTUP_UPD_SYSREPORT, bV))
    {
        setUpdaterRunSysReport(bV);
        _TRACE("Updater run sysreport set to %d", bV);
    }

    if (result.objectGetInt(STARTUP_MODEL_SESSION_ID, n))
    {
        setSessionID(n);
        _MESG("Model sid set to %d", n);
    }
    else _MESG("WebRTC config %s not found", STARTUP_MODEL_SESSION_ID);

    if (result.objectGetInt(STARTUP_WEBRTC_WIDTH, n))
    {
        setWidth(n);
        _TRACE("WEBRTC width set to %d", n);
    }
    else _TRACE("WebRTC config %s not found", STARTUP_WEBRTC_WIDTH);

    if (result.objectGetInt(STARTUP_WEBRTC_HEIGHT, n))
    {
        setHeight(n);
        _TRACE("WEBRTC Height set to %d", n);
    }

    if (result.objectGetString(CONFIG_TOKEN, sR))
    {
        setToken(sR);
    }

    int64_t nStamp = 0;
    if (result.objectGetInt(CONFIG_TOKEN, nStamp))
    {
        time_t t = (time_t)nStamp;
        setTokenStamp(t);
    }

    if (result.objectGetString(STARTUP_WEBRTC_PROTOCOL, sR))
    {
        setProtocol(sR);
        _TRACE("WEBRTC protocol set to %s", sR.c_str());
    }

    if (result.objectGetString(STARTUP_WEBRTC_CODEC, sR))
    {
        setCodec(sR);
        _TRACE("WEBRTC codec set to %s", sR.c_str());
    }

    if (result.objectGetString(STARTUP_WEBRTC_USER, sR))
    {
        setModelUserName(sR);
        _TRACE("WEBRTC Model Username set to %s", sR.c_str());
    }

    if (result.objectGetString(STARTUP_WEBRTC_PWD, sR))
    {
        setModelPwd(sR);
        _TRACE("WEBRTC Model pwd set to %s", sR.c_str());
    }

    if (result.objectGetString(STARTUP_WEBRTC_MID, sR))
    {
        setModelID(sR);
        _TRACE("WEBRTC modelid set to %s", sR.c_str());
    }

    return true;
}

//---------------------------------------------------------------------------
// saveDefaults
//
// save defaults back to the obs profile.
bool CPluginParameterBlock::SaveDefaults()
{
    bool retVal = false;
#ifdef UNUSED_CODE
    if (CObsUtil::isValidConfig())
    {
        // setConfig is threadsafe
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_HEARTBEAT_INTERVAL,  getHeartBeatInterval());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_DEADCNT,             getDeadCount());

        // Visual studio sometimes has stack issues with return objects being passed directly to a function.
        string s;
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_URL,                 getBroadcastURL().c_str());

        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_MODELID,             getModelID().c_str());

        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_SEND_LOGS,           isSendLogs());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_UPD_UPDATER,         isUpdateUpdater());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_UPD_SYSREP,          isUpdaterRunSysReport());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_VERSION,             getVersion().c_str());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_KEY,                 getModelStreamKey().c_str());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_ALLOW_CONNECT,       getAllowConnection());

        // webrtc parameters.
        if (CObsUtil::setConfig(CONFIG_SECTION, CONFIG_CAM_SCORE, stdprintf("%.f", getCamScore()).c_str()))
            _MESG("set camscore to %.f OK!", getCamScore());

        if (CObsUtil::setConfig(CONFIG_SECTION, CONFIG_SESSION_ID, getSessionID()))
        {
            _MESG("WebRTC savedefaults block for sid to %d OK!", getSessionID());
        }
        else _TRACE("WebRTC savedefaults block for sid FAILED!");

        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_PROTOCOL,            getProtocol().c_str());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_HEIGHT,              getHeight());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_WIDTH,               getWidth());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_CODEC,               getCodec().c_str());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_FRAME_RATE,          getFrameRate());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_MODEL_USER_NAME,     getModelUserName());
        CObsUtil::setConfig(CONFIG_SECTION, CONFIG_MODEL_PWD,           getModelPwd());

        config_t* pConfigProfile = obs_frontend_get_profile_config();
        config_save_safe(pConfigProfile, "tmp",nullptr);

        CObsServicesJson services;

        // update the profile json.
        services.updateProfileSettings(getModelStreamKey(), getBroadcastURL());

        // write the url back to the original services.json so when the dialog is
        // reopened
        if (services.load(getServicesFilename()))
        {
            strVec pArrNames;
            strVec pArrURL;
            bool bFnd = false;
            services.getURLList(&pArrNames, &pArrURL);
            for (int i = 0; i < pArrURL.size() && !bFnd; i++)
            {
                string s = pArrURL[i];
                if (s == getBroadcastURL())
                {
                    bFnd = true;
                }
            }
            if (!bFnd)
            {
                strVec arrNames;
                strVec arrURL;
                arrNames.push_back(MFC_SERVICES_JSON_PRIMARY_SERVER_NAME);
                string s = getBroadcastURL();
                arrURL.push_back(s);
                services.setURLList(arrNames, arrURL);
                services.save();
            }
        }

        setDirty(false);
        retVal = true;
    }
    else _TRACE("config not valid, not saving defaults back to settings json");
#endif
    return retVal;
}

//---------------------------------------------------------------------------
// getMSK
//
// get model streaming key for the rest api.  Everything after the sk=
const string CPluginParameterBlock::getMSK()
{
    string sRv;
    string s = getModelStreamKey();
    if (s.length() > 0)
    {
        size_t fnd = s.find("sk=");
        if (fnd != string::npos)
        {
            fnd += 3;
            sRv = s.substr(fnd);
        }
    }
    return sRv;
}

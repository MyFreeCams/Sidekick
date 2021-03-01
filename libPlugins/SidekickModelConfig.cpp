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
#include <cctype>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

#include <libobs/obs-module.h>
#include <libobs/util/config-file.h>
#include <obs-frontend-api.h>

// solution includes
// #include "libfcs/fcslib_string.h"
// #include "libfcs/Log.h"
// #include "libfcs/MfcJson.h"
#include "libfcs/fcs_b64.h"

// project includes
//#include "MFCConfigConstants.h"
#include "ObsServicesJson.h"
#include "ObsUtil.h"
#include "SidekickModelConfig.h"
#include <libPlugins/MFCConfigConstants.h>

bool                        SidekickModelConfig::sm_initialized = false;
size_t                      SidekickModelConfig::sm_nRefCx = 0;
map< string, MfcJsonObj >   SidekickModelConfig::sm_reqProps;
vector< string >            SidekickModelConfig::sm_vAllocs;


#ifdef _WIN32
bool DirectoryExists(const char* szPath)
{
    DWORD dwAttrib = GetFileAttributesA(szPath);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
#endif

void SidekickModelConfig::initializeDefaults(void)
{
    unsigned int n;

    if (!sm_initialized)
    {
        sm_initialized = true;

        sm_vAllocs.clear();
        sm_reqProps.clear();

        // Initialize default properties:
        // integers, strings, booleans, double/floats, and time_ts -- their key name string and default
        // value, by data type.
        //
        // TODO: Change over to a single json initialized block of key/values instead of this
        // kludgey hack.
        //
        const char* ppszKeys1[] = { "sid",          "uid",          "retrycount",   "hbinterval"    };
        int64_t     pnVals[]    = { 0,              0,              5,              60              };

        const char* ppszKeys2[] = { "username",     "codec",        "prot",         "pwd",          "streamkey",    "ctx",          "version",      "streamName",   "tok",          "region",       "videoserver",  "streamurl" };
        const char* ppszVals[]  = { "",    "h264",         "TCP",          "",             "",             "",             "default",      "ext_x_0.f4v",  "" ,            "",             "",             ""          };

        const char* ppszKeys3[] = { "sendlogs",     "updupd",       "updsr",        "allowConnect"  };
        bool        pfVals[]    = { false,          false,          false,          true            };

        const char* ppszKeys4[] = { "camscore"  };
        double      pdVals[]    = { 0.00        };

        const char* ppszKeys5[] = { "tok_tm",       "stamp" };
        time_t      ptmVals[]   = { 0,              0       };

        // these keys arent saved to the profile config, only the global plugin config
        //const char* ppszPluginKeys[] = { "tm_tok", "tok", "ctx", "pwd", "username" };

        // Build required properties map with each key's default JSON value
        for (n = 0; n < sizeof(ppszKeys1) / sizeof(ppszKeys1[0]); n++)
            sm_reqProps[ ppszKeys1[ n ] ] = MfcJsonObj( pnVals[ n ] );

        for (n = 0; n < sizeof(ppszKeys2) / sizeof(ppszKeys2[0]); n++)
            sm_reqProps[ ppszKeys2[ n ] ] = MfcJsonObj( ppszVals[ n ] );

        for (n = 0; n < sizeof(ppszKeys3) / sizeof(ppszKeys3[0]); n++)
            sm_reqProps[ ppszKeys3[ n ] ] = MfcJsonObj( pfVals[ n ] );

        for (n = 0; n < sizeof(ppszKeys4) / sizeof(ppszKeys4[0]); n++)
            sm_reqProps[ ppszKeys4[ n ] ] = MfcJsonObj( pdVals[ n ] );

        for (n = 0; n < sizeof(ppszKeys5) / sizeof(ppszKeys5[0]); n++)
            sm_reqProps[ ppszKeys5[ n ] ] = MfcJsonObj( (int64_t)ptmVals[ n ] );

        sm_initialized = true;
    }
}

bool SidekickModelConfig::writeProfileConfig(void) const
{
    bool retVal = false;

    // scoped lock of mutex if we are a sharedCtx instance
    unique_lock< recursive_mutex >  lk(m_csMutex, std::defer_lock);
    if (isSharedCtx)                lk.lock();

    /*
    std::string sPluginPath = obs_module_config_path("");

#ifdef _WIN32
    if ( ! DirectoryExists(sPluginPath.c_str()) )
        if ( ! CreateDirectoryA(sPluginPath.c_str(), NULL) )
            _MESG("PATHDBG: CreateDirectory() failed to mkdir '%s'", sPluginPath.c_str());
#else
    mkdir(sPluginPath.c_str(), 0770);
#endif

    std::string sPluginCfg = obs_module_config_path("sidekick.json");

#ifdef _WIN32

#endif
    */
    string sProfilePath = CObsUtil::getProfilePath() + "/" + SERVICE_JSON_FILE;
    MfcJsonObj jsProfileData;
    string sData;
        
    if (jsProfileData.loadFromFile(sProfilePath))
    {
        jsProfileData.objectAdd("sidekick", m_jsConfig);
        if (jsProfileData.Serialize(sData, MfcJsonObj::JSOPT_PRETTY) > 0)
        {
            if (stdSetFileContents(sProfilePath, sData))
            {
                _MESG("SVCDBG: wrote %zu bytes to profile config at %s", sData.size(), sProfilePath.c_str());
                retVal = true;
            }
            else _MESG("failed to write %zu bytes of profile config to %s", sData.size(), sProfilePath.c_str());
        }
        else _MESG("failed to re-serialize profile data with sidekick added");
    }
    else _MESG("SVCDBG: loaded profile data from: %s", sProfilePath.c_str());

    return retVal;
}

bool SidekickModelConfig::readProfileConfig(void)
{
    bool retVal = false;

    /*
    std::string sPluginCfg = obs_module_config_path("sidekick.json");
    if ( m_jsConfig.loadFromFile( sPluginCfg ) )
    {
        retVal = true;
    }
    else _MESG("config failed to load, unable to open '%s' for reading", sPluginCfg.c_str());
    */

    string sProfilePath = CObsUtil::getProfilePath() + "/" + SERVICE_JSON_FILE;
    MfcJsonObj jsProfileData;

    if (jsProfileData.loadFromFile(sProfilePath))
    {
        if (jsProfileData.objectGetObject("sidekick", m_jsConfig))
        {
            retVal = true;
        }
        else _MESG("'sidekick' property missing from profile's service: %s", sProfilePath.c_str());
    }
    else _MESG("config failed to load, unable to open '%s' for reading", sProfilePath.c_str());

    return retVal;
}


bool SidekickModelConfig::Serialize(MfcJsonObj& js)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    js = m_jsConfig;
    return true;
}

bool SidekickModelConfig::Serialize(string& sData)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    return m_jsConfig.Serialize(sData, MfcJsonObj::JSOPT_PRETTY);
}

// Reads a json string and deserializes it, loading it into this instance
// of sidekick model config data, clearing any previously held data first
bool SidekickModelConfig::Deserialize(const string& sData)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    bool retVal = false;

    m_jsConfig.clear();
    if (sData.size() > 3 && sData.size() < 1024 * 1024)
    {
        retVal = m_jsConfig.Deserialize(sData);
    }

    return retVal;
}

bool SidekickModelConfig::set(const string& sKey, const string& sVal)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    return m_jsConfig.objectAdd(sKey, sVal);
}

bool SidekickModelConfig::set(const string& sKey, int64_t nVal)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    return m_jsConfig.objectAdd(sKey, nVal);
}

#ifndef _WIN32
bool SidekickModelConfig::set(const string& sKey, time_t nVal)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    return m_jsConfig.objectAdd(sKey, nVal);
}
#endif

bool SidekickModelConfig::set(const string& sKey, float dVal)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    return m_jsConfig.objectAdd(sKey, dVal);
}

bool SidekickModelConfig::set(const string& sKey, int nVal)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    return m_jsConfig.objectAdd(sKey, nVal);
}

bool SidekickModelConfig::set(const string& sKey, bool fVal)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    return m_jsConfig.objectAdd(sKey, fVal);
}

bool SidekickModelConfig::getFloat(const string& sKey, float& dValue) const
{
    unique_lock< recursive_mutex > lk = sharedLock();
    map< string, MfcJsonObj >::iterator iProp;
    bool retVal = false;
    double dVal;

    initializeDefaults();

    if (m_jsConfig.objectGetFloat(sKey, dVal))
    {
        // Found non-default value for this key, copied into dValue
        dValue = (float)dVal;
        retVal = true;
    }
    else if ((iProp = sm_reqProps.find(sKey)) != sm_reqProps.end())
    {
        if (iProp->second.isFloat())
        {
            // Found default float value for this key, copied to dValue
            dValue = (float)iProp->second.m_dVal;
            retVal = true;
        }
        else if (iProp->second.isInt())
        {
            // Found default int value for this key, cast to
            // float and copied to dValue
            dValue = (float)iProp->second.m_nVal;
            retVal = true;
        }
    }

    return retVal;
}

bool SidekickModelConfig::getBool(const string& sKey, bool& fValue) const
{
    unique_lock< recursive_mutex > lk = sharedLock();
    map< string, MfcJsonObj >::iterator iProp;
    bool retVal = false;

    initializeDefaults();

    if (m_jsConfig.objectGetBool(sKey, fValue))
    {
        // Found non-default value for this key, copied into nValue
        retVal = true;
    }
    else if ((iProp = sm_reqProps.find(sKey)) != sm_reqProps.end())
    {
        if (iProp->second.isBoolean())
        {
            // Found default int value for this key, copoed to nValue
            fValue = iProp->second.m_fVal;
            retVal = true;
        }
        else if (iProp->second.isInt())
        {
            // Found default float value for this key, cast to int and copied to nValue
            fValue = (iProp->second.m_nVal == 0 ? false : true);
            retVal = true;
        }
    }

    return retVal;
}

bool SidekickModelConfig::getInt(const string& sKey, int& nValue) const
{
    unique_lock< recursive_mutex > lk = sharedLock();
    bool retVal = false;
    int64_t nVal;

    if (getInt(sKey, nVal))
    {
        nValue = (int)nVal;
        retVal = true;
    }
    return retVal;
}

bool SidekickModelConfig::getTime(const string& sKey, time_t& nValue) const
{
    unique_lock< recursive_mutex > lk = sharedLock();
    bool retVal = false;
    int64_t nVal;

    if (getInt(sKey, nVal))
    {
        nValue = (time_t)nVal;
        retVal = true;
    }
    return retVal;
}


bool SidekickModelConfig::getInt(const string& sKey, int64_t& nValue) const
{
    unique_lock< recursive_mutex > lk = sharedLock();
    map< string, MfcJsonObj >::iterator iProp;
    bool retVal = false;

    initializeDefaults();

    if (m_jsConfig.objectGetInt(sKey, nValue))
    {
        // Found non-default value for this key, copied into nValue
        retVal = true;
    }
    else if ((iProp = sm_reqProps.find(sKey)) != sm_reqProps.end())
    {
        if (iProp->second.isInt())
        {
            // Found default int value for this key, copoed to nValue
            nValue = iProp->second.m_nVal;
            retVal = true;
        }
        else if (iProp->second.isFloat())
        {
            // Found default float value for this key, cast to int and copied to nValue
            nValue = (int)iProp->second.m_dVal;
            retVal = true;
        }
    }

    return retVal;
}

bool SidekickModelConfig::getString(const string& sKey, string& sValue) const
{
    unique_lock< recursive_mutex > lk = sharedLock();
    map< string, MfcJsonObj >::iterator iProp;
    bool retVal = false;

    initializeDefaults();

    if (m_jsConfig.objectGetString(sKey, sValue))
    {
        // Found non-default value for this key, copied into sValue
        retVal = true;
    }
    else if ((iProp = sm_reqProps.find(sKey)) != sm_reqProps.end())
    {
        if (iProp->second.isString())
        {
            // Found default int value for this key, copied to sValue
            sValue = iProp->second.m_sVal;
            retVal = true;
        }
        else if (iProp->second.isInt())
        {
            // Found default int value for this key, copied to sValue
            stdprintf(sValue, "%d", iProp->second.m_nVal);
            retVal = true;
        }
        else if (iProp->second.isFloat())
        {
            // Found default float value for this key, cast to int and copied to nValue
            stdprintf(sValue, "%f", iProp->second.m_dVal);
            retVal = true;
        }
        else
        {
            // For anything else, serialize the value into a string
            // if the type doesn't match
            retVal = iProp->second.Serialize(sValue);
        }
    }

    return retVal;
}

// Helper method that returns an integer if found (or defaulted), otherwise 0
int SidekickModelConfig::getInt(const string& sKey) const
{
    int nVal = 0;
    getInt(sKey, nVal);
    return nVal;
}

float SidekickModelConfig::getFloat(const string& sKey) const
{
    float dVal = 0;
    getFloat(sKey, dVal);
    return dVal;
}

time_t SidekickModelConfig::getTime(const string& sKey) const
{
    time_t tmVal = 0;
    getTime(sKey, tmVal);
    return tmVal;
}

void SidekickModelConfig::clear(void)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    m_jsConfig.clear();
}

const char* SidekickModelConfig::getString(const string& sKey) const
{
    unique_lock< recursive_mutex > lk = sharedLock();
    string sVal;

    if (getString(sKey, sVal))
    {
        sm_vAllocs.push_back(sVal);
    }
    else sm_vAllocs.push_back("");

    return sm_vAllocs.back().c_str();
}


#ifdef UNUSED_CODE
bool SidekickModelConfig::loadConfig(const string& sFilename)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    bool retVal = false;
    string sData;

    m_jsConfig.clear();
    if (stdGetFileContents(sFilename, sData) > 0)
    {
        if (Deserialize(sData))
        {
            // Now add any defaults that werent found in sData
            //for (auto iProp = sm_reqProps.begin(); iProp != sm_reqProps.end(); ++iProp)
            //    if ( ! m_jsConfig.objectHas( iProp->first ) )
            //        m_jsConfig.objectAdd( iProp->first, iProp->second );

            retVal = true;
        }
    }

    return retVal;
}
size_t SidekickModelConfig::saveConfig(const string& sFilename)
{
    unique_lock< recursive_mutex > lk = sharedLock();
    size_t nWrote = 0, nSz;
    string sData;
    int nFd, n;

    if (Serialize(sData))
    {
        nSz = sData.size();

#ifdef _WIN32
        if (_sopen_s(&nFd, sFilename.c_str(), _O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC, _SH_DENYRW, _S_IWRITE) == 0)
#else
        if ((nFd = open(sFilename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) != -1)
#endif
        {
            while (nWrote < nSz)
            {
                if ((n = write(nFd, sData.c_str() + nWrote, (unsigned int)(nSz - nWrote))) < 0)
                {
                    _MESG(  "unable to write() to %s [%u of %u bytes]: %s (%d)",
                        nWrote,
                        nSz,
                        sFilename.c_str(),
                        strerror(errno),
                        errno);
                    break;
                }
                else nWrote += (size_t)n;
            }
            close(nFd);

            if (nSz > 0 && nWrote != nSz)
            {
                // Unable to write compelte file for some reason? Erase partial
                unlink(sFilename.c_str());
                nWrote = 0;
            }
        }
    }

    return nWrote;
}
#endif

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

#include "ObsServicesJson.h"

// obs includes
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>

#include <sys/stat.h>

// mfc includes
#include <libfcs/fcslib_string.h>
#include <libfcs/Log.h>
#include <libfcs/MfcJson.h>

// project/solution includes
#include <libPlugins/MFCConfigConstants.h>
#include <libPlugins/ObsUtil.h>
#include <libPlugins/Portable.h>
#include <libPlugins/SidekickModelConfig.h>

// for g_ctx
#include <ObsBroadcast/ObsBroadcast.h>

// System Includes
#include <string>
#include <iostream>
#include <fstream>

#include <sys/stat.h>

using njson = nlohmann::json;
using std::ifstream;
using std::string;

string CObsServicesJson::sm_sFileHash;
string CObsServicesJson::sm_sFilename;

extern CBroadcastCtx g_ctx;         // part of MFCLibPlugins.lib::MfcPluginAPI.obj

//---------------------------------------------------------------------------
// CObsServicesJson
//
// helper class to manage the services.json file.
CObsServicesJson::CObsServicesJson()
    : m_nVersion(0), m_bLoaded(false), m_bisDirty(false)
{}


//--------------------------------------------------------------------------
// load
//
// read the services file.
bool CObsServicesJson::load(const string& sFile)
{
    setLoaded(false);

    //_MESG("PATHDBG: Trying to load services from just file: %s", sFile.c_str());

    setServicesFilename(sFile);
    string sFilename = getNormalizedServiceFile(sFile);//obs_module_config_path("services.json");

    //_MESG("PATHDBG: trying to load %s ...", sFilename.c_str());
    parseFile(sFilename);

    return isLoaded();
}

bool CObsServicesJson::checkFileHash(void)
{
    bool fileChanged = false;
    struct stat st;

    if (stat(sm_sFilename.c_str(), &st) == 0)
    {
        string sOldHash(sm_sFileHash);
        size_t nSz;

        SidekickModelConfig::calcCheckSum(sm_sFilename, sm_sFileHash, nSz);

        if (sOldHash != sm_sFileHash)
            fileChanged = true;
    }
    else if (!sm_sFileHash.empty())
    {
        // file removed? clear our cache of file data
        sm_sFileHash.clear();
        fileChanged = true;
    }
   
    return fileChanged; 
}


/*
bool CObsServicesJson::load(const string& sFile, const string& sProgramFile)
{
    setLoaded(false);

    setServicesFilename(sFile);
    string sFilename = getNormalizedServiceFile(sFile);//obs_module_config_path("services.json");
    //_MESG("PATHDBG: Trying to load services from file: %s, program file: %s", sFilename.c_str(), sProgramFile.c_str());

    if (!parseFile(sFilename))
    {
        // if we failed to load, we might have a bad file version.
        if (Update(sFile, sProgramFile))
            parseFile(sFilename);
    }

    return isLoaded();
}
*/

//--------------------------------------------------------------------------
// save
//
// update the services file.
bool CObsServicesJson::save()
{
    bool retVal = false;

    string sData = m_njson.dump(4);
    string sFilename = getFilename();

    _MESG("PATHDBG: Writing %zu bytes to %s...", sData.size(), sFilename.c_str());
    if (stdSetFileContents(sFilename, sData))
        retVal = setDirty(false);

    return retVal;
}

#ifdef _WIN32
time_t CObsServicesJson::convertWindowsTimeToUnixTime(long long int input)
{
    #define TICKS_PER_SECOND        10000000
    #define EPOCH_DIFFERENCE        11644473600LL
    
    long long int temp;
    temp = input / TICKS_PER_SECOND; //convert from 100ns intervals to seconds;
    temp = temp - EPOCH_DIFFERENCE;  //subtract number of seconds between epochs
    return (time_t) temp;
}
#endif

const string CObsServicesJson::getNormalizedServiceFile(const string& sFile)
{
    string sFilename = sFile;//obs_module_config_path("services.json");
    assert(sFilename.size() > 0);
    string sFind = BROADCAST_FILENAME;
    string sReplace = "rtmp-services";
    size_t nFnd = sFilename.find(sFind);
    if (nFnd != string::npos)
        sFilename.replace(nFnd, sFind.length(), sReplace);
    return sFilename;
}


int CObsServicesJson::getJsonVersion(const string& sFile)
{
    string sFilename = getNormalizedServiceFile(sFile);
    //_TRACE("Attempting to load %s", sFilename.c_str());
    int nVer = INT_MAX;

    ifstream str(sFilename);
    if (str.is_open())
    {
        njson j;
        str >> j;

        if (j.find("format_version") != j.end())
        {
            nVer = j["format_version"].get<int>();
            //_TRACE("%s, file version: %d", sFilename.c_str(), nVer);
        }
        else _TRACE("Error Parsing file version: %s", sFilename.c_str());
    }
    else _TRACE("Error loading services.json %s", sFilename.c_str());

    return nVer;
}

/*
bool CObsServicesJson::Update(const string& sFileProfile, const string& sFileProgram)
{
    _MESG("PATHDBG: Update profile settings? profile: %s, program: %s", sFileProfile.c_str(), sFileProgram.c_str());
    int nProfileVer = getJsonVersion(sFileProfile);
    if (nProfileVer > RTMP_SERVICES_FORMAT_VERSION)
    {
        int nProgramVer = getJsonVersion(sFileProgram);
        // profile version is wrong.
        if (nProgramVer <= RTMP_SERVICES_FORMAT_VERSION)
        {
            string sFilename = getNormalizedServiceFile(sFileProgram);
            string sData;
            if (stdGetFileContents(sFilename, sData))
            {
//
//              _TRACE("updating service.jsons from profile %d to program files %d", nProfileVer, nProgramVer);
//              sFilename = getNormalizedServiceFile(sFileProfile);
#ifdef _WIN32
//              char szPath[_MAX_PATH + 1] = { '\0' };
//              char* pFile = NULL;
//              GetFullPathNameA(sFilename.c_str(), sizeof(szPath), szPath, &pFile);
//              string filepath = szPath;
//              string dirpath = filepath.substr(0, filepath.rfind('\\'));
//              CreateDirectoryA(dirpath.c_str(), NULL);
//              _TRACE("Creating %s just in case", dirpath.c_str());
#endif
//              _TRACE("PATHDBG: Update profile settings? profile: %s, program: %s",
//                     sFilename.c_str(), getNormalizedServiceFile(sFileProgram).c_str());
//              if (stdSetFileContents(sFilename, sData))
//                  _TRACE("updated service.jsons");
//              else
//                  _TRACE("failed to update %s", sFilename.c_str());

                return true;
            }
            else _TRACE("Error, failed to read source file %s", sFilename.c_str());
        }
        else
        {
            // both files are not the correct version.  obs-studio will throw an exception. let it handle it. bye
            _TRACE("Both service.jsons have bad version: profile %s program files %d expected %d", nProfileVer, nProgramVer, RTMP_SERVICES_FORMAT_VERSION);
        }
    }
    return false;
}
*/


//--------------------------------------------------------------------------
// loadDefaultRTMPService
//
// Load the default webrtc MFC service values into obs-studio json
/*
bool CObsServicesJson::loadDefaultRTMPService(njson& arr)
{
    njson jsRTMP =
    {
        { "name",   MFC_SERVICES_JSON_NAME_RTMP_VALUE },
        { "common", true                                },
        { "servers",
            {
                {
                    { "name",   MFC_SERVICES_JSON_PRIMARY_SERVER_NAME   },
                    { "url",    MFC_DEFAULT_BROADCAST_URL               }
                },
                {
                    { "name",   MFC_AUSTRALIA_SERVER_NAME               },
                    { "url",    MFC_AUSTRALIA_BROADCAST_URL             }
                },
                {
                    { "name",   MFC_EAST_ASIA_SERVER_NAME               },
                    { "url",    MFC_EAST_ASIA_BROADCAST_URL             }
                },
                {
                    { "name",   MFC_EUROPE_EAST_SERVER_NAME             },
                    { "url",    MFC_EUROPE_EAST_BROADCAST_URL           }
                },
                {
                    { "name",   MFC_EUROPE_WEST_SERVER_NAME             },
                    { "url",    MFC_EUROPE_WEST_BROADCAST_URL           }
                },
                {
                    { "name",   MFC_NORTH_AMERICA_EAST_SERVER_NAME      },
                    { "url",    MFC_NORTH_AMERICA_EAST_BROADCAST_URL    }
                },
                {
                    { "name",   MFC_NORTH_AMERICA_WEST_SERVER_NAME      },
                    { "url",    MFC_NORTH_AMERICA_WEST_BROADCAST_URL    }
                },
                {
                    { "name",   MFC_SOUTH_AMERICA_SERVER_NAME           },
                    { "url",    MFC_SOUTH_AMERICA_BROADCAST_URL         }
                }
            }
        },
        { "recommended",
            {
                { "keyint",               MFC_SERVICES_JSON_KEYINT_VALUE            },
                { "profile",              MFC_SERVICES_JSON_X264_PROFILE_VALUE      },
                { "max fps",              MFC_SERVICES_JSON_MAX_FPS_VALUE           },
                { "max video bitrate",    MFC_SERVICES_JSON_VIDEO_BITRATE_VALUE     },
                { "max audio bitrate",    MFC_SERVICES_JSON_AUDIO_BITRATE_VALUE     },
                { "x264opts",             MFC_SERVICES_JSON_X264OPTS_VALUE          },
                //{ "bframes",              MFC_SERVICES_JSON_BFRAMES_VALUE           },
                //{ "supported resolutions",
                //    {
                //        "1920x1080",
                //        "1280x720",
                //        "960x540",
                //        "640x360"
                //    }
                //}
            }
        }
    };
    arr.push_back(jsRTMP);
    return setDirty(true);
}
*/

//--------------------------------------------------------------------------
// loadDefaultWegbRTCService
//
// Load the default webrtc MFC service values into obs-studio json
bool CObsServicesJson::loadDefaultWebRTCService(njson& arr)
{
    njson jswebRTC =
    {
        { "name",       MFC_SERVICES_JSON_NAME_WEBRTC_VALUE },
        { "common",     true                                },
        { "servers",
            {
                {
                    { "name",   MFC_SERVICES_JSON_PRIMARY_SERVER_NAME   },
                    { "url",    "Automatic"                             }
                }
            }
        },
        { "recommended",
            {
                { "output",             MFC_DEFAULT_WEBRTC_OUTPUT               },
                { "keyint",             MFC_SERVICES_JSON_KEYINT_VALUE          },
                { "max fps",            MFC_SERVICES_JSON_MAX_FPS_VALUE         },
                { "max video bitrate",  MFC_SERVICES_JSON_VIDEO_BITRATE_VALUE   },
                { "max audio bitrate",  MFC_SERVICES_JSON_AUDIO_BITRATE_VALUE   },
                //{ "supported resolutions",
                //    {
                //        "1920x1080",
                //        "1280x720",
                //        "960x540",
                //        "640x360"
                //    }
                //}
            }
        }
    };
    arr.push_back(jswebRTC);
    return setDirty(true);
}


//--------------------------------------------------------------------------
// getURLList
//
// replace the URL server list.
void CObsServicesJson::getURLList(strVec* parrNames, strVec* parrURL)
{
    _ASSERT(isLoaded());
    if (isLoaded())
    {
        njson mfcServers;
        if (findMFCRTMPServerJson(&mfcServers))
        {
            for (njson::iterator itr = mfcServers.begin(); itr != mfcServers.end(); itr++)
            {
                njson& srv = *itr;
                string sServerName = srv["name"].get<string>();
                string sURL = srv["url"].get<string>();
                parrNames->push_back(sServerName);
                parrURL->push_back(sURL);
            }
        }
        else _TRACE("No MFC servers defined. This really shouldn't happen");
    }
}


//--------------------------------------------------------------------------
// setURLList
//
// replace the current MFC URL server list
void CObsServicesJson::setURLList(strVec& arrNames, strVec& arrURL)
{
    _ASSERT(isLoaded());
    string sData;
    if (isLoaded())
    {
        njson jsMFCServers;
        if (findMFCRTMPServerJson(&jsMFCServers))
        {
            jsMFCServers.clear();
            for (size_t c = 0; c < arrNames.size(); c++)
            {
                string sName = arrNames[c];
                string sURL = arrURL[c];
                njson jSvr = { { "name", sName.c_str() }, { "url", sURL.c_str() } };
                jsMFCServers.push_back(jSvr);
            }
            sData = jsMFCServers.dump(4);
            setDirty(false);
        }
    }
}


//--------------------------------------------------------------------------
// getVersion
//
// get the version of the file.
bool CObsServicesJson::getVersion(int& nVersion)
{
    bool foundVal = false;

    nVersion = INT_MAX;
    if (m_njson.find("format_version") != m_njson.end())
    {
        nVersion = m_njson["format_version"].get<int>();
        foundVal = true;
    }

    return foundVal;
}


//---------------------------------------------------------------------------
// getMFCServiceJson
//
// find the MFC service json

bool CObsServicesJson::findMFCRTMPServerJson(njson* pArr)
{
    if (m_njson.find("services") != m_njson.end() && m_njson["services"].is_array())
    {
        njson& arr = m_njson["services"];

        //setServiceNJson(arr);
        _TRACE("Got services array\n");
        // find myfreecams.
        njson mfc;
        if (findRTMPService(arr, &mfc))
        {
            njson::iterator itr = mfc.find("servers");
            if (itr != mfc.end())
            {
                *pArr = mfc["servers"];
                return true;
            }
            else
            {
                _ASSERT(!"No server object found");
                string sData = mfc.dump(4);
                _TRACE("No server object found %s", sData.c_str());
            }
        }
    } // end for each array elelment
    return false;;
}


bool CObsServicesJson::findRTMPService(njson& arr, njson* pSrv)
{
    return findMFCServiceJson(arr, pSrv, string(MFC_SERVICES_JSON_NAME_RTMP_VALUE));
}

bool CObsServicesJson::findWebRtcService(njson& arr, njson* pSrv)
{
    return findMFCServiceJson(arr, pSrv, string(MFC_SERVICES_JSON_NAME_WEBRTC_VALUE));
}


//---------------------------------------------------------------------------
// getMFCServiceJson
//
// find the MFC service json

bool CObsServicesJson::findMFCServiceJson(njson& arr, njson* pSrv, const string& sSvcName)
{
    assert(arr.is_array());

    for (njson::iterator itr = arr.begin(); itr != arr.end(); itr++)
    {
        njson j = *itr;
        string sName = j[MFC_SERVICES_JSON_NAME].get<string>();

        if (sName == sSvcName)
        {
            *pSrv = j;
            return true;
        } // endif this object is not MyFreeCams.
    }
    // _ASSERT(!"Name object not found in json");
    string sData = arr.dump(4);
    // _TRACE("Offending json: %s", sData.c_str());
    _TRACE("found service.json");

    return false;
}

//--------------------------------------------------------------------------
// parseFile
//
// parse the services json file.
bool CObsServicesJson::parseFile(const string& sFilename)
{
    sm_sFilename = sFilename;
    bool bFnd = false;

    ifstream str(sFilename);
    if (str.is_open())
    {
        m_njson.clear();

        str >> m_njson;
        string js = m_njson.dump(4);

        string sVersion;
        int nVer = 0;

        if (m_njson.find("format_version") != m_njson.end())
        {
            nVer = m_njson["format_version"].get<int>();

            m_nVersion = nVer;
            if (nVer <= RTMP_SERVICES_FORMAT_VERSION)
            {
                //_TRACE("services.json version: %d", nVer);
                if (m_njson.find("services") != m_njson.end() && m_njson["services"].is_array())
                {
                    njson& arr = m_njson["services"];
                    njson mfc;

                    if (!findWebRtcService(arr, &mfc))
                    {
                        _TRACE("%s service not found!", MFC_SERVICES_JSON_NAME_WEBRTC_VALUE);
                        if (loadDefaultWebRTCService(arr))
                        {
#ifdef UNIT_TEST
                            if (!findRTMPService(arr, &mfc))
                                bFnd = false;
                            if (!findWebRtcService(arr, &mfc))
                                bFnd = false;
#endif
                            bFnd = true;
                        }
                    }
                    else
                    {
                        bFnd = true;
                    }
                }
                else
                {
                    _ASSERT(!"Services is not an array? this really shouldn't happen");
                    _TRACE("Json format error, services is not an array");
                }
            }
            else
            {
                _TRACE("Invalid version: %d. Version expected: %d", nVer, RTMP_SERVICES_FORMAT_VERSION);
                bFnd = false;
            }
        }
    }

    // update our hash/modification time of parsed filename,
    // even if it failed, which clears static member vars
    checkFileHash();

    if (bFnd)
        setLoaded(true);

    return bFnd;
}


bool CObsServicesJson::updateProfileSettings(const string& sKey, const string& sURL)
{
    bool retVal = false;
    MfcJsonObj js;

    if (g_ctx.cfg.loadProfileConfig(js))
    {
        MfcJsonObj* pSet = js.objectGet(SERVICE_JSON_SETTING);
        if (pSet)
        {
            bool mfcService = false;
            string sName;
            if (!pSet->objectGetString(SERVICE_JSON_SERVICE, sName))
                sName = "Custom";

            string sKeyCurrent, sURLCurrent;
            pSet->objectGetString(SERVICE_JSON_STREAM_KEY, sKeyCurrent);
            pSet->objectGetString(SERVICE_JSON_STREAM_URL, sURLCurrent);

            // only save if the current service starts with 'MyFreeCams',
            // such as services 'MyFreeCams RTMP' or 'MyFreeCams WebRTC',
            // or if it's a Custom service with an MFC server URL detected.
            if ( sName.find("MyFreeCams") == 0 )
            {
                mfcService = true;
            }
            else if (sName == "Custom")
            {
                if (    sURL.find(".myfreecams.com/NxServer") != string::npos
                    ||  sURLCurrent.find(".myfreecams.com/NxServer") != string::npos)
                {
                    mfcService = true;
                }
            }

            if (mfcService)
            {
                if (sKeyCurrent != sKey || sURLCurrent != sURL)
                {
                    size_t updates = 0;
                    // if either the sKey or sURL are set to the string "(null)", then we skip
                    // any updating of them. This is so we can update one of properties without updating
                    // the other, in cases where we have only some of the data available.

                    if (sKey != "(null)")
                    {
                        pSet->objectAdd(string(SERVICE_JSON_STREAM_KEY), sKey);
                        updates++;
                    }

                    if (sURL != "(null)")
                    {
                        pSet->objectAdd(string(SERVICE_JSON_STREAM_URL), sURL);
                        updates++;
                    }

                    if (updates > 0)
                    {
                        // write current profile back to disk with a call to g_ctx.cfg.writeProfileConfig() ?
                        // ...
                        //
                        // ... pass sKey/sUrl to g_ctx.cfg .... ?
                        //
                        if ((retVal = g_ctx.cfg.writeProfileConfig(js)) != false)
                        {
                            blog(100, "[SVCDBG] wroteProfileConfig OK");
                        }
                        else blog(100, "[SVCDBG] wroteProfileConfig FAILED");


                        /*
                        string sCurData, sData = js.prettySerialize();

                        stdGetFileContents(sFilename, sCurData);
                        if (sData != sCurData)
                        {
                            if ((retVal = stdSetFileContents(sFilename, sData)) == true)
                            {
                                blog(100, "[wrote profile config] %s; %zu => %zu bytes", sFilename.c_str(), sCurData.size(), sData.size());
                            }
                            else _MESG("FAILED TO SET %s with data: %s", sFilename.c_str(), sData.c_str());
                        }
                        */
                    }
                    else retVal = true;
                }
            }
            else _MESG("SKIPPED non-mfc service profile %s", sName.c_str());
        }
        else _MESG("failed to find settings");
    }
    else _TRACE("failed to load profile from disk");

    return retVal;
}


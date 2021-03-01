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

#include <nlohmann/json.hpp>

#include <string>
#include <vector>
    
using njson = nlohmann::json;
using std::vector;
using std::string;

typedef vector<string> strVec;

// helper class to manage the services.json file.
class CObsServicesJson
{
public:
    CObsServicesJson();

    bool load(const string& sFilename);
    bool load(const string& sFilename, const string& sProgamFile);
    bool save();

    bool getVersion(int& nVersion);

    void getURLList(strVec* parrNames, strVec* parrURL);
    void setURLList(strVec& arrNames, strVec& arrURL);

 #ifdef _WIN32
    static time_t convertWindowsTimeToUnixTime(long long int input);
 #endif

    static time_t getFileModifyTm(const string& sFile);

    // helper function for debugging.
    string prettySerialize() { return m_njson.dump(4); }

    // true if we successfully loaded json data.
    bool isLoaded() { return m_bLoaded;}
    void setLoaded(bool b) { m_bLoaded = b; }

    // true if the json data has been changed and needs saving.
    bool isDirty() { return m_bisDirty;}
    bool setDirty(bool b) { m_bisDirty = b; return true; }

    bool findRTMPService(njson& arr, njson* pSrv);
    bool findWebRtcService(njson& arr, njson* pSrv);

    bool findMFCServiceJson(njson& arr, njson* pSvr, const string& sName);
    bool findMFCRTMPServerJson(njson* pSrv);

    void setServicesFilename(const string& sFile)
    {
        m_sServicesFilename = sFile;
    }
    string getServicesFilename() { return m_sServicesFilename; }
    bool Update(const string& sFileProfile, const string& sFileProgram);

    string getProfileServiceJson() { return m_sProfileService;  }
    void setProfileServiceJson(const string& s) { m_sProfileService = s; }

    string getProgramServiceJson() { return m_sProgramFileServices; }
    void setProgramServiceJson(const string& s) { m_sProgramFileServices = s; }

    bool updateProfileSettings(const string& sKey, const string& sURL);
    bool refreshProfileSettings(string& sKey, string& sURL);

protected:
    bool parseFile(const string& sFilename);

    bool loadDefaultWebRTCService(njson &);
    //bool loadDefaultRTMPService(njson &);

    njson& getTopLevelJson() { return m_njson; }

    njson& getServicesNJson() { return m_njsonServices; }
    void setServicesNJson(njson& j) { m_njsonServices = j; }

    const string& getFilename() { return m_sFilename;}
    const string getNormalizedServiceFile(const string& sFile);
    int getJsonVersion(const string& sFile);

private:
    string m_sFilename;
    string m_sData;
    //MfcJsonObj m_json;
    int m_nVersion;
    //MfcJsonObj* m_jsonServices;
    bool m_bLoaded;
    bool m_bisDirty;
    string m_sServicesFilename;
    string m_sProfileService;
    string m_sProgramFileServices;
    njson m_njsonServices;
    njson m_njson;
};

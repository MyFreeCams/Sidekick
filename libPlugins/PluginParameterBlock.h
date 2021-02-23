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

#include <time.h>
#include <string>

using std::string;

class MfcJsonObj;

class CPluginParameterBlock
{
public:
    CPluginParameterBlock();
    //CPluginParameterBlock(const CPluginParameterBlock &src);

    const CPluginParameterBlock &operator=(const CPluginParameterBlock &src);
    //bool operator==(const CPluginParameterBlock &src);

    void LoadDefaults();
    bool SaveDefaults();
    bool refresh();
    bool Deserialize(MfcJsonObj &result);

    int getHeartBeatInterval()
    {
        return m_nHeartBeatInterval;
    }
    void setHeartBeatInterval(int n)
    {
        if (m_nHeartBeatInterval != n)
            setDirty(true);
        m_nHeartBeatInterval = n;
    }

    string getToken(void)
    {
        return sidekickToken;
    }

    void setToken(const string& sTok, bool setTokenTime = true)
    {
        sidekickToken = sTok;
        if (setTokenTime)
            time(&sidekickTokenTm);
    }

    time_t getTokenStamp(void)
    {
        return sidekickTokenTm;
    }
    void setTokenStamp(time_t nStamp) { sidekickTokenTm = nStamp; }

    bool tokenExpired(void)
    {
        return (time(NULL) - sidekickTokenTm > 300);
    }

    int getDeadCount() { return m_nDeadCount; }
    void setDeadCount(int n)
    {
        if (m_nDeadCount != n)
            setDirty(true);
        m_nDeadCount = n;
    }

    bool isSendLogs() { return m_bSendLogs; }
    void setSendLogs(bool b)
    {
        if (m_bSendLogs != b)
            setDirty(true);
        m_bSendLogs = b;
    }

    bool isUpdateUpdater() { return m_bUpdateUpdater; }
    void setUpdateUpdater(bool b)
    {
        if (m_bUpdateUpdater != b)
            setDirty(true);
        m_bUpdateUpdater = b;
    }

    bool isUpdaterRunSysReport() { return m_bUpdaterRunSysReport; }
    void setUpdaterRunSysReport(bool b)
    {
        if (m_bUpdaterRunSysReport != b)
            setDirty(true);
        m_bUpdaterRunSysReport = b;
    }

    const string &getBroadcastURL() { return m_sBroadCastURL; }
    void setBroadcastURL(const string& s)
    {
        if (m_sBroadCastURL != s)
            setDirty(true);
        m_sBroadCastURL = s;
    }

    const string &getSessionTicket() { return m_sSessionTicket; }
    void setSessionTIcket(const string& s)
    {
        if (m_sSessionTicket != s)
            setDirty(true);
        m_sSessionTicket = s;
    }

    const string &getSessionTKX() { return m_sSessionTKX; }
    void setSessionTKX(const char *p) { m_sSessionTKX = p; }
    void setSessionTKX(string &s) { m_sSessionTKX = s; }

    const string &getModelID() { return m_sModelID; }
    void setModelID(const string& s)
    {
        if (m_sModelID != s)
            setDirty(true);
        m_sModelID = s;
    }

    const string &getBinPath() { return m_sBinPath; }
    void setBinPath(const string &s)
    {
        if (m_sBinPath != s)
            setDirty(true);
        m_sBinPath = s;
    }

    const string &getModelStreamKey() { return m_sStreamingKey; }
    void setModelStreamKey(const char *p) { setModelStreamKey(string(p)); }
    void setModelStreamKey(const string &s)
    {
        if (m_sStreamingKey != s)
            setDirty(true);
        m_sStreamingKey = s;
    }

    const string &getVersion() { return m_sVersion; }
    void  setVersion(const char *p) { setVersion(string(p)); }
    void  setVersion(const string &s)
    {
        if (m_sVersion != s)
            setDirty(true);
        m_sVersion = s;
    }

    bool getAllowConnection() { return m_bAllowConnection; }
    void setAllowConnection(bool b)
    {
        if (m_bAllowConnection != b)
            setDirty(true);
        m_bAllowConnection = b;
    }

    const string getMSK();

    void setServicesFilename(const string &sFile)
    {
        if (m_sServicesFilename != sFile)
            setDirty(true);
        m_sServicesFilename = sFile;
    }

    string getServicesFilename() { return m_sServicesFilename; }

    const string getModelUserName() { return m_sModelUserName; }
    void setModelUserName(const string &s) { m_sModelUserName = s; }
    void setModelUserName(const char *p) { m_sModelUserName = p; }

    const string getModelPwd() { return m_sModelPassword; }
    void setModelPwd(const string &s) { m_sModelPassword = s; }

    bool isDirty() { return m_bDirty; }
    void setDirty(bool b) { m_bDirty = b; }

    int getSessionID() { return m_nSessionID; }
    void setSessionID(int val) { m_nSessionID = val; }

    int  getHeight() { return m_nHeight; }
    void setHeight(int n) { m_nHeight = n; }

    int getWidth() { return m_nWidth; }
    void setWidth(int n) { m_nWidth = n; }

    int getFrameRate() { return m_nFrameRate; }
    void setFrameRate(int n) { m_nFrameRate = n; }

    const string getProtocol() { return m_sProtocol; }
    void setProtocol(string &s) { m_sProtocol = s; }

    const string getCodec() { return m_sCodec; }
    void setCodec(string &s) { m_sCodec = s; }

private:
    bool    m_bAllowConnection;
    int     m_nHeartBeatInterval;
    int     m_nDeadCount;
    bool    m_bSendLogs;
    bool    m_bUpdateUpdater;
    bool    m_bUpdaterRunSysReport;
    string  m_sBroadCastURL;
    string  m_sSessionTicket;
    string  m_sSessionTKX;
    string  m_sStreamingKey;
    string  m_sModelID;
    string  m_sBinPath;
    string  m_sVersion;
    string  m_sServicesFilename;
    string  m_sModelUserName;
    string  m_sModelPassword;
    int     m_nSessionID;
    int     m_nHeight;
    int     m_nWidth;
    int     m_nFrameRate;
    string  m_sProtocol;
    string  m_sCodec;

    string  sidekickToken;       // an auth token to continue a sidekick heartbeat polling session
    time_t  sidekickTokenTm;     // time the last sidekick token was issued. If > 5min ago, session has expired.

    bool    m_bDirty;
};

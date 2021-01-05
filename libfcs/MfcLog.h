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

#ifndef MFC_LOG_H
#define MFC_LOG_H

#include <stdio.h>
#include <stdarg.h>
#ifndef _WIN32
#include <unistd.h>
#include <syslog.h>
#endif
#include <time.h>

#include "ILog.h"

class MfcLog : public ILog
{
public:
    MfcLog() { Setup("/tmp"); }
    ~MfcLog() override = default;

    ILog* GetILog(void) { return (ILog*)this; }

    // Internal method implementing actual log
    void _Mesg(ILog::LogLevel nLevel, const char* pszMesg);

    void MesgStr(const string& sMsg)  { _Mesg(ILog::NOTICE, sMsg.c_str()); }
    void DebugStr(const string& sMsg) { _Mesg(ILog::DBG,    sMsg.c_str()); }
    void TraceStr(const string& sMsg) { _Mesg(ILog::TRACE,  sMsg.c_str()); }

    void MesgStr(ILog::LogLevel nLevel, const string& sMsg)
    {
        _Mesg(nLevel, sMsg.c_str());
    }

    struct ILog::LogData m_Data;

    //-- [ ILog Interface Implementation ] --------------------------------
    //
    void Setup(const string& sLogDir) override;

    void Debug(const char* pszFmt, ...) override;
    void Trace(const char* pszFmt, ...) override;
    void Mesg(const char* pszFmt, ...) override;

    void Mesg(ILog::LogLevel nLevel, const char* pszFmt, ...) override;

    void Flush(void) override;

    void SetLog(LogClass nClass, const char* pszFile, bool fUnlink) override;
    bool GetLog(LogClass nClass, string& sFile) override;
    size_t DeleteLogs(LogClass nClass = MAX_LOGCLASS) override;

    void SetStampMask(int nValue) override;
    void AddStampMask(int nValue) override;
    void SetAutoRotate(size_t nSz) override;

    void SetOutputMask(ILog::LogLevel nLevel, int nValue) override;
    void AddOutputMask(ILog::LogLevel nLevel, int nValue) override;
    void SubOutputMask(ILog::LogLevel nLevel, int nValue) override;
    bool HasOutputMask(ILog::LogLevel nLevel, int nValue) override;
    int  GetOutputMask(ILog::LogLevel nLevel) override;

    void TraceMarker(const char* pszFile, const char* pszFunction, int nLine,
                     ILog::LogLevel nLevel, const char* pszFmt, ...) override;

    struct ILog::LogData* Data(void) override { return &m_Data; }
    //
    // -------------------------------------------------------------------

private:
    bool OpenLog(LogClass nClass);

    inline LogClass ClassOf(ILog::LogLevel nLevel)
    {
        if (nLevel == DBG)      return ILog::LC_DEBUG;
        if (nLevel == TRACE)    return ILog::LC_TRACE;
        return ILog::LC_MAIN;
    }
};

#endif // MFC_LOG_H

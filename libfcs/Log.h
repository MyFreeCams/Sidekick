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

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#ifndef _WIN32
#include <unistd.h>
#include <syslog.h>
#endif
#include <time.h>
#include "Compat.h"

#include "ILog.h"
#include "MfcLog.h"

#define _TRACE(pszFmt, ...)         Log::TraceMarker(               __FILE__, __FUNCTION__, __LINE__, ILog::TRACE,  pszFmt, ##__VA_ARGS__)
#ifdef _LOGDEBUG_
#define _DBG(pszFmt, ...)           Log::TraceMarker(               __FILE__, __FUNCTION__, __LINE__, ILog::DBG,    pszFmt, ##__VA_ARGS__)
#else
#define _DBG(...)
#endif
#define _MESG(pszFmt, ...)          Log::TraceMarker(               __FILE__, __FUNCTION__, __LINE__, ILog::NOTICE, pszFmt, ##__VA_ARGS__)
#define _RMESG(retVal, pszFmt, ...) Log::TraceMarkerRetVal(retVal,  __FILE__, __FUNCTION__, __LINE__, ILog::NOTICE, pszFmt, ##__VA_ARGS__)

/*

Static/Global version of MfcLog interface.  Don't use in multithreaded environment.

*/
class Log
{
public:
    Log() = default;
    virtual ~Log() = default;

    //-- [ ILog Interface Implementation ] --------------------------------
    //
    static void Setup(const string& sLogDir);

    static void Debug(const char* pszFmt, ...);
    static void Trace(const char* pszFmt, ...);
    static void Mesg(const char* pszFmt, ...);
    static void Mesg(ILog::LogLevel nLevel, const char* pszFmt, ...);

    static void MesgStr(const string& sMsg)
    {
        _Mesg(ILog::NOTICE, sMsg.c_str());
    }
    static void MesgStr(ILog::LogLevel nLevel, const string& sMsg)
    {
        _Mesg(nLevel, sMsg.c_str());
    }
    static void DebugStr(const string& sMsg)
    {
        _Mesg(ILog::DBG, sMsg.c_str());
    }
    static void TraceStr(const string& sMsg)
    {
        _Mesg(ILog::TRACE, sMsg.c_str());
    }

    static void Flush(void);

    static void _Mesg(ILog::LogLevel nLevel, const char* pszMsg);           // Internal format that writes the log, no variable args

    static bool TraceFor(uint32_t dwUserId)
    {
        switch (dwUserId)
        {
            case 100:           // AaronCam
            case 16558:         // mfcguy
            case 36948:         // mfcuser
            case 127465:        // xmfc1
            case 127466:        // xmfc2
            case 127467:        // xmfc3
            case 164056:        // xmfc4
            case 469516:        // TestCam2
            case 469517:        // TestCam3
            case 469518:        // TestCam1
            case 70001:         // TestCam7
                return true;
        }
        return false;
    }

    static void SetLog(ILog::LogClass nClass, const char* pszFile, bool fUnlink = false);
    static bool GetLog(ILog::LogClass nClass, std::string& sFile);
    static size_t DeleteLogs(ILog::LogClass nClass = ILog::MAX_LOGCLASS);

    static void SetStampMask(int nValue);
    static void AddStampMask(int nValue);
    static void SetAutoRotate(size_t nSz);
    static void SetOutputMask(ILog::LogLevel nLevel, int nValue);
    static void AddOutputMask(ILog::LogLevel nLevel, int nValue);
    static void SubOutputMask(ILog::LogLevel nLevel, int nValue);
    static bool HasOutputMask(ILog::LogLevel nLevel, int nValue);
    static int  GetOutputMask(ILog::LogLevel nLevel);

    static void TraceMarker(                            const char* pszFile, const char* pszFunction, int nLine, ILog::LogLevel nLevel, const char* pszFmt, ...);
    static bool TraceMarkerRetVal(bool retVal,          const char* pszFile, const char* pszFunction, int nLine, ILog::LogLevel nLevel, const char* pszFmt, ...);
    static uint32_t TraceMarkerRetVal(uint32_t retVal,  const char* pszFile, const char* pszFunction, int nLine, ILog::LogLevel nLevel, const char* pszFmt, ...);

    static struct ILog::LogData* Data(void)
    {
        return &sm_Log.m_Data;
    }
    //
    // -------------------------------------------------------------------

    static MfcLog sm_Log;
};

#endif // LOG_H

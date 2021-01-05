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


/*

 How to use this class:

 Same as MfcLog, but entire thing is static so its similar to a global variable.

 ------------------------------------------------------------------------
 NOTES: NOT THREAD SAFE. DO NOT USE MULTITHREADED.
 ------------------------------------------------------------------------

*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>

#include <libPlugins/Portable.h>

#include "Compat.h"
#include "ILog.h"
#include "Log.h"
#include "fcslib_string.h"
#include "MfcLog.h"

void proxy_blog(int nLevel, const char* pszMsg);
MfcLog Log::sm_Log;


void Log::Setup(const string& sLogDir)
{
    sm_Log.Setup(sLogDir);
}


void Log::SetLog(ILog::LogClass nClass, const char* pszFile, bool fUnlink)
{
    sm_Log.SetLog(nClass, pszFile, fUnlink);
}


bool Log::GetLog(ILog::LogClass nClass, string& sFile)
{
    return sm_Log.GetLog(nClass, sFile);
}


size_t Log::DeleteLogs(ILog::LogClass nClass)
{
    return sm_Log.DeleteLogs(nClass);
}


void Log::Mesg(const char* pszFmt, ...)
{
    char szData[16384];
    va_list vaList;
    va_start(vaList, pszFmt);
#ifndef _WIN32
    vsnprintf(szData, sizeof(szData), pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(ILog::NOTICE, szData);
#else
    _vsnprintf_s(szData, sizeof(szData), _TRUNCATE, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(ILog::NOTICE, szData);
#endif
}


void Log::_Mesg(ILog::LogLevel nLevel, const char* pszMsg)
{
    sm_Log._Mesg(nLevel, pszMsg);
}


void Log::Debug(const char* pszFmt, ...)
{
    char szData[16384];
    va_list vaList;
    va_start(vaList, pszFmt);
#ifndef _WIN32
    vsnprintf(szData, sizeof(szData), pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(ILog::DBG, szData);
#else
    _vsnprintf_s(szData, sizeof(szData), _TRUNCATE, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(ILog::DBG, szData);
#endif
}


void Log::Trace(const char* pszFmt, ...)
{
    char szData[16384];
    va_list vaList;
    va_start(vaList, pszFmt);
#ifndef _WIN32
    vsnprintf(szData, sizeof(szData), pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(ILog::TRACE, szData);
#else
    _vsnprintf_s(szData, sizeof(szData), _TRUNCATE, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(ILog::TRACE, szData);
#endif
}


void Log::Mesg(ILog::LogLevel nLevel, const char* pszFmt, ...)
{
    char szData[16384];
    va_list vaList;
    va_start(vaList, pszFmt);
#ifndef _WIN32
    vsnprintf(szData, sizeof(szData), pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(nLevel, szData);
#else
    _vsnprintf_s(szData, sizeof(szData), _TRUNCATE, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(nLevel, szData);
#endif
}


bool Log::TraceMarkerRetVal(bool retVal, const char* pszFile, const char* pszFunction, int nLine, ILog::LogLevel nLevel, const char* pszFmt, ...)
{
    char szData[16384];
    const char* pszTraceFmt = "(%s:%d) ";
    const char* pszShortFile = pszFile;
    string sFile;
    va_list vaList;

    // Optionally support inclusion of function in trace header
    if (sm_Log.m_Data.fTraceFunction)
    {
#ifndef _WIN32
        // Don't include full path on mac -- set pszShortFile to filename after last slash
        if ((pszShortFile = strrchr(pszFile, '/')) != NULL)
        {
            sFile = (const char*)(pszShortFile + 1);
            pszShortFile = sFile.c_str();
        }
        else pszShortFile = pszFile;
#else
        // Don't include full path on windows -- set pszShortFile to filename after last backslash
        if ((pszShortFile = strrchr(pszFile, '\\')) != NULL)
        {
            sFile = (const char*)(pszShortFile + 1);
            pszShortFile = sFile.c_str();
        }
        else pszShortFile = pszFile;
#endif
        pszTraceFmt = "[%s:%d, %s()]  ";
    }

    int nLen = snprintf(szData, sizeof(szData), pszTraceFmt, pszShortFile, nLine, pszFunction);

    // An error or truncated header? then we drop the tracefmt header by writing the remaining log data to szData+0
    if (nLen < 0 || nLen > (int)sizeof(szData))
        nLen = 0;

    va_start(vaList, pszFmt);
#ifndef _WIN32
    vsnprintf(szData + nLen, sizeof(szData) - nLen, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(nLevel, szData);
#else
    _vsnprintf_s(szData + nLen, sizeof(szData) - nLen, _TRUNCATE, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(nLevel, szData);
#endif

    return retVal;
}


uint32_t Log::TraceMarkerRetVal(uint32_t retVal, const char* pszFile, const char* pszFunction, int nLine, ILog::LogLevel nLevel, const char* pszFmt, ...)
{
    char szData[16384];
    const char* pszTraceFmt = "(%s:%d) ";
    const char* pszShortFile = pszFile;
    string sFile;
    va_list vaList;

    // Optionally support inclusion of function in trace header
    if (sm_Log.m_Data.fTraceFunction)
    {
#ifndef _WIN32
        // Don't include full path on mac -- set pszShortFile to filename after last slash
        if ((pszShortFile = strrchr(pszFile, '/')) != NULL)
        {
            sFile = (const char*)(pszShortFile + 1);
            pszShortFile = sFile.c_str();
        }
        else pszShortFile = pszFile;
#else
        // Don't include full path on windows -- set pszShortFile to filename after last backslash
        if ((pszShortFile = strrchr(pszFile, '\\')) != NULL)
        {
            sFile = (const char*)(pszShortFile + 1);
            pszShortFile = sFile.c_str();
        }
        else pszShortFile = pszFile;
#endif
        pszTraceFmt = "[%s:%d, %s()]  ";
    }

    int nLen = snprintf(szData, sizeof(szData), pszTraceFmt, pszShortFile, nLine, pszFunction);

    // An error or truncated header? then we drop the tracefmt header by writing the remaining log data to szData+0
    if (nLen < 0 || nLen > (int)sizeof(szData))
        nLen = 0;

    va_start(vaList, pszFmt);
#ifndef _WIN32
    vsnprintf(szData + nLen, sizeof(szData) - nLen, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(nLevel, szData);
#else
    _vsnprintf_s(szData + nLen, sizeof(szData) - nLen, _TRUNCATE, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(nLevel, szData);
#endif

    return retVal;
}


void Log::TraceMarker(const char* pszFile, const char* pszFunction, int nLine, ILog::LogLevel nLevel, const char* pszFmt, ...)
{
    char szData[16384];
    const char* pszTraceFmt = "(%s:%d) ";
    const char* pszShortFile = pszFile;
    string sFile;
    va_list vaList;

    // Optionally support inclusion of function in trace header
    if (sm_Log.m_Data.fTraceFunction)
    {
#ifndef _WIN32
        // Don't include full path on mac -- set pszShortFile to filename after last slash
        if ((pszShortFile = strrchr(pszFile, '/')) != NULL)
        {
            sFile = (const char*)(pszShortFile + 1);
            pszShortFile = sFile.c_str();
        }
        else pszShortFile = pszFile;
#else
        // Don't include full path on windows -- set pszShortFile to filename after last backslash
        if ((pszShortFile = strrchr(pszFile, '\\')) != NULL)
        {
            sFile = (const char*)(pszShortFile + 1);
            pszShortFile = sFile.c_str();
        }
        else pszShortFile = pszFile;
#endif
        pszTraceFmt = "[%s:%d, %s()]  ";
    }

    int nLen = snprintf(szData, sizeof(szData), pszTraceFmt, pszShortFile, nLine, pszFunction);

    // An error or truncated header? then we drop the tracefmt header by writing the remaining log data to szData+0
    if (nLen < 0 || nLen > (int)sizeof(szData))
        nLen = 0;

    va_start(vaList, pszFmt);
#ifndef _WIN32
    vsnprintf(szData + nLen, sizeof(szData) - nLen, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(nLevel, szData);
#else
    _vsnprintf_s(szData + nLen, sizeof(szData) - nLen, _TRUNCATE, pszFmt, vaList);
    va_end(vaList);
    sm_Log._Mesg(nLevel, szData);
    proxy_blog(100, szData);
#endif
}


void Log::Flush(void)
{
    sm_Log.Flush();
}


void Log::SetStampMask(int nValue)
{
    sm_Log.SetStampMask(nValue);
}


void Log::AddStampMask(int nValue)
{
    sm_Log.AddStampMask(nValue);
}


void Log::SetAutoRotate(size_t nSz)
{
    sm_Log.SetAutoRotate(nSz);
}


void Log::SetOutputMask(ILog::LogLevel nLevel, int nValue)
{
    sm_Log.SetOutputMask(nLevel, nValue);
}


void Log::AddOutputMask(ILog::LogLevel nLevel, int nValue)
{
    sm_Log.AddOutputMask(nLevel, nValue);
}


void Log::SubOutputMask(ILog::LogLevel nLevel, int nValue)
{
    sm_Log.SubOutputMask(nLevel, nValue);
}


bool Log::HasOutputMask(ILog::LogLevel nLevel, int nValue)
{
    return sm_Log.HasOutputMask(nLevel, nValue);
}


int Log::GetOutputMask(ILog::LogLevel nLevel)
{
    return sm_Log.GetOutputMask(nLevel);
}

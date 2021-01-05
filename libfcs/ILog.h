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

#ifndef ILOG_H
#define ILOG_H

#include <stdio.h>
#include <stdarg.h>
#ifndef _WIN32
#include <unistd.h>
#include <syslog.h>
#include <sys/time.h>
#endif
#include <time.h>

#include <string>

using namespace std;

#define ANSI_DARKGRAY   "\x1b[1;30m"
#define ANSI_RED        "\x1b[1;31m"
#define ANSI_GREEN      "\x1b[1;32m"
#define ANSI_YELLOW     "\x1b[1;33m"
#define ANSI_BLUE       "\x1b[1;34m"
#define ANSI_MAGENTA    "\x1b[1;35m"
#define ANSI_CYAN       "\x1b[1;36m"
#define ANSI_WHITE      "\x1b[1;37m"
#define ANSI_D_RED      "\x1b[0;31m"
#define ANSI_D_GREEN    "\x1b[0;32m"
#define ANSI_D_YELLOW   "\x1b[0;33m"
#define ANSI_D_BLUE     "\x1b[0;34m"
#define ANSI_D_MAGENTA  "\x1b[0;35m"
#define ANSI_D_CYAN     "\x1b[0;36m"
#define ANSI_D_WHITE    "\x1b[0;37m"
#define ANSI_RESET      "\x1b[0m"

// ILog Interface class
class ILog
{
public:
    ILog() = default;
    virtual ~ILog() = default;

    // NOTE: If MAX_LOGLEVEL is > 3, must manually add initializers to static
    // definition of nLogFds in Log.cpp (can't auto-initialize to -1 in C99)

    enum LogClass
    {
        LC_MAIN     = 0,            // to sLogFiles[ ILog::LC_MAIN ] for LogLevel NOTICE...EMERG
        LC_DEBUG,                   // to sLogFiles[ ILog::LC_DEBUG ] for LogLevel DEBUG (LOG_INFO) messages
        LC_TRACE,                   // to sLogFiles[ ILog::LC_TRACE ] for LogLevel TRACE (LOG_DEBUG) messages
        MAX_LOGCLASS                // size for arrays relating to LogClass
    };

    // Log level of a message routes it to a given LogClass (for file logging) or directly maps to syslog log levels
    // EMERG = LOG_EMERG from syslog.h, ERR = LOG_ERR, etc

    enum LogLevel
    {
#ifdef _WIN32
        EMERG       = 0,
#else
        EMERG       = LOG_EMERG,    // Sent to LC_MAIN file.  Be warned, used with syslog it will output to all logged in terminals too
#endif
        ALERT,
        CRIT,
        ERR,
        WARNING,
        NOTICE,
        DBG,        /* = LOG_INFO */    // Sent to LC_DEBUG file
        TRACE,      /* = LOG_DEBUG */   // Sent to LC_TRACE file
        MAX_LOGLEVEL
    };

    enum TimeStamp
    {
        TS_NONE         = 0,
        TS_PID          = 1,            // process id in %05d format
        TS_MONTHDAY     = 2,            // MM-DD
        TS_YEAR         = 4,            // Year in %04d format (only if & MONTHDAY too)
        TS_HOURMIN      = 8,            // HH:MM
        TS_SEC          = 16,           // :SS (only if & HOURMIN too)
        TS_MSEC         = 32,           // :ssss - Millesecond in %04d format (only if & SEC too)
        TS_PROGNAME     = 64,           // name of process (from __progname)
    };

    enum OutputType
    {
        OF_NONE         = 0,
        OF_STDOUT       = 1,
        OF_STDERR       = 2,
        OF_FILE         = 4,
        OF_SYSLOG       = 8,            // syslog to facility LOG_USER, level LOG_DEBUG
        OF_HWND         = 16,           // send to debug HWND (only applicable in MfcClient win32 app)
        OF_DEBUGGER     = 32,           // OutputDebugString() in win32 (win32's version of stderr)
    };

    struct LogData
    {
        bool fTraceFunction;            // Defaults false, if true TraceMarker() calls will also log pszFunction (otherwise just pszFile & nLine)
        int nOutputMasks[MAX_LOGLEVEL]; // Output types used when logging (i.e. file, stdout, syslog, etc) for each LogLevel
        int nStampMask;                 // Timestamp format written to log (i.e. pid, year, month & day, etc)
        size_t nAutoRotateSz;           // If >0, stat file every Mesg() (at most once every 5 seconds) and unlink if file is > than rotate sz
        time_t nAutoRotateTm;           // Last time size of output fds were checked for auto-rotate threshold

        // File output (OF_FILE) related data members
        string sLogDir;                             // What folder by default logs are written to
        string sLogFiles[MAX_LOGCLASS];             // Filenames for each LogClass
        int nLogFds[MAX_LOGCLASS];                  // File handles for each LogClass
        struct timeval tvLastOpen[MAX_LOGCLASS];    // gettimeofday() for the last time nLogFds[] descriptor was opened
    };


    //-- [ Interface Methods ]--------------------------------------------
    //
    virtual void Setup(const string& sLogDir) = 0;          // Set defaults on this instance's m_Data

    virtual void Mesg(const char* pszFmt, ...) = 0;         // wraps Mesg(), Default to LogLevel NOTICE
    virtual void Debug(const char* pszFmt, ...) = 0;        // wraps Mesg(), Default to LogLevel DEBUG
    virtual void Trace(const char* pszFmt, ...) = 0;        // wraps Mesg(), Default to LogLevel TRACE

    virtual void Mesg(LogLevel nLevel, const char* pszFmt, ...) = 0;    // Specify a specific LogLevel

    virtual void Flush(void) = 0;                           // Close (and flush pending writes) to any open log fds

    // Set log filename for given class.  Pass true on fUnlink to remove file before using.
    virtual void SetLog(LogClass nClass, const char* pszFile, bool fUnlink = false) = 0;

    // Get log filename for given class.
    virtual bool GetLog(LogClass nClass, string& sFile) = 0;
    virtual size_t DeleteLogs(LogClass nClass = MAX_LOGCLASS) = 0;

    // Accessor methods for making it easier to update LogData values
    virtual void SetStampMask(int nValue) = 0;
    virtual void AddStampMask(int nValue) = 0;
    virtual void SetAutoRotate(size_t nSz) = 0;

    virtual void SetOutputMask(LogLevel nLevel, int nValue) = 0;
    virtual void AddOutputMask(LogLevel nLevel, int nValue) = 0;
    virtual void SubOutputMask(LogLevel nLevel, int nValue) = 0;
    virtual bool HasOutputMask(LogLevel nLevel, int nValue) = 0;
    virtual int  GetOutputMask(LogLevel nLevel) = 0;

    // Wrapper for _Mesg that also accepts file, function, and line number from LogTraceMarker macro
    virtual void TraceMarker(const char* pszFile, const char* pszFunction, int nLine, LogLevel nLevel, const char* pszFmt, ...) = 0;

    // Returns LogData struct of log instance
    virtual struct LogData* Data(void) = 0;
    //
    // -------------------------------------------------------------------
};

#endif // ILOG_H

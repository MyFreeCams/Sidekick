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


#ifdef _WIN32
#include <stdio.h>
#include <direct.h>
#endif

#ifdef snprintf
#undef snprintf
#endif

#include "Log.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

namespace MFCIPC
{
class IPCUtil  // should this just be a namespace?
{
public:

    //---------------------------------------------------------------------
    // setupLogPath
    //
    // setup the log path.
    //
    // todo: we need a debug and release version!
    static void setupLogPath(bool  = false)
    {

#ifdef _WIN32
        // boost::filesystem crashes on windows.
        char buf[_MAX_PATH] = { '\0' };
        _getcwd(buf, _MAX_PATH);
        std::string sPath = buf;
        if (!sPath.empty()
            && sPath.back() != '/'
            && sPath.back() != '\\')
        {
            sPath.append("\\");
        }
        sPath += "../../Logs";
        Log::AddOutputMask(ILog::LogLevel::MAX_LOGLEVEL, 10);
        Log::Setup(sPath);
#else
        boost::filesystem::path pathLog = boost::filesystem::current_path();
        pathLog /= "../../Logs";

        if (! boost::filesystem::exists(pathLog))
            boost::filesystem::create_directories(pathLog);

        Log::AddOutputMask(ILog::LogLevel::MAX_LOGLEVEL, 10);
        std::string sP = pathLog.string();
        Log::Setup( sP );
#endif


    }

    //---------------------------------------------------------------------
    // nowPlusSeconds
    //
    // create time object that is now plus N Seconds.,
    static boost::posix_time::ptime NowPlusSeconds(int )
    {
        boost::posix_time::time_duration td = boost::posix_time::seconds(360);
        boost::posix_time::ptime tm = IPCUtil::Now();
        tm += td;
        return tm;
    }

    //---------------------------------------------------------------------
    // Now
    //
    // return boost time object with current time.
    static boost::posix_time::ptime Now()
    {
        typedef boost::posix_time::ptime system_time;
        return boost::date_time::second_clock<system_time>::local_time();
        //o ::microsec_clock<system_time>::local_time());
    }

    //---------------------------------------------------------------------
    // isEqual
    //
    // use boost to determine if 2 strings are equal.
    static bool isEqual(std::string &s, std::string &s2);

};


}

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


#include "EvtDefines.h"
//#include <string>
#include <map>

#include <boost/thread/thread.hpp>

namespace MFCIPC
{
boost::mutex g_mutexMap;

//
// convert event type to names for logging.
static std::map<int,std::string>  g_mapTypeNames =
        {
                {  EVT_NOT_IN_USE, std::string("empty")}
                ,{ EVT_GENERIC, std::string("Generic")}
                ,{ EVT_TIMED, std::string("Timed")}
                ,{ EVT_POST_IT, std::string("Postit")}
            ,{EVT_SYSTEM, std::string("System") }
            ,{EVT_MAINTENANCE_START, std::string("Maint Start")}
            ,{EVT_MAINTENANCE_END, std::string("Maint End")}
            ,{EVT_MAINTENANCE_STATUS, std::string("Shmem Status")}
            ,{EVT_POST_IT, std::string("Post it") }
            ,{MSG_TYPE_PING, std::string("Ping") }
            ,{MSG_TYPE_START, std::string("Start") }
            ,{MSG_TYPE_SHUTDOWN, std::string("Shutdown") }
            ,{MSG_TYPE_DOLOGIN, std::string("DoLogin") }
            ,{MSG_TYPE_LOGIN_DENY, std::string("Login Deny") }
            ,{MSG_TYPE_LOGIN_AUTH, std::string("Login_auth") }
            ,{MSG_TYPE_SET_MSK, std::string("SetMsk") }
            ,{MSG_TYPE_DOCREDENTIALS, std::string("Credentials") }
            ,{MSG_TYPE_LOG, std::string("Log") }

        };

std::string getMessageTypeName(int n)
{
    boost::mutex::scoped_lock scoped_lock(g_mutexMap);
    
    std::string s("unk type");
    if (g_mapTypeNames.find(n) != g_mapTypeNames.end())
    {
        s = g_mapTypeNames[n];
    }
    return s;
}

}



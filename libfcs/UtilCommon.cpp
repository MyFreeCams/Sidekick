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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#ifndef _WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#endif
#include <errno.h>

#include <string>
#include <vector>
#include <sstream>

#include "fcslib_string.h"
#include "Compat.h"
#include "MfcJson.h"
#include "MfcTimer.h"
#include "Log.h"

// The initialization/definition of static shared global timeval cache
struct timeval      MfcTimer::sm_tvNow;
struct tm           MfcTimer::sm_ctNow;
/*set< ProfTimer* >   ProfTimer::sm_stActiveObjs;
size_t              ProfTimer::sm_nUnalignedOverlaps = 0;
size_t              ProfTimer::sm_nOverlaps = 0;*/

#ifndef _WIN32
// Sleep a number of milleseconds using nanosleep() instead of select() or usleep()
int SleepNs(int nNs)
{
    struct timespec tsTimeout,rem;
    struct timespec *in,*out,*tmp;

    tsTimeout.tv_sec = 0;
    tsTimeout.tv_nsec = nNs * 1000000;

    in = &tsTimeout;
    out = &rem;

    /* nanosleep() replaces usleep() because it is a) more precise, and b) It
     * can be written like below in the event that a signal() is hit and it has
     * to return EINTR but not sleep for the proper amount of time.  If this
     * happen the out timespec is written with the remaining time, and then
     * because of the wile() loop we nanosleep() for the remaining time.
     */
    errno=0;
    while(nanosleep(in,out)==-1)
    {
        if(errno==EINTR)
        {
            tmp = in;
            in = out;
            out = tmp;
            errno=0; // Reset error number
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

void SleepMs(int nMs)
{
    struct timeval tvTimeout;

    tvTimeout.tv_sec = 0;
    tvTimeout.tv_usec = nMs * 1000;
    select(256, NULL, NULL, NULL, &tvTimeout);
}


size_t Linux_GetMemUsage(int& nPercent)
{
    static size_t s_nTotalMemory = 0;
    size_t nMemory = 0;
    char szBuf[1024];
    FILE *f;

    if ((f = fopen(stdprintf("/proc/%u/status", getpid()).c_str(), "r")) != NULL)
    {
        while (!feof(f))
        {
            if (fgets(szBuf, sizeof(szBuf) - 1, f))
            {
                if (strncmp(szBuf, "VmRSS:\t", 7) == 0)
                {
                    sscanf(szBuf + 7, "%lu kB", &nMemory);
                    break;
                }
            }
        }
        fclose(f);
    }

    // Get total memory and calculate percent used
    if (s_nTotalMemory == 0)
    {
        if ((f = fopen("/proc/meminfo", "r")) != NULL)
        {
            while (!feof(f))
            {
                if (fgets(szBuf, sizeof(szBuf) - 1, f))
                {
                    if (strncmp(szBuf, "MemTotal:", 9) == 0)
                    {
                        sscanf(szBuf + 9, "%lu kB", &s_nTotalMemory);
                        break;
                    }
                }
            }
            fclose(f);
        }
    }

    if (s_nTotalMemory > 0)
        nPercent = (int)( ((double)nMemory * 100 ) / (double)s_nTotalMemory );
    else
        nPercent = 0;

    return (nMemory * 1024);
}

size_t GetLocalIPs(strVec &vIps)
{
    struct ifaddrs* pIntAddr = NULL;
    struct ifaddrs* pInt = NULL;

    vIps.clear();
    getifaddrs(&pIntAddr);

    for (pInt = pIntAddr; pInt != NULL; pInt = pInt->ifa_next)
    {
        if (pInt->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in* pAddr = (struct sockaddr_in *)pInt->ifa_addr;
            char szAddr[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &pAddr->sin_addr, szAddr, INET_ADDRSTRLEN);
            if (strcmp(szAddr, "127.0.0.1") != 0)
                vIps.push_back(szAddr);
        }
    }

    if (pIntAddr)
        freeifaddrs(pIntAddr);

    return vIps.size();
}
#else

void SleepMs(int nMs)
{
    int nB = 2;
//    struct timeval tvTimeout;
//    tvTimeout.tv_sec = 0;
//    tvTimeout.tv_usec = nMs * 1000;
//    select(256, NULL, NULL, NULL, &tvTimeout);
}

#endif

FcListSet NumSetOf(size_t nSz, ...)
{
    FcListSet st;
    va_list vaArgs;
    va_start(vaArgs, nSz);

    for (size_t n = 0; n < nSz; n++)
        st.insert( va_arg(vaArgs, uint32_t) );

    return st;
}

set< uint32_t > NumSetOf2(size_t nSz, ...)
{
    set< uint32_t > st;
    va_list vaArgs;
    va_start(vaArgs, nSz);

    for (size_t n = 0; n < nSz; n++)
        st.insert( va_arg(vaArgs, uint32_t) );

    return st;
}

bool getSqlRes(const MfcJsonObj& jsConfig, const string& sLabel, string& sUser, string& sPass, string& sDatabase, string& sHost)
{
    MfcJsonObj sql, res, servers;
    bool retVal = false;

    if (jsConfig.objectGetObject("sql", sql))
    {
        if (sql.objectGetObject(sLabel, res))
        {
            if (res.objectGetObject("servers", servers))
            {
                if (res.objectGetString("user", sUser))
                {
                    if (res.objectGetString("pass", sPass))
                    {
                        if (res.objectGetString("database", sDatabase))
                        {
                            if (servers.arrayLen() > 0)
                            {
                                size_t nIdx = rand() % servers.arrayLen();
                                MfcJsonObj* pName = servers.arrayAt(nIdx);
                                if (pName && pName->isString())
                                {
                                    sHost = pName->m_sVal;
                                    retVal = true;
                                }
                                else _MESG("Unable to receive index %u of servers, or is not a string from resource '%s' in sql config", nIdx, sLabel.c_str()); 
                            }
                            else _MESG("Unable to receive hostname, servers is not an array (or empty) in resource '%s' of sql config", sLabel.c_str());
                        }
                        else _MESG("Unable to retreive database from resource '%s' in sql config", sLabel.c_str()); 
                    }
                    else _MESG("Unable to retreive pass from resource '%s' in sql config", sLabel.c_str()); 
                }
                else _MESG("Unable to retreive user from resource '%s' in sql config", sLabel.c_str()); 
            }
            else _MESG("Unable to retreive servers array from resource '%s' in sql config", sLabel.c_str());
        }
        else _MESG("Unable to retreive resource '%s' in sql config", sLabel.c_str());
    }
    else _MESG("Unable to retreive sql data in jsConfig obj");

    return retVal;
}

bool getRedisRes(const MfcJsonObj& jsConfig, const string& sLabel, string& sHost)
{
    MfcJsonObj redis, label; 
    bool retVal = false;

    if (jsConfig.objectGetObject("redis", redis))
    {
        if (redis.objectGetObject(sLabel, label))
        {
            // If it's a string, there's only one resouce, this string is it
            if (label.isString())
            {
                sHost = label.m_sVal;
                retVal = true;
            }

            // If it's an array, pick a random array index to use as our resource for this label
            else if (label.isArray())
            {
                strVec vAddrs;
                if (label.arrayRead(vAddrs) > 0)
                {
                    sHost = vAddrs[ rand() % vAddrs.size() ];
                    retVal = true;
                }
            }
            else _MESG("Unable to retreive resource '%s' in redis config, neither string or array of strings: %s", sLabel.c_str(), jsConfig.prettySerialize().c_str());
        }
        else _MESG("Unable to retrieve resource '%s' in redis config: %s", sLabel.c_str(), jsConfig.prettySerialize().c_str());
    }
    else _MESG("Unable to retrieve redis data in jsConfig obj");

    return retVal;
}


#ifdef _WIN32
int getpid(void)
{
	return (int)GetCurrentProcessId();
}
#endif



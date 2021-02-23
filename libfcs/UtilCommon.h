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

#include "Compat.h"
#include "MfcJson.h"


//
// UtilCommon.cpp prototypes
//

// a htonll/ntohll implementation customized for int64_t's
static inline int64_t hton64(int64_t value)
{
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        // Convert from little to big endian (swap)
        const uint32_t high_part = htonl(static_cast< uint32_t >(value >> 32));
        const uint32_t low_part = htonl(static_cast< uint32_t >(value & 0xFFFFFFFFLL));
        return (int64_t)((static_cast< uint64_t >(low_part) << 32) | high_part);
    #else
        return value;
    #endif
}


static inline int64_t ntoh64(int64_t value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // Convert from big to little endian (swap)
    const uint32_t high_part = htonl(static_cast< uint32_t >(value >> 32));
    const uint32_t low_part = htonl(static_cast< uint32_t >(value & 0xFFFFFFFFLL));
    return (int64_t)((static_cast< uint64_t >(low_part) << 32) | high_part);
#else
    return value;
#endif
}


#ifndef _WIN32
int SleepNs(int nNs);
#endif
void SleepMs(int nMs);
char* BpsFormat(char *psz, int nSz, size_t nBytes);
size_t Linux_GetMemUsage(int& nPercent);
size_t GetLocalIPs(strVec &vIps);
bool getSqlRes(const MfcJsonObj& jsConfig, const string& sLabel, string& sUser, string& sPass, string& sDatabase, string& sHost);

// Will pick a random redis resource out of array for specific label, save to sHost
bool getRedisRes(const MfcJsonObj& jsConfig, const string& sLabel, string& sHost);

// Builds a vector of uint32_ts without requiring C++0x
FcListSet NumSetOf(size_t nSz, ...);            // unordered_set< >
set< uint32_t > NumSetOf2(size_t nSz, ...);     // set< >

#ifdef _WIN32
int gettimeofday(struct timeval * tp, struct timezone * tzp);  // gettimeofday.cpp
#endif

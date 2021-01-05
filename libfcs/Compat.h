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

#include <sys/types.h>

#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <unordered_set>

using std::vector;
using std::string;
using std::map;
using std::set;
using std::unordered_set;
using std::pair;

#include <stdio.h>

typedef vector< string > strVec;
typedef vector< vector< string > > strVecVec;
typedef map< string,string > strMap;



// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#ifdef _WIN32
#define ZERO64                      0ULL
#define ONE64                       1ULL
#else
#define ZERO64                      0UL
#define ONE64                       1UL
#endif

// Platform specific printf modifiers
#ifndef PRId64
#define PRId64 "I64"
#endif
#ifndef PRIu64
#define PRIu64 "I64u"
#endif
#ifndef PRIu32
#define PRIu32 "u"
#endif

#ifndef _WIN32

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <syslog.h>
#include <unistd.h>

#ifndef BYTE
typedef uint8_t             BYTE;
#endif

// For 64bit linux/gcc
#define INT64_FMT   "ld"

#else       // _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include <In6addr.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif



using std::wstring;

#define strdup              _strdup

#define snprintf(s, ...)    _snprintf_s(s, _TRUNCATE, __VA_ARGS__)

#ifdef ENVIRONMENT64
#define INT64_FMT   "ld"        // 64bit windows
#else
#define INT64_FMT   "lld"       // 32bit windows
#endif

typedef vector< wstring > wstrVec;

#ifndef in_addr_t
typedef uint32_t	in_addr_t;
#endif

#endif       // _WIN32

#ifndef MAKE_I16
#define MAKE_I16(a, b)      ((uint16_t)(((BYTE)(a)) | ((uint16_t)((BYTE)(b))) << 8))
#endif

#ifndef MAKE_I32
#define MAKE_I32(a, b)      ((uint32_t)(((uint16_t)(a)) | ((uint32_t)((uint16_t)(b))) << 16))
#endif

#ifndef MAKE_I64
#define MAKE_I64(a, b)      ((uint64_t)(((uint32_t)(a)) | ((uint64_t)((uint32_t)(b))) << 32))
#endif

#ifndef HI_I32
#define HI_I32(dw)          ((uint32_t)(((uint64_t)(dw) >> 32) & 0xFFFFFFFF))
#endif

#ifndef HI_I16
#define HI_I16(w)           ((uint16_t)(((uint32_t)(w) >> 16) & 0xFFFF))
#endif

#ifndef HIBYTE
#define HIBYTE(ch)          ((BYTE)(((uint16_t)(ch) >> 8) & 0xFF))
#endif

#ifndef LO_I32
#define LO_I32(dw)          ((uint32_t)(dw))
#endif

#ifndef LO_I16
#define LO_I16(w)           ((uint16_t)(w))
#endif

#ifdef __APPLE__
#undef INT64_FMT
#define INT64_FMT "llu"
int semtimedop(int, struct sembuf*, int, struct timespec*);
#endif

typedef unordered_set< uint32_t >               FcListSet;
typedef unordered_set< uint32_t >::iterator     FcListSetIter;


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

//
//  Portable.h
//  libfcs
//
//  Created by Todd Anderson on 1/29/19.
//
#pragma once

#ifndef __Portable_h__
#define __Portable_h__

#ifdef _WIN32

#ifdef _WIN64
#define BIT_STRING "64bit"
#else
#define BIT_STRING "32bit"
#endif

#else  // #ifdef _WIN32

#ifndef APPLE
#define APPLE
#endif

#if defined(__cplusplus)
#include <cassert>
#else
#include <assert.h>
#endif

#ifdef _DEBUG
#define _ASSERT(exp) assert(exp)
#define _ASSERT_EXPR(exp,msg) assert(exp)
#else
#define _ASSERT(exp)
#define _ASSERT_EXPR(expr,msg)
#endif

#define S_OK        0L
#define S_FALSE     1L

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(p) (p)
#endif

#ifndef DBG_UNREFERENCED_LOCAL_VARIABLE
#define DBG_UNREFERENCED_LOCAL_VARIABLE(p) (p)
#endif

//#define _close close
//#define _write write
//#define _unlink unlink
#define _getpid getpid

typedef unsigned int DWORD;
typedef unsigned char BYTE;

typedef void * HANDLE;

#define sprintf_s   sprintf

#define _MAX_PATH   256
#define MAX_PATH   256

#endif  // #ifdef _WIN32

#endif // __Portable_h__

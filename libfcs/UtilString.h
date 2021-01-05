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

#ifndef UTIL_STRING_H_
#define UTIL_STRING_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include <set>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "Compat.h"

char* BpsFormat(char* psz, int nSz, size_t nBytes);
const char* TextDigest(const std::vector<uint8_t>& vDigest, std::string& sText);

/**
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * @returns Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t fcs_strlcpy(char* dst, const char* src, size_t siz);
size_t fcs_strlcpy_range(char chStart, char chStop, char* dst, const char* src, size_t siz);
size_t fcs_strlcat(char* dst, const char* src, size_t siz);

bool strEndsWith(const std::string& full, const std::string& part);

/**
 * builds a "IN(101,102,325,...)" string from uint32_t vector, at most up to 1000 elements.
 * Typically used in a loop where vIds may be larger:
 * @example
 * // Batch delete friends_lists
 * std::string sTmp;
 * while (vIds.size() > 0)
 *     sql.Query("DELETE FROM some_list WHERE user_id = %u AND friend_id %s", whereIn(vIds, sTmp));
 */ 
const char* whereIn(std::vector<uint32_t>& vIds, std::string& sBuf);
const char* whereIn(std::set<uint32_t>& stIds, std::string& sBuf);

size_t stdprintf(std::string& sBuf, const char* pszFmt, ...);
std::string stdprintf(const char* pszFmt, ...);

const char* stderror(std::string& sBuf, int nErr);
std::string stderror(int nErr);

size_t stdsplit(const std::string& s, char delim, std::vector<std::string>& v);
std::vector<std::string> stdsplit(const std::string& s, char delim);

size_t stdGetFileContents(const std::string& sFilename, std::string& sData);
bool stdSetFileContents(const std::string& sFilename, std::string& sData);
size_t stdGetFileContents(const std::string& sFilename, std::vector<uint8_t>& vData);

std::vector<std::string> strVecOf(size_t nSz, ...);
bool strVecHas(const std::vector<std::string>& vList, const std::string& sItem);

#ifdef _WIN32
std::vector<std::wstring> wstrVecOf(size_t nSz, ...);

std::wstring wstdprintf(const WCHAR* pwszFmt, ...);
size_t wstdprintf(std::wstring& wstr, const WCHAR* pwszFmt, ...);

size_t stdGetWindowText(HWND hWnd, std::string& sText);
std::string stdGetWindowText(HWND hWnd);

size_t stdWideToMulti(std::string& sOut, WCHAR* pwszIn);
std::string stdWideToMulti(WCHAR* pwszIn);

std::string getWin32Error(DWORD dwErr);
#endif  // _WIN32

std::string& stdLeftTrim(std::string& sLine);

#endif  // UTIL_STRING_H_

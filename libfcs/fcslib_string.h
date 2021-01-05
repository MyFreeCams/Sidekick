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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string>
#include <vector>
#include <set>
#include <map>

#include "Compat.h"

//--- UtilString.cpp prototypes --------------------------------------

// Formats binary data to hexadecimal string, returns sText.c_str()
const char* TextDigest(const vector< BYTE >& vDigest, string& sText);

string stdprintf(const char *pszFmt, ...);                          // A snprintf() that returns a string
size_t stdprintf(string& sBuf, const char *pszFmt, ...);            // A stdprintf() that operates on string reference instead of returning copy of local string
strVec strVecOf(size_t nSz, ...);                                   // returns a vector<string> made up of char*'s passed into va args

const char* stderror(string& sBuf, int nErr);                       // Formats sBuf to "Error_Message (ErrorNumber)" via strerror() and returns sBuf.c_str()
string stderror(int nErr);                                          // Returns a string formatted to "Error_Message (ErrorNumber)" via strerror()
bool strEndsWith(const string& full, const string& part);           // returns true if 'full' string ends with 'part' string
const char* whereIn(vector< uint32_t >& vIds, string& sBuf);        // Builds a " IN (%u,%u,%u...) " string from vIds, removes
                                                                    // a uint32_t from vector from back but adds at most 1000 ids at a time.
                                                                    // returns sBuf.c_str().  Call in loop to build multiple where id in()
                                                                    // queries if more than 1000 id's are needed.
const char* whereIn(set< uint32_t >& vIds, string& sBuf);           // Same as vector<> version to avoid client needing to convert set -> vector to run

#ifdef _WIN32
wstring wstdprintf(const WCHAR* pwszFmt, ...);                       // Unicode version of strprintf
size_t wstdprintf(wstring& wstr, const WCHAR* pwszFmt, ...);
wstrVec wstrVecOf(size_t nSz, ...);                                 // Unicode version of strVecOf
string stdGetWindowText(HWND hWnd);
size_t stdGetWindowText(HWND hWnd, string& sInput);
size_t stdWideToMulti(string& sOut, WCHAR* pwszIn);
string stdWideToMulti(WCHAR* pwszIn);
string getWin32Error(DWORD dwErr);
//void OutputConsoleString(const string& sMsg);						// Special output function for sending to debug HWND console
#endif

bool strVecHas(const strVec& vList, const string& sItem);           // Checks a strVec to see if it contains a given string
strVec stdsplit(const string& s, char delim);                       // std string replacements for strtok()
size_t stdsplit(const string& s, char delim, strVec& v);

size_t stdGetFileContents(const string& sFilename, string& sData);  // Get contents of text file into a std string, return size written to sData string
size_t stdGetFileContents(const string& sFilename, vector< BYTE >& vData);  // Different implementation that returns contents of file in a vector of bytes

bool stdSetFileContents(const string& sFilename, string& sData);

size_t fcs_strlcpy(char *dst, const char *src, size_t siz);         // portable strlcpy()
size_t fcs_strlcat(char *dst, const char *src, size_t siz);         // portable strlcat()

// Like fcs_strlcpy, but only copies values from src that are >= chStart and <= chEnd, and returns number of bytes copied, not strlen(src)
size_t fcs_strlcpy_range(char chStart, char chStop, char *dst, const char *src, size_t siz);

std::string &stdLeftTrim(std::string &s);		// left trim

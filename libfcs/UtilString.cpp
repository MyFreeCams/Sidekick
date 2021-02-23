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

#include "UtilString.h"

#include <fstream>
#include <sstream>

using namespace std;
typedef vector<string> strVec;
typedef vector<wstring> wstrVec;

char* BpsFormat(char* psz, int nSz, size_t nBytes)
{
    if (psz && nSz > 4)
    {
        if      (nBytes > 1024UL*1024*1024) snprintf(psz, nSz, "%.1fGb", (double)nBytes / (1024*1024*1024)); // Gigs
        else if (nBytes > 1024UL*1024)      snprintf(psz, nSz, "%.1fMb", (double)nBytes / (1024*1024));      // Megs
        else if (nBytes > 1024UL)           snprintf(psz, nSz, "%.0fKb", (double)nBytes / 1024);             // Kilos
        else                                snprintf(psz, nSz, "%ub",    (unsigned int)nBytes);              // just bytes (or bits)
    }

    return psz;
}


const char* TextDigest(const vector<uint8_t>& vDigest, string& sText)
{
    char szBuf[8] = { '\0' };

    for (size_t n = 0; n < vDigest.size(); n++)
    {
        snprintf(szBuf, sizeof(szBuf), "%02x", vDigest[n]);
        sText += szBuf;
    }

    return sText.c_str();
}


// Copy src to string dst of size siz.  At most siz-1 characters
// will be copied.  Always NUL terminates (unless siz == 0).
// Returns strlen(src); if retval >= siz, truncation occurred.
size_t fcs_strlcpy(char* dst, const char* src, size_t siz)
{
    size_t nCopied = 0;

    if (dst && src && siz > 0)
    {
        char* d = dst;
        const char* s = src;
        size_t n = siz;

        // Copy as many bytes as will fit
        if (n != 0)
        {
            while (--n != 0)
            {
                if ((*d++ = *s++) == '\0')
                    break;
            }
        }

        // Not enough room in dst, add NUL and traverse rest of src
        if (n == 0)
        {
            if (siz != 0)
                *d = '\0';        /* NUL-terminate dst */
            while (*s++)
                ;
        }

        nCopied = (s - src - 1);       /* count does not include NUL */
    }

    return nCopied;
}


size_t fcs_strlcpy_range(char chStart, char chStop, char* dst, const char* src, size_t siz)
{
    size_t nCopy = 0;

    if (dst && src && siz > 0)
    {
        char* d = dst;
        const char* s = src;
        size_t n = siz;

        // Copy as many bytes as will fit
        if (n != 0)
        {
            while (--n != 0)
            {
                if ((*d = *s) == '\0')
                {
                    d++;
                    s++;
                    break;
                }
                else
                {
                    // Only move copy forward if its a valid character
                    if (*s >= chStart && *s <= chStop)
                    {
                        d++;
                        nCopy++;
                    }
                    s++;
                }
            }

            // Not enough room in dst, add NUL
            if (n == 0)
                *d = '\0';        // NUL-terminate dst
        }
    }

    return nCopy;
}


size_t fcs_strlcat(char* dst, const char* src, size_t siz)
{
    char*       d = dst;
    const char* s = src;
    size_t      n = siz;
    size_t      dlen;

    // Find the end of dst and adjust bytes left but don't go past end
    while (n-- != 0 && *d != '\0')
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return (dlen + strlen(s));
    while (*s != '\0')
    {
        if (n != 1)
        {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (dlen + (s - src));  // count does not include NUL
}


bool strEndsWith(const string& full, const string& part)
{
    if (full.length() >= part.length())
        return (0 == full.compare (full.length() - part.length(), part.length(), part));

    return false;
}


// builds a "IN(101,102,325,...)" string from uint32_t vector, at most up to 1000 elements.
// Typically used in a loop where vIds may be larger:
//
// // Batch delete friends_lists
// string sTmp;
// while (vIds.size() > 0)
//    sql.Query("DELETE FROM some_list WHERE user_id = %u AND friend_id %s", whereIn(vIds, sTmp));
//
const char* whereIn(vector<uint32_t>& vIds, string& sBuf)
{
    size_t nIds = 0;
    char szTmp[32];

    sBuf = " IN(";
    while (nIds < 1000 && vIds.size() > 0)
    {
        // Prefix with comma if this is not the first id added
        snprintf(szTmp, sizeof(szTmp), "%c%u", nIds > 0 ? ',' : ' ', vIds.back());
        sBuf += szTmp;
        vIds.pop_back();
        nIds++;
    }
    sBuf += ") ";
    return sBuf.c_str();
}


const char* whereIn(set<uint32_t>& stIds, string& sBuf)
{
    size_t nIds = 0;
    char szTmp[32];

    sBuf = " IN(";
    while (nIds < 1000 && !stIds.empty())
    {
        uint32_t dwVal = *(stIds.begin());

        // Prefix with comma if this is not the first id added
        snprintf(szTmp, sizeof(szTmp), "%c%u", nIds > 0 ? ',' : ' ', dwVal);
        sBuf += szTmp;
        stIds.erase( dwVal );
        nIds++;
    }
    sBuf += ") ";
    return sBuf.c_str();
}


size_t stdprintf(string& sBuf, const char* pszFmt, ...)
{
    size_t nSz = 256;
    va_list vaList;

    sBuf.clear();

    for (;;)
    {
        sBuf.resize(nSz);
        va_start(vaList, pszFmt);

#ifdef _WIN32
        int n = vsnprintf_s((char*)sBuf.c_str(), nSz, _TRUNCATE, pszFmt, vaList);
#else
        int n = vsnprintf((char*)sBuf.c_str(), nSz, pszFmt, vaList);
#endif
        va_end(vaList);

        if (n > -1 && n < (int)nSz)
        {
            sBuf.resize(n);
            break;
        }

        if (n > -1)
            nSz = n + 1;
        else
            nSz *= 2;
    }

    return sBuf.size();
}


string stdprintf(const char* pszFmt, ...)
{
    size_t nSz = 256;
    string str;
    va_list vaList;

    for (;;)
    {
        str.resize(nSz);
        va_start(vaList, pszFmt);

#ifdef _WIN32
        int n = vsnprintf_s((char*)str.c_str(), nSz, _TRUNCATE, pszFmt, vaList);
#else
        int n = vsnprintf((char*)str.c_str(), nSz, pszFmt, vaList);
#endif
        va_end(vaList);

        if (n > -1 && n < (int)nSz)
        {
            str.resize(n);
            break;
        }

        if (n > -1)
            nSz = n + 1;
        else
            nSz *= 2;
    }

    return str;
}


const char* stderror(string& sBuf, int nErr)
{
    sBuf.clear();
#ifdef _WIN32
    char szErrMsg[512];
    if (strerror_s(szErrMsg, nErr) != 0)
        strcpy_s(szErrMsg, "<UnknownErrCode>");

    stdprintf(sBuf, "%s (%d)", szErrMsg, nErr);
#else
    stdprintf(sBuf, "%s (%d)", strerror(nErr), nErr);
#endif
    return sBuf.c_str();
}


string stderror(int nErr)
{
    string sBuf;
    stderror(sBuf, nErr);
    return sBuf;
}


size_t stdsplit(const string& s, char delim, strVec& v)
{
    stringstream ss(s);
    string item;

    while (getline(ss, item, delim))
        v.push_back(item);

    return v.size();
}


strVec stdsplit(const string& s, char delim)
{
    strVec v;
    stdsplit(s, delim, v);
    return v;
}


size_t stdGetFileContents(const string& sFilename, string& sData)
{
    ifstream streamIn(sFilename.c_str(), ios::in | ios::binary);
    sData.clear();

    if (streamIn)
    {
        streamIn.seekg(0, ios::end);
        sData.resize((unsigned int)streamIn.tellg());
        streamIn.seekg(0, ios::beg);
        streamIn.read(&sData[0], sData.size());
        streamIn.close();
    }

    return sData.size();
}


bool stdSetFileContents(const string& sFilename, string& sData)
{
    FILE* pFile;
    ofstream streamOut(sFilename.c_str(), ios::out | ios::binary);
    if (streamOut)
    {
        streamOut << sData;
        streamOut.close();
        return true;
    }
    return false;
}


size_t stdGetFileContents(const string& sFilename, vector<uint8_t>& vData)
{
    ifstream ifs(sFilename.c_str(), ios::binary);   // Open file
    streampos nFileSz;                              // File sz var
    vData.clear();                                  // Clear old data

    if ( (ifs.rdstate() & std::ifstream::failbit ) == 0 )
    {
        ifs.seekg(0, ios::end);                         // Set to EOF
        nFileSz = ifs.tellg();                          // Get size
        ifs.seekg(0, ios::beg);                         // Reset to beginning
        vData.resize((unsigned int)nFileSz);            // Reserve filesize for vector

        ifs.read((char*)&vData[0], nFileSz);            // Read contents of file
        if (!ifs)
        {
            //_MESG("Read err: only %u of %u could be read", (uint32_t)ifs.gcount(), (uint32_t)nFileSz);
            vData.clear();
        }
    }
    //else _MESG("Unable to open %s!", sFilename.c_str());

    return vData.size();                            // Return # bytes read
}


strVec strVecOf(size_t nSz, ...)
{
    strVec vec;
    va_list vaArgs;
    va_start(vaArgs, nSz);

    for (size_t n = 0; n < nSz; n++)
        vec.push_back(string(va_arg(vaArgs, char*)));

    return vec;
}


bool strVecHas(const strVec& vList, const string& sItem)
{
    for (size_t n = 0; n < vList.size(); n++)
    {
        if (vList[n] == sItem)
            return true;
    }

    return false;
}


#ifdef _WIN32
wstrVec wstrVecOf(size_t nSz, ...)
{
    wstrVec vec;
    va_list vaArgs;
    va_start(vaArgs, nSz);

    for (size_t n = 0; n < nSz; n++)
        vec.push_back(wstring(va_arg(vaArgs, WCHAR*)));

    return vec;
}


wstring wstdprintf(const WCHAR* pwszFmt, ...)
{
    size_t nSz = 256;
    wstring wstr;
    va_list vaList;

    for (;;)
    {
        wstr.resize(nSz);
        va_start(vaList, pwszFmt);

        int n = _vsnwprintf_s((WCHAR*)wstr.c_str(), nSz, _TRUNCATE, pwszFmt, vaList);

        va_end(vaList);

        if (n > -1 && n < (int)nSz)
        {
            wstr.resize(n);
            break;
        }

        if (n > -1)
            nSz = n + 1;
        else
            nSz *= 2;
    }

    return wstr;
}


size_t wstdprintf(wstring& wstr, const WCHAR* pwszFmt, ...)
{
    size_t nSz = 256;
    va_list vaList;

    wstr.clear();

    for (;;)
    {
        wstr.resize(nSz);
        va_start(vaList, pwszFmt);

        int n = _vsnwprintf_s((WCHAR*)wstr.c_str(), nSz, _TRUNCATE, pwszFmt, vaList);

        va_end(vaList);

        if (n > -1 && n < (int)nSz)
        {
            wstr.resize(n);
            break;
        }

        if (n > -1)
            nSz = n + 1;
        else
            nSz *= 2;
    }

    return wstr.size();
}


size_t stdGetWindowText(HWND hWnd, string& sText)
{
    size_t nSz = GetWindowTextLength(hWnd);
    sText.clear();

    if (nSz > 0)
    {
        vector<char> vData(nSz+1);
        GetWindowTextA(hWnd, &vData[0], nSz+1);
        sText = string(vData.begin(), vData.end());
    }

    return sText.size();
}


string stdGetWindowText(HWND hWnd)
{
    string s;
    stdGetWindowText(hWnd, s);
    return s;
}


size_t stdWideToMulti(string& sOut, WCHAR* pwszIn)
{
    char szBuf[32768];

    sOut.clear();
    if (pwszIn)
    {
        int nSz = WideCharToMultiByte(CP_ACP, 0, pwszIn, -1, szBuf, sizeof(szBuf) - 2, NULL, NULL);
        if (nSz >= 0 && nSz < sizeof(szBuf))
        {
            szBuf[nSz] = '\0';
            sOut = szBuf;
        }
    }

    return sOut.size();
}


string stdWideToMulti(WCHAR* pwszIn)
{
    string sOut;
    stdWideToMulti(sOut, pwszIn);
    return sOut;
}


string getWin32Error(DWORD dwErr)
{
    LPVOID lpMsgBuf;
    string sErrMsg;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwErr,
        0,                  // Default language
        (LPSTR)&lpMsgBuf,   // Write to this pointer
        0,
        NULL);

    // Trim CR/LF from end of string
    char* pch = (char*)lpMsgBuf;
    size_t nLen;
    while (((nLen = strlen(pch)) > 0) && (pch[nLen - 1] == '\n' || pch[nLen - 1] == '\r'))
    {
        pch[nLen - 1] = '\0';
    }

    // Format to std::string
    stdprintf(sErrMsg, "%s (%u)", pch, dwErr);
    LocalFree(lpMsgBuf);

    return sErrMsg;
}
#endif


string& stdLeftTrim(string& sLine)
{
#ifdef _WIN32
    sLine.erase(std::find_if(sLine.rbegin(), sLine.rend(),
                [](int ch) { return !isspace(ch); }).base(), sLine.end());
#else
    sLine.erase(std::find_if(sLine.rbegin(), sLine.rend(),
                [](int ch) { return !std::isspace(ch); }).base(), sLine.end());
#endif
    return sLine;
}

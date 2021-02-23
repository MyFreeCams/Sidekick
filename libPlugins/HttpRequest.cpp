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

// uncomment to enable libcurl debugging.
//#define _CURL_TRACE

// MFC includes
#include <libfcs/Log.h>

// project includes
#include "HttpRequest.h"

using std::string;


/// Helper class for the libCurl file download callback to manage
/// memory allocation of the HTTP download.
class CWriteBackBuffer
{
public:
    CWriteBackBuffer()
        : m_pBuf(nullptr), m_nSize(0)
    {}

    void append(uint8_t* p, size_t nSize)
    {
        if (nullptr == m_pBuf)
        {
            m_pBuf = static_cast<uint8_t*>(malloc(nSize + 1));
            if (m_pBuf != nullptr)
            {
                memset(m_pBuf, '\0', nSize + 1);
                memcpy(m_pBuf, p, nSize);
                m_nSize = nSize;
            }
        }
        else
        {
            m_pBuf = static_cast<uint8_t*>(realloc(m_pBuf, m_nSize + nSize));
            if (m_pBuf != nullptr)
            {
                memcpy(m_pBuf + m_nSize, p, nSize);
                m_nSize += nSize;
            }
        }
    }

    uint8_t* getBuffer() { return m_pBuf; }
    size_t getSize() { return m_nSize; }

private:
    uint8_t* m_pBuf;
    size_t m_nSize;
};


/// Callback function to alloc memory and get the downloaded data.
size_t write_data(void* buffer, size_t size, size_t nmemb, void* pUser)
{
    // seems weird????
    // https://curl.haxx.se/libcurl/c/getinmemory.html
    size_t realsize = size * nmemb;
    CWriteBackBuffer* pBuf = nullptr;
    pBuf = reinterpret_cast<CWriteBackBuffer*>(pUser);
    pBuf->append(reinterpret_cast<uint8_t*>(buffer), realsize);

    return realsize;
}


/// HTTP POST Request
/// @return pointer to response, nullptr on error
uint8_t* CCurlHttpRequest::Post(const string& sUrl, unsigned int* pSize, const string& sPayload,
                                PROGRESS_CALLBACK pfnProgress)
{
    CURL* curl;
    CURLcode res = CURLE_OK;
    CWriteBackBuffer buf;
    uint8_t* pResponse = nullptr;
    char pErrorBuffer[CURL_ERROR_SIZE + 1] = { '\0' };

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if (curl)
    {
        // Set the URL.
        curl_easy_setopt(curl, CURLOPT_URL, sUrl.c_str());

#ifdef _CURL_TRACE
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, libcurl_Trace);
        // The DEBUGFUNCTION has no effect until we enable VERBOSE.
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

        // Set the call back function.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

        // Set the payload.
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPayload.c_str());

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the write back buffer. This will be the last parameter on the callback.
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

        // Some servers don't like requests that are made without a user-agent field.
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.73.0");

        // Call back so we can stop the request if OBS exits.
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, pfnProgress);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, pErrorBuffer);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Perform the request, res will get the return code.
        res = curl_easy_perform(curl);
        // Check for errors.
        string sE;
        if (res != CURLE_OK)
        {
            // Both error messages are useful.
            sE = curl_easy_strerror(res);
            sE += "/";
            sE += pErrorBuffer;
            _TRACE("curl_easy_perform() failed: %s\n", sE.c_str());
            setResultString(sE.c_str());
            if (nullptr != buf.getBuffer())
                free(buf.getBuffer());
            *pSize = 0;
        }
        else
        {
            pResponse = buf.getBuffer();
            *pSize = (unsigned int)buf.getSize();
        }
        setResult(res);
        // Always cleanup.
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return pResponse;
}


/// HTTP GET request
/// @return pointer to response, nullptr on error
uint8_t* CCurlHttpRequest::Get(const string& sUrlBase, const string& sContentType, unsigned int* pSize,
                               const string& sPayload, PROGRESS_CALLBACK pfnProgress)
{
    CURL* curl;
    CURLcode res;
    CWriteBackBuffer buf;
    uint8_t* pResponse = nullptr;
    char pErrorBuffer[CURL_ERROR_SIZE + 1] = { '\0' };

    curl_global_init(CURL_GLOBAL_ALL);
    string sUrl(sUrlBase);

    curl = curl_easy_init();
    if (curl)
    {
        if (! sPayload.empty())
        {
            sUrl += "/?";
            sUrl += sPayload;
        }
        // Set the URL.
        curl_easy_setopt(curl, CURLOPT_URL, sUrl.c_str());

        // Set the callback function.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

        if (! sContentType.empty())
        {
            struct curl_slist* headers = nullptr;
            const string contentTypeHeader("Accept: " + sContentType);
            headers = curl_slist_append(headers, contentTypeHeader.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        // Set the write back buffer. This will be the last parameter on the callback.
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

        // Some servers don't like requests that are made without a user-agent field.
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.73.0");

        // Call back so we can stop the request if OBS exits.
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, pfnProgress);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, pErrorBuffer);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Perform the request, res will get the return code.
        res = curl_easy_perform(curl);
        // Check for errors.
        string sE;
        if (res != CURLE_OK)
        {
            // Both error messages are useful.
            sE = curl_easy_strerror(res);
            sE += "/";
            sE += pErrorBuffer;
            _TRACE("curl_easy_perform() failed: %s\n", sE.c_str());
            setResultString(sE.c_str());
            if (nullptr != buf.getBuffer())
                free(buf.getBuffer());
            if (nullptr != pSize)
                *pSize = 0;
        }
        else
        {
            pResponse = buf.getBuffer();
            if (nullptr != pSize)
                *pSize = (unsigned int)buf.getSize();
        }
        setResult(res);
        // Always cleanup.
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return pResponse;
}

/// HTTP GET request
/// @return pointer to response, nullptr on error
uint8_t* CCurlHttpRequest::Get(const string& sUrl, unsigned int* pSize, const string& sPayload, PROGRESS_CALLBACK pfnProgress)
{
    return Get(sUrl, "", pSize, sPayload, pfnProgress);
}

/// HTTP GET request
/// @return pointer to response, nullptr on error
uint8_t* CCurlHttpRequest::Get(const string& sUrl, const string& sContentType, unsigned int* pSize, PROGRESS_CALLBACK pfnProgress)
{
    return Get(sUrl, sContentType, pSize, "", pfnProgress);
}

/// HTTP GET request
/// @return pointer to response, nullptr on error
uint8_t* CCurlHttpRequest::Get(const string& sUrl, const string& sContentType, unsigned int* pSize)
{
    return Get(sUrl, sContentType, pSize, "", nullptr);
}

/// HTTP GET request
/// @return pointer to response, nullptr on error
uint8_t* CCurlHttpRequest::Get(const string& sUrl, unsigned int* pSize, PROGRESS_CALLBACK pfnProgress)
{
    return Get(sUrl, "", pSize, "", pfnProgress);
}

/// HTTP GET request
/// @return pointer to response, nullptr on error
uint8_t* CCurlHttpRequest::Get(const string& sUrl, unsigned int* pSize)
{
    return Get(sUrl, "", pSize, "", nullptr);
}

/// HTTP GET request
/// @return pointer to response, nullptr on error
uint8_t* CCurlHttpRequest::Get(const string& sUrl)
{
    return Get(sUrl, "", nullptr, "", nullptr);
}


/// Return the result code from the last HTTP request.
int CCurlHttpRequest::getResult() { return m_nResult; }
void CCurlHttpRequest::setResult(int n) { m_nResult = n; }


/// Get the English translation of the result code.
string CCurlHttpRequest::getResultString() { return m_sResult; }
void CCurlHttpRequest::setResultString(const char* p) { m_sResult = p; }


#ifdef _CURL_TRACE
/// Another debugging tool.
static void dump(const char* text, unsigned char* ptr, size_t size)
{
    size_t i;
    size_t c;
    unsigned int width = 0x10;

    _TRACE("%s, %10.10ld bytes (0x%8.8lx)\n", text, (long)size, (long)size);

    string sOut;
    for (i = 0; i < size; i += width) {
        _TRACE("%4.4lx: ", (long)i);

        // show hex to the left
        string sBuf;
        for (c = 0; c < width; c++) {
            if (i + c < size)
            {
                char buf[20] = { '\0' };
               sprintf_s(buf, "%02x ", ptr[i + c]);
               sBuf += buf;
            }
            else
            {
                sBuf += " ";
            }
        }
        _TRACE(sBuf.c_str());
        sBuf = "";
        // show data on the right
        for (c = 0; (c < width) && (i + c < size); c++) {
            char x = (ptr[i + c] >= 0x20 && ptr[i + c] < 0x80) ? ptr[i + c] : '.';
            sBuf += x;
        }
        _TRACE(sBuf.c_str());
    }
}
#endif


#ifdef _CURL_TRACE
/// Debugging tool.
static int libcurl_Trace(CURL* handle, curl_infotype type, char* data, size_t size, void* userp)
{
    const char* text;
    (void)handle; /* prevent compiler warning */
    (void)userp;

    switch (type)
    {
    case CURLINFO_TEXT:
        _TRACE("== Info: %s", data);
    default: /* in case a new one is introduced to shock us */
        return 0;

    case CURLINFO_HEADER_OUT:
        text = "=> Send header";
        break;
    case CURLINFO_DATA_OUT:
        text = "=> Send data";
        break;
    case CURLINFO_SSL_DATA_OUT:
        text = "=> Send SSL data";
        break;
    case CURLINFO_HEADER_IN:
        text = "<= Recv header";
        break;
    case CURLINFO_DATA_IN:
        text = "<= Recv data";
        break;
    case CURLINFO_SSL_DATA_IN:
        text = "<= Recv SSL data";
        break;
    }

    dump(text, (unsigned char*)data, size);
    return 0;
}
#endif

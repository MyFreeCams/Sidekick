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
#ifndef MFC_CHTTPREQUEST_H___
#define MFC_CHTTPREQUEST_H___

#include <curl/curl.h>

#include <string>

// curl_off_t is an _int64
typedef int (*PROGRESS_CALLBACK)(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                 curl_off_t ultotal, curl_off_t ulnow);


/// Base class for new libCurl HTTP object.
class CHttpRequest
{
public:
    virtual ~CHttpRequest() = default;

    /// HTTP POST request.
    virtual uint8_t* Post(const std::string& sUrl, unsigned int* pSize, const std::string& sPayload,
                          PROGRESS_CALLBACK pfnProgress) = 0;
    /// HTTP GET request.
    virtual uint8_t* Get(const std::string& sUrl, const std::string& sContentType, unsigned int* pSize,
                         const std::string& sPayload, PROGRESS_CALLBACK pfnProgress) = 0;


    /// Result code from the last HTTP request.
    virtual int getResult() = 0;
    virtual void setResult(int n) = 0;

    /// English translation of the last result code.
    virtual std::string getResultString() = 0;
    virtual void setResultString(const char* p) = 0;
};


/// libCurl implementation of the HTTP request.
class CCurlHttpRequest : public CHttpRequest
{
public:
    ~CCurlHttpRequest() override = default;

    /// HTTP POST request.
    uint8_t* Post(const std::string& sUrl, unsigned int* pSize, const std::string& sPayload,
                  PROGRESS_CALLBACK pfnProgress) override;

    /// HTTP GET request.
    uint8_t* Get(const std::string& sUrl, const std::string& sContentType, unsigned int* pSize,
                 const std::string& sPayload, PROGRESS_CALLBACK pfnProgress) override;

    uint8_t* Get(const std::string& sUrl, unsigned int* pSize, const std::string& sPayload, PROGRESS_CALLBACK pfnProgress);
    uint8_t* Get(const std::string& sUrlBase, const std::string& sContentType, unsigned int* pSize, PROGRESS_CALLBACK pfnProgress);
    uint8_t* Get(const std::string& sUrl, const std::string& sContentType, unsigned int* pSize);
    uint8_t* Get(const std::string& sUrl, unsigned int* pSize, PROGRESS_CALLBACK pfnProgress);
    uint8_t* Get(const std::string& sUrl, unsigned int* pSize);
    uint8_t* Get(const std::string& sUrl);

    /// Result code from the last HTTP request.
    int getResult() override;
    void setResult(int n) override;

    /// English translation of the last result code.
    std::string getResultString() override;
    void setResultString(const char* p) override;

private:
    int m_nResult = 0;
    std::string m_sResult;
};

#endif  // MFC_CHTTPREQUEST_H___

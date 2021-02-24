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

#ifndef CEF_EVENT_HANDLER_H_
#define CEF_EVENT_HANDLER_H_

#include "include/cef_client.h"

#include <list>
#include <string>

#ifdef _WIN32
#include "include/internal/cef_types.h"
#include "libfcs/MfcJson.h"

const char* _MapTid(void);
void fcs_strReplaceAll(std::string& source, const std::string& from, const std::string& to);
#endif

class cefEventHandler : public CefClient,
                        public CefDisplayHandler,
                        public CefLifeSpanHandler,
                        public CefLoadHandler
{
public:
    explicit cefEventHandler(bool use_views);
    ~cefEventHandler() OVERRIDE;

    static cefEventHandler* GetInstance();

    // CefClient methods:
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE { return this; }

    // CefDisplayHandler methods:
    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) OVERRIDE;

    // CefLifeSpanHandler methods:
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
    bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

    // CefLoadHandler methods:
    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     ErrorCode errorCode,
                     const CefString& errorText,
                     const CefString& failedUrl) OVERRIDE;

    // Request that all existing browser windows close.
    void CloseAllBrowsers(bool force_close);

    bool IsClosing() const { return is_closing_; }

    CefRefPtr<CefBrowser> getBrowser() { return m_pBrowser; }
    int getBrowserCnt() { return m_nBrowserCount; }
    int getBrowserID() { return m_nBrowserId; }

#ifndef _WIN32
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) OVERRIDE
    {
        return false;
    }
#else
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) OVERRIDE;

    static LRESULT CALLBACK FilterWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

    LRESULT stopDragWindow(void);
    LRESULT moveDragWindow(void);

    LRESULT stopResizeWindow(void);
    LRESULT moveResizeWindow(void);

    static cefEventHandler* sm_pThis;

    bool draggingWindow;
    bool resizingWindow;

    size_t m_nResizeWinMinWidth;
    size_t m_nResizeWinMinHeight;
    size_t m_nResizeWinMaxWidth;
    size_t m_nResizeWinMaxHeight;

    MfcJsonObj m_jsConfig;

    POINT m_ptStartDrag;
    WNDPROC m_fpOldWndProc;
    HWND m_hWndChild;
    HWND m_hWnd;
#endif

private:
    // Platform-specific implementation.
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);

    // True if the application is using the Views framework.
    const bool use_views_;

    CefRefPtr<CefBrowser> m_pBrowser;
    int m_nBrowserId;
    int m_nBrowserCount;

    // List of existing browser windows.Only accessed on the CEF UI thread.
    typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
    BrowserList m_lstBrowsers;

    bool is_closing_;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(cefEventHandler);
};

#endif  // CEF_EVENT_HANDLER_H_
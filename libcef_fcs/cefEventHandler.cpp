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
#include "cefEventHandler.h"

#include <sstream>
#include <string>

// cef include files
#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

#ifndef _WIN32
namespace {
cefEventHandler* g_pEvent = NULL;
}
#else
#include <list>
#include "include/cef_client.h"
#include "include/internal/cef_types.h"
#include <libfcs/Log.h>
#include <libfcs/MfcJson.h>
#include <libPlugins/IPCShared.h>

cefEventHandler* cefEventHandler::sm_pThis = NULL;
extern MFC_Shared_Mem::CMessageManager g_LocalRenderMemManager;
#endif

// Returns a data: URI with the specified contents.
std::string GetDataURI(const std::string& data, const std::string& mime_type)
{
    std::string uriEncoded = CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
    return "data:" + mime_type + ";base64," + uriEncoded;
}

// handle call backs for the browser process.
cefEventHandler::cefEventHandler(bool use_views)
    : use_views_(use_views), m_nBrowserId(0), m_nBrowserCount(0), is_closing_(false)
{
#ifndef _WIN32
    DCHECK(!g_pEvent);
    g_pEvent = this;
#else
    m_fpOldWndProc = NULL;

    draggingWindow = false;
    resizingWindow = false;

    m_nResizeWinMinWidth    = ~0;
    m_nResizeWinMinHeight   = ~0;
    m_nResizeWinMaxWidth    = ~0;
    m_nResizeWinMaxHeight   = ~0;

    if (sm_pThis == NULL)
        sm_pThis = this;
#endif
}

cefEventHandler::~cefEventHandler()
{
#ifndef _WIN32
    g_pEvent = NULL;
#else
    if (!g_LocalRenderMemManager.isInitialized())
        g_LocalRenderMemManager.init(false);

    MFC_Shared_Mem::CSharedMemMsg msg(ADDR_OBS_BROADCAST_Plugin,
                                      ADDR_CEF_JSEXTENSION,
                                      MSG_TYPE_SHUTDOWN,
                                      stdprintf("%u", _getpid()).c_str());
    g_LocalRenderMemManager.sendMessage(msg);

    if (sm_pThis == this)
        sm_pThis = NULL;
#endif
}

cefEventHandler *cefEventHandler::getInstance()
{
#ifndef _WIN32
    return g_pEvent;
#else
    return sm_pThis;
#endif
}

void cefEventHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    CEF_REQUIRE_UI_THREAD();

    if (use_views_)
    {
        // Set the title of the window using the Views framework.
        CefRefPtr<CefBrowserView> browser_view = CefBrowserView::GetForBrowser(browser);
        if (browser_view)
        {
            CefRefPtr<CefWindow> window = browser_view->GetWindow();
            if (window)
                window->SetTitle(title);
        }
    }
    else
    {
        // Set the title of the window using platform APIs.
        PlatformTitleChange(browser, title);
    }
}

void cefEventHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    if (!m_pBrowser.get())
    {
        // Keep a reference to the main browser.
        m_pBrowser = browser;
        m_nBrowserId = browser->GetIdentifier();
    }
    m_lstBrowsers.push_back(browser);
    m_nBrowserCount++;
#ifdef _WIN32
    CefRefPtr< CefBrowserHost > spHost = browser->GetHost();
    if (spHost)
    {
        if ((m_hWnd = spHost->GetWindowHandle()) != NULL)
        {
            if ((m_hWndChild = GetWindow(m_hWnd, GW_CHILD)) != NULL)
            {
                m_fpOldWndProc = (WNDPROC)SetWindowLongPtr(m_hWndChild, GWLP_WNDPROC, (LONG_PTR)FilterWndProc);
            }
        }
    }
#endif
}

bool cefEventHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed destription of this
    // process.
    if (m_lstBrowsers.size() == 1)
    {
        // Set a flag to indicate that the window close should be allowed.
        is_closing_ = true;
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void cefEventHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    // Remove from the list of existing browsers.
    BrowserList::iterator bit = m_lstBrowsers.begin();
    for (; bit != m_lstBrowsers.end(); ++bit)
    {
        if ((*bit)->IsSame(browser))
        {
            // Free the browser pointer so that the browser can be destroyed.
            // m_pBrowser = NULL;
            m_lstBrowsers.erase(bit);
            break;
        }
    }

    if (m_lstBrowsers.empty())
    {
        if (m_pBrowser.get())
            m_pBrowser = NULL;
        // All browser windows have closed. Quit the application message loop.
        CefQuitMessageLoop();
    }
}

void cefEventHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  ErrorCode errorCode,
                                  const CefString& errorText,
                                  const CefString& failedUrl)
{
    CEF_REQUIRE_UI_THREAD();

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    // Display a load error message.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL "
        << std::string(failedUrl) << " with error " << std::string(errorText)
        << " (" << errorCode << ").</h2></body></html>";

    //frame->LoadString(ss.str(), failedUrl);
    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void cefEventHandler::CloseAllBrowsers(bool force_close)
{
    if (!CefCurrentlyOn(TID_UI))
    {
        // Execute on the UI thread.
        CefPostTask(TID_UI, base::Bind(&cefEventHandler::CloseAllBrowsers,
                                       this, force_close));
        return;
    }

    if (m_lstBrowsers.empty())
        return;

    BrowserList::const_iterator it = m_lstBrowsers.begin();
    for (; it != m_lstBrowsers.end(); ++it)
        (*it)->GetHost()->CloseBrowser(force_close);
}

#ifdef _WIN32

bool cefEventHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                               CefRefPtr<CefFrame> frame,
                                               CefProcessId source_process,
                                               CefRefPtr<CefProcessMessage> msg)
{
    bool retVal = false;
    //_MESG("thread %s got msg: '%s'", _MapTid(), msg->GetName().ToString().c_str());
    if (msg)
    {
        string sMethod = msg->GetName().ToString();
        if (sMethod == "ibx_resizeTo")
        {
            CefRefPtr<CefListValue> args = msg->GetArgumentList();
            if (args)
            {
                if (args->GetSize() == 4)
                {
                    int nX  = args->GetInt(0);
                    int nY  = args->GetInt(1);
                    int nCx = args->GetInt(2);
                    int nCy = args->GetInt(3);
                    _MESG("[%s] exec resizeTo(%d,%d, %d width, %d height) !", _MapTid(), nX, nY, nCx, nCy);

                    CefRefPtr< CefBrowserHost > spHost = browser->GetHost();
                    if (spHost)
                    {
                        HWND hWnd = spHost->GetWindowHandle();
                        SetWindowPos(hWnd, NULL, nX, nY, nCx, nCy, SWP_NOZORDER);
                        _MESG("window is 0x%X", hWnd);
                    }
                    retVal = true;
                }
                else _MESG("[%s] args size(%zu) != 4, dropping %s", _MapTid(), args->GetSize(), msg->GetName().ToString().c_str());
            }
            else _MESG("[%s] args NULL, dropping %s", _MapTid(), msg->GetName().ToString().c_str());
        }
        else if (sMethod == "ibx_exit")
        {
            _MESG("Javascript requested exit CEF.");
            CloseAllBrowsers(false);
            retVal = true;
        }
        else if (sMethod == "ibx_log")
        {
            CefRefPtr<CefListValue> args = msg->GetArgumentList();
            if (args)
            {
                string sMsg;
                for (size_t n = 0; n < args->GetSize(); n++)
                {
                    sMsg += args->GetString(n).ToString();
                }
                _MESG("ibx: %s", sMsg.c_str());
            }
        }
        else if (sMethod == "ibx_resizeWindow")
        {
            if (m_nResizeWinMinWidth == ~0)
            {
                m_nResizeWinMinWidth    = 0;
                m_nResizeWinMinHeight   = 0;
                m_nResizeWinMaxWidth    = 0;
                m_nResizeWinMaxHeight   = 0;

                // Enable drag window. Extract current max width/height options from json config, if present
                MfcJsonPtr pApp, pWin, pSize;
                if ((pApp = m_jsConfig.objectGet("app")) != NULL)
                {
                    if ((pWin = pApp->objectGet("window")) != NULL)
                    {
                        if ((pSize = pWin->objectGet("resize_window")) != NULL)
                        {
                            pSize->objectGetInt("min_width", m_nResizeWinMinWidth);
                            pSize->objectGetInt("min_height", m_nResizeWinMinHeight);
                            pSize->objectGetInt("max_width", m_nResizeWinMaxWidth);
                            pSize->objectGetInt("max_height", m_nResizeWinMaxHeight);
                            _MESG("WindowSize Limits: min [%zu,%zu]  max [%zu,%zu]", m_nResizeWinMinWidth, m_nResizeWinMinHeight, m_nResizeWinMaxWidth, m_nResizeWinMaxHeight);
                        }
                    }
                }
            }
            GetCursorPos(&m_ptStartDrag);
            resizingWindow = true;
        }
        else if (sMethod == "ibx_dragWindow")
        {
            // Enable drag window.
            GetCursorPos(&m_ptStartDrag);
            draggingWindow = true;
        }
        else _MESG("[%s] dropping unhandled or unrecognized message %s", _MapTid(), msg->GetName().ToString().c_str());
    }
    else _MESG("[%s] dropping null msg", _MapTid());

    return retVal;
}

LRESULT CALLBACK cefEventHandler::FilterWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    cefEventHandler* pThis = sm_pThis;
    if (pThis)
    {
        if (pThis->draggingWindow)
        {
            if (nMsg == WM_LBUTTONUP)
                return pThis->stopDragWindow();
            else if (nMsg == WM_MOUSEMOVE)
                return pThis->moveDragWindow();
        }
        else if (pThis->resizingWindow)
        {
            if (nMsg == WM_LBUTTONUP)
                return pThis->stopResizeWindow();
            else if (nMsg == WM_MOUSEMOVE)
                return pThis->moveResizeWindow();
        }
        if (pThis->m_fpOldWndProc)
            return CallWindowProc(pThis->m_fpOldWndProc, hWnd, nMsg, wParam, lParam);
    }
    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

LRESULT cefEventHandler::stopDragWindow(void)
{
    moveDragWindow();
    draggingWindow = false;
    return 0;
}

LRESULT cefEventHandler::moveDragWindow(void)
{
    POINT ptStopDrag;
    RECT rcPos;

    GetCursorPos(&ptStopDrag);
    GetWindowRect(m_hWnd, &rcPos);
    int nX = ptStopDrag.x - m_ptStartDrag.x;
    int nY = ptStopDrag.y - m_ptStartDrag.y;
    if (nX != 0 || nY != 0)
        SetWindowPos(m_hWnd, NULL, rcPos.left + nX, rcPos.top + nY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    //_MESG("from [%d,%d] => [%d,%d] .. %c%dx, %c%dy", m_ptStartDrag.x, m_ptStartDrag.y, ptStopDrag.x, ptStopDrag.y, nX < 0 ? '-' : '+', nX, nY < 0 ? '-' : '+', nY);
    m_ptStartDrag = ptStopDrag;
    return 0;
}

LRESULT cefEventHandler::stopResizeWindow(void)
{
    moveResizeWindow();
    resizingWindow = false;
    return 0;
}

LRESULT cefEventHandler::moveResizeWindow(void)
{
    POINT ptStopDrag;
    RECT rcPos;

    GetCursorPos(&ptStopDrag);
    GetWindowRect(m_hWnd, &rcPos);
    int nX = ptStopDrag.x - m_ptStartDrag.x;
    int nY = ptStopDrag.y - m_ptStartDrag.y;

    if (nX != 0 || nY != 0)
    {
        size_t nNewWidth = (rcPos.right - rcPos.left) + nX;
        size_t nNewHeight = (rcPos.bottom - rcPos.top) + nY;

        if (nNewWidth > 0 && nNewWidth < 32768 && nNewHeight > 0 && nNewHeight < 32768)
        {
            if (nNewWidth >= m_nResizeWinMinWidth)
            {
                if (nNewHeight >= m_nResizeWinMinHeight)
                {
                    if (nNewWidth > m_nResizeWinMaxWidth && m_nResizeWinMaxWidth > 0)
                        nNewWidth = m_nResizeWinMaxWidth;

                    if (nNewHeight > m_nResizeWinMaxHeight && m_nResizeWinMaxHeight > 0)
                        nNewHeight = m_nResizeWinMaxHeight;

                    SetWindowPos(m_hWnd, NULL, 0, 0, nNewWidth, nNewHeight, SWP_NOMOVE | SWP_NOZORDER);
                    m_ptStartDrag = ptStopDrag;
                }
            }
        }
    }
    //_MESG("from [%d,%d] => [%d,%d] .. %c%dx, %c%dy", m_ptStartDrag.x, m_ptStartDrag.y, ptStopDrag.x, ptStopDrag.y, nX < 0 ? '-' : '+', nX, nY < 0 ? '-' : '+', nY);
    return 0;
}

LRESULT FilterWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return LRESULT();
}

const char* _MapTid(void)
{
    if      (CefCurrentlyOn(TID_UI))                    return "TID_UI";
    //else if (CefCurrentlyOn(TID_DB))                    return "TID_DB";
    else if (CefCurrentlyOn(TID_FILE))                  return "TID_FILE";
    else if (CefCurrentlyOn(TID_FILE_USER_BLOCKING))    return "TID_FILE_USER_BLOCKING";
    else if (CefCurrentlyOn(TID_PROCESS_LAUNCHER))      return "TID_PROCESS_LAUNCHER";
    //else if (CefCurrentlyOn(TID_CACHE))                 return "TID_CACHE";
    else if (CefCurrentlyOn(TID_IO))                    return "TID_IO";
    else if (CefCurrentlyOn(TID_RENDERER))              return "TID_RENDERER";
    return "Unknown_TID";
}

void fcs_strReplaceAll(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());  // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while (std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    source.swap(newString);
}

#endif

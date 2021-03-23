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

#include <csignal>
#include <cstdlib>
#include <list>
#include <mutex>
#include <string>

// CEF includes
#include <include/cef_version.h>
#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_command_line.h>
#include <include/views/cef_browser_view.h>
#include <include/views/cef_window.h>
#include <include/wrapper/cef_helpers.h>

#include "cefJSExtensions.h"
#include "cefEventHandler.h"
#include <libfcs/Log.h>
#include <libfcs/MfcJson.h>

namespace {

// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class cefLoginWindowDelegate : public CefWindowDelegate
{
public:
    explicit cefLoginWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
        : browser_view_(browser_view)
    {}

    void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE
    {
        // Add the browser view and show the window.
        window->AddChildView(browser_view_);
        window->Show();

        // Give keyboard focus to the browser view.
        browser_view_->RequestFocus();
    }

    void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE
    {
        browser_view_ = nullptr;
    }

    bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE
    {
        // Allow the window to close if the browser says it's OK.
        CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
        if (browser)
            return browser->GetHost()->TryCloseBrowser();
        return true;
    }

    CefSize GetPreferredSize(CefRefPtr<CefView> view) OVERRIDE
    {
        return CefSize(800, 600);
    }

private:
    CefRefPtr<CefBrowserView> browser_view_;

    IMPLEMENT_REFCOUNTING(cefLoginWindowDelegate);
    DISALLOW_COPY_AND_ASSIGN(cefLoginWindowDelegate);
};

}  // namespace


// Implement application-level callbacks for the browser process.
template <class T>
class CMFCCefApp : public CefApp,
                   public CefBrowserProcessHandler,
                   public CefRenderProcessHandler  // to override OnContextCreated
{
public:
#ifndef _WIN32
    CMFCCefApp() {}
#else
    explicit CMFCCefApp(const MfcJsonObj& js)
        : m_jsConfig(js)
    {}
#endif

    ~CMFCCefApp() OVERRIDE
    {
        m_spHandler = nullptr;
        // remove extensions
        for (auto itr = getExtensionList().begin(); itr != getExtensionList().end(); ++itr)
        {
            delete *itr;
        }
    }

    // CEF conversion to browser handler (CefApp method)
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE
    {
        return this;
    }

    // Add js extensions to the context.
    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) OVERRIDE
    {
        _TRACE("OnContextCreated adding version......");
        auto info = CefV8Value::CreateObject(nullptr, nullptr);
        info->SetValue("major", CefV8Value::CreateString("0"), V8_PROPERTY_ATTRIBUTE_READONLY);
        info->SetValue("minor", CefV8Value::CreateString("1"), V8_PROPERTY_ATTRIBUTE_READONLY);

        // Add list of extensions defined in the constructor to the context.
        for (auto itr = getExtensionList().begin(); itr != getExtensionList().end(); ++itr)
        {
            cefJSExtensionBase* pExt = *itr;
            pExt->addExtension(browser, frame, context);
            _TRACE("Adding extension %s", pExt->getName().c_str());
        }
    }

    // |handler| in the worker thread that communicates with the plugin.
    void setHandler(const CefRefPtr<cefEventHandler>& handler)
    {
        // base::AutoLock lock_scope(lock_);
        std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        m_spHandler = handler;
    }

    CefRefPtr<cefEventHandler> getHandler()
    {
        // base::AutoLock lock_scope(lock_);
        std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        return m_spHandler;
    }

    // CEF renderer conversion
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE
    {
        return this;
    }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) OVERRIDE
    {
        std::string msgName(message->GetName());
        _TRACE("processing cef message %s", msgName.c_str());
        for (auto itr = getExtensionList().begin(); itr != getExtensionList().end(); ++itr)
        {
            cefJSExtensionBase *pFn = *itr;
            pFn->executeCallBack(browser, source_process, message);
        }
        return false;
    }

    // CefBrowserProcessHandler method
    void OnContextInitialized() OVERRIDE
    {
        // CEF_REQUIRE_UI_THREAD();
        if (!CefCurrentlyOn(TID_UI))
        {
            _MESG("CEF not currently on UI thread");
            return;
        }

        setURL(MFC_CEF_LOGIN_URL);

        CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();

        const bool enable_chrome_runtime = command_line->HasSwitch("enable-chrome-runtime");

        // Create the browser using the Views framework if "--use-views" is specified
        // via the command-line. Otherwise, create the browser using the native
        // platform framework.
        const bool use_views = command_line->HasSwitch("use-views");
        // cefEventHandler implements browser-level callbacks.
        CefRefPtr<cefEventHandler> handler = createEventHandler(use_views);

        // Save handler so we can adjust the url used on the fly.
        setHandler(handler.get());

        // Specify CEF browser settings here.
        CefBrowserSettings browser_settings;
        std::string url;

#ifdef _WIN32
        MfcJsonPtr pApp = nullptr, pWin = nullptr;
        const std::string sConfig(m_jsConfig.prettySerialize());
        _MESG("jsconfig: %s", sConfig.c_str());

        if (m_jsConfig.objectGetObject("app", &pApp) && pApp && pApp->objectGetObject("window", &pWin))
        {
            if (!pApp->objectGetString("start_url", url) || url.empty())
            {
                url = getURL();
                _MESG("DBG: Falling back to using static default start url[%s], json missing app.start_url key/value.", url.c_str());
            }
            else
            {
                if (url.at(0) == '/')
                {
                    replace( url.begin(), url.end(), '\\', '/'); // replace all 'x' to 'y'
                    fcs_strReplaceAll(url, " ", "%20");
                    url = "file://" + url;
                }
                _MESG("DBG: Loaded start_url \"%s\" from json config", url.c_str());
            }

            // Check if a "--url=" value was provided via the command-line. If so, use
            // that instead of the default URL.  If no --url= argument found, look next
            // in json config for start_url under app.  If not found there either, error
            const std::string optUrl(command_line->GetSwitchValue("url"));
            if (!optUrl.empty())
            {
                _MESG("DBG: Using cmdline url option of '%s'", optUrl.c_str());
                url = optUrl;
            }
        }

        if (url.empty())
            url = MFC_CEF_LOGIN_URL;

        handler->m_jsConfig = m_jsConfig;
#else
        // Check if a "--url=" value was provided via the command-line. If so, use
        // that instead of the default URL.
        url = command_line->GetSwitchValue("url");
        if (url.empty())
            url = getURL();
#endif
        if (use_views && !enable_chrome_runtime)
        {
            // Create the BrowserView.
            CefRefPtr<CefBrowserView> browser_view =
                CefBrowserView::CreateBrowserView(handler, url, browser_settings, nullptr, nullptr, nullptr);

            // Create the Window. It will show itself after creation.
            CefWindow::CreateTopLevelWindow(new cefLoginWindowDelegate(browser_view));
        }
        else
        {
            // Information used when creating the native window.
            CefWindowInfo window_info;
#ifdef _WIN32
            // On Windows we need to specify certain flags that will be passed to
            // CreateWindowEx().
            window_info.SetAsPopup(NULL, "Sidekick MFC Login");

            bool startFullScreen = false;
            if (pApp && pApp->objectGetBool("fullscreen", startFullScreen))
                _MESG("DBG: Loaded fullscreen bool of %s from config", startFullScreen ? "TRUE" : "FALSE");

            // for full screen mode, change window_info settings here:
            if (startFullScreen)
            {
                HMONITOR hmon = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi = { sizeof(mi) };
                if (GetMonitorInfo(hmon, &mi))
                {
                    window_info.x = mi.rcMonitor.left;
                    window_info.y = mi.rcMonitor.top;
                    window_info.width = mi.rcMonitor.right - mi.rcMonitor.left;
                    window_info.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
                    window_info.style = WS_POPUPWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
                }
            }
            else
            {
                // Try and get position and size config data from json for main window
                if (!pWin || !pWin->objectGetInt("x", window_info.x))
                    window_info.x = 200;
                if (!pWin || !pWin->objectGetInt("y", window_info.y))
                    window_info.y = 200;
                if (!pWin || !pWin->objectGetInt("width", window_info.width))
                    window_info.width = 1020;
                if (!pWin || !pWin->objectGetInt("height", window_info.height))
                    window_info.height = 600;
            }
#else
            window_info.width = 660;
            window_info.height = 520;
#endif
            // Create the first browser window.
            CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, nullptr, nullptr);
        }
    }

#if 0
    // CefBrowserProcessHandler method
    CefRefPtr<CefClient> GetDefaultClient() OVERRIDE
    {
        // Called when a new browser window is created via the Chrome runtime UI.
        return cefEventHandler::GetInstance();
    }
#endif

    // Create default event handler. This method can be overridden.
    virtual CefRefPtr<cefEventHandler> createEventHandler(const bool useViews)
    {
        return new T(useViews);
    }

    CefString& getURL() { return m_sURL; }
    void setURL(const char* p) { m_sURL = p; }
    void setURL(const std::string& s) { m_sURL = s.c_str(); }
    void setURL(const CefString& s) { m_sURL = s; }

    void addExtension(cefJSExtensionBase* p) { m_list.push_back(p); }
    std::list<cefJSExtensionBase*>& getExtensionList() { return m_list; }

private:
#ifdef _WIN32
    CMFCCefApp();
#endif
    std::recursive_mutex m_Mutex;
    CefRefPtr<cefEventHandler> m_spHandler;
    // Lock used to protect access to |m_spHandler|.
    base::Lock lock_;
    CefString m_sURL;
    std::list<cefJSExtensionBase*> m_list;
    MfcJsonObj m_jsConfig;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(CMFCCefApp);
};

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
// system
#include <list>

// cef include files
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

// solution include files
// #include <libfcs/Log.h>
#include <libfcs/fcslib_string.h>
#include <libfcs/MfcJson.h>

#include <libPlugins/IPCShared.h>
// #include <libcef_fcs/cefEventHandler.h>
#include "MFCCefEventHandler.h"


CMFCCefEventHandler::CMFCCefEventHandler(const bool use_views)
        : myBaseClass(use_views)
{
}
#ifdef USE_OLD_MEMMANAGER
//MFC_Shared_Mem::CMessageManager g_LocalRenderMemManager;
#endif

//---------------------------------------------------------------------
// OnPRocessMessageRecieved
//
//
// the extensions want the login window to close!
bool CMFCCefEventHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                                   CefRefPtr<CefFrame> frame,
                                                   CefProcessId source_process,
                                                   CefRefPtr<CefProcessMessage> message)
{

    std::string msgName = message->GetName();

    if (msgName == "CloseWindows")
    {
      CloseAllBrowsers(false);
    }
    return false;
}

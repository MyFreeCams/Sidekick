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

MFC_Shared_Mem::CMessageManager g_LocalRenderMemManager;

//---------------------------------------------------------------------
// OnPRocessMessageRecieved
//
// call back when a CEF IPC message is sent to this process from the
// the helper app.
//
// On MAC the helper process can't access the boost shared memory
// so we use the cef IPC and then relay the message to MFCBroadcast.

bool CMFCCefEventHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                                   CefRefPtr<CefFrame> frame,
                                                   CefProcessId source_process,
                                                   CefRefPtr<CefProcessMessage> message)
{
    string msgName, sData;
    bool closeWin = false;
    MfcJsonObj js;

    msgName = message->GetName();

    if (!g_LocalRenderMemManager.isInitialized())
        g_LocalRenderMemManager.init(false);

    if (msgName == "doCredentials")
    {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        sData = args->GetString(0);

        if (js.Deserialize(sData))
        {
            // send message directly to the broadcast plugin,
            MFC_Shared_Mem::CSharedMemMsg msg(ADDR_OBS_BROADCAST_Plugin, ADDR_CEF_JSEXTENSION, MSG_TYPE_DOCREDENTIALS, sData.c_str());
            g_LocalRenderMemManager.sendMessage(msg);
            if (js.objectGetBool("closeWindow", closeWin) && closeWin)
                CloseAllBrowsers(false);
        }
    }
    else if (msgName == "doLogin")
    {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        string sName = args->GetString(0);
        string sPwd = args->GetString(1);
        // send message directly to the broadcast plugin,
        string buf = stdprintf("%s,%s", sName.c_str(), sPwd.c_str());
        g_LocalRenderMemManager.sendMessage( MFC_Shared_Mem::CSharedMemMsg(ADDR_OBS_BROADCAST_Plugin, ADDR_CEF_JSEXTENSION, MSG_TYPE_DOLOGIN, buf.c_str()) );
    }
    else if (msgName == "setModelStreamingKey")
    {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        string sMSK = args->GetString(0);
        // send message directly to the broadcast plugin,
        g_LocalRenderMemManager.sendMessage( MFC_Shared_Mem::CSharedMemMsg(ADDR_OBS_BROADCAST_Plugin, ADDR_CEF_JSEXTENSION, MSG_TYPE_SET_MSK, sMSK.c_str()) );
    }

    return false;
}

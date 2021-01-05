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

// MyFreeCams JavaScript extensions.

// system includes
#include <string>
#include <map>

// cef includes
#include "include/cef_app.h"

// solution includes
#include <libfcs/fcslib_string.h>
#include <libfcs/Log.h>
#include <libPlugins/SidekickModelConfig.h>
#include <libPlugins/IPCShared.h>
#include <libcef_fcs/cefJSExtensions.h>

// project includes
#include "MFCJsExtensions.h"

#ifdef _DIRECT_TO_BROADCAST
MFC_Shared_Mem::CMessageManager g_RenderMemManager;
#endif

//----------------------------------------------------------------------------
// CMFCJsCredentials
//
// handle remote login call.
//
// adds doLogin method to the  javascript DOM
CMFCJsCredentials::CMFCJsCredentials()
    : cefJSExtensionBase("fcsAPI", "credentials")
{}

CMFCJsCredentials::~CMFCJsCredentials()
{}

//----------------------------------------------------------------------------
// execute
//
// call back when the CMFCJsCredentials api call has been executed.
bool CMFCJsCredentials::execute(CefRefPtr<CefV8Value> object,
                                const CefV8ValueList &arguments,
                                CefRefPtr<CefV8Value> &retval)
{
    if (arguments.size() == 1)
    {
        // parameter 1 is a json package of login credentials.
        std::string sJson = arguments[0]->GetStringValue();
#ifdef _DIRECT_TO_BROADCAST
        // This caused all kinds of issues and would crash.  Better to use
        // the existing cef internal message system to talk with the plugin
        if (!g_RenderMemManager.isInitialized())
            g_RenderMemManager.init(false);

        // send message directly to the broadcast plugin,
        std::string buf = stdprintf("%s,%s", sName.c_str(), sPwd.c_str());
        _TRACE("Login attempt %s", sName.c_str());

        g_RenderMemManager.sendMessage(MFC_Shared_Mem::CSharedMemMsg(ADDR_OBS_BROADCAST_Plugin, ADDR_CEF_JSEXTENSION, MSG_TYPE_DOLOGIN, buf.c_str()));
#else
        // send via the existing cef internal ipc message system.

        // send the name of the extension and the json package.
        // send message to browser plugin!
        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("doCredentials");
        CefRefPtr<CefListValue> args = msg->GetArgumentList();
        args->SetString(0, sJson);
        CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
        context->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
#endif
    }
    else
    {
        assert(!"invalid number of parameters");
        return false;
    }
    return true;
}

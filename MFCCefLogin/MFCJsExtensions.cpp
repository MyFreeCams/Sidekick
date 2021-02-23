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

// libipc includes
#include "../libipc/mfc_ipc.h"
#ifdef USE_OLD_MEMMANAGER
#ifdef _DIRECT_TO_BROADCAST
MFC_Shared_Mem::CMessageManager g_RenderMemManager;
#endif
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

        // send via the existing cef internal ipc message system.
        MFCIPC::CRouter *pRTR = MFCIPC::CRouter::getInstance();

        MfcJsonObj js;
        int n = js.Deserialize(sJson);
        pRTR->sendEvent("doCredentials",ADDR_OBS_BROADCAST_Plugin,ADDR_CEF_JSEXTENSION,MSG_TYPE_DOCREDENTIALS,"%s",sJson.c_str());
        bool closeWin = false;
        if (js.objectGetBool("closeWindow", closeWin) && closeWin)
        {
            CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("CloseWindows");
            CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
            context->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
        }
    }
    else
    {
      assert(!"invalid number of parameters");
      return false;
    }
    return true;

}

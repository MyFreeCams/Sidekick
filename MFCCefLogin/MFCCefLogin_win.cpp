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
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif
#include <list>

// cef include files
#include "include/cef_sandbox_win.h"
#include "include/cef_app.h"

// solution includes
#include <libfcs/Log.h>
#include <libPlugins/IPCShared.h>
#include <libcef_fcs/cefEventHandler.h>
#include <libcef_fcs/cefLogin_app.h>
#include "MFCJsExtensions.h"

// project includes
#include "IPCWorkerThread.h"
#include "MFCCefEventHandler.h"

// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically if using the required compiler version. Pass -DUSE_SANDBOX=OFF
// to the CMake command-line to disable use of the sandbox.
// Uncomment this line to manually enable sandbox support.
// #define CEF_USE_SANDBOX 1

#if defined(CEF_USE_SANDBOX)
// The cef_sandbox.lib static library may not link successfully with all VS
// versions.
#pragma comment(lib, "cef_sandbox.lib")
#endif

#ifndef MFC_LOG_LEVEL
#define MFC_LOG_LEVEL ILog::LogLevel::DBG
#endif

#ifndef MFC_LOG_OUTPUT_MASK
#define MFC_LOG_OUTPUT_MASK 10
#endif

#ifdef _WIN32
char* __progname = "MFCCefLogin";
#else
const char* __progname = "MFCCefLogin";
#endif

string cefGetInstallPath(void);

extern MFC_Shared_Mem::CMessageManager g_LocalRenderMemManager;

//---------------------------------------------------------------------
// Entry point function for all processes for windows
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Enable High-DPI support on Windows 7 or newer.
    CefEnableHighDPISupport();

    void* sandbox_info = NULL;
    std::string sPath;
    std::string sLogPath;
#ifdef MFC_OBS_PLUGIN_LOG_PATH
    sLogPath = MFC_OBS_PLUGIN_LOG_PATH;
    assert(sLogPath.size());
#else
    sPath = getenv("USERPROFILE");
    sLogPath = sPath + "\\AppData\\Roaming\\obs-studio\\plugin_config\\MFCBroadcast\\Log";
#endif
    Log::Setup(sLogPath);
    Log::AddOutputMask(MFC_LOG_LEVEL, MFC_LOG_OUTPUT_MASK);
    _TRACE("%s has been loaded", __progname);

#if defined(CEF_USE_SANDBOX)
    // Manage the life span of the sandbox information object. This is necessary
    // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
    CefScopedSandboxInfo scoped_sandbox;
    sandbox_info = scoped_sandbox.sandbox_info();
#endif

    // Provide CEF with command-line arguments.
    CefMainArgs main_args(hInstance);
    string sData, sFile;
    MfcJsonObj js;

    sFile = cefGetInstallPath();
    sFile += "\\mfc_login.js";

    // Get MFC CEF Login js config from Sidekick_root/Resources/cef/mfc_login.js
    if (stdGetFileContents(sFile, sData) > 0)
    {
        if (js.Deserialize(sData))
        {
            _MESG("Successfully loaded mfc_login.js config data: \r\n%s", js.prettySerialize().c_str());
        }
        else _MESG("Unable to deserialize json from %s: %s", sFile.c_str(), sData.c_str());
    }
    else _MESG("Unable to get json config data from %s", sFile.c_str());

    // MFCLoginApp implements application-level callbacks for the browser process.
    // It will create the first browser instance in OnContextInitialized() after
    // CEF has initialized.
    CefRefPtr<CMFCCefApp<CMFCCefEventHandler>> app(new CMFCCefApp< CMFCCefEventHandler >(js));

    // add new extensions
    app->addExtension(new CMFCJsCredentials);

    CIPCWorkerThread worker(*app);
    if (worker.init() || 1 == 1)
    {
        // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
        // that share the same executable. This function checks the command-line and,
        // if this is a sub-process, executes the appropriate logic.
        int exit_code = CefExecuteProcess(main_args, app.get(), sandbox_info);
        if (exit_code >= 0)
        {
            // The sub-process has completed so return here.
            return exit_code;
        }

        // On Windows, we only get this far if this is the browser process.

        // Specify CEF global settings here.
        CefSettings settings;

        _TRACE("Debugging enabled. To attach chrome go to: http://localhost:8080");
        // to attach the chrome debug tools: http://localhost:8080
        settings.remote_debugging_port = 8080;
        // settings.log_severity = LOGSEVERITY_DISABLE;
#if !defined(CEF_USE_SANDBOX)
        settings.no_sandbox = true;
#endif
        // Initialize CEF.
        CefInitialize(main_args, settings, app.get(), sandbox_info);

        // start worker thread
        worker.Start();

        // run cef message loop.  This won't return until we exit.
        CefRunMessageLoop();

        // shutdown worker thread
        worker.End();

        // Shut down CEF.
        CefShutdown();
    }
    else
    {
        _TRACE("Failed to created shared memory file.  Maybe MFCBroadcast is NOT running?");
    }

    return 0;
}

string cefGetInstallPath(void)
{
    //
    // Check registry for location of Sidekick install first, if not found
    // fallback to checking env vars to deduce location based on Public/AllUsersProfile
    // env vars (used by innosetup in same search order).
    //
    char szInstallDir[MAX_PATH + 1] = { '\0' };
    DWORD dwSize = sizeof(szInstallDir), dwType = REG_SZ;
    string sRootPath;
    HKEY hKey;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\MyFreeCams\\Sidekick", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, "InstallDir", NULL, &dwType, (BYTE*)szInstallDir, &dwSize) == ERROR_SUCCESS)
        {
            sRootPath = szInstallDir;
            _TRACE("PATHDBG: Using registry key data for installDir: %s", sRootPath.c_str());
        }
        RegCloseKey(hKey);
    }

    if (sRootPath.empty())
    {
        if (GetEnvironmentVariableA("PUBLIC", szInstallDir, sizeof(szInstallDir)) == 0)
        {
            if (GetEnvironmentVariableA("ALLUSERSPROFILE", szInstallDir, sizeof(szInstallDir)) == 0)
            {
                _TRACE("ERR: Unable to both PUBLIC and ALLUSERSPROFILE env vars, defaulting to C:\\Users\\Public !");
                strcpy(szInstallDir, "C:\\Users\\Public");
            }
            else _TRACE("PATHDBG: Using ALLUSERSPROFILE env var for install root: %s", szInstallDir);
        }
        else _TRACE("PATHDBG: Using PUBLIC env var for install root: %s", szInstallDir);

        sRootPath = szInstallDir;
    }
    _MESG("PATHDBG: getInstallPath() ret   %s\\cef ", sRootPath.c_str());

    return stdprintf("%s\\cef", sRootPath.c_str());
}

void proxy_blog(int nLevel, const char* pszMsg)
{
    // blog(nLevel, "%s", pszMsg);
}

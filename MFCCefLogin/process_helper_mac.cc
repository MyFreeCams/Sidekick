// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <pwd.h>

#include "include/cef_app.h"
#include "include/wrapper/cef_library_loader.h"

const char* __progname = "MFCCefLogin_helper";

#include <libcef_fcs/cefEventHandler.h>
#include "MFCCefEventHandler.h"
#include <libcef_fcs/cefLogin_app.h>
#include <libfcs/Log.h>
#include "MFCJsExtensions.h"

// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically. Pass -DUSE_SANDBOX=OFF to the CMake command-line to disable
// use of the sandbox.
#if defined(CEF_USE_SANDBOX)
#include "include/cef_sandbox_mac.h"
#endif

#ifndef MFC_LOG_LEVEL
#define MFC_LOG_LEVEL ILog::LogLevel::DBG
#endif

#ifndef MFC_LOG_OUTPUT_MASK
#define MFC_LOG_OUTPUT_MASK 10
#endif

// Entry point function for sub-processes.
int main(int argc, char* argv[])
{
    struct passwd *pw = getpwuid(getuid());
    std::string sLogPath = pw->pw_dir;
    sLogPath += "/Logs";
    Log::Setup(sLogPath);
    Log::AddOutputMask(MFC_LOG_LEVEL, MFC_LOG_OUTPUT_MASK);
    _TRACE("Helper has started 8");

#if defined(CEF_USE_SANDBOX)
    // Initialize the macOS sandbox for this helper process.
    CefScopedSandboxContext sandbox_context;
    if (!sandbox_context.Initialize(argc, argv))
        return 1;
#endif

    // Load the CEF framework library at runtime instead of linking directly
    // as required by the macOS sandbox implementation.
    CefScopedLibraryLoader library_loader;
    if (!library_loader.LoadInHelper())
        return 1;

    // Provide CEF with command-line arguments.
    CefMainArgs main_args(argc, argv);

    CefRefPtr<CMFCCefApp<CMFCCefEventHandler>> app(new CMFCCefApp<CMFCCefEventHandler>);
    app->addExtension(new CMFCJsCredentials);

    // Execute the sub-process.
    int n = CefExecuteProcess(main_args, app, nullptr);
    _TRACE("CefExecuteProcess returned %d",n);
}

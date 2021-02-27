/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
 * Copyright (c) 2013 The Chromium Embedded Framework Authors.
 * Portions copyright (c) 2010 The Chromium Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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

#import <Cocoa/Cocoa.h>

#include <pwd.h>

// CEF includes
#include <include/cef_application_mac.h>
#include <include/cef_command_line.h>
#include <include/wrapper/cef_helpers.h>
#include <include/wrapper/cef_library_loader.h>

// solution includes
#include <libcef_fcs/cefLogin_app.h>
#include <libcef_fcs/cefEventHandler.h>
#include <libPlugins/IPCShared.h>
#include <libfcs/Log.h>

// project includes
#include "IPCWorkerThread.h"
#include "MFCJsExtensions.h"
#include "build_number.h"

const char* __progname = "MFCCefLogin";

// Receives notifications from the application.
@interface fcsLoginAppDelegate : NSObject <NSApplicationDelegate>
{
@private
    bool with_chrome_runtime_;
}

- (id)initWithChromeRuntime:(bool)with_chrome_runtime;
- (void)createApplication:(id)object;
- (void)tryToTerminateApplication:(NSApplication*)app;
@end

// Provide the CefAppProtocol implementation required by CEF.
@interface fcsLoginApplication : NSApplication <CefAppProtocol>
{
@private
    BOOL handlingSendEvent_;
}
@end

@implementation fcsLoginApplication
- (BOOL)isHandlingSendEvent
{
    return handlingSendEvent_;
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent
{
    handlingSendEvent_ = handlingSendEvent;
}

- (void)sendEvent:(NSEvent*)event
{
    CefScopedSendingEvent sendingEventScoper;
    [super sendEvent:event];
}

// |-terminate:| is the entry point for orderly "quit" operations in Cocoa. This
// includes the application menu's quit menu item and keyboard equivalent, the
// application's dock icon menu's quit menu item, "quit" (not "force quit") in
// the Activity Monitor, and quits triggered by user logout and system restart
// and shutdown.
//
// The default |-terminate:| implementation ends the process by calling exit(),
// and thus never leaves the main run loop. This is unsuitable for Chromium
// since Chromium depends on leaving the main run loop to perform an orderly
// shutdown. We support the normal |-terminate:| interface by overriding the
// default implementation. Our implementation, which is very specific to the
// needs of Chromium, works by asking the application delegate to terminate
// using its |-tryToTerminateApplication:| method.
//
// |-tryToTerminateApplication:| differs from the standard
// |-applicationShouldTerminate:| in that no special event loop is run in the
// case that immediate termination is not possible (e.g., if dialog boxes
// allowing the user to cancel have to be shown). Instead, this method tries to
// close all browsers by calling CloseBrowser(false) via
// ClientHandler::CloseAllBrowsers. Calling CloseBrowser will result in a call
// to ClientHandler::DoClose and execution of |-performClose:| on the NSWindow.
// DoClose sets a flag that is used to differentiate between new close events
// (e.g., user clicked the window close button) and in-progress close events
// (e.g., user approved the close window dialog). The NSWindowDelegate
// |-windowShouldClose:| method checks this flag and either calls
// CloseBrowser(false) in the case of a new close event or destructs the
// NSWindow in the case of an in-progress close event.
// ClientHandler::OnBeforeClose will be called after the CEF NSView hosted in
// the NSWindow is dealloc'ed.
//
// After the final browser window has closed ClientHandler::OnBeforeClose will
// begin actual tear-down of the application by calling CefQuitMessageLoop.
// This ends the NSApplication event loop and execution then returns to the
// main() function for cleanup before application termination.
//
// The standard |-applicationShouldTerminate:| is not supported, and code paths
// leading to it must be redirected.
- (void)terminate:(id)sender
{
    fcsLoginAppDelegate* delegate = static_cast<fcsLoginAppDelegate*>([NSApp delegate]);
    [delegate tryToTerminateApplication:self];
    // Return, don't exit. The application is responsible for exiting on its own.
}
@end

@implementation fcsLoginAppDelegate
- (id)initWithChromeRuntime:(bool)with_chrome_runtime
{
    if (self = [super init])
        with_chrome_runtime_ = with_chrome_runtime;

    return self;
}

// Create the application on the UI thread.
- (void)createApplication:(id)object
{
    if (!with_chrome_runtime_)
    {
        // Chrome will create the top-level menu programmatically in
        // chrome/browser/ui/cocoa/main_menu_builder.h
        // TODO(chrome-runtime): Expose a way to customize this menu.
        [[NSBundle mainBundle] loadNibNamed:@"MainMenu"
                                      owner:NSApp
                            topLevelObjects:nil];
    }

    // Set the delegate for application events.
    [[NSApplication sharedApplication] setDelegate:self];
}

- (void)tryToTerminateApplication:(NSApplication*)app
{
    cefEventHandler* handler = cefEventHandler::GetInstance();
    if (handler && !handler->IsClosing())
        handler->CloseAllBrowsers(false);
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    return NSTerminateNow;
}
@end

//---------------------------------------------------------------------
// Entry point function for the browser process.
int main(int argc, char *argv[])
{
    struct passwd* pw = getpwuid(getuid());
    std::string sLogPath = pw->pw_dir;
    sLogPath += "/Logs";  // /Users/username/Logs/
    Log::Setup(sLogPath);
    Log::AddOutputMask(MFC_LOG_LEVEL, MFC_LOG_OUTPUT_MASK);
    _TRACE("Startup build # %d", MFC_BUILD_NUMBER);

    // Load the CEF framework library at runtime instead of linking directly
    // as required by the macOS sandbox implementation.
    CefScopedLibraryLoader library_loader;
    if (!library_loader.LoadInMain())
        return 1;

    // Provide CEF with command-line arguments.
    CefMainArgs main_args(argc, argv);

    // Using ARC (automatic reference counting)
    @autoreleasepool
    {
        // Initialize the fcsLoginApplication instance.
        [fcsLoginApplication sharedApplication];

        // If there was an invocation to NSApp prior to this method, then the NSApp
        // will not be a fcsLoginApplication, but will instead be an NSApplication.
        // This is undesirable and we must enforce that this doesn't happen.
        CHECK([NSApp isKindOfClass:[fcsLoginApplication class]]);

        // Parse command-line arguments for use in this method.
        CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
        command_line->InitFromArgv(argc, argv);

        // Specify CEF global settings here.
        CefSettings settings;

        const bool with_chrome_runtime = command_line->HasSwitch("enable-chrome-runtime");

        if (with_chrome_runtime)
        {
            // Enable experimental Chrome runtime. See issue #2969 for details.
            settings.chrome_runtime = true;
        }

        _TRACE("Debugging engaged. To attach the chrome debug tools: http://localhost:8080");
        settings.remote_debugging_port = 8080;

        // When generating projects with CMake the CEF_USE_SANDBOX value will be
        // defined automatically. Pass -DUSE_SANDBOX=OFF to the CMake command-line
        // to disable use of the sandbox.
#if !defined(CEF_USE_SANDBOX)
        settings.no_sandbox = true;
#endif

        // fcsLoginApp implements application-level callbacks for the browser process.
        // It will create the first browser instance in OnContextInitialized() after
        // CEF has initialized.
        CefRefPtr< CMFCCefApp<CMFCCefEventHandler> > app(new CMFCCefApp<CMFCCefEventHandler>);

        std::string s = MFC_CEF_LOGIN_URL;
        app->setURL(MFC_CEF_LOGIN_URL);
        app->addExtension(new CMFCJsCredentials);

        // Initialize CEF for the browser process.
        CefInitialize(main_args, settings, app.get(), NULL);

        // Create the application delegate.
        NSObject* delegate = [[fcsLoginAppDelegate alloc] initWithChromeRuntime:with_chrome_runtime];
        [delegate performSelectorOnMainThread:@selector(createApplication:)
                                   withObject:nil
                                waitUntilDone:NO];

        CIPCWorkerThread myThread(*app);
        if (1 == 1 || myThread.init())
        {
            // Run the CEF message loop. This will block until CefQuitMessageLoop() is called.
            CefRunMessageLoop();
        }
        // Shut down CEF.
        CefShutdown();

        // Release the delegate.
#if !__has_feature(objc_arc)
        [delegate release];
#endif
        delegate = nil;

    }  // @autoreleasepool

    return 0;
}

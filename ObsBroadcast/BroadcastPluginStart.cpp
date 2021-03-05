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

// BroadcastPluginStart.cpp : Defines the exported functions for the DLL application.

//#ifndef DO_SERVICE_JSON_COPY
//#define DO_SERVICE_JSON_COPY 1
//#endif
#ifndef PANEL_UNIT_TEST
#define PANEL_UNIT_TEST 0
#endif
#ifndef MFC_AGENT_EDGESOCK
#define MFC_AGENT_EDGESOCK 1
#endif
#ifndef SIDEKICK_ENABLE_VIRTUALCAM
#define SIDEKICK_ENABLE_VIRTUALCAM 1
#endif
#ifndef SIDEKICK_CONSOLE
#define SIDEKICK_CONSOLE 0
#endif
#ifndef SIDEKICK_VERBOSE_CONSOLE
#define SIDEKICK_VERBOSE_CONSOLE 0
#endif
#ifndef SIDEKICK_SET_WEBRTC
#define SIDEKICK_SET_WEBRTC 0
#endif

#ifdef _WIN32
#include "targetver.h"
#include <libfcs/UtilCommon.h>  // gettimeofday()
#endif

#include <cstdint>
#include <regex>
#include <string>

#ifdef __APPLE__
#include <cstdio>
#include <cstdarg>
#include <ctime>
#endif

// qt
#include <QMessageBox>
#include <QMetaObject>
#include <QObject>
#include <QScrollBar>
#include <QString>
#include <QTimer>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

#include <libPlugins/ObsServicesJson.h>  // do not move

// obs
#include <libobs/obs-module.h>
#include <obs-frontend-api.h>

// solution
#include <libfcs/fcs_b64.h>
#include <libfcs/MfcTimer.h>
#include <libPlugins/build_version.h>
#include <libPlugins/EdgeChatSock.h>
#include <libPlugins/IPCShared.h>
#include <libPlugins/MFCConfigConstants.h>
#include <libPlugins/ObsUtil.h>
#include <libPlugins/Portable.h>

// project
#include "SidekickTypes.h"
#include "HttpThread.h"
#include "ObsBroadcast.h"
#include "ObsCallbackEvent.h"
#include "SidekickProperties.h"

#define _SKLOG(pszFmt, ...) SKLogMarker(__FILE__, __FUNCTION__, __LINE__, pszFmt, ##__VA_ARGS__)

extern CBroadcastCtx g_ctx;  // part of MFCLibPlugins.lib::MfcPluginAPI.obj
extern char g_dummyCharVal;  // Part of MfcLog.cpp in MFClibfcs.lib
extern "C" struct obs_output_info wowza_output_info;

CHttpThread g_thread;
SidekickPropertiesUI* sidekick_prop = nullptr;
MFCDock* pMFCDock = nullptr;
bool firstProfileCheck = false;
bool servicesUpdated = false;
QTimer* g_pServicesTimer = NULL;

const char* MapObsEventType(enum obs_frontend_event eventType);
void onObsEvent(obs_frontend_event eventType, void* pCtx);
void onObsProfileChange(obs_frontend_event eventType);
void setupSidekickUI(void);
void showAccountLinkStatus(void);
void SKLogMarker(const char* pszFile, const char* pszFunction, int nLine, const char* pszFmt, ...);
void proxy_blog(int nLevel, const char* pszMsg);
void checkServices(void);

void ui_onUserUpdate(int nChange, uint32_t nCurUid, uint32_t nNewUid, uint32_t nProfileChanged);
void ui_onSessionUpdate(int nChange, uint32_t nCurSid, uint32_t nNewSid, uint32_t nProfileChanged);
void ui_onServerUpdate(const std::string& sCurUrl, const std::string& sNewUrl);
void ui_onStreamkeyUpdate(const std::string& sCurKey, const std::string& sNewKey);
void ui_onModelState(uint32_t nChange, SidekickActiveState oldState, SidekickActiveState newState);
void ui_onUnlinkEvent(void);
void ui_appendConsoleMsg(const std::string& sMsg);
void ui_replaceConsoleMsg(const std::string& sMsg);
int ui_SidekickMsgBox(const char* pszTitle, const char* pszBody, int nButtons = QMessageBox::Ok, QWidget* pParent = nullptr);

#ifdef _WIN32
char* __progname = BROADCAST_FILENAME;
#else
const char* __progname = BROADCAST_FILENAME;
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(__progname, "en-US")


MODULE_EXPORT const char* obs_module_description(void)
{
    static char s_szDescription[256];
#ifdef _WIN32
    _snprintf_s(s_szDescription, sizeof(s_szDescription), "MFC Sidekick v%s", SIDEKICK_VERSION_STR);
#else
    snprintf(s_szDescription, sizeof(s_szDescription), "MFC Sidekick v%s", SIDEKICK_VERSION_STR);
#endif
    return s_szDescription;
}


//---------------------------------------------------------------------------
// ObsFrontEndEvent
//
// This is a call back method for obs front end events.
// This function is registered with the obs_frontend_add_event_callback call
//
// Currently we are not using the call backs, so it's off for release.
#ifdef _DEBUG
void OBSFrontendEvent(obs_frontend_event event, void* ptr)
{
    UNUSED_PARAMETER(ptr);
    // We don't need to process any of these events yet.
    // CEventList::getEventList().push(CObsCallBackEvent(event));
    switch ((int)event)
    {
    case OBS_FRONTEND_EVENT_RECORDING_STARTED:
        break;
    case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
        break;
    }
}
#endif


void onSidekickSetup(void* pCtx)
{
    UNUSED_PARAMETER(pCtx);
    // Open/Reopen/Bring to foreground the MFCCefLogin process.
    CObsUtil::ExecMFCLogin();
}


// Event handler for browser panel unit test
#if PANEL_UNIT_TEST
void openBrowserPanelMenuHandler(void*)
{
#if (defined(MFC_BROWSER_AVAILABLE) && (MFC_BROWSER_AVAILABLE > 0))
    // TODO: Update dimensions to be calculated based on available screen size
    std::string url = MFC_CEF_LOGIN_URL;
    CObsUtil::openBrowserPanel("sidekick", url.c_str(), 614, 556);
#endif
}
#endif


void loadServices(void)
{
    // Get location of obs installed services.json
//#ifdef _WIN32
    // services.json is located with the rtmp-services plugin
    //std::string sFilename2 = "../../data/obs-plugins/rtmp-services/services.json";
//#else
    //obs_module_t* pModule = obs_current_module();
    //std::string sT = obs_get_module_data_path(pModule);
    //std::string sFilename2 = CObsUtil::AppendPath(sT, "services.json");
//#endif


    // wait for upto 10 seconds for services.json to show up
    CObsServicesJson svc;

    // Get location of profile (module) version of services.json
    string sFilename = obs_module_config_path("services.json");
    sFilename = svc.getNormalizedServiceFile(sFilename);

    //
    // We need to verify that mfc service is in the services.json file.
    //
    // There are two versions of the file in different directories:
    //
    // 1. Profile (module) version. This one is preferred (c:\users\todd\appdata\roaming\obs-studio\.....
    // It must be the current version: RTMP_SERVICES_FORMAT_VERSION
    // OBS <= v26.0.2 (10/2020): version is 2
    // OBS >= v26.1 (11/27/20): version is 3
    //
    // 2. OBS installed version. If profile version is not the current version, we fall back to the
    // version installed with obs-studio: c:\program files\obs-studio\data\obs-plugins\rtmp-services\services.json
    // Since we do NOT have write permissions to the installed version, we copy the installed services.json
    // to the profile (module) location.
    //
    if (svc.load(sFilename))
    {
        // We successfully loaded the json
        servicesUpdated = true;

        // If the object is "dirty" (the MyFreeCams service was not found so we added it)
        // after the load operation: update the json for obs.
        if (svc.isDirty())
            svc.save();
    }
    else _MESG("Failed to load services.json: %s", sFilename.c_str());

    if (servicesUpdated)
        g_thread.setServicesFilename(svc.getServicesFilename());
}

void checkServices(void)
{
    // defaults to checking again in 500ms, unless checkFileHash() returns false
    int checkIntervalMs = 500;         

    if (servicesUpdated)
    {
        // see if the file has changed since we loaded services, if so reparse it
        CObsServicesJson svc;
        if (svc.checkFileHash())
        {
            _MESG("SVCDBG: json services file changed, calling loadServices again..");
            loadServices();
        }
        else checkIntervalMs = 5000;    // if it hasn't, set our interval to 5sec instead of 500ms
    }
    else 
    {
        _MESG("SVCDBG: servicesUpdated false, so calling loadServices first time from checkServices...");
        loadServices();
    }

    // On startup, emulate initial profile change event so we
    // read the current profile and setup g_ctx accordingly if
    // the profile changed event wasnt already fired
    //
    if ( ! firstProfileCheck )
        onObsProfileChange(OBS_FRONTEND_EVENT_PROFILE_CHANGED);

    QTimer::singleShot(checkIntervalMs, qApp, checkServices);
}

//---------------------------------------------------------------------------
// obs_module_load
//
// load module for ObsBroadcast Plugin
bool obs_module_load(void)
{
    Log::Setup( CObsUtil::getLogPath() );
    Log::AddOutputMask(MFC_LOG_LEVEL, MFC_LOG_OUTPUT_MASK);


    SidekickModelConfig::initializeDefaults();
    g_ctx.clear(false);

    _TRACE("%s OBS Plugin has been loaded", __progname);
    _MESG("MFCBroadcast module (ver %s) started", SIDEKICK_VERSION_STR);
#ifdef _DEBUG
    // Register for event call backs.
    obs_frontend_add_event_callback(OBSFrontendEvent, nullptr);
#endif

    obs_register_output(&wowza_output_info);
    static SidekickTimer* s_pTimer = new SidekickTimer();

    // Register event handler to catch streaming start/stop events
    obs_frontend_add_event_callback(onObsEvent, nullptr);

    QMainWindow* main_window = (QMainWindow*)obs_frontend_get_main_window();
    if (main_window)
    {
        obs_frontend_push_ui_translation(obs_module_get_string);

#if SIDEKICK_CONSOLE
        sidekick_prop = new SidekickPropertiesUI( main_window );
        if (sidekick_prop)
        {
            QString qsTitle( QString::fromStdString( stdprintf("MFC Sidekick v%s", SIDEKICK_VERSION_STR) ) );
            sidekick_prop->setWindowTitle(qsTitle);
            obs_frontend_pop_ui_translation();
            QAction* action = (QAction*)obs_frontend_add_tools_menu_qaction(obs_module_text("Sidekick Properties"));
            auto menu_cb = [=]()
            {
                sidekick_prop->ShowHideDialog();
            };
            QAction::connect(action, &QAction::triggered, menu_cb);
        }
        else _MESG("DBG: ** Unable to create SidekickProp - sidekick_prop NULL **");
#endif

        pMFCDock = new MFCDock(main_window);
        //QAction* addDockAction = (QAction*)obs_frontend_add_dock(pMFCDock);
        main_window->addDockWidget(Qt::BottomDockWidgetArea, (QDockWidget*)pMFCDock);
    }
    else _MESG("DBG: ** Unable to get MainWindow ptr **");


    // Start monitoring services.json in rtmp-services. Start after the sidekick dock
    // widget is created (if it was created)
    checkServices();

    // Start the monitor thread and let OBS continue to start.
    g_thread.Start();

#if PANEL_UNIT_TEST
#if (defined(MFC_BROWSER_AVAILABLE) && (MFC_BROWSER_AVAILABLE > 0))
    obs_frontend_add_tools_menu_item("broadcast browser panel", openBrowserPanelMenuHandler, nullptr);
#endif
#endif

    return true;
}


void SKLogMarker(const char* pszFile, const char* pszFunction, int nLine, const char* pszFmt, ...)
{
    static std::vector< std::string > s_vQueuedMsgs;
    static char szData[16384]{};
    QTextEdit* pConsole = nullptr;
    QTextDocument* pDoc = nullptr;
    QScrollBar* pScroll = nullptr;
    va_list vaList;

    va_start(vaList, pszFmt);
#ifdef _WIN32
    _vsnprintf_s(szData, sizeof(szData), _TRUNCATE, pszFmt, vaList);
#else
    vsnprintf(szData, sizeof(szData), pszFmt, vaList);
#endif
    va_end(vaList);

#if SIDEKICK_CONSOLE
    pConsole = (QTextEdit*)g_ctx.getConsole();
    if (!pConsole)
    {
        // If document is unavailable, we might be logging before the dialog is done initializing,
        // so queue szData for when console is available (up to 3 messages).
        if (s_vQueuedMsgs.size() < 3)
            s_vQueuedMsgs.push_back(szData);
        else
            s_vQueuedMsgs.clear();

        _MESG("[SidekickConsole CONSOLE Unavailable] %s\r\n", szData);
        return;
    }
    s_vQueuedMsgs.push_back(szData);

    pDoc = pConsole->document();
    if (!pDoc)
        return (void)_MESG("[SidekickConsole DOCUMENT unavailable] %s", szData);

    pScroll = pConsole->verticalScrollBar();
    if (!pScroll)
        return (void)_MESG("[SidekickConsole SCROLL unavailable] %s", szData);

    for (size_t n = 0; n < s_vQueuedMsgs.size(); n++)
    {
        pConsole->moveCursor(QTextCursor::End);
        pConsole->insertHtml(QString(s_vQueuedMsgs[n].c_str()));
        pConsole->moveCursor(QTextCursor::End);
        pConsole->append(QString("<br>"));
        // Move to bottom of console
        pScroll->setValue( pScroll->maximum() - 1 );
    }
    s_vQueuedMsgs.clear();
#else
    string sPath(pszFile);
    string sBase = sPath.substr(sPath.find_last_of("/\\") + 1);
    
    Log::Mesg("[%s:%d %s] %s", sBase.c_str(), nLine, pszFunction, szData);
    //_MESG("%s", szData);
#endif
}


void ui_appendConsoleMsg(const std::string& sMsg)
{
    //if (!sMsg.empty())
    //    _SKLOG("%s", sMsg.c_str());
}


void ui_replaceConsoleMsg(const std::string& sMsg)
{
    QTextEdit* pConsole = (QTextEdit*)g_ctx.getConsole();
    if (pConsole)
        pConsole->clear();

    //if (!sMsg.empty())
    //    _SKLOG("%s", sMsg.c_str());
}


void ui_onUserUpdate(int nChange, uint32_t nCurUid, uint32_t nNewUid, uint32_t nProfileChanged)
{
    if (sidekick_prop)
        sidekick_prop->relabelPropertiesText();

    if (pMFCDock)
        pMFCDock->relabelPropertiesText();

    bool isWebRTC = false, isMfc = false, isLinked = false, isLoggedIn = false;
    std::string sUsername;
    {
        auto lk     = g_ctx.sharedLock();
        sUsername   = g_ctx.cfg.getString("username");
        isLoggedIn  = g_ctx.isLoggedIn;
        isWebRTC    = g_ctx.isWebRTC;
        isLinked    = g_ctx.isLinked;
        isMfc       = g_ctx.isMfc;
    }

    if (nCurUid != nNewUid)
        showAccountLinkStatus();
}


void ui_onSessionUpdate(int nChange, uint32_t nCurSid, uint32_t nNewSid, uint32_t nProfileChanged)
{
    if (sidekick_prop)
        sidekick_prop->relabelPropertiesText();

    if (pMFCDock)
        pMFCDock->relabelPropertiesText();

    // If both profile and sid changed, we cannot determine whether the sid change
    // is a modelweb login/logout event. It may simply be that the new profile
    // differs in service or link state. Therefore, we don't bother with logging
    // a session id change as a modelweb login/logout event in this case since
    // usually it'll be a byproduct of a larger change, and not relevant enough to log.

    // If sid changed with no apparent profile change, the model has:
    // logged in to the modelweb (sid: zero --> non-zero), or
    // logged out from the modelweb (sid: non-zero --> zero).
    if (nProfileChanged == 0)
    {
        std::string consoleMessage;
#if SIDEKICK_VERBOSE_CONSOLE
        if (nCurSid > 0 && nNewSid == 0)
            consoleMessage = "<div style=\"font-size:14px;\"><b>ModelWeb Logoff</b> detected! Login to ModelWeb before attempting to stream.</div>";
        else if (nCurSid == 0 && nNewSid > 0)
            consoleMessage = "<div style=\"font-size:14px;\"><b>ModelWeb Login</b> detected! You are now able to start streaming.</div>";
        ui_appendConsoleMsg(consoleMessage);
#else
        if (nCurSid > 0 && nNewSid == 0)
            consoleMessage = "<div style=\"font-size:14px;\">Login to ModelWeb before attempting to stream.</div>";
        else if (nCurSid == 0 && nNewSid > 0)
            consoleMessage = "<div style=\"font-size:14px;\">You are now able to start streaming.</div>";
        ui_replaceConsoleMsg(consoleMessage);
#endif
    }
}


void ui_onServerUpdate(const std::string& sCurUrl, const std::string& sNewUrl)
{
    // Set the server url for the current profile if the profile isMfc and has no server url set.
    if (g_ctx.isMfc && (g_ctx.isRTMP || g_ctx.isWebRTC) && !g_ctx.isCustom)
    {
        _MESG(  "DBG: setting url from %s => %s (isMfc true, isRtmp? %s, isWebRTC? %s, isCustom? %s)",
                sCurUrl.c_str(),
                sNewUrl.c_str(),
                g_ctx.isRTMP ? "true" : "false",
                g_ctx.isWebRTC ? "true" : "false",
                g_ctx.isCustom ? "true" : "false");

        CObsServicesJson services;
        services.updateProfileSettings("(null)", sNewUrl);
        CObsUtil::setCurrentSetting("server", sNewUrl.c_str());
    }
}


void ui_onStreamkeyUpdate(const std::string& sCurKey, const std::string& sNewKey)
{
    // Set the stream key for the current profile if the profile isMfc and has no stream key set.
    if (g_ctx.isMfc && (g_ctx.isRTMP || g_ctx.isWebRTC))
    {
        CObsServicesJson services;
        services.updateProfileSettings(sNewKey, "(null)");
        CObsUtil::setCurrentSetting("key", sNewKey.c_str());
    }
}


void ui_onUnlinkEvent(void)
{
    QTextEdit* pConsole = (QTextEdit*)g_ctx.getConsole();
    if (pConsole)
        pConsole->clear();
    std::string consoleMessage;
#if SIDEKICK_VERBOSE_CONSOLE
    consoleMessage = "<div style=\"font-size:14px;\"><b>Account Unlinked</b>! Link Sidekick to your account and login to ModelWeb to start streaming.</div>";
#else
    consoleMessage = "<div style=\"font-size:14px;\">Click on the <b>Link Sidekick</b> button to authenticate with MyFreeCams.</div>";
#endif

    if (sidekick_prop)
        sidekick_prop->relabelPropertiesText();

    if (pMFCDock)
        pMFCDock->relabelPropertiesText();

#if SIDEKICK_VERBOSE_CONSOLE
    ui_appendConsoleMsg(consoleMessage);
#else
    ui_replaceConsoleMsg(consoleMessage);
#endif
}


void ui_onModelState(uint32_t nChange, SidekickActiveState oldState, SidekickActiveState newState)
{
    if (CBroadcastCtx::sm_edgeSock)
        CBroadcastCtx::sm_edgeSock->onStateChange(oldState, newState);
}


#if SIDEKICK_SET_WEBRTC
void ui_onSetWebRtc(int nState)
{
    const char* service_id = "rtmp_custom";
    if (g_ctx.isMfc)
    {
        if (g_ctx.isRTMP && nState == 1)
        {
            // TODO: set profile to use WebRTC instead of RTMP
            // ...
            _MESG("Set profile to use WebRTC ...");
            //service_id =
        }
        else if (g_ctx.isWebRTC && nState == 0)
        {
            // TODO: set profile to use RTMP isntead of WebRTC
            // ...
            _MESG("Set profile to use RTMP ...");
            // code for changing service in a profile (?)
            //obs_service_t* oldService = main->GetService();
            //OBSData hotkeyData = obs_hotkeys_save_service(oldService);
            //obs_data_release(hotkeyData);
            //OBSData settings = obs_data_create();
            //obs_data_set_string(settings, "service", QT_TO_UTF8(ui->service->currentText()));
            //obs_data_release(settings);

            //CObsServicesJson services;
            //services.updateProfileSettings("(null)", sNewUrl);
            //CObsUtil::setCurrentSetting("server", sNewUrl.c_str());
        }
    }
}
#endif


void SidekickTimer::onTimerEvent()
{
    static size_t s_nPulse = 0;

    
    if (s_nPulse == 0)
        setupSidekickUI();

    if (CBroadcastCtx::sm_edgeSock)
        CBroadcastCtx::sm_edgeSock->onTimerEvent(s_nPulse);

#ifdef SIDEKICK_TIMER_EXAMPLE
    // example for running an event only after at least 500ms elapsed
    //
    static struct timeval tvLastStateUpdate = { 0, 0 };
    struct timeval tvNow;
    uint64_t diffMicro;
    gettimeofday(&tvNow, NULL);
    if ((diffMicro = MfcTimer::DiffMicro(tvNow, tvLastStateUpdate)) > 500000)
    {
       // execute every 500ms timer/action ...
       tvLastStateUpdate = tvNow;
    }
#endif

    // Move any queued UI events out of g_ctx
    std::deque<SIDEKICK_UI_EV> uiEvents;
    {
        auto lk = g_ctx.eventLock();
        g_ctx.sm_eventQueue.swap(uiEvents);
    }

    // Queued events are pushed onto the front of the deque, so the latest event will be read
    // first as we iterate through in the normal forward direction. Any events that are meant to
    // only be processed once per event cycle, we mark once processing that event and skip any
    // other events found of the same type later in the iteration of the deque.
    std::set<SidekickEventType> skipEvents;
    for (auto iEvent = uiEvents.begin(); iEvent != uiEvents.end(); ++iEvent)
    {
        if (skipEvents.find(iEvent->ev) == skipEvents.end())
        {
            switch (iEvent->ev)
            {
            case SkSessionId:
                ui_onSessionUpdate(iEvent->dwArg1, iEvent->dwArg2, iEvent->dwArg3, iEvent->dwArg4);
                skipEvents.insert(iEvent->ev);
                break;

            case SkUserId:
                ui_onUserUpdate(iEvent->dwArg1, iEvent->dwArg2, iEvent->dwArg3, iEvent->dwArg4);
                skipEvents.insert(iEvent->ev);
                break;

            case SkLog:
                _SKLOG(">> %s", iEvent->sArg1.c_str());
                break;

            case SkModelState:
                ui_onModelState(iEvent->dwArg1, static_cast<SidekickActiveState>(iEvent->dwArg2), static_cast<SidekickActiveState>(iEvent->dwArg3));
                skipEvents.insert(iEvent->ev);
                break;

            case SkStreamKey:
                //_MESG("WDBG: SkStreamKey caught, args: %s,   %s", iEvent->sArg1.c_str(), iEvent->sArg2.c_str());
                ui_onStreamkeyUpdate(iEvent->sArg1, iEvent->sArg2);
                skipEvents.insert(iEvent->ev);
                break;

            case SkServerUrl:
                //_MESG("WDBG: SkServerUrl caught, args: %s,   %s", iEvent->sArg1.c_str(), iEvent->sArg2.c_str());
                ui_onServerUpdate(iEvent->sArg1, iEvent->sArg2);
                skipEvents.insert(iEvent->ev);
                break;

            case SkUnlink:
                ui_onUnlinkEvent();
                skipEvents.insert(iEvent->ev);
                break;

            case SkReadProfile:
                skipEvents.insert(SkUserId);
                onObsProfileChange(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
                break;

            case SkSetWebRtc:
#if SIDEKICK_SET_WEBRTC
                ui_onSetWebRtc(iEvent->dwArg1);
#endif
                break;

            case SkRoomEv:
                _MESG("HTTP THREAD [RoomEvent]: %s", iEvent->sArg1.c_str());
                break;

            case SkNull:
                _MESG("HTTP THREAD [NullEvent]");
                break;
            }
        }
    }

    // Every 1500ms (6 cycles of 250ms timer events), check to see if the stream settings
    // for the current profile have been updated by OBS main UI (such as clicking Settings
    // and changing the service from Custom -> MyFreeCams WebRTC). If profile's config file
    // on disk is newer than our last file mod time cached, reparse the config and apply
    // any changes that were made (if applicable)
    //
    if ((s_nPulse % 6) == 0)
    {
        bool isWebRTC = false, isStreaming = false, isMfc = false, isCustom = false, profileChanged = false;
        
        SidekickActiveState curState = SkUninitialized;
        {
            auto lk     = g_ctx.sharedLock();
            curState    = g_ctx.activeState;
            isStreaming = g_ctx.isStreaming;
            isWebRTC    = g_ctx.isWebRTC;
            isCustom    = g_ctx.isCustom;
            isMfc       = g_ctx.isMfc;
        }

        if (g_ctx.cfg.checkProfileChanged())
        {
            _MESG("SVCDBG: Detected change in profile on disk!");
            profileChanged = true;
        }
        else
        {
            obs_service_t* pService = obs_frontend_get_streaming_service();
            if (pService)
            {
                const char* pszType = obs_service_get_output_type(pService);
                if (pszType)
                {
                    std::string sSvcOutputType(pszType);
                    if ( sSvcOutputType == "mfc_wowza_output")
                    {
                        if (!isWebRTC)
                        {
                            _MESG("SVCDBG: ** WebRTC Service activated on profile change **");
                            profileChanged = true;
                        }
                    }
                    else if ( isWebRTC )
                    {
                        _MESG("SVCDBG: ** WebRTC Service deactivated on profile change **");
                        profileChanged = true;
                    }
                    else if ( ! isMfc && curState != SkUnknownProfile )
                    {
                        _MESG("SVCDBG: ** Change to non-MFC service detected");
                        profileChanged = true;
                    }
                }
            }
        }

        // went from non-webrtc to webrtc, the reverse, changed to non-mfc service, or profile on disk changed?
        if (profileChanged)
            onObsProfileChange(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
    }
    s_nPulse++;
}


void showAccountLinkStatus(void)
{
    auto lk = g_ctx.sharedLock();
    std::string consoleMessage;
    if (g_ctx.isMfc)
    {
        if (g_ctx.isLinked)
        {
#if SIDEKICK_VERBOSE_CONSOLE
            if (g_ctx.isLoggedIn)
                consoleMessage = "<div style=\"font-size:14px;\"><b>Account Linked</b>! ModelWeb login found, you are now able to start streaming.</div>";
            else
                consoleMessage = "<div style=\"font-size:14px;\"><b>Account Linked</b>! Login to ModelWeb before attempting to stream.</div>";
#else
            if (g_ctx.isLoggedIn)
                consoleMessage = "<div style=\"font-size:14px;\">You are now able to start streaming.</div>";
            else
                consoleMessage = "<div style=\"font-size:14px;\">Login to ModelWeb before attempting to stream.</div>";
#endif

#if (defined(MFC_BROWSER_LOGIN) && (MFC_BROWSER_LOGIN > 0))
                    // Close the sidekick login browser panel if it's open and the browser panel code is active
                    CObsUtil::closeBrowserPanel("sidekick");
#endif
        }
        else
        {
#if SIDEKICK_VERBOSE_CONSOLE
            consoleMessage = "<div style=\"font-size:14px;\"><b>Account Unlinked</b>! Click on the <i>Link Sidekick</i> button to authenticate Sidekick.</div>";
#else
            consoleMessage = "<div style=\"font-size:14px;\">Click on the <b>Link Sidekick</b> button to authenticate with MyFreeCams.</div>";
#endif
        }
    }
    else
    {
        consoleMessage =    "<div style=\"font-size:14px;\">Select or create a <b>MyFreeCams profile</b> to activate Sidekick.</div>"                                     \
                            "<div style=\"margin-top:20px;font-size:14px;\">Either load a saved profile configured with a MyFreeCams service, " \
                            "or create a new profile<br/>and choose <b>MyFreeCams RTMP</b> or <b>MyFreeCams WebRTC</b> "                        \
                            "as the streaming service.</div>";
    }

#if SIDEKICK_VERBOSE_CONSOLE
    ui_appendConsoleMsg(consoleMessage);
#else
    ui_replaceConsoleMsg(consoleMessage);
#endif
}


void onObsProfileChange(obs_frontend_event eventType)
{
    UNREFERENCED_PARAMETER(eventType);

    auto lk = g_ctx.sharedLock();

    if (!servicesUpdated)
        loadServices();

    bool isWebRTC = false, isRtmp = false, isCustom = false, isMfc = false; 
    std::string svcName, sOldProfile(g_ctx.profileName), sUser, streamUrl, sProt("unknown");
    SidekickActiveState curState = g_ctx.activeState;
    static bool s_firstProfileLoad = true;
    size_t updatesSent = 0;

    if (g_ctx.sm_edgeSock)
        updatesSent = g_ctx.sm_edgeSock->m_updatesSent;

    g_ctx.clear(false);

    QTextEdit* pCon = (QTextEdit*)g_ctx.getConsole();
    if (pCon)
        pCon->clear();

    char* pszProfile = obs_frontend_get_current_profile();
    if (pszProfile != nullptr)
    {
        g_ctx.profileName = pszProfile;
        bfree(pszProfile);
    }

    bool profileChanged = (s_firstProfileLoad || g_ctx.profileName != sOldProfile);

    CObsUtil::getCurrentSetting("service", svcName);
    CObsUtil::getCurrentSetting("server", streamUrl);

    if (svcName == "MyFreeCams WebRTC")
    {
        isMfc = true;
        isRtmp = false;
        isCustom = false;
        isWebRTC = true;
        sProt = "WebRTC";
    }
    else if (svcName == "MyFreeCams RTMP" || svcName == "MyFreeCams")
    {
        isMfc = true;
        isRtmp = true;
        isCustom = false;
        isWebRTC = false;
        sProt = "RTMP";
    }
    else if ((svcName == "Custom" || svcName == "") && streamUrl.find(".myfreecams.com/NxServer") != string::npos)
    {
        isMfc = true;
        isRtmp = true;
        isCustom = true;
        isWebRTC = false;
        sProt = "RTMP";
    }
    else
    {
        _MESG("Unknown service svcName: %s   / streamUrl: %s  ", svcName.c_str(), streamUrl.c_str());
        isMfc = false;
        isRtmp = false;
        isWebRTC = false;
        svcName = "Unavailable";
    }

    // Read any plugin config for this profile if it has changed since last profile loaded (or first profile to load)
    if (profileChanged)
    {
        if (!isMfc)
        {
            // clear sidekick config if we switched to a non-mfc profile
            g_ctx.clear(false);
        }
        else g_ctx.importProfileConfig();
    }


#if SIDEKICK_VERBOSE_CONSOLE
    if (profileChanged)
    {
        std::string sMsg;
        if (isMfc)
            stdprintf(sMsg, "%s <i><b>%s</b></i>, using <b>%s</b>", firstProfileCheck ? "Switching to" : "Loading profile", g_ctx.profileName.c_str(), sProt.c_str());
        else
            stdprintf(sMsg, "%s <i>%s</i> (<i><b>Sidekick disabled</b></i>)", firstProfileCheck ? "Switching to" : "Loading profile", g_ctx.profileName.c_str());
        ui_appendConsoleMsg(sMsg);
    }
#endif

    firstProfileCheck = true;
    g_ctx.cfg.set("serviceType", svcName);

    // read config if we are webRTC, clear config if we are not
    if (isWebRTC)
    {
        g_ctx.isWebRTC  = true;
        g_ctx.isMfc     = true;
        g_ctx.isRTMP    = false;
        g_ctx.isCustom  = false;
    }
    else if (isRtmp)
    {
        // we are an MFC rtmp stream if we appear to be attached to an mfc video server
        g_ctx.isWebRTC  = false;
        g_ctx.isRTMP    = true;
        g_ctx.isCustom  = isCustom;
        g_ctx.isMfc     = true;
    }
    else
    {
        g_ctx.isWebRTC  = false;
        g_ctx.isRTMP    = false;
        g_ctx.isMfc     = false;    // unknown service type, so dont set isMfc true even if it matches url pattern
        _MESG("** Unknown Service detected on profile change **");
    }

    sUser.clear();

    if ( ! g_ctx.isMfc )
    {
        if (g_ctx.activeState != SkUnknownProfile)
            g_ctx.activeState = SkUnknownProfile;
    }
    
    if (s_firstProfileLoad)
    {
#if SIDEKICK_CONSOLE
        QTimer::singleShot(500, qApp, [=]()
        {
            if (sidekick_prop)
            {
                sidekick_prop->show();
                sidekick_prop->setVisible(true);
            }
        });
#endif
        s_firstProfileLoad = false;
    }

    // write any updated info back to plugin config for this profile if any of it changed or is new, and an mfc profile
    if (g_ctx.isMfc)
    {
        g_ctx.cfg.writeProfileConfig();

        // if no update was automatically sent due to triggered changes in profile
        if (g_ctx.sm_edgeSock && g_ctx.sm_edgeSock->m_updatesSent == updatesSent)
        {
            // force sending an update now to make sure model's agent monitor knows current profile/status
            g_ctx.sm_edgeSock->sendUpdate();
        }
    }

    if (pMFCDock)
        pMFCDock->relabelPropertiesText();

    if (sidekick_prop)
        sidekick_prop->relabelPropertiesText();

    showAccountLinkStatus();

    // start/stop polling if anything changed as part of this profile event
    g_thread.setCmd(THREADCMD_PROFILE);
}


void setupSidekickUI(void)
{
#if SIDEKICK_CONSOLE
    if (g_ctx.getConsole() == nullptr)
    {
        QMainWindow* pWindow = (QMainWindow*)obs_frontend_get_main_window();
        if (pWindow)
        {
            if (sidekick_prop)
            {
                // Setup multi-line edit control for log info
                QTextEdit* pConsole = sidekick_prop->createConsole();
                g_ctx.setConsole(pConsole);
            }
            else _MESG("sidekick_prop dialog ptr window is NULL; cannot setup sidekick UI!");
        }
    }
#endif
}


void onObsEvent(obs_frontend_event eventType, void* pCtx)
{
    UNREFERENCED_PARAMETER(pCtx);

    if (eventType == OBS_FRONTEND_EVENT_STREAMING_STARTING)
    {
        auto lk = g_ctx.sharedLock();

        if ( g_ctx.isMfc )
        {
            if (   g_ctx.isLinked                           // model account linked?
                && g_ctx.isLoggedIn                         // modelweb logged in?
                && g_ctx.activeState >= SkStreamStopping)   // state is either stopping or stopped?
            {
                g_ctx.activeState = SkStreamStarting;       // Change state to starting
            }
            else
            {
                _MESG(  "OBS_FRONTEND_EVENT_STREAMING_STARTING; linked: %s, logged in: %s, activeState: %u",
                        g_ctx.isLinked ? "true" : "false",
                        g_ctx.isLoggedIn ? "true" : "false",
                        (unsigned int)g_ctx.activeState);
            }
        }
        else _MESG("OBS_FRONTEND_EVENT_STREAMING_STARTING, but g_ctx.isMfc is false, so skipped");
    }
    else if (eventType == OBS_FRONTEND_EVENT_STREAMING_STARTED)
    {
        auto lk = g_ctx.sharedLock();

        if ( g_ctx.isMfc )
        {
            if ( ! g_ctx.isLinked )
            {
                QTimer::singleShot(500, qApp, [=]()
                {
                    obs_frontend_streaming_stop();
                    ui_SidekickMsgBox(  g_ctx.isWebRTC ? "Unable to start WebRTC Stream" : "Unable to start RTMP Stream",
                                        "You must first link Sidekick to your MFC model account before starting a stream.");
                });
            }
            else if ( ! g_ctx.isLoggedIn )
            {
                QTimer::singleShot(500, qApp, [=]()
                {
                    obs_frontend_streaming_stop();
                    ui_SidekickMsgBox(  g_ctx.isWebRTC ? "Unable to start WebRTC Stream" : "Unable to start RTMP Stream",
                                        "You must first login to ModelWeb and set your video setting to 'External Broadcaster'.");
                });
            }
            else
            {
                _MESG("DBG: Valid sid[%u]; marking streaming as started on profile:%s to server:%s for %s",
                      g_ctx.cfg.getInt("sid"),
                      g_ctx.profileName.c_str(),
                      g_ctx.serverName.c_str(),
                      g_ctx.cfg.getString("username"));

                // When streaming starts, pause polling agentSvc.php
                g_ctx.onStartStreaming();
                g_thread.setCmd(THREADCMD_STREAMSTART);

#if SIDEKICK_ENABLE_VIRTUALCAM
#if (LIBOBS_API_MAJOR_VER > 26)
                obs_frontend_start_virtualcam();
#else
                QMainWindow* main = (QMainWindow*)obs_frontend_get_main_window();
                QMetaObject::invokeMethod(main, "StartVirtualCam");
#if MFC_AGENT_EDGESOCK
                if (g_ctx.sm_edgeSock)
                    g_ctx.sm_edgeSock->sendVirtualCameraState(true);
#endif
#endif
#endif
            }
        }
        else _MESG( "OBS_FRONTEND_EVENT_STREAMING_STARTED; but not isMfc so ignoring");
    }
    else if (eventType == OBS_FRONTEND_EVENT_STREAMING_STOPPED)
    {
        auto lk = g_ctx.sharedLock();
        if ( g_ctx.isMfc )
        {
            _MESG(  "DBG: Streaming ended, marking stream as stopped for sid[%u] on profile:%s to server:%s for %s",
                    g_ctx.cfg.getInt("sid"),
                    g_ctx.profileName.c_str(),
                    g_ctx.serverName.c_str(),
                    g_ctx.cfg.getString("username"));

            // When streaming stops, we resume polling agentSvc.php
            g_ctx.onStopStreaming();
            g_thread.setCmd(THREADCMD_STREAMSTOP);

#if SIDEKICK_ENABLE_VIRTUALCAM
#if (LIBOBS_API_MAJOR_VER > 26)
            obs_frontend_stop_virtualcam();
#else
            QMainWindow* main = (QMainWindow*)obs_frontend_get_main_window();
            QMetaObject::invokeMethod(main, "StopVirtualCam");
#if MFC_AGENT_EDGESOCK
            if (g_ctx.sm_edgeSock)
                g_ctx.sm_edgeSock->sendVirtualCameraState(false);
#endif
#endif
#endif
        }
    }
    else if (eventType == OBS_FRONTEND_EVENT_PROFILE_CHANGED)
    {
        onObsProfileChange(eventType);
    }
    else if (eventType == OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED)
    {
        onObsProfileChange(eventType);
    }
    else if (eventType == OBS_FRONTEND_EVENT_EXIT)
    {
        g_thread.setCmd(THREADCMD_SHUTDOWN);
    }
#if (MFC_AGENT_EDGESOCK && LIBOBS_API_MAJOR_VER > 26)
    else if (eventType == OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED)
    {
        if (g_ctx.sm_edgeSock)
            g_ctx.sm_edgeSock->sendVirtualCameraState(true);
    }
    else if (eventType == OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED)
    {
        if (g_ctx.sm_edgeSock)
            g_ctx.sm_edgeSock->sendVirtualCameraState(false);
    }
#endif
}


const char* MapObsEventType(enum obs_frontend_event eventType)
{
    static char s_szObsEventType[128] = { '\0' };
    switch (eventType)
    {
    case OBS_FRONTEND_EVENT_STREAMING_STARTING:             return "OBS_FRONTEND_EVENT_STREAMING_STARTING";
    case OBS_FRONTEND_EVENT_STREAMING_STARTED:              return "OBS_FRONTEND_EVENT_STREAMING_STARTED";
    case OBS_FRONTEND_EVENT_STREAMING_STOPPING:             return "OBS_FRONTEND_EVENT_STREAMING_STOPPING";
    case OBS_FRONTEND_EVENT_STREAMING_STOPPED:              return "OBS_FRONTEND_EVENT_STREAMING_STOPPED";
    case OBS_FRONTEND_EVENT_RECORDING_STARTING:             return "OBS_FRONTEND_EVENT_RECORDING_STARTING";
    case OBS_FRONTEND_EVENT_RECORDING_STARTED:              return "OBS_FRONTEND_EVENT_RECORDING_STARTED";
    case OBS_FRONTEND_EVENT_RECORDING_STOPPING:             return "OBS_FRONTEND_EVENT_RECORDING_STOPPING";
    case OBS_FRONTEND_EVENT_RECORDING_STOPPED:              return "OBS_FRONTEND_EVENT_RECORDING_STOPPED";
    case OBS_FRONTEND_EVENT_SCENE_CHANGED:                  return "OBS_FRONTEND_EVENT_SCENE_CHANGED";
    case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:             return "OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED";
    case OBS_FRONTEND_EVENT_TRANSITION_CHANGED:             return "OBS_FRONTEND_EVENT_TRANSITION_CHANGED";
    case OBS_FRONTEND_EVENT_TRANSITION_STOPPED:             return "OBS_FRONTEND_EVENT_TRANSITION_STOPPED";
    case OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED:        return "OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED";
    case OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED:    return "OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED";
    case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:       return "OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED";
    case OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED:  return "OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED";
    case OBS_FRONTEND_EVENT_PROFILE_CHANGED:                return "OBS_FRONTEND_EVENT_PROFILE_CHANGED";
    case OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED:           return "OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED";
    case OBS_FRONTEND_EVENT_EXIT:                           return "OBS_FRONTEND_EVENT_EXIT";
    case OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED:            return "OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED";
    case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING:         return "OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING";
    case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED:          return "OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED";
    case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING:         return "OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING";
    case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED:          return "OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED";
    case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:            return "OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED";
    case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:           return "OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED";
    case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:          return "OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED";
    case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:       return "OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP";
    case OBS_FRONTEND_EVENT_FINISHED_LOADING:               return "OBS_FRONTEND_EVENT_FINISHED_LOADING";
    case OBS_FRONTEND_EVENT_RECORDING_PAUSED:               return "OBS_FRONTEND_EVENT_RECORDING_PAUSED";
    case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:             return "OBS_FRONTEND_EVENT_RECORDING_UNPAUSED";
#if (LIBOBS_API_MAJOR_VER > 26)
    case OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED:             return "OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED";
    case OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED:             return "OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED";
#endif
    }

    snprintf(s_szObsEventType, sizeof(s_szObsEventType), "OBS_UnknownEvent_%u", eventType);
    return s_szObsEventType;
}


//---------------------------------------------------------------------------
// obs_module_unload
//
// stop broadcast thread and shutdown cef.
void obs_module_unload()
{
    _TRACE("obs_module_unload called, stopping thread");
    g_thread.Stop(-1);
    _TRACE("%s OBS Plugin has been Unloaded", __progname);

    CObsUtil::TerminateMFCLogin();
}


int ui_SidekickMsgBox(const char* pszTitle, const char* pszBody, int nButtons, QWidget* pParent)
{
    int retVal = -1;
    if (pszBody)
    {
        std::string sMsg(pszBody);
        ui_appendConsoleMsg(sMsg);

        if (pszTitle)
        {
            QMessageBox mb(QMessageBox::Information, pszTitle, pszBody,
                           QMessageBox::StandardButtons(nButtons), pParent);

            mb.setButtonText( QMessageBox::Ok, QStringLiteral("OK") );
            mb.setDefaultButton( QMessageBox::Ok );
            retVal = mb.exec();
        }
    }

    return retVal;
}


std::unique_ptr<FcsWebsocket> proxy_createFcsWebsocket(void)
{
    return createFcsWebsocket();
}


void proxy_blog(int nLevel, const char* pszMsg)
{
    static char s_szModuleName[MAX_PATH] = { '\0' };
    struct timeval tvNow;
    struct tm tmNow;
    std::string sLog;

#ifdef _INCLUDE_MODULE_NAME_
#ifdef _WIN32
    if (s_szModuleName[0] == '\0')
    {
        HMODULE phModule = NULL;
        char szFile[MAX_PATH];

        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)&g_dummyCharVal, &phModule))
        {

            GetModuleFileNameA(phModule, szFile, sizeof(szFile));
            char* pch = strrchr(szFile, '\\');
            fcs_strlcpy(s_szModuleName, pch ? pch + 1 : szFile, sizeof(s_szModuleName));
        }
        else fcs_strlcpy(s_szModuleName, "error-getting-module-handle", sizeof(s_szModuleName));
    }

    gettimeofday(&tvNow, NULL);
    _localtime32_s(&tmNow, (__time32_t*)&tvNow.tv_sec);
#else
    if (s_szModuleName[0] == '\0')
    {
        // TODO: get module handle name for mac
        fcs_strlcpy(s_szModuleName, "module-handle", sizeof(s_szModuleName));
    }

    gettimeofday(&tvNow, NULL);
    struct tm*  pCt = localtime(&tvNow.tv_sec);
    if (pCt)    tmNow = *pCt;
    else        memset(&tmNow, 0, sizeof(tmNow));
#endif
#else
    
#ifdef _WIN32
    gettimeofday(&tvNow, NULL);
    _localtime32_s(&tmNow, (__time32_t*)&tvNow.tv_sec);
#else
    gettimeofday(&tvNow, NULL);
    struct tm*  pCt = localtime(&tvNow.tv_sec);
    if (pCt)    tmNow = *pCt;
    else        memset(&tmNow, 0, sizeof(tmNow));
#endif
#endif

    stdprintf(sLog,
              "[%s%02d-%02d %02d:%02d:%02d.%04d] %s",
              s_szModuleName,
              tmNow.tm_mon + 1, tmNow.tm_mday,
              tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec,
              tvNow.tv_usec / 1000,
              pszMsg);

    blog(nLevel, "%s", sLog.c_str());
}

// MFC-OBSPluginStart.cpp : Defines the exported functions for the DLL application.
//

#include "ObsUpdater.h"

// OBS Includes
#include <libobs/obs-module.h>
#include <obs-frontend-api.h>
#include <libobs/util/config-file.h>
#include <libobs/util/platform.h>
#include <libobs/util/threading.h>

// MFC Includes
#include <libfcs/Log.h>
#include <libfcs/fcslib_string.h>
#include <libfcs/MfcTimer.h>

// solution includes
#include <libPlugins/Portable.h>
#include <libPlugins/PluginParameterBlock.h>
#include <libPlugins/SysParam.h>
#include <libPlugins/MFCConfigConstants.h>
#include <libPlugins/ObsUtil.h>
#include <libPlugins/EdgeChatSock.h>

//qt
#include <QtWidgets/QMainWindow>
#include <QMessageBox>
#include <QString>

int ui_SidekickMsgBox(const char* pszTitle, const char* pszBody, int nButtons = QMessageBox::Ok, QWidget* pParent = nullptr);

#ifndef _WIN32
#include <sys/time.h>
#else
#include <libfcs/UtilCommon.h>
#endif

// project Includes
#include "ManifestFile.h"
#include "FileUpdater.h"

#ifdef _WIN32
char* __progname = UPDATER_FILENAME;
#else
const char *__progname = UPDATER_FILENAME;
#endif

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE(__progname, "en-US")

#ifndef MFC_LOG_LEVEL
#define MFC_LOG_LEVEL ILog::LogLevel::MAX_LOGLEVEL
#endif

#ifndef MFC_LOG_OUTPUT_MASK
#define MFC_LOG_OUTPUT_MASK 10
#endif

CFileUpdater upd;
extern char g_dummyCharVal; // Part of MfcLog.cpp in MFClibfcs.lib

//---------------------------------------------------------------------------
// obs_module_load
//
// load module for ObsUpdater Plugin
bool obs_module_load(void)
{
    std::string s = OBS_INSTALL_PREFIX;
    //MessageBoxA(NULL, "obsupdater startup", "obsupdater", MB_OK);
    Log::Setup( CObsUtil::getLogPath() );
    Log::AddOutputMask(MFC_LOG_LEVEL, MFC_LOG_OUTPUT_MASK);
    _TRACE("%s OBS Plugin has been loaded", __progname);

#if 0
#ifdef WIN32
    // add new module path so we can put our
    // broadcast plugin into a directory that
    // we can access.
    string sRootPath( CObsUtil::getInstallPath() );
    string sDataDir( CObsUtil::AppendPath(sRootPath, "data") );
    string sBinDir( CObsUtil::AppendPath(sRootPath, "bin") );

    //obs_add_module_path(sBinDir.c_str(), sDataDir.c_str());

#endif
#endif

    // getDataPath() for module should always already exist?
    //char *cache_dir = CObsUtil::getDataPath("");
    // make sure the cache directory exists.
    //os_mkdir(cache_dir);

#if MFC_NO_UPDATES == 0
    upd.ProcessCachedFiles();
    upd.doUpdate(0);
#else
    _TRACE("Updates disabled");
#endif
   // Make sure that Sidekick's install bindir has a current copy of libcurl.dll,
    return true;
}

void obs_module_unload()
{
    upd.Stop();
    _TRACE("%s OBS Plugin has been xxagain Unloaded", __progname);
}

void ui_onSessionUpdate(int nChange, uint32_t nCurSid, uint32_t nNewSid)
{
    _MESG("UITHread queue callback ignored in Updater");
    // Queue a call to UI thread to update them about sessionid change
    //QTimer::singleShot(100, App(), [=]()
    //{
    // initialProfileCheck();
    //});
}

int ui_SidekickMsgBox(const char* pszTitle, const char* pszBody, int nButtons, QWidget* pParent)
{
    QMessageBox mb(QMessageBox::Information,
                   pszTitle,
                   pszBody,
                   QMessageBox::StandardButtons( nButtons ),
                   pParent);

    mb.setButtonText( QMessageBox::Ok, "OK" );
    mb.setDefaultButton( QMessageBox::Ok );
    return mb.exec();
}

FcsWebsocket* proxy_createFcsWebsocket(void)
{
    return NULL;
}

void proxy_blog(int nLevel, const char* pszMsg)
{
    char szModuleName[MAX_PATH];
    struct timeval tvNow;
    struct tm tmNow;
    string sLog;

#ifdef _WIN32
    //GetModuleFileNameExA(GetCurrentProcess(), GetModuleHandle(NULL), szModuleName, sizeof(szModuleName));
    HMODULE phModule = NULL;
    if ( GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR) &g_dummyCharVal,
        &phModule) )
    {
        char szFile[MAX_PATH];
        GetModuleFileNameA(phModule, szFile, sizeof(szFile));
        char* pch = strrchr(szFile, '\\');
        if (pch)
            fcs_strlcpy(szModuleName, pch + 1, sizeof(szModuleName));
        else fcs_strlcpy(szModuleName, szFile, sizeof(szModuleName));
    }
    else fcs_strlcpy(szModuleName, "error-getting-module-handle", sizeof(szModuleName));
#else
    fcs_strlcpy(szModuleName, "module-handle", sizeof(szModuleName));
#endif

    gettimeofday(&tvNow, NULL);

#ifdef _WIN32
    _localtime32_s(&tmNow, (__time32_t*)&tvNow.tv_sec);
#else
    struct tm* pCt = localtime(&tvNow.tv_sec);
    if (pCt)
        tmNow = *pCt;
    else
        memset(&tmNow, 0, sizeof(tmNow));
#endif

    stdprintf(sLog,
              "[%s %02d-%02d %02d:%02d:%02d.%04d] %s",
              szModuleName,
              tmNow.tm_mon + 1, tmNow.tm_mday,
              tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec,
              tvNow.tv_usec / 1000,
              pszMsg);

    blog(nLevel, "%s", sLog.c_str());
}

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

#include <map>
#include <regex>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tlhelp32.h>
#else
#include <climits>
#include <cstdio>
#include <cstring>
#include <pwd.h>
#include <spawn.h>
#include <unistd.h>
#endif

// obs
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>
#include <util/platform.h>
#include <util/threading.h>
#include <util/darray.h>

// solution
#include <libfcs/fcslib_string.h>
#include <libfcs/Log.h>

// project
#include "ObsUtil.h"
#include "Portable.h"

#ifdef _WIN32
static const char* module_bin[] = { "../../obs-plugins/" BIT_STRING };  // from obs-windows.c
#else
static const char* module_bin[] = { "../obs-plugins", OBS_INSTALL_PREFIX "obs-plugins" };  // from obs-cocoa.m
#endif

using std::string;

//---------------------------------------------------------------------------
// isValidConfig
//
// is the obs profile config valid?  Allows us to cheat and determine if
// we have a valid obs connection yet.
bool CObsUtil::isValidConfig()
{
    config_t* pConfigProfile = obs_frontend_get_profile_config();
    if (pConfigProfile)
    {
        bfree(pConfigProfile);
        return true;
    }
    return false;
}


//---------------------------------------------------------------------------
// setConfig
//
// various threadsafe methods to set a value into the obs profile
bool CObsUtil::setConfig(const char* pSection, const char* pVariable, const char* value)
{
    bool bRv = false;
    try
    {
        config_t* pConfigProfile = obs_frontend_get_profile_config();
        if (pConfigProfile)
        {
            config_set_string(pConfigProfile, pSection, pVariable, value);
            bRv = true;
            bfree(pConfigProfile);
        }
    }
    catch (...)
    {
        _TRACE("Unknown Exception Caught");
    }
    return bRv;
}


//---------------------------------------------------------------------------
// setConfig
//
// various threadsafe methods to set a value into the obs profile
bool CObsUtil::setConfig(const char* pSection, const char* pVariable, const string& value)
{
    bool bRv = false;
    try
    {
        config_t* pConfigProfile = obs_frontend_get_profile_config();
        if (pConfigProfile)
        {
            config_set_string(pConfigProfile, pSection, pVariable, value.c_str());
            bRv = true;
            bfree(pConfigProfile);
        }
    }
    catch (...)
    {
        _TRACE("Unknown Exception Caught");
    }
    return bRv;
}


//---------------------------------------------------------------------------
// setConfig
//
// various threadsafe methods to set a value into the obs profile
bool CObsUtil::setConfig(const char* pSection, const char* pVariable, int value)
{
    bool bRv = false;
    try
    {
        config_t* pConfigProfile = obs_frontend_get_profile_config();
        if (pConfigProfile)
        {
            config_set_int(pConfigProfile, pSection, pVariable, value);
            bRv = true;
            bfree(pConfigProfile);
        }
    }
    catch (...)
    {
        _TRACE("Unknown Exception Caught");
    }
    return bRv;
}


//---------------------------------------------------------------------------
// setConfig
//
// various threadsafe methods to set a value into the obs profile
bool CObsUtil::setConfig(const char* pSection, const char* pVariable, bool value)
{
    bool bRv = false;
    try
    {
        config_t* pConfigProfile = obs_frontend_get_profile_config();
        if (pConfigProfile)
        {
            config_set_bool(pConfigProfile, pSection, pVariable, value);
            bRv = true;
            bfree(pConfigProfile);
        }
    }
    catch (...)
    {
        _TRACE("Unknown Exception Caught");
    }
    return bRv;
}


//---------------------------------------------------------------------------
// GetConfigOrDefault
//
// Get a value from the obs profile.  If obs is not valid or value doesn't
// exist return the default value.
const string CObsUtil::getConfigOrDefault(const char* pSection, const char* pVariable, const char* pDefault)
{
    string sVal(pDefault != nullptr ? pDefault : "");
    const char* pBuf;
    try
    {
        config_t* pConfigProfile = obs_frontend_get_profile_config();
        if (pConfigProfile)
        {
            if ((pBuf = config_get_string(pConfigProfile, pSection, pVariable)) != nullptr)
                sVal = pBuf;
            bfree(pConfigProfile);
        }
    }
    catch (...)
    {
        _TRACE("Unknown Exception Caught");
    }
    return sVal;
}


//---------------------------------------------------------------------------
// GetConfigOrDefault
//
// Get a value from the obs profile.  If obs is not valid or value doesn't
// exist return the default value.
int CObsUtil::getConfigOrDefault(const char* pSection, const char* pVariable, int nDefault)
{
    int nRv = nDefault;
    try
    {
        config_t* pConfigProfile = obs_frontend_get_profile_config();
        if (pConfigProfile)
        {
            int nV = static_cast<int>(config_get_int(pConfigProfile, pSection, pVariable));
            if (nV != 0)
                nRv = nV;
            bfree(pConfigProfile);
        }
    }
    catch (...)
    {
        _TRACE("Unknown Exception Caught");
    }
    return nRv;
}


//---------------------------------------------------------------------------
// GetConfigOrDefault
//
// Get a time_t value from the obs profile.  If obs is not valid or value doesn't
// exist return the default value.
time_t CObsUtil::getConfigOrDefault(const char* pSection, const char* pVariable, time_t nDefault)
{
    time_t nRv = nDefault;
    try
    {
        config_t* pConfigProfile = obs_frontend_get_profile_config();
        if (pConfigProfile)
        {
            time_t nV = static_cast<time_t>(config_get_int(pConfigProfile, pSection, pVariable));
            if (nV != 0)
                nRv = nV;
            bfree(pConfigProfile);
        }
    }
    catch (...)
    {
        _TRACE("Unknown Exception Caught");
    }
    return nRv;
}


//
// HACK: this uses obs internals from config-file.c, and should probably be rewritten to use whatever the correct
// format is for enumerating key value pairs in a config block!!
//
size_t CObsUtil::getConfigItems(const string& sBlock, std::vector< std::pair<string,string> > &vItems)
{
    typedef struct
    {
        char*           file;
        struct darray   sections;   /* struct config_section */
        struct darray   defaults;   /* struct config_section */
        pthread_mutex_t mutex;
    } _obs_config_data;

    typedef struct
    {
        char*           name;
        struct darray   items;      /* struct config_item */
    } _obs_config_section;

    typedef struct
    {
        char* name;
        char* value;
    } _obs_config_item;

    _obs_config_data* pConfig;
    _obs_config_section* pSec;
    _obs_config_item* pItem;

    vItems.clear();

    config_t* pConfigProfile = obs_frontend_get_profile_config();
    if (pConfigProfile)
    {
        pConfig = (_obs_config_data*)pConfigProfile;
        for (size_t i = 0; i < pConfig->sections.num; i++)
        {
            pSec = (_obs_config_section*)darray_item(sizeof(_obs_config_section), &pConfig->sections, i);
            if (CObsUtil::strcmpi(pSec->name, sBlock.c_str()) == 0)
            {
                for (size_t j = 0; j < pSec->items.num; j++)
                {
                    pItem = (_obs_config_item*)darray_item(sizeof(_obs_config_item), &pSec->items, j);
                    if (pItem)
                    {
                        if (pItem->name)
                        {
                            std::pair<string,string> keyVal( pItem->name, (pItem->value ? pItem->value : "") );
                            vItems.push_back( keyVal );
                        }
                        else _TRACE("skipping config item %zu for block %s with null name", j, sBlock.c_str());
                    }
                    else _TRACE("failed to retreive config item %zu in block %s", j, sBlock.c_str());
                }
            }
        }
        bfree(pConfigProfile);
    }
    else _TRACE("failed to get frontend profile config");

    return vItems.size();
}


bool CObsUtil::removeConfig(const string& sBlock, const string& sVar)
{
    bool removed = false;
    try
    {
        config_t* pConfigProfile = obs_frontend_get_profile_config();
        if (pConfigProfile)
        {
            removed = config_remove_value(pConfigProfile, sBlock.c_str(), sVar.c_str());
            bfree(pConfigProfile);
        }
    }
    catch (...)
    {
        _TRACE("Unknown Exception Caught");
    }
    return removed;
}


//---------------------------------------------------------------------------
// GetConfigOrDefault
//
// Get a value from the obs profile.  If obs is not valid or value doesn't
// exist return the default value.
bool CObsUtil::getConfigOrDefault(const char* pSection, const char* pVariable, bool bDefault)
{
    bool bRv = bDefault;
    try
    {
        config_t* pConfigProfile = obs_frontend_get_profile_config();
        if (pConfigProfile)
        {
            bRv = config_get_bool(pConfigProfile, pSection, pVariable);
            bfree(pConfigProfile);
        }
    }
    catch (...)
    {
        _TRACE("Unknown Exception Caught");
    }
    return bRv;
}


//---------------------------------------------------------------------------
// AppendPath
//
// Append a file or dir name to a path and handle the trailing "/"
string CObsUtil::AppendPath(const string& sPath, const string& sFile)
{
    string retVal(sPath);
    if (!retVal.empty() && retVal.back() != '/' && retVal.back() != '\\')
    {
        retVal.append("/");
    }
    return retVal.append(sFile);
}


//---------------------------------------------------------------------------
// FindPluginDirectory
//
// find the plugin dir and return a valid path
string CObsUtil::FindPluginDirectory(void)
{
    char szPath[_MAX_PATH + 1] = { '\0' };
    char* pFile = nullptr;

#ifdef _WIN32
    ::GetCurrentDirectoryA(sizeof(szPath), szPath);
    string sPath( CObsUtil::AppendPath(szPath, "../../obs-plugins/") );  // hard coded by obs (see obs-windows.c)
    sPath = CObsUtil::AppendPath(sPath, BIT_STRING);
    //_MESG("PATHDBG: FindPluginDirectory()::GetCurrentDirectoryA() + ../../obs-plugins => %s", sPath.c_str());

    DWORD dw = GetFullPathNameA(sPath.c_str(), sizeof(szPath), szPath, &pFile);
    if (0 == dw)
        _MESG("PATHDBG: Obs Plugin Directory not found: %s", sPath.c_str());
#else
    getcwd(szPath, sizeof(szPath));
    string sPath( CObsUtil::AppendPath(szPath, "../obs-plugins") );  // hard coded by obs (see obs-cocoa.m)

    if ((pFile = realpath(sPath.c_str(), szPath)) == nullptr)
    {
        string sPathNotFound = sPath;
        getcwd(szPath, sizeof(szPath));
        sPath = CObsUtil::AppendPath(szPath, "../Plugins");  // OBS 24.0.5+
        if ((pFile = realpath(sPath.c_str(), szPath)) == nullptr)
        {
            string sPathNotFound2 = sPath;
            sPath = "/Library/Application Support/obs-studio/plugins";
            if ((pFile = realpath(sPath.c_str(), szPath)) == nullptr)
            {
                _MESG("Obs Plugin Directory not found: %s", sPathNotFound.c_str());
                _MESG("Obs Plugin Directory not found: %s", sPathNotFound2.c_str());
                _MESG("Obs Plugin Directory not found: %s", sPath.c_str());
            }
        }
    }
#endif

    //_MESG("PATHDBG: FindPluginDirectory(): %s", szPath);
    return string(szPath);
}


//---------------------------------------------------------------------------
// getFilenameCachePath
//
// get the config path for this file.
const string CObsUtil::getFilenameCachePath(const string& sFile)
{
    string sRv;
    char* p = obs_module_file(sFile.c_str());
    if (nullptr == p)
    {
        // if local_dir is null, ../data/plugins/ObsUpdater doesn't exist.
        p = obs_module_config_path(sFile.c_str());
    }
    sRv = p;
    bfree(p);
    return sRv;
}


//---------------------------------------------------------------------------
// getFilenamePluginPath
//
// get filename plugin path
const string CObsUtil::getFilenamePluginPath(const string& sFile)
 {
    string sDir( CObsUtil::FindPluginDirectory() );
    string::reverse_iterator itr = sDir.rbegin();

    if (itr != sDir.rend())
    {
        if (*itr != '\\' && *itr != '/')
            sDir += "/";
    }
    sDir += sFile;

    _MESG("PATHDBG: getFilenamePluginPath(): %s", sDir.c_str());
    return sDir;
}


//---------------------------------------------------------------------------
// getProfilePath
//
// get the profile path
const string CObsUtil::getProfilePath()
{
    char profiles_path[_MAX_PATH + 1] = { '\0' };

    string sProfile("obs-studio/basic/profiles/");
    string sProfileDir( config_get_string(obs_frontend_get_global_config(), "Basic", "ProfileDir") );
    sProfile += sProfileDir;

    // translate to a formatted file name like obs.
    os_get_config_path(profiles_path, sizeof(profiles_path), sProfile.c_str());
    sProfile = profiles_path;

    //_MESG("PATHDB: getProfilePath(): %s ", sProfile.c_str());
    return sProfile;
}


const string CObsUtil::getLogPath()
{
    string sLogPath( CObsUtil::FindPluginDirectory() );
#ifdef UNUSED_CODE
    obs_module_t* pModule = obs_current_module();
    sLogPath = obs_module_get_config_path(pModule, "");

    if (!os_file_exists(sLogPath.c_str()))
        os_mkdir(sLogPath.c_str());
    sLogPath = obs_module_get_config_path(pModule, "Log");;
#endif
    sLogPath = CObsUtil::AppendPath(sLogPath, "logs");

    if (!os_file_exists(sLogPath.c_str()))
        os_mkdir(sLogPath.c_str());

    sLogPath += "/";

    //_MESG("PATHDBG: getLogPath() ret %s ", sLogPath.c_str());
    return sLogPath;
}


#ifdef _WIN32
DWORD getProcess()
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return(0);

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        string s(stderror(GetLastError()));
        _TRACE("Process32First failed: %s", s.c_str());  // show cause of failure
        CloseHandle(hProcessSnap);  // clean the snapshot object
        return(0);
    }

    DWORD nMyProcess = GetCurrentProcessId();
    DWORD rv = 0;
    do
    {
        char buf[_MAX_PATH + 1] = { 0 };
        wcstombs(buf, pe32.szExeFile, _MAX_PATH);

        if (pe32.th32ParentProcessID == nMyProcess && strcmp(buf, MFC_CEF_APP_EXE_NAME) == 0)
            rv = pe32.th32ProcessID;
    } while (Process32Next(hProcessSnap, &pe32) && 0 == rv);

    CloseHandle(hProcessSnap);
    return(rv);
}


BOOL CALLBACK EnumWindowsProc(HWND windowHandle, LPARAM lParam)
{
    DWORD searchedProcessId = (DWORD)lParam;  // process ID searcheed for (passed from BringToForeground as lParam)
    DWORD windowProcessId = 0;
    GetWindowThreadProcessId(windowHandle, &windowProcessId);
    if (searchedProcessId == windowProcessId)
    {
        // check if already has focus
        if (windowHandle != GetForegroundWindow())
        {
            // check if window is minimized
            if (IsIconic(windowHandle))
                ShowWindow(windowHandle, SW_RESTORE);

            // Simulate a key press
            //keybd_event(0, 0, 0, 0);
            //keybd_event(VK_MENU, 0xb8, 0, 0);

            SetForegroundWindow(windowHandle);
        }
        return FALSE;  // Stop enumerating windows
    }
    return TRUE;  // Continue enumerating
}


void BringToForeground(DWORD processId)
{
    EnumWindows(&EnumWindowsProc, (LPARAM)processId);
}
#endif  // _WIN32


string CObsUtil::getInstallPath(void)
{
#ifdef _WIN32
    // Check registry for location of Sidekick install first, if not found
    // fallback to checking env vars to deduce location based on Public/AllUsersProfile
    // env vars (used by innosetup in same search order).
    char szInstallDir[MAX_PATH + 1] = { '\0' };
    DWORD dwSize = sizeof(szInstallDir), dwType = REG_SZ;
    string sRootPath;
    HKEY hKey = nullptr;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\MyFreeCams\\Sidekick", 0,
                      KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, "InstallDir", nullptr, &dwType,
                             (BYTE*)szInstallDir, &dwSize) == ERROR_SUCCESS)
        {
            sRootPath = szInstallDir;
            //_TRACE("PATHDBG: Using registry key data for installDir: %s", sRootPath.c_str());
        }
        RegCloseKey(hKey);
    }

    if (sRootPath.empty())
    {
        if (GetEnvironmentVariableA("PUBLIC", szInstallDir, sizeof(szInstallDir)) == 0)
        {
            if (GetEnvironmentVariableA("ALLUSERSPROFILE", szInstallDir, sizeof(szInstallDir)) == 0)
            {
                strcpy(szInstallDir, "C:\\Users\\Public");
                _TRACE("ERR: Unable to both PUBLIC and ALLUSERSPROFILE env vars, defaulting to: %s", szInstallDir);
            }
            else _TRACE("PATHDBG: Using ALLUSERSPROFILE env var for install root: %s", szInstallDir);
        }
        else _TRACE("PATHDBG: Using PUBLIC env var for install root: %s", szInstallDir);
        sRootPath = szInstallDir;
    }

    return sRootPath;
#else
    string sLogPath = "/Library/Application Support/obs-studio/sidekick";
    return sLogPath;
#endif
}


// call back into the browser plugin.
typedef void (*OpenBrowserPanelFunc)(const char* pCaption, const char* pURL, int nWidth, int nHeight);
OpenBrowserPanelFunc g_fnOpenBrowserPanel = nullptr;

typedef void (*CloseBrowserPanelFunc)(const char* pCaption);
CloseBrowserPanelFunc g_fnCloseBrowserPanel = nullptr;

typedef void (*ClearBrowserPanelFunc)();
ClearBrowserPanelFunc g_fnClearBrowserPanel = nullptr;

void* g_pBrowserPlugin = nullptr;


// load the call back to create a cef browser panel
bool CObsUtil::connect_browser(void)
{
    bool bRv = false;
    OpenBrowserPanelFunc pfnOpenBrowserPanel = nullptr;
    CloseBrowserPanelFunc pfnCloseBrowserPanel = nullptr;
    ClearBrowserPanelFunc pfnClearBrowserPanel = nullptr;

#ifdef _WIN32
    const string browserPath("obs-browser");
#else
    const string browserPath("../obs-plugins/obs-browser");
#endif

    g_pBrowserPlugin = os_dlopen(browserPath.c_str());
    if (g_pBrowserPlugin)
    {
        pfnOpenBrowserPanel = (decltype(pfnOpenBrowserPanel))os_dlsym(g_pBrowserPlugin, "openBrowserPanel");
        pfnCloseBrowserPanel = (decltype(pfnCloseBrowserPanel))os_dlsym(g_pBrowserPlugin, "closeBrowserPanel");
        pfnClearBrowserPanel = (decltype(pfnClearBrowserPanel))os_dlsym(g_pBrowserPlugin, "clearBrowserPanel");

        if (pfnOpenBrowserPanel && pfnCloseBrowserPanel && pfnClearBrowserPanel) // success
        {
            g_fnOpenBrowserPanel = pfnOpenBrowserPanel;
            g_fnClearBrowserPanel = pfnClearBrowserPanel;
            g_fnCloseBrowserPanel = pfnCloseBrowserPanel;
            bRv = true;
        }
        else  // can't find function
        {
            os_dlclose(g_pBrowserPlugin);
            g_pBrowserPlugin = nullptr;
            g_fnOpenBrowserPanel = nullptr;
            g_fnClearBrowserPanel = nullptr;
            g_fnCloseBrowserPanel = nullptr;
            bRv = false;
        }
    }
    else  // failed to find library
    {
        pfnOpenBrowserPanel = nullptr;
        bRv = false;
    }
    return bRv;
}


// disconnect from the browser.
void CObsUtil::disconnect_browser()
{
    g_fnOpenBrowserPanel = nullptr;
    g_fnClearBrowserPanel = nullptr;
    g_fnCloseBrowserPanel = nullptr;
    os_dlclose(g_pBrowserPlugin);
}


// open a CEF browser panel window.
bool CObsUtil::openBrowserPanel(const char* pCaption, const char* pURL,	int nWidth, int nHeight)
{
    if (g_fnOpenBrowserPanel || connect_browser())
    {
        g_fnOpenBrowserPanel(pCaption, pURL, nWidth, nHeight);
        return true;
    }
    return false;
}


// close (hide) the browser panel
bool CObsUtil::closeBrowserPanel(const char* pCaption)
{
    if (g_fnCloseBrowserPanel || connect_browser())
    {
        g_fnCloseBrowserPanel(pCaption);
        return true;
    }
    return false;
}


// remove all open browser panels
void CObsUtil::clearBrowserPanel()
{
    if (g_fnClearBrowserPanel || connect_browser())
    {
        g_fnClearBrowserPanel();
    }
}


unsigned long CObsUtil::ExecProcess(const char* pPath, const char* pFile)
{
#ifdef _WIN32
    char szCmd[_MAX_PATH + 1] = { '\0' };
    unsigned long dwPid = 0;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    _snprintf_s(szCmd, sizeof(szCmd), "\"%s\\%s\"", pPath, pFile);

    // return 100;
    if (CreateProcessA(NULL, szCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        dwPid = pi.dwProcessId;
        _MESG("Successfully started CEF Process on pid %u", dwPid);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        DWORD dwErrVal = GetLastError();
        string sErr(getWin32Error(dwErrVal));
        _MESG("CEF Login CreateProcess('%s') FAILED: %s", szCmd, sErr.c_str());
    }
    return dwPid;
#else
    bool bRv = true;
    // system(cmd.c_str());
    char app[1024] = { '\0' };
    // HACKHACK: Remove --args --disable-gpu for production builds when we dont use VMs to execute
    // CEF apps, or just expect the login process wont ever need hardware acceleration (simple HTML only?)
    // -AM 12-11-19
    snprintf(app, 1023, "open -n \"%s/%s\" --args --disable-gpu", pPath, pFile);
    int nRet = system(app);
    return bRv;
#endif
}

//#define RUN_CEFLOGIN_FROM_DEBUGGER 1

// open cef mfc login app or panel
bool CObsUtil::ExecMFCLogin()
{
    unsigned long dwProc = 0;
    bool bRv = false;
#if MFC_BROWSER_LOGIN
    bRv = CObsUtil::openBrowserPanel("sidekick", MFC_CEF_LOGIN_URL, 614, 556);
#else
#ifdef _WIN32
    // check if cef login app is already running.
    dwProc = getProcess();
    if (dwProc != 0)
    {
        BringToForeground(dwProc);
    }
    else
    {
        string sPathBin( getInstallPath() );
#ifndef RUN_CEFLOGIN_FROM_DEBUGGER
        sPathBin += "\\cef";
#endif
        dwProc = CObsUtil::ExecProcess(sPathBin.c_str(), MFC_CEF_APP_EXE_NAME);
    }
#else
#ifdef RUN_CEFLOGIN_FROM_DEBUGGER
    dwProc = CObsUtil::ExecProcess("../../../plugins/MyFreeCams/Sidekick/MFCCefLogin/Debug", MFC_CEF_APP_EXE_NAME);
#else
    dwProc = CObsUtil::ExecProcess(MFC_OBS_CEF_LOGIN_BIN_PATH, MFC_CEF_APP_EXE_NAME);
#endif

#endif  // _WIN32
    bRv = (dwProc > 0);
#endif  // MFC_BROWSER_LOGIN
    return bRv;
}


bool CObsUtil::TerminateMFCLogin()
{
#ifdef _WIN32
    DWORD dw = getProcess();
    if (dw != 0)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dw);
        if (hProcess == nullptr)
        {
            return false;
        }
        else
        {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
        return true;
    }
#endif
    return false;
}


string CObsUtil::expandTokens(const string& s)
{
    std::map<string, string> map;
    map[string("MFC_OBS_INSTALL_PATH")] = getInstallPath();  // MFC_OBS_PLUGIN_BIN_PATH
    map[string("MFC_PLATFORM")] = string(MFC_PLATFORM);

    string sRv = s;
    std::regex e("\\$\\{([A-z_][A-z0-9_ ]*)\\}");

    std::sregex_iterator tokens_begin = std::sregex_iterator(sRv.begin(), sRv.end(), e);
    std::sregex_iterator tokens_end = std::sregex_iterator();
    for (std::sregex_iterator itr = tokens_begin; itr != tokens_end; itr++)
    {
        std::smatch match = *itr;
        string sToken = match[1];
        string sValue;
        string ssss = match.str();
        if (map.find(sToken) != map.end())
        {
            sValue = map[sToken];
            sRv = std::regex_replace(sRv, e, sValue);
        }
        else _TRACE("Unknown token %s", sToken.c_str());
    }

    return sRv;
}


int CObsUtil::strcmpi(const char* p, const char* p2)
{
    string s1 = p;
    string s2 = p2;

    std::transform(s1.begin(), s1.end(),s1.begin(), ::toupper);
    std::transform(s2.begin(), s2.end(),s2.begin(), ::toupper);
    return strcmp(s1.c_str(),s2.c_str());
}


bool CObsUtil::getCurrentSetting(const char* pszName, string& sVal)
{
    bool retVal = false;

    sVal.clear();

    obs_service_t* pService = obs_frontend_get_streaming_service();
    if (pService)
    {
        obs_data_t* settings = obs_service_get_settings(pService);
        const char* pszVal = obs_data_get_string(settings, pszName);
        if (pszVal)
        {
            sVal = pszVal;
            retVal = true;
        }
        obs_data_release(settings);
    }
    else _MESG("[SVCDBG] Unable to retreive frontend streaming service; can't collect value for %s", pszName);

    return retVal;
}


bool CObsUtil::setCurrentSetting(const char* pszName, const char* pszValue)
{
    bool retVal = false;

    obs_service_t* pService = obs_frontend_get_streaming_service();
    if (pService)
    {
        obs_data_t* settings = obs_service_get_settings(pService);
        obs_data_set_string(settings, pszName, pszValue);
        obs_service_update(pService, settings);
        retVal = true;
        _MESG("[SVCDBG] Set %s: %s", pszName, pszValue);
        obs_data_release(settings);
    }
    else _MESG("[SVCDBG] Unable to retreive frontend streaming service; can't set value for %s", pszName);

    return retVal;
}


#ifdef UNUSED_CODE
//---------------------------------------------------------------------------
// getDataPath
//
// get the module path for this file
char* CObsUtil::getDataPath(const char* pFile)
{
    char* local_dir = obs_module_file(pFile);
    if (nullptr == local_dir)
    {
        // if local_dir is null then ../data/plugins/ObsUpdater directory doesn't exist.
        char* cache_dir = obs_module_config_path(pFile);
        return cache_dir;
    }
    return local_dir;
}


//--------------------------------------------------------------------------
// AddModulePath
//
// Add new path that obs will search and log plugins.
void CObsUtil::AddModulePath(const char* pBinPath, const char* pDataPath)
{
    assert(strlen(pBinPath));
    assert(strlen(pDataPath));
    _TRACE("Adding %s to obs module path", pBinPath);
    obs_add_module_path(pBinPath, pDataPath);
}
#endif

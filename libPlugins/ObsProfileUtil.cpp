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

#include "ObsProfileUtil.hpp"

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


#include <shellapi.h>
#include <shlobj.h>
#include <Dwmapi.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

#include <util/windows/WinHandle.hpp>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>

// obs
//#include <obs.hpp>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>
//#include <util/platform.h>
#include <util/threading.h>
#include <util/darray.h>
//#include <util/util.hpp>

// solution
#include <libfcs/fcslib_string.h>
#include <libfcs/Log.h>

// project
#include "ObsUtil.h"
//#include "Portable.h"

#include <cstddef>
#include <ctime>
#include <obs-data.h>
#include <obs.h>
#include <obs.hpp>
#include <QGuiApplication>
#include <QMessageBox>
#include <QShowEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QScreen>
#include <QColorDialog>
#include <QSizePolicy>
#include <QScrollBar>
#include <QTextStream>
#include <QtGui>

#include <util/dstr.h>
#include <util/util.hpp>
#include <util/platform.h>
#include <util/profiler.hpp>
#include <util/dstr.hpp>

#include <UI/display-helpers.hpp>
#include <UI/qt-wrappers.hpp>
#include <UI/obs-app.hpp>
//#include <ShlObj_core.h>
//#include <UI/window-basic-settings.hpp>
//#include <UI/window-basic-main.hpp>

//#include "ui_OBSBasic.h"

//#include <json11.hpp>

#define VOLUME_METER_DECAY_FAST 23.53
#define VOLUME_METER_DECAY_MEDIUM 11.76
#define VOLUME_METER_DECAY_SLOW 8.57

//using namespace json11;
using namespace std;

//#include "ui-config.h"

static const double scaled_vals[] = {1.0,         1.25, (1.0 / 0.75), 1.5,
                     (1.0 / 0.6), 1.75, 2.0,          2.25,
                     2.5,         2.75, 3.0,          0.0};

//extern void DestroyPanelCookieManager();
//extern void DuplicateCurrentCookieProfile(ConfigFile &config);
//extern void CheckExistingCookieId();
//extern void DeleteCookies();

//extern OBSBasic basicConfig;

#if 0
template<typename T> static T GetOBSRef(QListWidgetItem *item)
{
    return item->data(static_cast<int>(QtDataRole::OBSRef)).value<T>();
}

template<typename T> static void SetOBSRef(QListWidgetItem *item, T &&val)
{
    item->setData(static_cast<int>(QtDataRole::OBSRef),
              QVariant::fromValue(val));
}
#endif


#if 0
static void AddExtraModulePaths()
{
    char* plugins_path = getenv("OBS_PLUGINS_PATH");
    char* plugins_data_path = getenv("OBS_PLUGINS_DATA_PATH");
    if (plugins_path && plugins_data_path)
    {
        string data_path_with_module_suffix;
        data_path_with_module_suffix += plugins_data_path;
        data_path_with_module_suffix += "/%module%";
        obs_add_module_path(plugins_path, data_path_with_module_suffix.c_str());
    }

    char base_module_dir[512];
#if defined(_WIN32) || defined(__APPLE__)
    int ret = GetProgramDataPath(base_module_dir, sizeof(base_module_dir), "obs-studio/plugins/%module%");
#else
    int ret = GetConfigPath(base_module_dir, sizeof(base_module_dir), "obs-studio/plugins/%module%");
#endif

    if (ret <= 0)
        return;

    string path = base_module_dir;
#if defined(__APPLE__)
    obs_add_module_path((path + "/bin").c_str(), (path + "/data").c_str());

    BPtr<char> config_bin =
        os_get_config_path_ptr("obs-studio/plugins/%module%/bin");
    BPtr<char> config_data =
        os_get_config_path_ptr("obs-studio/plugins/%module%/data");
    obs_add_module_path(config_bin, config_data);

#elif ARCH_BITS == 64
    obs_add_module_path((path + "/bin/64bit").c_str(), (path + "/data").c_str());
#else
    obs_add_module_path((path + "/bin/32bit").c_str(), (path + "/data").c_str());
#endif
}
#endif


bool GetFileSafeName(const char* name, std::string& file)
{
    size_t base_len = strlen(name);
    size_t len = os_utf8_to_wcs(name, base_len, nullptr, 0);
    std::wstring wfile;

    if (!len)
        return false;

    wfile.resize(len);
    os_utf8_to_wcs(name, base_len, &wfile[0], len + 1);

    for (size_t i = wfile.size(); i > 0; i--)
    {
        size_t im1 = i - 1;
        if (iswspace(wfile[im1]))
            wfile[im1] = '_';
        else if (wfile[im1] != '_' && !iswalnum(wfile[im1]))
            wfile.erase(im1, 1);
    }

    if (wfile.size() == 0)
        wfile = L"characters_only";

    len = os_wcs_to_utf8(wfile.c_str(), wfile.size(), nullptr, 0);
    if (!len)
        return false;

    file.resize(len);
    os_wcs_to_utf8(wfile.c_str(), wfile.size(), &file[0], len + 1);
    return true;
}


bool GetClosestUnusedFileName(std::string& path, const char* extension)
{
    size_t len = path.size();
    if (extension)
    {
        path += ".";
        path += extension;
    }

    if (!os_file_exists(path.c_str()))
        return true;

    int index = 1;
    do
    {
        path.resize(len);
        path += std::to_string(++index);
        if (extension)
        {
            path += ".";
            path += extension;
        }
    } while (os_file_exists(path.c_str()));

    return true;
}


void EnumProfiles(std::function<bool(const char*, const char*)>&& cb)
{
    char path[512];
    os_glob_t *glob;

    int ret = os_get_config_path(path, sizeof(path), "obs-studio/basic/profiles/*");
    if (ret <= 0)
    {
        blog(LOG_WARNING, "Failed to get profiles config path");
        return;
    }

    if (os_glob(path, 0, &glob) != 0)
    {
        blog(LOG_WARNING, "Failed to glob profiles");
        return;
    }

    for (size_t i = 0; i < glob->gl_pathc; i++)
    {
        const char* filePath = glob->gl_pathv[i].path;
        const char* dirName = strrchr(filePath, '/') + 1;

        if (!glob->gl_pathv[i].directory)
            continue;

        if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0)
            continue;

        std::string file = filePath;
        file += "/basic.ini";

        ConfigFile config;
        int ret = config.Open(file.c_str(), CONFIG_OPEN_EXISTING);
        if (ret != CONFIG_SUCCESS)
            continue;

        const char *name = config_get_string(config, "General", "Name");
        if (!name)
            name = strrchr(filePath, '/') + 1;

        if (!cb(name, filePath))
            break;
    }

    os_globfree(glob);
}


static bool ProfileExists(const char* findName)
{
    bool found = false;
    auto func = [&](const char* name, const char*)
    {
        if (strcmp(name, findName) == 0)
        {
            found = true;
            return false;
        }
        return true;
    };

    EnumProfiles(func);
    return found;
}


static bool GetProfileName(const std::string& name, std::string& file)
{
    if (ProfileExists(name.c_str()))
    {
        blog(LOG_WARNING, "Profile '%s' exists", name.c_str());
        return false;
    }
    if (!GetFileSafeName(name.c_str(), file))
    {
        blog(LOG_WARNING, "Failed to create safe file name for '%s'", name.c_str());
        return false;
    }

    char path[512];
    int ret;
    ret = os_get_config_path(path, sizeof(path), "obs-studio/basic/profiles/");
    if (ret <= 0)
    {
        blog(LOG_WARNING, "Failed to get profiles config path");
        return false;
    }

    file.insert(0, path);
    if (!GetClosestUnusedFileName(file, nullptr))
    {
        blog(LOG_WARNING, "Failed to get closest file name for %s", file.c_str());
        return false;
    }

    file.erase(0, ret);
    return true;
}


int ObsProfileUtil::GetProfilePath(char* path, size_t size, const char* file) const
{
	char profiles_path[512];
	const char* profile = config_get_string(obs_frontend_get_global_config(), "Basic", "ProfileDir");
	int ret;

	if (!profile)
		return -1;
	if (!path)
		return -1;
	if (!file)
		file = "";

    ret = os_get_config_path(profiles_path, 512, "obs-studio/basic/profiles");
	if (ret <= 0)
		return ret;

	if (!*file)
		return snprintf(path, size, "%s/%s", profiles_path, profile);

	return snprintf(path, size, "%s/%s/%s", profiles_path, profile, file);
}


bool ObsProfileUtil::CopyProfile(const char* fromPartial, const char* to)
{
    char dir[512];
    int ret;

    ret = os_get_config_path(dir, sizeof(dir), "obs-studio/basic/profiles/");
    if (ret <= 0)
    {
        blog(LOG_WARNING, "Failed to get profiles config path");
        return false;
    }

    char path[514];
    snprintf(path, sizeof(path), "%s%s/*", dir, fromPartial);

    os_glob_t* glob;
    if (os_glob(path, 0, &glob) != 0)
    {
        blog(LOG_WARNING, "Failed to glob profile '%s'", fromPartial);
        return false;
    }

    for (size_t i = 0; i < glob->gl_pathc; i++)
    {
        const char* filePath = glob->gl_pathv[i].path;
        if (glob->gl_pathv[i].directory)
            continue;

        ret = snprintf(path, sizeof(path), "%s/%s", to, strrchr(filePath, '/') + 1);
        if (ret > 0)
        {
            if (os_copyfile(filePath, path) != 0)
            {
                blog(LOG_WARNING, "CopyProfile: Failed to copy file %s to %s",
                     filePath, path);
            }
        }
    }

    os_globfree(glob);
    return true;
}

#if 0
void Auth::Save()
{
    ObsProfileUtil* main = ObsProfileUtil::Get();
    Auth* auth = main->auth.get();
    if (!auth)
    {
        if (config_has_user_value(main->Config(), "Auth", "Type"))
        {
            config_remove_value(main->Config(), "Auth", "Type");
            config_save_safe(main->Config(), "tmp", nullptr);
        }
        return;
    }

    config_set_string(main->Config(), "Auth", "Type", auth->service());
    auth->SaveInternal();
    config_save_safe(main->Config(), "tmp", nullptr);
}
#endif


#if 0
ObsProfileUtil* ObsProfileUtil::Get()
{
    return reinterpret_cast<ObsProfileUtil*>(obs_frontend_get_main_window());
}
#endif


config_t* ObsProfileUtil::Config() const
{
    return obs_frontend_get_profile_config();
}


bool ObsProfileUtil::AddProfile(const std::string& newName, bool copyProfile)
{
    std::string newDir;
    std::string newPath;

    if (!GetProfileName(newName, newDir))
        return false;

    auto globalConfig = obs_frontend_get_global_config();

    config_set_bool(obs_frontend_get_global_config(), "Basic", "ConfigOnNewProfile", false);

    std::string curDir = config_get_string(obs_frontend_get_global_config(), "Basic", "ProfileDir");
    blog(LOG_INFO, "current profile directory: %s", curDir.c_str());
    std::string profilePath = CObsUtil::getProfilePath();
    blog(LOG_INFO, "current profile path: %s", profilePath.c_str());

    char baseDir[512];
    int ret = os_get_config_path(baseDir, sizeof(baseDir), "obs-studio/basic/profiles/");
    if (ret <= 0)
    {
        blog(LOG_WARNING, "Failed to get profiles config path");
        return false;
    }

    newPath = baseDir + newDir;
    blog(LOG_INFO, "new profile directory: %s", newPath.c_str());

    if (os_mkdir(newPath.c_str()) < 0)
    {
        blog(LOG_WARNING, "Failed to create profile directory '%s'", newDir.c_str());
        return false;
    }

	if (copyProfile)
		CopyProfile(curDir.c_str(), newPath.c_str());

    newPath += "/basic.ini";

    config_t* config;
    if (config_open(&config, newPath.c_str(), CONFIG_OPEN_ALWAYS) != 0)
    {
        blog(LOG_ERROR, "Failed to open new config file '%s'", newDir.c_str());
        return false;
    }

    config_set_string(obs_frontend_get_global_config(), "Basic", "Profile", newName.c_str());
    config_set_string(obs_frontend_get_global_config(), "Basic", "ProfileDir", newDir.c_str());

    //Auth::Save();
    //auth.reset();
    //DestroyPanelCookieManager();

    config_set_string(config, "General", "Name", newName.c_str());
    config_save_safe(config, "tmp", nullptr);

    config_t* basicConfig = obs_frontend_get_profile_config();
    config_save_safe(basicConfig, "tmp", nullptr);

    // Swap config & basicConfig
	config_t* newConfig = basicConfig;
	basicConfig = config;
	config = newConfig;

    //QMainWindow* main = (QMainWindow*)obs_frontend_get_main_window();
    //QMetaObject::invokeMethod(main, "on_actionRefreshProfiles_triggered");

    //QMetaObject::invokeMethod(main, "InitBasicConfigDefaults");
    //QMetaObject::invokeMethod(main, "InitBasicConfigDefaults2");
    //QMetaObject::invokeMethod(main, "RefreshProfiles");
    //QMetaObject::invokeMethod(main, "ResetProfileData");

    //InitBasicConfigDefaults();
    //InitBasicConfigDefaults2();
    //RefreshProfiles();
    //ResetProfileData();

    blog(LOG_INFO, "Created profile '%s' (%s, %s)", newName.c_str(), "clean", newDir.c_str());
    blog(LOG_INFO, "------------------------------------------------");

    config_save_safe(obs_frontend_get_global_config(), "tmp", nullptr);
    //UpdateTitleBar();
    //QMetaObject::invokeMethod(main, "UpdateTitleBar");

    //if (api)
    //{
    //    api->on_event(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
    //    api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
    //}
    return true;
}


void ObsProfileUtil::CreateStreamingService(const string& serviceName, const string& server, const string& key)
{
    obs_data_t* settings = obs_data_create();
    obs_data_set_string(settings, "service", serviceName.c_str());
    obs_data_set_string(settings, "server", server.c_str());
    obs_data_set_string(settings, "key", key.c_str());
    obs_data_set_bool(settings, "bwtest", false);
    obs_service_t* newService = obs_service_create("rtmp_common", serviceName.c_str(), settings, nullptr);
    obs_frontend_set_streaming_service(newService);
    obs_frontend_save_streaming_service();
    obs_data_release(settings);
    obs_service_release(newService);
}


#if 0
void ObsProfileUtil::ChangeProfile()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    ConfigFile config;
    std::string path;

    if (!action)
        return;

    path = QT_TO_UTF8(action->property("file_name").value<QString>());
    if (path.empty())
        return;

    const char* oldName = config_get_string(obs_frontend_get_global_config(), "Basic", "Profile");
    if (action->text().compare(QT_UTF8(oldName)) == 0)
    {
        action->setChecked(true);
        return;
    }

    size_t path_len = path.size();
    path += "/basic.ini";

    if (config.Open(path.c_str(), CONFIG_OPEN_ALWAYS) != 0)
    {
        blog(LOG_ERROR, "ChangeProfile: Failed to load file '%s'", path.c_str());
        return;
    }

    path.resize(path_len);

    const char* newName = config_get_string(config, "General", "Name");
    const char* newDir = strrchr(path.c_str(), '/') + 1;

    config_set_string(obs_frontend_get_global_config(), "Basic", "Profile", newName);
    config_set_string(obs_frontend_get_global_config(), "Basic", "ProfileDir", newDir);

    //Auth::Save();
    //auth.reset();
    //DestroyPanelCookieManager();

    config.Swap(basicConfig);
    QMainWindow* main = (QMainWindow*)obs_frontend_get_main_window();
    QMetaObject::invokeMethod(main, "InitBasicConfigDefaults");
    QMetaObject::invokeMethod(main, "InitBasicConfigDefaults2");
    QMetaObject::invokeMethod(main, "ResetProfileData");
    QMetaObject::invokeMethod(main, "RefreshProfiles");

    //InitBasicConfigDefaults();
    //InitBasicConfigDefaults2();
    //ResetProfileData();
    //RefreshProfiles();
    config_save_safe(obs_frontend_get_global_config(), "tmp", nullptr);
    //UpdateTitleBar();
    QMetaObject::invokeMethod(main, "UpdateTitleBar");

    //Auth::Load();

    //CheckForSimpleModeX264Fallback();
    QMetaObject::invokeMethod(main, "CheckForSimpleModeX264Fallback");

    blog(LOG_INFO, "Switched to profile '%s' (%s)", newName, newDir);
    blog(LOG_INFO, "------------------------------------------------");

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
}
#endif


#if 0
void ObsProfileUtil::RefreshProfiles()
{
    QList<QAction*> menuActions = ui->profileMenu->actions();
    int count = 0;

    for (int i = 0; i < menuActions.count(); i++)
    {
        QVariant v = menuActions[i]->property("file_name");
        if (v.typeName() != nullptr)
            delete menuActions[i];
    }

    const char* curName = config_get_string(obs_frontend_get_global_config(), "Basic", "Profile");

    auto addProfile = [&](const char* name, const char* path)
    {
        std::string file = strrchr(path, '/') + 1;

        QAction *action = new QAction(name, this);
        action->setProperty("file_name", path);
        connect(action, &QAction::triggered, this, &ObsProfileUtil::ChangeProfile);
        action->setCheckable(true);

        action->setChecked(strcmp(name, curName) == 0);

        ui->profileMenu->addAction(action);
        count++;
        return true;
    };

    EnumProfiles(addProfile);

    ui->actionRemoveProfile->setEnabled(count > 1);
}
#endif


#if 0
void ObsProfileUtil::ResetProfileData()
{
    ResetVideo();
    service = nullptr;
    InitService();
    ResetOutputs();
    ClearHotkeys();
    CreateHotkeys();

    /* load audio monitoring */
#if defined(_WIN32) || defined(__APPLE__) || HAVE_PULSEAUDIO
    const char *device_name =
        config_get_string(basicConfig, "Audio", "MonitoringDeviceName");
    const char *device_id =
        config_get_string(basicConfig, "Audio", "MonitoringDeviceId");

    obs_set_audio_monitoring_device(device_name, device_id);

    blog(LOG_INFO, "Audio monitoring device:\n\tname: %s\n\tid: %s",
         device_name, device_id);
#endif
}
#endif


#if 0
void ObsProfileUtil::UpdateTitleBar()
{
    stringstream name;

    const char *profile =
        config_get_string(obs_frontend_get_global_config(), "Basic", "Profile");
    const char *sceneCollection = config_get_string(
        obs_frontend_get_global_config(), "Basic", "SceneCollection");

    name << "OBS ";
    if (previewProgramMode)
        name << "Studio ";

    name << App()->GetVersionString();
    if (App()->IsPortableMode())
        name << " - Portable Mode";

    name << " - " << Str("TitleBar.Profile") << ": " << profile;
    name << " - " << Str("TitleBar.Scenes") << ": " << sceneCollection;

    setWindowTitle(QT_UTF8(name.str().c_str()));
}
#endif



string GetDefaultVideoSavePath()
{
	wchar_t path_utf16[MAX_PATH];
	char path_utf8[MAX_PATH] = {};

	SHGetFolderPathW(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT,
			 path_utf16);

	os_wcs_to_utf8(path_utf16, wcslen(path_utf16), path_utf8, MAX_PATH);
	return string(path_utf8);
}


bool ObsProfileUtil::InitBasicConfigDefaults()
{
    QList<QScreen*> screens = QGuiApplication::screens();

    if (!screens.size())
    {
        blog(LOG_ERROR,
             "There appears to be no monitors.  Er, this "
             "technically shouldn't be possible.");
        return false;
    }

    QScreen* primaryScreen = QGuiApplication::primaryScreen();

    uint32_t cx = primaryScreen->size().width();
    uint32_t cy = primaryScreen->size().height();

#ifdef SUPPORTS_FRACTIONAL_SCALING
    cx *= devicePixelRatioF();
    cy *= devicePixelRatioF();
#else
    cx *= devicePixelRatio();
    cy *= devicePixelRatio();
#endif

    /* use 1920x1080 for new default base res if main monitor is above
     * 1920x1080, but don't apply for people from older builds -- only to
     * new users */
    //if ((cx * cy) > (1920 * 1080)) {
        cx = 1920;
        cy = 1080;
    //}

    bool changed = false;
    config_t* basicConfig = obs_frontend_get_profile_config();

    /* ----------------------------------------------------- */
    /* move bitrate enforcement setting to new value         */
    if (config_has_user_value(basicConfig, "SimpleOutput", "EnforceBitrate") &&
        !config_has_user_value(basicConfig, "Stream1", "IgnoreRecommended") &&
        !config_has_user_value(basicConfig, "Stream1", "MovedOldEnforce"))
    {
        bool enforce = config_get_bool(basicConfig, "SimpleOutput", "EnforceBitrate");
        config_set_bool(basicConfig, "Stream1", "IgnoreRecommended", !enforce);
        config_set_bool(basicConfig, "Stream1", "MovedOldEnforce", true);
        changed = true;
    }

    /* ----------------------------------------------------- */

    if (changed)
        config_save_safe(basicConfig, "tmp", nullptr);

    /* ----------------------------------------------------- */

    config_set_default_string(basicConfig, "Output", "Mode", "Simple");

    config_set_default_bool(basicConfig, "Stream1", "IgnoreRecommended", false);

    config_set_default_string(basicConfig, "SimpleOutput", "FilePath", GetDefaultVideoSavePath().c_str());
    config_set_default_string(basicConfig, "SimpleOutput", "RecFormat", "mkv");
    config_set_default_uint(basicConfig, "SimpleOutput", "VBitrate", 2500);
    config_set_default_uint(basicConfig, "SimpleOutput", "ABitrate", 160);
    config_set_default_bool(basicConfig, "SimpleOutput", "UseAdvanced", false);
    config_set_default_string(basicConfig, "SimpleOutput", "Preset", "veryfast");
    config_set_default_string(basicConfig, "SimpleOutput", "NVENCPreset", "hq");
    config_set_default_string(basicConfig, "SimpleOutput", "RecQuality", "Stream");
    config_set_default_bool(basicConfig, "SimpleOutput", "RecRB", false);
    config_set_default_int(basicConfig, "SimpleOutput", "RecRBTime", 20);
    config_set_default_int(basicConfig, "SimpleOutput", "RecRBSize", 512);
    config_set_default_string(basicConfig, "SimpleOutput", "RecRBPrefix", "Replay");

    config_set_default_bool(basicConfig, "AdvOut", "ApplyServiceSettings",  true);
    config_set_default_bool(basicConfig, "AdvOut", "UseRescale", false);
    config_set_default_uint(basicConfig, "AdvOut", "TrackIndex", 1);
    config_set_default_uint(basicConfig, "AdvOut", "VodTrackIndex", 2);
    config_set_default_string(basicConfig, "AdvOut", "Encoder", "obs_x264");

    config_set_default_string(basicConfig, "AdvOut", "RecType", "Standard");

    config_set_default_string(basicConfig, "AdvOut", "RecFilePath", GetDefaultVideoSavePath().c_str());
    config_set_default_string(basicConfig, "AdvOut", "RecFormat", "mkv");
    config_set_default_bool(basicConfig, "AdvOut", "RecUseRescale", false);
    config_set_default_uint(basicConfig, "AdvOut", "RecTracks", (1 << 0));
    config_set_default_string(basicConfig, "AdvOut", "RecEncoder", "none");
    config_set_default_uint(basicConfig, "AdvOut", "FLVTrack", 1);

    config_set_default_bool(basicConfig, "AdvOut", "FFOutputToFile", true);
    config_set_default_string(basicConfig, "AdvOut", "FFFilePath", GetDefaultVideoSavePath().c_str());
    config_set_default_string(basicConfig, "AdvOut", "FFExtension", "mp4");
    config_set_default_uint(basicConfig, "AdvOut", "FFVBitrate", 2500);
    config_set_default_uint(basicConfig, "AdvOut", "FFVGOPSize", 250);
    config_set_default_bool(basicConfig, "AdvOut", "FFUseRescale", false);
    config_set_default_bool(basicConfig, "AdvOut", "FFIgnoreCompat", false);
    config_set_default_uint(basicConfig, "AdvOut", "FFABitrate", 160);
    config_set_default_uint(basicConfig, "AdvOut", "FFAudioMixes", 1);

    config_set_default_uint(basicConfig, "AdvOut", "Track1Bitrate", 160);
    config_set_default_uint(basicConfig, "AdvOut", "Track2Bitrate", 160);
    config_set_default_uint(basicConfig, "AdvOut", "Track3Bitrate", 160);
    config_set_default_uint(basicConfig, "AdvOut", "Track4Bitrate", 160);
    config_set_default_uint(basicConfig, "AdvOut", "Track5Bitrate", 160);
    config_set_default_uint(basicConfig, "AdvOut", "Track6Bitrate", 160);

    config_set_default_bool(basicConfig, "AdvOut", "RecRB", false);
    config_set_default_uint(basicConfig, "AdvOut", "RecRBTime", 20);
    config_set_default_int(basicConfig, "AdvOut", "RecRBSize", 512);

    config_set_default_uint(basicConfig, "Video", "BaseCX", cx);
    config_set_default_uint(basicConfig, "Video", "BaseCY", cy);

    /* don't allow BaseCX/BaseCY to be susceptible to defaults changing */
    if (!config_has_user_value(basicConfig, "Video", "BaseCX") ||
        !config_has_user_value(basicConfig, "Video", "BaseCY"))
    {
        config_set_uint(basicConfig, "Video", "BaseCX", cx);
        config_set_uint(basicConfig, "Video", "BaseCY", cy);
        config_save_safe(basicConfig, "tmp", nullptr);
    }

    config_set_default_string(basicConfig, "Output", "FilenameFormatting",
                  "%CCYY-%MM-%DD %hh-%mm-%ss");

    config_set_default_bool(basicConfig, "Output", "DelayEnable", false);
    config_set_default_uint(basicConfig, "Output", "DelaySec", 20);
    config_set_default_bool(basicConfig, "Output", "DelayPreserve", true);

    config_set_default_bool(basicConfig, "Output", "Reconnect", true);
    config_set_default_uint(basicConfig, "Output", "RetryDelay", 10);
    config_set_default_uint(basicConfig, "Output", "MaxRetries", 20);

    config_set_default_string(basicConfig, "Output", "BindIP", "default");
    config_set_default_bool(basicConfig, "Output", "NewSocketLoopEnable", false);
    config_set_default_bool(basicConfig, "Output", "LowLatencyEnable", false);

    int i = 0;
    uint32_t scale_cx = cx;
    uint32_t scale_cy = cy;

    /* use a default scaled resolution that has a pixel count no higher
     * than 1280x720 */
    while (((scale_cx * scale_cy) > (1280 * 720)) && scaled_vals[i] > 0.0)
    {
        double scale = scaled_vals[i++];
        scale_cx = uint32_t(double(cx) / scale);
        scale_cy = uint32_t(double(cy) / scale);
    }

    config_set_default_uint(basicConfig, "Video", "OutputCX", scale_cx);
    config_set_default_uint(basicConfig, "Video", "OutputCY", scale_cy);

    /* don't allow OutputCX/OutputCY to be susceptible to defaults
     * changing */
    if (!config_has_user_value(basicConfig, "Video", "OutputCX") ||
        !config_has_user_value(basicConfig, "Video", "OutputCY"))
    {
        config_set_uint(basicConfig, "Video", "OutputCX", scale_cx);
        config_set_uint(basicConfig, "Video", "OutputCY", scale_cy);
        config_save_safe(basicConfig, "tmp", nullptr);
    }

    config_set_default_uint(basicConfig, "Video", "FPSType", 0);
    config_set_default_string(basicConfig, "Video", "FPSCommon", "30");
    config_set_default_uint(basicConfig, "Video", "FPSInt", 30);
    config_set_default_uint(basicConfig, "Video", "FPSNum", 30);
    config_set_default_uint(basicConfig, "Video", "FPSDen", 1);
    config_set_default_string(basicConfig, "Video", "ScaleType", "bicubic");
    config_set_default_string(basicConfig, "Video", "ColorFormat", "NV12");
    config_set_default_string(basicConfig, "Video", "ColorSpace", "709");
    config_set_default_string(basicConfig, "Video", "ColorRange", "Partial");

    config_set_default_string(basicConfig, "Audio", "MonitoringDeviceId", "default");
    config_set_default_string(
        basicConfig, "Audio", "MonitoringDeviceName",
        Str("Basic.Settings.Advanced.Audio.MonitoringDevice"
            ".Default"));
    config_set_default_uint(basicConfig, "Audio", "SampleRate", 48000);
    config_set_default_string(basicConfig, "Audio", "ChannelSetup", "Stereo");
    config_set_default_double(basicConfig, "Audio", "MeterDecayRate", VOLUME_METER_DECAY_FAST);
    config_set_default_uint(basicConfig, "Audio", "PeakMeterType", 0);

    //CheckExistingCookieId();

    return true;
}


#if 0
extern bool EncoderAvailable(const char *encoder);
#endif

#if 0
void ObsProfileUtil::InitBasicConfigDefaults2()
{
    bool oldEncDefaults = config_get_bool(obs_frontend_get_global_config(), "General",
                          "Pre23Defaults");
    bool useNV = EncoderAvailable("ffmpeg_nvenc") && !oldEncDefaults;

    config_set_default_string(basicConfig, "SimpleOutput", "StreamEncoder",
                  useNV ? SIMPLE_ENCODER_NVENC
                    : SIMPLE_ENCODER_X264);
    config_set_default_string(basicConfig, "SimpleOutput", "RecEncoder",
                  useNV ? SIMPLE_ENCODER_NVENC
                    : SIMPLE_ENCODER_X264);
}
#endif


#if 0
void ObsProfileUtil::CheckForSimpleModeX264Fallback()
{
    const char* curStreamEncoder = config_get_string(basicConfig, "SimpleOutput", "StreamEncoder");
    const char* curRecEncoder = config_get_string(basicConfig, "SimpleOutput", "RecEncoder");

    bool qsv_supported = false;
    bool amd_supported = false;
    bool nve_supported = false;
    bool changed = false;
    size_t idx = 0;
    const char* id;

    while (obs_enum_encoder_types(idx++, &id))
    {
        if (strcmp(id, "amd_amf_h264") == 0)
            amd_supported = true;
        else if (strcmp(id, "obs_qsv11") == 0)
            qsv_supported = true;
        else if (strcmp(id, "ffmpeg_nvenc") == 0)
            nve_supported = true;
    }

    auto CheckEncoder = [&](const char*& name)
    {
        if (strcmp(name, SIMPLE_ENCODER_QSV) == 0)
        {
            if (!qsv_supported)
            {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
        }
        else if (strcmp(name, SIMPLE_ENCODER_NVENC) == 0)
        {
            if (!nve_supported)
            {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
        }
        else if (strcmp(name, SIMPLE_ENCODER_AMD) == 0)
        {
            if (!amd_supported)
            {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
        }

        return true;
    };

    if (!CheckEncoder(curStreamEncoder))
        config_set_string(basicConfig, "SimpleOutput", "StreamEncoder", curStreamEncoder);
    if (!CheckEncoder(curRecEncoder))
        config_set_string(basicConfig, "SimpleOutput", "RecEncoder", curRecEncoder);
    if (changed)
        config_save_safe(basicConfig, "tmp", nullptr);
}
#endif

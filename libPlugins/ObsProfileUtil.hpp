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

#pragma once

#ifndef OBS_PROFILE_UTIL_H_
#define OBS_PROFILE_UTIL_H_

#include <QApplication>
#include <QMainWindow>
#include <QPointer>

#include <QBuffer>
#include <QAction>
#include <QWidgetAction>
#include <QSystemTrayIcon>
#include <QStyledItemDelegate>

#include <obs.hpp>

#include <vector>
#include <memory>

#include <obs-frontend-internal.hpp>

#include <functional>
//#include <memory>
#include <string>
#include <utility>
//#include <vector>
#include <deque>


#include <util/platform.h>
#include <util/threading.h>
#include <util/util.hpp>

#include <QPointer>

//class QMessageBox;
class QListWidgetItem;
class VolControl;
class OBSBasicStats;
//class QWidget;

//#include "ui_OBSBasic.h"
//#include "ui_ColorSelect.h"

#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()

#define SIMPLE_ENCODER_X264 "x264"
#define SIMPLE_ENCODER_X264_LOWCPU "x264_lowcpu"
#define SIMPLE_ENCODER_QSV "qsv"
#define SIMPLE_ENCODER_NVENC "nvenc"
#define SIMPLE_ENCODER_AMD "amd"

#if 0
class Auth : public QObject {
    Q_OBJECT

protected:
    virtual void SaveInternal() = 0;
    virtual bool LoadInternal() = 0;

    bool firstLoad = true;

    struct ErrorInfo
    {
        std::string message;
        std::string error;

        ErrorInfo(std::string message_, std::string error_)
            : message(message_)
            , error(error_)
        {
        }
    };

public:
    enum class Type
    {
        None,
        OAuth_StreamKey,
    };

    struct Def
    {
        std::string service;
        Type type;
    };

    typedef std::function<std::shared_ptr<Auth>()> create_cb;

    inline Auth(const Def& d) : def(d) {}
    virtual ~Auth() = default;

    inline Type type() const { return def.type; }
    inline const char *service() const { return def.service.c_str(); }

    virtual void LoadUI() {}

    virtual void OnStreamConfig() {}

    static std::shared_ptr<Auth> Create(const std::string& service);
    static Type AuthType(const std::string& service);
    static void Load();
    static void Save();

protected:
    static void RegisterAuth(const Def& d, create_cb create);

private:
    Def def;
};
#endif


class OBSMainWindoww : public QMainWindow {
    Q_OBJECT

public:
    inline OBSMainWindoww(QWidget *parent) : QMainWindow(parent) {}

    virtual config_t *Config() const = 0;
    virtual void OBSInit() = 0;

    virtual int GetProfilePath(char *path, size_t size,
                   const char *file) const = 0;
};


#if 0
class OBSAppp : public QApplication
{
    Q_OBJECT

private:
    std::string locale;
    std::string theme;
    ConfigFile globalConfig;
    TextLookup textLookup;
    QPointer<OBSMainWindoww> mainWindow;
    profiler_name_store_t *profilerNameStore = nullptr;

    bool UpdatePre22MultiviewLayout(const char *layout);

    bool InitGlobalConfig();
    bool InitGlobalConfigDefaults();
    bool InitLocale();
    bool InitTheme();

public:
    OBSAppp(int &argc, char **argv, profiler_name_store_t *store);
    ~OBSAppp();

    void AppInit();
    bool OBSInit();

    inline QMainWindow *GetMainWindow() const { return mainWindow.data(); }
    inline config_t *GlobalConfig() const { return globalConfig; }

signals:
    void StyleChanged();
};

inline OBSAppp* App() { return static_cast<OBSAppp *>(qApp); }
#endif

int GetConfigPath(char *path, size_t size, const char *name);
char* GetConfigPathPtr(const char *name);

//int GetProgramDataPath(char *path, size_t size, const char *name);
//char* GetProgramDataPathPtr(const char *name);


class ObsProfileUtil : public OBSMainWindoww
{
    Q_OBJECT
    friend class Auth;

public:
    //static ObsProfileUtil* Get();

    bool AddProfile(const char* title, const char* text, const char* init_text = nullptr);

private:
    obs_frontend_callbacks* api = nullptr;
    std::shared_ptr<Auth> auth;
    ConfigFile basicConfig;
    //std::unique_ptr<Ui::ObsProfileUtil> ui;

    //void ChangeProfile();
    //void RefreshProfiles();
    //void ResetProfileData();
    //void UpdateTitleBar();
	//bool InitBasicConfigDefaults();
	//void InitBasicConfigDefaults2();
	//bool InitBasicConfig();
    //void CheckForSimpleModeX264Fallback();
};

#endif  // OBS_PROFILE_UTIL_H_

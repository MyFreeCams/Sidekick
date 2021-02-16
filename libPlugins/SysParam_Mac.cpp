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

// System Includes
#include <Availability.h>
#include <AvailabilityMacros.h>

#include <list>
#include <string>

#include <objc/objc.h>
#include <objc/message.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDManager.h>

#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>

// MFC Includes
#include <libfcs/MfcJson.h>
#include <libfcs/Log.h>
#include <ObsBroadcast/ObsBroadcast.h>

// project includes
#include "MFCConfigConstants.h"
#include "SysParam_Mac.h"
#include "build_version.h"

extern CBroadcastCtx g_ctx; // part of MFCLibPlugins.lib::MfcPluginAPI.obj


int CSysParam::ToJson(MfcJsonObj& json)
{
    int nRv = 0;

    MfcJsonObj js;
    js.objectAdd(std::string("Category"), m_sCategory.c_str());
    js.objectAdd(std::string("Device"), m_sDevice.c_str());
    js.objectAdd(std::string("Details"), MfcJsonObj::encodeURIComponent(m_sDetails).c_str());
#ifdef _DEBUG
    std::string s = js.prettySerialize();
    std::string s2 = js.Serialize();
#endif

    json.arrayAdd(js);
    return nRv;
}


int CSysParamList::ToJson(MfcJsonObj& json)
{
    int nRv = 0;
    MfcJsonObj js;
    js.clearArray();
    for (auto itr = begin(); nRv == 0 && itr != end(); itr++)
    {
        CSysParam &pm = *itr;
        nRv = pm.ToJson(js);
    }

    MfcJsonObj js2;
    js2.objectAdd(std::string("SysReport"), js);
    json.objectAdd(std::string("systemDetails"), js2);

#ifdef _DEBUG
    std::string s = json.prettySerialize();
    std::string s2 = json.Serialize();
#endif

    return nRv;
}


//---------------------------------------------------------------------------
// CollectData
//
// collect all data.
void CSysParamList::CollectData(const std::string& sBinPath)
{
    UNUSED_PARAMETER(sBinPath);
    struct utsname buf{};

    uname(&buf);
    // these contain garbage on mac.
    push_back(CSysParam("System","SysName",buf.sysname));
    push_back(CSysParam("System","NodeName",buf.nodename));
    push_back(CSysParam("System","release",buf.release));
    push_back(CSysParam("System","Version",buf.version));
    push_back(CSysParam("System","Machine",buf.machine));

    get_processor_name();
    get_processor_speed();
    get_processor_cores();
    get_available_memory();
    get_os();
    get_kernel_version();
}


//---------------------------------------------------------------------------
// get_os_name
//
// fetch the OS name
std::string get_os_name(id pInfo, SEL UTF8Str)
{
#ifdef __MAC_10_15
    typedef int (*os_func)(id, SEL);
    os_func operatingSystem = (os_func)objc_msgSend;
    auto os_id = (unsigned long)operatingSystem(pInfo, sel_registerName("operatingSystem"));

    typedef id (*os_name_func)(id, SEL);
    os_name_func operatingSystemName = (os_name_func)objc_msgSend;
    id os = operatingSystemName(pInfo, sel_registerName("operatingSystemName"));

    typedef const char*(*utf8_func)(id, SEL);
    utf8_func UTF8String = (utf8_func)objc_msgSend;
    const char* name = UTF8String(os, UTF8Str);
#else
    auto os_id = (unsigned long)objc_msgSend(pInfo, sel_registerName("operatingSystem"));

    id os = objc_msgSend(pInfo, sel_registerName("operatingSystemName"));
    const char* name = (const char*)objc_msgSend(os, UTF8Str);

    if (os_id !== 5 /*NSMACHOperatingSystem*/)
    {
        name = "Unknown";
    }
#endif

    return std::string(name);
}


//-------------------------------------------------------------------------
// get_os_version
//
// get the os version
std::string get_os_version(id pInfo, SEL UTF8StringSel)
{
#ifdef __MAC_10_15
    typedef id (*version_func)(id, SEL);
    version_func operatingSystemVersionString = (version_func)objc_msgSend;
    id vs = operatingSystemVersionString(
            pInfo, sel_registerName("operatingSystemVersionString"));
    typedef const char*(*utf8_func)(id, SEL);
    utf8_func UTF8String = (utf8_func)objc_msgSend;
    const char* version = UTF8String(vs, UTF8StringSel);
    return std::string(version);
#else
    id vs = objc_msgSend(pInfo, sel_registerName("operatingSystemVersionString"));
    const char* version = (const char*)objc_msgSend(vs, UTF8StringSel);

    return std::string(version ? version : "Unknown");
#endif
}


//---------------------------------------------------------------------------
// get_processor_name
//
// get cpu name
void CSysParamList::get_processor_name(void)
{
    char*  name = NULL;
    size_t size;
    int    ret;

    ret = sysctlbyname("machdep.cpu.brand_string", NULL, &size, NULL, 0);
    if (ret != 0)
    {
        _TRACE("Failed to get processor name %d",ret);
        return;
    }

    name = (char*)malloc(size);

    ret = sysctlbyname("machdep.cpu.brand_string", name, &size, NULL, 0);
    if (ret == 0)
    {
        _TRACE("CPU Name: %s", name);
        push_back(CSysParam("Hardware","CPU Name",name));
    }
    free(name);
    name = NULL;
}


//---------------------------------------------------------------------------
// get_processor_speed
//
// get the processor speed
void CSysParamList::get_processor_speed(void)
{
    size_t    size;
    long long freq;
    int       ret;

    size = sizeof(freq);
    ret = sysctlbyname("hw.cpufrequency", &freq, &size, NULL, 0);
    std::string s = stdprintf("%lldMHz", freq / 1000000);
    if (ret == 0)
    {
        push_back(CSysParam("Hardware","CPU Speed",s.c_str()));
    }
    else
    {
        _TRACE("Failed to get cpu speed %d",ret);
    }
}


//---------------------------------------------------------------------------
// get_processor_cores
//
// get number of cores
void CSysParamList::get_processor_cores(void)
{
    std::string s = stdprintf("%d",os_get_physical_cores());
    push_back(CSysParam("Hardware","Physical Cores",s.c_str()));
    s = stdprintf("%d",os_get_logical_cores());
    push_back(CSysParam("Hardware","Logical Cores",s.c_str()));
}


//----------------------------------------------------------------------------
// get_available_memory
//
//
void CSysParamList::get_available_memory(void)
{
    size_t    size;
    long long memory_available;
    int       ret;

    size = sizeof(memory_available);
    ret = sysctlbyname("hw.memsize", &memory_available, &size, NULL, 0);
    if (ret == 0)
    {
        std::string s = stdprintf("%lldMB",memory_available / 1024 / 1024);
        push_back(CSysParam("Hardware","Physical Memory", s.c_str()));
    }
}


//------------------------------------------------------------------------------
// get_os
//
// get the current os
void CSysParamList::get_os(void)
{
    //Class NSProcessInfo = objc_getClass("NSProcessInfo");
    //    id pInfo  = objc_msgSend((id)NSProcessInfo,
      //                    sel_registerName("processInfo"));
    Class NSProcessInfo = objc_getClass("NSProcessInfo");
    typedef id (*func)(id, SEL);
    func processInfo = (func)objc_msgSend;
    id pInfo = processInfo((id)NSProcessInfo, sel_registerName("processInfo"));

    SEL UTF8String = sel_registerName("UTF8String");
    std::string s = get_os_name(pInfo, UTF8String);
    push_back(CSysParam("Hardware","Mac OS x",s.c_str()));

    s = get_os_version(pInfo, UTF8String);
    push_back(CSysParam("Hardware","Os Version",s.c_str()));
}


//---------------------------------------------------------------------------
// get_kernel_version
//
// get kernel version
void CSysParamList::get_kernel_version(void)
{
    char   kernel_version[1024];
    size_t size = sizeof(kernel_version);
    int    ret;

    ret = sysctlbyname("kern.osrelease", kernel_version, &size,
                       NULL, 0);
    if (ret == 0)
    {
        push_back(CSysParam("Hardware","Kernel Version",kernel_version));
    }
}


//---------------------------------------------------------------------------
// get_computer_name
//
// get computer name
std::string get_computer_name(void)
{
    std::string computer_name = "";
    char        computer_hostname[128];
    size_t      size = sizeof(computer_hostname);
    int         ret;

    auto strip_local = [&](const std::string& in)
    {
        return in.substr(0, in.find(".local"));
    };

    ret = sysctlbyname("kern.hostname", computer_hostname, &size,
                       NULL, 0);
    if (ret == 0)
    {
        computer_name = strip_local(computer_hostname);
        // push_back(CSysParam("System","SysName",computer_name.c_str()));
    }

    return computer_name;
}


//---------------------------------------------------------------------------
// collectSystemInfo
//
// little helper function to get mac os info
//
bool collectSystemInfo(MfcJsonObj& js)
{
    Class NSProcessInfo = objc_getClass("NSProcessInfo");
    typedef id (*func)(id, SEL);
    func processInfo = (func)objc_msgSend;
    id pInfo = processInfo((id)NSProcessInfo, sel_registerName("processInfo"));

    SEL UTF8String = sel_registerName("UTF8String");
    std::string os_name = get_os_name(pInfo, UTF8String);
    std::string os_ver = get_os_version(pInfo, UTF8String);
#ifdef _DEBUG
    cout << "os_name:" << os_name << "\r\n";
    cout << "os_ver:" << os_ver << "\r\n";
#endif
    js.objectAdd("os_name", os_name);
    js.objectAdd("os_ver", os_ver);

    string sObsVer(obs_get_version_string());
    js.objectAdd("skv", SIDEKICK_VERSION_STR);
    js.objectAdd("appver", stdprintf("OBS %s", sObsVer.c_str()));

    std::string computer_name = get_computer_name();
    js.objectAdd("nm", computer_name);

    char** ppNames = obs_frontend_get_profiles();
    char** ppName = ppNames;

    int64_t nCurrent = -1;
    int64_t n = 0;
    MfcJsonPtr pList = new MfcJsonObj(JSON_T_ARRAY);
    const char* pszProfile = obs_frontend_get_current_profile();
    while (ppName && *ppName)
    {
        if (pszProfile && strcmp(pszProfile, *ppName) == 0)
            nCurrent = n;

        pList->arrayAdd(*ppName);
        ppName++;
        n++;
    }

    bfree((void*)pszProfile);
    bfree(ppNames);

    js.objectAdd("profiles", pList);
    if (nCurrent >= 0 && nCurrent < n)
        js.objectAdd("curprofile", nCurrent);

    js.objectAdd("streamtype", FCVIDEO_TX_IDLE);

    std::string sTransport("Unknown");

    if (g_ctx.isMfc)
    {
        if      (g_ctx.isRTMP)      sTransport = "RTMP";
        else if (g_ctx.isWebRTC)    sTransport = "WebRTC";
    }
    else sTransport = "Non-MFC";
    js.objectAdd("transport", sTransport);

    return true;
}

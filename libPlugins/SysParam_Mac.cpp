
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

#include <util/platform.h>

// MFC Includes
#include <libfcs/MfcJson.h>
#include <libfcs/Log.h>

// project includes
#include "MFCConfigConstants.h"
#include "SysParam_Mac.h"

int CSysParam::ToJson(MfcJsonObj &json)
{
    int nRv = 0;

    // MfcJsonObj json(JSON_type::JSON_T_ARRAY_BEGIN);
    MfcJsonObj js;
    js.objectAdd(std::string("Category"), this->m_sCategory.c_str());
    js.objectAdd(std::string("Device"), this->m_sDevice.c_str());
    js.objectAdd(std::string("Details"), MfcJsonObj::encodeURIComponent(m_sDetails).c_str());
#ifdef _DEBUG
    std::string s = js.prettySerialize();
    std::string s2 = js.Serialize();
#endif

    json.arrayAdd(js);
    return nRv;
}


int CSysParamList::ToJson(MfcJsonObj &json)
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
void CSysParamList::CollectData(const std::string &sBinPath)
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
#ifdef __MAC_10_15
std::string get_os_name(id pInfo, SEL UTF8StringSel)
{
    typedef int (*os_func)(id, SEL);
    os_func operatingSystem = (os_func)objc_msgSend;
    unsigned long os_id = (unsigned long)operatingSystem(
        pInfo, sel_registerName("operatingSystem"));

    typedef id (*os_name_func)(id, SEL);
    os_name_func operatingSystemName = (os_name_func)objc_msgSend;
    id os = operatingSystemName(pInfo,
                                sel_registerName("operatingSystemName"));
    typedef const char *(*utf8_func)(id, SEL);
    utf8_func UTF8String = (utf8_func)objc_msgSend;
    const char *name = UTF8String(os, UTF8StringSel);

    return std::string(name);

}
#else
std::string get_os_name(id pInfo, SEL UTF8String)
{
    unsigned long os_id = (unsigned long)objc_msgSend(pInfo, sel_registerName("operatingSystem"));

    id os = objc_msgSend(pInfo, sel_registerName("operatingSystemName"));
    const char *name = (const char*)objc_msgSend(os, UTF8String);

    if (os_id == 5 /*NSMACHOperatingSystem*/)
    {
    return std::string(name);
    }
    return std::string("Unknown");

}
#endif


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
    typedef const char *(*utf8_func)(id, SEL);
    utf8_func UTF8String = (utf8_func)objc_msgSend;
    const char *version = UTF8String(vs, UTF8StringSel);
    return std::string(version);
#else
    id vs = objc_msgSend(pInfo, sel_registerName("operatingSystemVersionString"));
    const char *version = (const char*)objc_msgSend(vs, UTF8StringSel);

    return std::string(version ? version : "Unknown");
#endif
}


//---------------------------------------------------------------------------
// get_processor_name
//
// get cpu name
void CSysParamList::get_processor_name(void)
{
    char   *name = NULL;
    size_t size;
    int    ret;

    ret = sysctlbyname("machdep.cpu.brand_string", NULL, &size, NULL, 0);
    if (ret != 0)
    {
        _TRACE("Failed to get processor name %d",ret);
        return;
    }

    name = (char *)malloc(size);

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
// collectSystemInfo
//
// little helper function to get mac os info
//
bool collectSystemInfo(MfcJsonObj &js)
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
    return true;
}

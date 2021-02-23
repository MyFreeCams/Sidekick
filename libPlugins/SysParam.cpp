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

#include <list>
#include <string>
#ifdef _WIN32
#include <atlsafe.h>
#endif
#include "SysParam.h"
#define _WIN32_DCOM
#include <iostream>

#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <setupapi.h>
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID
#define INITGUID
#include <tchar.h>
#include <stdio.h>

#include <comdef.h>
#include <Wbemidl.h>

// MFC Includes
#include <libfcs/MfcJson.h>
#include <libfcs/Log.h>

// project includes
#include "MFCConfigConstants.h"

#define BUF_SIZE 4096

#ifdef DEFINE_DEVPROPKEY
#undef DEFINE_DEVPROPKEY
#endif
#ifdef INITGUID
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY DECLSPEC_SELECTANY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#else
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY name
#endif // INITGUID

// include DEVPKEY_Device_BusReportedDeviceDesc from WinDDK\7600.16385.1\inc\api\devpkey.h
DEFINE_DEVPROPKEY(DEVPKEY_Device_BusReportedDeviceDesc, 0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 4);     // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_ContainerId, 0x8c7ed206, 0x3f8a, 0x4827, 0xb3, 0xab, 0xae, 0x9e, 0x1f, 0xae, 0xfc, 0x6c, 2);     // DEVPROP_TYPE_GUID
DEFINE_DEVPROPKEY(DEVPKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_DeviceDisplay_Category, 0x78c34fc8, 0x104a, 0x4aca, 0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57, 0x5a);  // DEVPROP_TYPE_STRING_LIST
DEFINE_DEVPROPKEY(DEVPKEY_Device_LocationInfo, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 15);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_Manufacturer, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 13);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_SecuritySDS, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 26);    // DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING

#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

#pragma comment (lib, "setupapi.lib")

typedef BOOL(WINAPI *FN_SetupDiGetDevicePropertyW)(
    __in       HDEVINFO DeviceInfoSet,
    __in       PSP_DEVINFO_DATA DeviceInfoData,
    __in       const DEVPROPKEY *PropertyKey,
    __out      DEVPROPTYPE *PropertyType,
    __out_opt  PBYTE PropertyBuffer,
    __in       DWORD PropertyBufferSize,
    __out_opt  PDWORD RequiredSize,
    __in       DWORD Flags
    );

//---------------------------------------------------------------------------
// ToJson
//
// convert this parameter to json
int CSysParam::ToJson(MfcJsonObj &json)
{
    MfcJsonObj js;
    js.objectAdd(std::string("Category"), this->m_sCategory.c_str());
    js.objectAdd(std::string("Device"), this->m_sDevice.c_str());
    js.objectAdd(std::string("Details"), MfcJsonObj::encodeURIComponent(m_sDetails).c_str());
#ifdef _DEBUG
    std::string s = js.prettySerialize();
    std::string s2 = js.Serialize();
#endif

    json.arrayAdd(js);
    return 0;
}

//---------------------------------------------------------------------------
// ToJson
//
// convert all collected parameters to json.
int CSysParamList::ToJson(MfcJsonObj &json)
{
    int nRv = 0;
    MfcJsonObj js;
    js.clearArray();

    for (CSysParamList::iterator itr = begin(); nRv == 0 && itr != end(); itr++)
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
    std::string sBrowser;

    getFileVersions(sBinPath);

    if (getCurrentBrowser(sBrowser))
    {
        _TRACE("%-10s %-30.30s %s", "OS", "DefaultBrowser", sBrowser.c_str());
        push_back(CSysParam("OS", "DefaultBrowser", sBrowser.c_str()));
    }
    else
    {
        _TRACE("No current browser");
    }

    CollectHardwareInfo();
    CollectWMIData();
}

//----------------------------------------------------------------------------------------------------
// ListDevice
//
// collect device information similar to device manager in windows.
void CSysParamList::ListDevices(CONST GUID *pClassGuid, LPCTSTR pszEnumerator, const char *pCategory)
{
    unsigned i;
    DWORD dwSize, dwPropertyRegDataType;
    DEVPROPTYPE ulPropertyType;
    CONFIGRET status;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    const static LPCTSTR arPrefix[3] = { TEXT("VID_"), TEXT("PID_"), TEXT("MI_") };
    TCHAR szDesc[BUF_SIZE];
    TCHAR szDeviceInstanceID[MAX_DEVICE_ID_LEN];
    WCHAR szBuffer[BUF_SIZE];

    FN_SetupDiGetDevicePropertyW fn_SetupDiGetDevicePropertyW = (FN_SetupDiGetDevicePropertyW)
        GetProcAddress(GetModuleHandle(TEXT("Setupapi.dll")), "SetupDiGetDevicePropertyW");

    // List all connected USB devices
    hDevInfo = SetupDiGetClassDevs(pClassGuid, pszEnumerator, NULL,
        pClassGuid != NULL ? DIGCF_PRESENT : DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
        return;

    // Find the ones that are driverless
    for (i = 0; ; i++)
    {
        char buf[BUF_SIZE + 1] = { '\0' };
        size_t nUselessRetVal = 0;
        std::string sInstanceID;
        std::string sDescription;
        std::string sBusDesc;
        std::string sManuf;
        std::string sFriendly;

        DeviceInfoData.cbSize = sizeof(DeviceInfoData);
        if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData))
            break;

        status = CM_Get_Device_ID(DeviceInfoData.DevInst, szDeviceInstanceID, MAX_PATH, 0);
        if (status != CR_SUCCESS)
            continue;

        // Display device instance ID
        wcstombs_s(&nUselessRetVal, buf, BUF_SIZE, szDeviceInstanceID, BUF_SIZE);
        sInstanceID = buf;

        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC,
            &dwPropertyRegDataType, (BYTE*)szDesc,
            sizeof(szDesc),   // The size, in bytes
            &dwSize))
        {
            // Display device instance ID
            wcstombs_s(&nUselessRetVal, buf, BUF_SIZE, szDesc, BUF_SIZE);
            sDescription = buf;
        }

        if (fn_SetupDiGetDevicePropertyW && fn_SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_BusReportedDeviceDesc,
            &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0))
        {

            if (fn_SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_BusReportedDeviceDesc,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0))
            {
                wcstombs_s(&nUselessRetVal, buf, BUF_SIZE, szBuffer, BUF_SIZE);
                sBusDesc = buf;

            }

            if (fn_SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_Manufacturer,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0))
            {
                wcstombs_s(&nUselessRetVal, buf, BUF_SIZE, szBuffer, BUF_SIZE);

                sManuf = buf;
            }

            if (fn_SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_FriendlyName,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0))
            {
                wcstombs_s(&nUselessRetVal, buf, BUF_SIZE, szBuffer, BUF_SIZE);
                sFriendly = buf;
            }

        } // endif

        std::string ss;
        if (!sBusDesc.empty())
            ss = sBusDesc;
        else
            ss = sDescription;

        if (!sManuf.empty())
            ss += " " + sManuf;

        push_back(CSysParam(pCategory, ((sFriendly.empty()) ? sDescription.c_str() : sFriendly.c_str()), ss.c_str()));
        //_TRACE("%-10s %-30.30s %s", pCategory, ((sFriendly.empty()) ? sDescription.c_str() : sFriendly.c_str()), ss.c_str());
    } // end for

    return;
}

//---------------------------------------------------------------------------
// CollectHardwareInfo
//
// Collect hardware info
int CSysParamList::CollectHardwareInfo()
{
    ListDevices(NULL, TEXT("USB"),"USB Devices");
    ListDevices(NULL, TEXT("USBSTOR"), "USB Storage");
    ListDevices(&GUID_DEVCLASS_DISKDRIVE, NULL, "Drives");
    return 0;
}

//-------------------------------------------------------------------------
// GetWMIProcess
//
// use WMI to collect process information
//
int CSysParamList::GetWMIProcess(IWbemServices *pSvc)
{
    IEnumWbemClassObject* pEnumerator = NULL;
    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_Service"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        _ASSERT("Failed to get process list");
        _TRACE("Query for process list failed. Error code = 0x%x", hres);
        return 1;               // Program has failed.
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
        pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
            break;

        VARIANT vtProp;

        std::string sName;
        std::string sCSName;
        std::string sDescripiton;
        std::string sCmd;

        GetWMIPropertyValue(pclsObj, L"Name", sName);// "WMI-Process", "Name");
        GetWMIPropertyValue(pclsObj, L"CSName", sCSName);// "WMI-Process", "CSName");
        GetWMIPropertyValue(pclsObj, L"Description", sDescripiton); // "WMI-Process", "Description");
        GetWMIPropertyValue(pclsObj, L"CommandLine", sCmd); // "WMI-Process", "CommandLine");

        std::string s = sDescripiton + " " + sCmd;

        //_TRACE("%-10s %-30.30s %s", "WMI_PROCESS",sName.c_str(), s.c_str());
        push_back(CSysParam("WMI_Process", sCSName.c_str(), s.c_str()));

        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pEnumerator->Release();
    return 0;
}

//---------------------------------------------------------------------------
// GetWMIService
//
// get service info using WMI
int CSysParamList::GetWMIService(IWbemServices *pSvc)
{

    IEnumWbemClassObject* pEnumerator = NULL;
    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_Service"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        _ASSERT(!"Failed to get service");
        //_TRACE("Query for service list failed again? Error code = 0x%x", hres);
        return 1;               // Program has failed.
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
        pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
            break;

        VARIANT vtProp;

        std::string sDisplayName;
        std::string sDescripiton;
        std::string sState;

        GetWMIPropertyValue(pclsObj, L"DisplayName", sDisplayName);// "WMI-SVC", "DisplayName");
        GetWMIPropertyValue(pclsObj, L"Description", sDescripiton);// "WMI-SVC", "Description");
        GetWMIPropertyValue(pclsObj, L"State", sState);// "WMI-SVC", "State");
        std::string s = sDisplayName + " - " + sDescripiton;

        //_TRACE("%-10s %-30.30s %s", "WMI_SVC", sState.c_str(), s.c_str());
        push_back(CSysParam("WMI_SVC", sState.c_str(), s.c_str()));

        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pEnumerator->Release();
    return 0;
}

//---------------------------------------------------------------------------
// GetWMIBios
//
// get WMI bios info
int CSysParamList::GetWMIBios(IWbemServices *pSvc)
{

    IEnumWbemClassObject* pEnumerator = NULL;
    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_BIOS"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        _ASSERT(!"Query for bios failed");
        _TRACE("Query for bios name failed. Error code = 0x%x", hres);
        return 1;               // Program has failed.
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
       pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
            break;

        VARIANT vtProp;

        // Get the value of the Name property
        //GetWMIProperty(pclsObj, L"BiosCharacteristics", "WMI-BIOS", "BiosCharacteristics");
        GetWMIProperty(pclsObj, L"BIOSVersion", "WMI-BIOS", "BIOSVersion");
        GetWMIProperty(pclsObj, L"BuildNumber", "WMI-BIOS", "BuildNumber");
        GetWMIProperty(pclsObj, L"Caption", "WMI-BIOS", "Caption");
        GetWMIProperty(pclsObj, L"CodeSet", "WMI-BIOS", "CodeSet");
        GetWMIProperty(pclsObj, L"CurrentLanguage", "WMI-BIOS", "CurrentLanguage");
        GetWMIProperty(pclsObj, L"Description", "WMI-BIOS", "Description");
        GetWMIProperty(pclsObj, L"EmbeddedControllerMajorVersion", "WMI-BIOS", "EmbeddedControllerMajorVersion");
        GetWMIProperty(pclsObj, L"EmbeddedControllerMinorVersion", "WMI-BIOS", "EmbeddedControllerMinorVersion");
        GetWMIProperty(pclsObj, L"IdentificationCode", "WMI-BIOS", "IdentificationCode");
        GetWMIProperty(pclsObj, L"InstallableLanguages", "WMI-BIOS", "InstallableLanguages");
        GetWMIProperty(pclsObj, L"InstallDate", "WMI-BIOS", "InstallDate");
        GetWMIProperty(pclsObj, L"LanguageEdition", "WMI-BIOS", "LanguageEdition");
        GetWMIProperty(pclsObj, L"ListOfLanguages", "WMI-BIOS", "ListOfLanguages");
        GetWMIProperty(pclsObj, L"Manufacturer", "WMI-BIOS", "Manufacturer");
        GetWMIProperty(pclsObj, L"Name", "WMI-BIOS", "Name");
        GetWMIProperty(pclsObj, L"OtherTargetOS", "WMI-BIOS", "OtherTargetOS");
        GetWMIProperty(pclsObj, L"PrimaryBIOS", "WMI-BIOS", "PrimaryBIOS");
        GetWMIProperty(pclsObj, L"ReleaseDate", "WMI-BIOS", "ReleaseDate");
        GetWMIProperty(pclsObj, L"SerialNumber", "WMI-BIOS", "SerialNumber");
        GetWMIProperty(pclsObj, L"SMBIOSBIOSVersion", "WMI-BIOS", "SMBIOSBIOSVersion");
        GetWMIProperty(pclsObj, L"SMBIOSMajorVersion", "WMI-BIOS", "SMBIOSMajorVersion");
        GetWMIProperty(pclsObj, L"SMBIOSMinorVersion", "WMI-BIOS", "SMBIOSMinorVersion");
        GetWMIProperty(pclsObj, L"SMBIOSPresent", "WMI-BIOS", "SMBIOSPresent");
        GetWMIProperty(pclsObj, L"SoftwareElementID", "WMI-BIOS", "SoftwareElementID");
        GetWMIProperty(pclsObj, L"SoftwareElementState", "WMI-BIOS", "SoftwareElementState");
        GetWMIProperty(pclsObj, L"Status", "WMI-BIOS", "Status");
        GetWMIProperty(pclsObj, L"SystemBiosMajorVersion", "WMI-BIOS", "SystemBiosMajorVersion");
        GetWMIProperty(pclsObj, L"SystemBiosMinorVersion", "WMI-BIOS", "SystemBiosMinorVersion");
        GetWMIProperty(pclsObj, L"TargetOperatingSystem", "WMI-BIOS", "TargetOperatingSystem");
        GetWMIProperty(pclsObj, L"Version", "WMI-BIOS", "Version");

        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pEnumerator->Release();
    return 0;
}

//---------------------------------------------------------------------------
// GetWMIOperatingSystem
//
// get OS data using WMI
int CSysParamList::GetWMIOperatingSystem(IWbemServices *pSvc)
{
    IEnumWbemClassObject* pEnumerator = NULL;
    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_OperatingSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        _ASSERT(!"Query for operating system name failed");
        _TRACE("Query for operating system name failed. Error code = 0x%x", hres);
         return 1;               // Program has failed.
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
        pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
            break;

        VARIANT vtProp;

        // Get the value of the Name property
        GetWMIProperty(pclsObj, L"BootDevice", "WMI-OS", "BootDevice");
        GetWMIProperty(pclsObj, L"BuildNumber", "WMI-OS", "BuildNumber");
        GetWMIProperty(pclsObj, L"BuildType", "WMI-OS", "BuildType");
        GetWMIProperty(pclsObj, L"Caption", "WMI-OS", "Caption");
        GetWMIProperty(pclsObj, L"CodeSet", "WMI-OS", "CodeSet");
        GetWMIProperty(pclsObj, L"CountryCode", "WMI-OS", "CountryCode");
        GetWMIProperty(pclsObj, L"CreationClassName", "WMI-OS", "CreationClassName");
        GetWMIProperty(pclsObj, L"CSCreationClassName", "WMI-OS", "CSCreationClassName");
        GetWMIProperty(pclsObj, L"CSDVersion", "WMI-OS", "CSDVersion");
        GetWMIProperty(pclsObj, L"CSName", "WMI-OS", "CSName");
        GetWMIProperty(pclsObj, L"CurrentTimeZone", "WMI-OS", "CurrentTimeZone");
        GetWMIProperty(pclsObj, L"DataExecutionPrevention_32BitApplications", "WMI-OS", "DataExecutionPrevention_32BitApplications");
        GetWMIProperty(pclsObj, L"DataExecutionPrevention_Available", "WMI-OS", "DataExecutionPrevention_Available");
        GetWMIProperty(pclsObj, L"DataExecutionPrevention_Drivers", "WMI-OS", "DataExecutionPrevention_Drivers");
        GetWMIProperty(pclsObj, L"DataExecutionPrevention_SupportPolicy", "WMI-OS", "DataExecutionPrevention_SupportPolicy");
        GetWMIProperty(pclsObj, L"Debug", "WMI-OS", "Debug");
        GetWMIProperty(pclsObj, L"Description", "WMI-OS", "Description");
        GetWMIProperty(pclsObj, L"Distributed", "WMI-OS", "Distributed");
        GetWMIProperty(pclsObj, L"EncryptionLevel", "WMI-OS", "EncryptionLevel");
        GetWMIProperty(pclsObj, L"ForegroundApplicationBoost", "WMI-OS", "ForegroundApplicationBoost");
        GetWMIProperty(pclsObj, L"FreePhysicalMemory", "WMI-OS", "FreePhysicalMemory");
        GetWMIProperty(pclsObj, L"FreeSpaceInPagingFiles", "WMI-OS", "FreeSpaceInPagingFiles");
        GetWMIProperty(pclsObj, L"FreeVirtualMemory", "WMI-OS", "FreeVirtualMemory");
        GetWMIProperty(pclsObj, L"InstallDate", "WMI-OS", "InstallDate");
        GetWMIProperty(pclsObj, L"LargeSystemCache", "WMI-OS", "LargeSystemCache");
        GetWMIProperty(pclsObj, L"LastBootUpTime", "WMI-OS", "LastBootUpTime");
        GetWMIProperty(pclsObj, L"LocalDateTime", "WMI-OS", "LocalDateTime");
        GetWMIProperty(pclsObj, L"Locale", "WMI-OS", "Locale");
        GetWMIProperty(pclsObj, L"Manufacturer", "WMI-OS", "Manufacturer");
        GetWMIProperty(pclsObj, L"MaxNumberOfProcesses", "WMI-OS", "MaxNumberOfProcesses");
        GetWMIProperty(pclsObj, L"MaxProcessMemorySize", "WMI-OS", "MaxProcessMemorySize");
        //     GetWMIProperty(pclsObj, L"MUILanguages", "WMI-OS", "MUILanguages");
        GetWMIProperty(pclsObj, L"Name", "WMI-OS", "Name");
        GetWMIProperty(pclsObj, L"NumberOfLicensedUsers", "WMI-OS", "NumberOfLicensedUsers");
        GetWMIProperty(pclsObj, L"NumberOfProcesses", "WMI-OS", "NumberOfProcesses");
        GetWMIProperty(pclsObj, L"NumberOfUsers", "WMI-OS", "NumberOfUsers");
        GetWMIProperty(pclsObj, L"OperatingSystemSKU", "WMI-OS", "OperatingSystemSKU");
        GetWMIProperty(pclsObj, L"Organization", "WMI-OS", "Organization");
        GetWMIProperty(pclsObj, L"OSArchitecture", "WMI-OS", "OSArchitecture");
        GetWMIProperty(pclsObj, L"OSLanguage", "WMI-OS", "OSLanguage");
        GetWMIProperty(pclsObj, L"OSProductSuite", "WMI-OS", "OSProductSuite");
        GetWMIProperty(pclsObj, L"OSType", "WMI-OS", "OSType");
        GetWMIProperty(pclsObj, L"OtherTypeDescription", "WMI-OS", "OtherTypeDescription");
        GetWMIProperty(pclsObj, L"PAEEnabled", "WMI-OS", "PAEEnabled");
        GetWMIProperty(pclsObj, L"PlusProductID", "WMI-OS", "PlusProductID");
        GetWMIProperty(pclsObj, L"PlusVersionNumber", "WMI-OS", "PlusVersionNumber");
        GetWMIProperty(pclsObj, L"PortableOperatingSystem", "WMI-OS", "PortableOperatingSystem");
        GetWMIProperty(pclsObj, L"Primary", "WMI-OS", "Primary");
        GetWMIProperty(pclsObj, L"ProductType", "WMI-OS", "ProductType");
        GetWMIProperty(pclsObj, L"RegisteredUser", "WMI-OS", "RegisteredUser");
        GetWMIProperty(pclsObj, L"SerialNumber", "WMI-OS", "SerialNumber");
        GetWMIProperty(pclsObj, L"ServicePackMajorVersion", "WMI-OS", "ServicePackMajorVersion");
        GetWMIProperty(pclsObj, L"ServicePackMinorVersion", "WMI-OS", "ServicePackMinorVersion");
        GetWMIProperty(pclsObj, L"SizeStoredInPagingFiles", "WMI-OS", "SizeStoredInPagingFiles");
        GetWMIProperty(pclsObj, L"Status", "WMI-OS", "Status");
        GetWMIProperty(pclsObj, L"SuiteMask", "WMI-OS", "SuiteMask");
        GetWMIProperty(pclsObj, L"SystemDevice", "WMI-OS", "SystemDevice");
        GetWMIProperty(pclsObj, L"SystemDirectory", "WMI-OS", "SystemDirectory");
        GetWMIProperty(pclsObj, L"SystemDrive", "WMI-OS", "SystemDrive");
        GetWMIProperty(pclsObj, L"TotalSwapSpaceSize", "WMI-OS", "TotalSwapSpaceSize");
        GetWMIProperty(pclsObj, L"TotalVirtualMemorySize", "WMI-OS", "TotalVirtualMemorySize");
        GetWMIProperty(pclsObj, L"TotalVisibleMemorySize", "WMI-OS", "TotalVisibleMemorySize");
        GetWMIProperty(pclsObj, L"Version", "WMI-OS", "Version");
        GetWMIProperty(pclsObj, L"WindowsDirectory", "WMI-OS", "WindowsDirectory");

        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pEnumerator->Release();
    return 0;
}

//---------------------------------------------------------------------------
// CollectWMIData
//
// Collect all WMI Data
int CSysParamList::CollectWMIData()
{
    HRESULT hres;

    IWbemLocator *pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID *)&pLoc);

    if (FAILED(hres))
    {
        _ASSERT(! "Failed to create IWbemLocator");
        _TRACE("Failed to create IWbemLocator object. Error code = 0x%x", hres);
        return 1;                 // Program has failed.
    }

    IWbemServices *pSvc = NULL;

    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (for example, Kerberos)
        0,                       // Context object
        &pSvc                    // pointer to IWbemServices proxy
    );

    if (FAILED(hres))
    {
        _ASSERT(! "Could nto connect");
        _TRACE("Could not connect Error code = 0x%x", hres);
        pLoc->Release();
        return 1;                // Program has failed.
    }

    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities
    );

    if (FAILED(hres))
    {
        _TRACE("Could not set proxy blanket. Error code = 0x");
        _ASSERT(!"Could not set proxy blanket");
        pSvc->Release();
        pLoc->Release();
        return 1;               // Program has failed.
    }

    GetWMIOperatingSystem(pSvc);
    GetWMIBios(pSvc);
    GetWMIService(pSvc);
    GetWMIProcess(pSvc);

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return 0;
}

//---------------------------------------------------------------------------
// GetWMIProperty
//
// get the WMI property and add to the parameter list.
void CSysParamList::GetWMIProperty(IWbemClassObject *pclsObj, WCHAR *pName, const char *pCategory, const char *pDevice)
{
    std::string sValue = "";
    if (GetWMIPropertyValue(pclsObj, pName, sValue))
    {
        //_TRACE("%-10s %-30.30s %s", pCategory, pDevice, sValue.c_str());
        push_back(CSysParam(pCategory, pDevice, sValue.c_str()));
    }
}

//---------------------------------------------------------------------------
// GetWMIPropertyValue
//
// get the wmi property value.
bool CSysParamList::GetWMIPropertyValue(IWbemClassObject *pclsObj, WCHAR *pName, std::string &sValue)
{
    bool bRv = FALSE;
    VARIANT vtProp;
    HRESULT hr = pclsObj->Get(pName, 0, &vtProp, 0, 0);
    if (SUCCEEDED(hr))
    {
        _variant_t vt = vtProp;

        if (V_VT(&vtProp) & VT_ARRAY)
        {

            if (V_VT(&vtProp) & VT_BSTR)
            {
                CComSafeArray<BSTR> sa(vt.parray);
                std::string ss = "";
                for (unsigned int n = 0; n < sa.GetCount(); n++)
                {
                    _bstr_t bs = sa.GetAt(n);
                    if (n > 0)
                        ss += ", ";
                    ss += _com_util::ConvertBSTRToString(bs);
                }
                sValue = ss;
                bRv = true;
            }

        }
        else if (vtProp.vt != VT_NULL)
        {

            _bstr_t sb = vtProp;
            std::string ss = _com_util::ConvertBSTRToString(sb);
            sValue = ss;
            bRv = TRUE;
        }
    }

    VariantClear(&vtProp);
    return bRv;
}

//---------------------------------------------------------------------------
// getCurrentBrowser
//
// get the default browser
bool CSysParamList::getCurrentBrowser(std::string &sBrowser)
{
    //HKEY_LOCAL_MACHINE\SOFTWARE\Clients\StartMenuInternet
    //HKEY_LOCAL_MACHINE\SOFTWARE\Clients\StartMenuInternet\XXX\shell\open\command

    //Computer\HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.html\UserChoice
    HKEY key;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.html\\UserChoice"),0,KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
    {
        USES_CONVERSION;

        TCHAR value[_MAX_PATH + 1] = { '\0' };
        DWORD nSize = _MAX_PATH;
        DWORD nType = REG_SZ;

        RegQueryValueEx(key, _T("Progid"), NULL, &nType, (BYTE *)value, &nSize);

        sBrowser = W2A(value);

        RegCloseKey(key);
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------
// GetMyFileVersion
//
// helper function to get the file version.
//
void GetMyFileVersion(PCHAR pFilePath,std::string *pV)
{
    DWORD  verHandle = 0;
    UINT   size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD  verSize = GetFileVersionInfoSizeA(pFilePath, &verHandle);
    char buf[MAX_PATH + 1] = { '\0' };
    *pV = "";

    if (verSize != NULL)
    {
        LPSTR verData = new char[verSize];

        if (GetFileVersionInfoA(pFilePath, verHandle, verSize, verData))
        {
            if (VerQueryValueA(verData, "\\", (VOID FAR* FAR*)&lpBuffer, &size))
            {
                if (size)
                {
                    VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
                    if (verInfo->dwSignature == 0xfeef04bd)
                    {
                        // Doesn't matter if you are on 32 bit or 64 bit,
                        // DWORD is always 32 bits, so first two revision numbers
                        // come from dwFileVersionMS, last two come from dwFileVersionLS
                        snprintf(buf, MAX_PATH, "%d.%d.%d.%d",
                            (verInfo->dwFileVersionMS >> 16) & 0xffff,
                            (verInfo->dwFileVersionMS >> 0) & 0xffff,
                            (verInfo->dwFileVersionLS >> 16) & 0xffff,
                            (verInfo->dwFileVersionLS >> 0) & 0xffff);
                        *pV = buf;
                    }
                }
                else
                {
                    *pV = "Version Info not found";
                }
            }
            else
            {
                *pV = "Version Info not found";
            }
        }
        else
        {
            *pV = "File not found";
        }
        delete[] verData;
    }
}

//------------------------------------------------------------------------------
// getFileVersions(const std::string &sBinPath)
//
bool CSysParamList::getFileVersions(const std::string &sBinPath)
{
    char pFile[_MAX_PATH + 1] = { '\0' };
    _snprintf_s(pFile, _MAX_PATH, "%s/%s.dll", sBinPath.c_str(), UPDATER_FILENAME);
    std::string sVersion;
    GetMyFileVersion(pFile,&sVersion);
    push_back(CSysParam("Version", UPDATER_FILENAME, sVersion.c_str()));

    _snprintf_s(pFile, _MAX_PATH, "%s/%s.dll", sBinPath.c_str(), BROADCAST_FILENAME);
    GetMyFileVersion(pFile, &sVersion);
    push_back(CSysParam("Version", BROADCAST_FILENAME, sVersion.c_str()));

    return true;
}

#ifndef _SYSPARAM_MAC_H__
#define _SYSPARAM_MAC_H__

#ifdef _WIN32
#include "SysParam.h"
#else

#include <string>
#include <list>

class MfcJsonObj;

bool collectSystemInfo(MfcJsonObj &js);

class CSysParam
{
public:
    CSysParam() = default;
    CSysParam(const char *pCat, const char *pDevice, const char *pDetails)
    {
        m_sCategory = pCat;
        m_sDevice = pDevice;
        m_sDetails = pDetails;

    }
    CSysParam(const CSysParam &src)
    {
        operator=(src);
    }
    const CSysParam &operator=(const CSysParam &src)
    {
        m_sCategory = src.m_sCategory;
        m_sDetails = src.m_sDetails;
        m_sDevice = src.m_sDevice;
        return *this;
    }
    ~CSysParam() = default;

    int ToJson(MfcJsonObj &);

    std::string &getCategory() { return m_sCategory; }
    std::string &getDevice() { return m_sDevice;}
    std::string &getDetails() { return m_sDetails; }

private:
    std::string m_sCategory;
    std::string m_sDevice;
    std::string m_sDetails;
};


class CSysParamList : public std::list<CSysParam>
{
public:
    CSysParamList() = default;
    ~CSysParamList() = default;

    int ToJson(MfcJsonObj &);
    void CollectData(const std::string &sBinPath);

    void get_processor_name(void);
    void get_processor_speed(void);
    void get_processor_cores(void);
    void get_available_memory(void);
    void get_os(void);
    void get_kernel_version(void);
};

#endif  // #ifdef _WIN32

#endif  // _SYSPARAM_MAC_H__

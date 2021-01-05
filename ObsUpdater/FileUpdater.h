#pragma once

#ifndef __FILE_UPDATER__H__
#define __FILE_UPDATER__H__

// solution includes
#include <libPlugins/PluginParameterBlock.h>

class CStringList : public std::list<std::string>
{
public:
    CStringList() = default;
};

class CFileUpdater
{
public:
    CFileUpdater();

    bool doUpdate(DWORD nTimeout);
    bool Stop();
    DWORD Process();

    void ProcessCachedFiles();
    void setUpdateCachePath(const std::string &s) { m_sUpdateCachePath = s; }
    const std::string &getUpdateCachePath() { return m_sUpdateCachePath; }

private:
    std::string m_sUpdateCachePath;

#ifdef _WIN32XXXX
    HANDLE m_hHandle;
#else
    pthread_t m_hHandle;
#endif
};

#endif  //  __FILE_UPDATER__H__

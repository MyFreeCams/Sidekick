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

#pragma once

#include <stdio.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <string>
#include <map>
#include <mutex>

//#include <util/platform.h>
// MFC Includes
#include <nlohmann/json.hpp>
#include <libfcs/Log.h>
#include <libfcs/fcslib_string.h>
#include <libfcs/MfcJson.h>

// Solutions includes
#include <libPlugins/Portable.h>

using std::map;
using std::recursive_mutex;
using std::string;
using std::unique_lock;
using std::vector;

class SidekickModelConfig
{
public:
    SidekickModelConfig() : isSharedCtx(false)
    {
        if (sm_nRefCx++ == 0)
            _TRACE("Initial instance of SidekickModelConfig created.");

        clear();
    }

    SidekickModelConfig(bool sharedCtx) : isSharedCtx(sharedCtx)
    {
        if (sm_nRefCx++ == 0)
            _TRACE("Initial instance of SidekickModelConfig created.");

        clear();
    }

    SidekickModelConfig(const SidekickModelConfig& other)
    {
        if (sm_nRefCx++ == 0)
            _TRACE("Initial instance of SidekickModelConfig created.");

        // lock mutex of ourself or other if either are a sharedCtx
        unique_lock< recursive_mutex > ourLock = sharedLock();
        unique_lock< recursive_mutex > otherLock = other.sharedLock();

        // Copy over jsConfig, but not any other
        // state vars like isSharedCtx or our mutex.
        m_jsConfig = other.m_jsConfig;
    }

    const SidekickModelConfig& operator=(const SidekickModelConfig& other)
    {
        // lock mutex of ourself or other if either are a sharedCtx
        unique_lock< recursive_mutex > ourLock = sharedLock();
        unique_lock< recursive_mutex > otherLock = other.sharedLock();

        // Copy over jsConfig, but not any other
        // state vars like isSharedCtx or our mutex.
        m_jsConfig = other.m_jsConfig;
        return *this;
    }

    virtual ~SidekickModelConfig()
    {
        clear();
        if (--sm_nRefCx == 0)
        {
            // This is crashing on the mac, because the static variables are destroyed BEFORE
            // the ObsBroadcastCtx static variable!
            // preprocess it away for now, it's on exit after all.
#ifdef _WIN32
            sm_vAllocs.clear();
            sm_reqProps.clear();
            sm_initialized = false;
#endif
        }
    }

    unique_lock< recursive_mutex > sharedLock(void) const
    {
        // scoped lock of mutex if we are a sharedCtx instance,
        // this doesnt unlock becase we are moving it in rvalue
        // move semantics
        unique_lock< recursive_mutex >  lk(m_csMutex, std::defer_lock);
        if (isSharedCtx)                lk.lock();
        return lk;
    }

    void clear(void);

    static void initializeDefaults(void);

    bool readProfileConfig(void);
    bool writeProfileConfig(void) const;

    // saves SiekickModelConfig memebr properties to json object
    bool Serialize(MfcJsonObj& js);
    bool Serialize(string& sData);
    bool Deserialize(const string& sData);

    bool    set(const string& sKey, const string& sVal);
#ifndef _WIN32
    bool    set(const string& sKey, time_t nVal);
#endif
    bool    set(const string& sKey, int64_t nVal);
    bool    set(const string& sKey, float dVal);
    bool    set(const string& sKey, bool fVal);
    bool    set(const string& sKey, int nVal);

    bool    getString(const string& sKey, string& sValue) const;
    bool    getFloat(const string& sKey, float& dValue) const;
    bool    getTime(const string& sKey, time_t& nValue) const;
    bool    getBool(const string& sKey, bool& fValue) const;
    bool    getInt(const string& sKey, int64_t& nValue) const;
    bool    getInt(const string& sKey, int& nValue) const;

    // Returns a char* string copy of any string value in config that is found (defaulted or not),
    // otherwise an empty string. As long as sm_vAllocs isn't cleared, this should be safe (meaning
    // the strings don't get freed as soon as the method or object instant go out of scope), although
    // in a multi-threaded scenario this method should have a mutex protecting the sm_vAllocs object
    // to ensure atomic operations on it
    const char* getString(const string& sKey) const;
    float       getFloat(const string& sKey) const;
    time_t      getTime(const string& sKey) const;
    int         getInt(const string& sKey) const;

    bool        isShared(void) const { return isSharedCtx; }


protected:
    // isSharedCtx set to true when this class is the MFCBroadcast's g_ctx instance,
    // which can be used across libraries within the same DLL/EXE and
    // the same process. Other processes may use this object, but won't
    // need to set sharedCtx to true unless they have their own shared/global
    // instance to synchronize access across multiple threads to.
    //
    mutable bool                        isSharedCtx;

    // critical section protection access to shared instances of CBroadcastCtx
    // between threads of same process
    mutable recursive_mutex             m_csMutex;

    MfcJsonObj                          m_jsConfig;

    static bool                         sm_initialized;
    static map< string, MfcJsonObj >    sm_reqProps;
    static vector< string >             sm_vAllocs;
    static size_t                       sm_nRefCx;



#ifdef UNUSED_CODE
public:
    SidekickModelConfig(const string& sFilename, bool sharedCtx = false) : isSharedCtx(sharedCtx)
    {
        if (sm_nRefCx++ == 0)
            _TRACE("Initial instance of SidekickModelConfig created.");

        if (!loadConfig(sFilename))
            _MESG("unable to read file %s", sFilename.c_str());
    }

    // old methods used when loading config from json file on disk
    bool        loadConfig(const string& sFilename);
    size_t      saveConfig(const string& sFilename);
#endif
};

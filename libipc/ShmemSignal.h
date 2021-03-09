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

#include "IPCUtil.h"
#include "ShmemDefines.h"
#include "ShmemVector.h"
#include "ShmemSimpleType.h"
#include "ShmemContainer.h"

#include <boost/thread/mutex.hpp>

namespace MFCIPC
{

//---------------------------------------------------------------------
// ShmemSignal
//
// shared memory container for a IPC signal
template <typename T>
class ShmemSignal
{
public:
    ShmemSignal(const char *pContainer)
            : m_sContainerName(pContainer)
    {
    }
    ShmemSignal(const std::string &s)
            : m_sContainerName(s)
    {}

    //----------------------------------------------------------------------
    // setContainerName
    //
    // named of the shared memory block.
    void setContainerName(const char *p)
    {
        m_sContainerName = p;
    }

    void setContainerName(const std::string &s)
    {
        m_sContainerName = s;
    }

    //---------------------------------------------------------------------
    // operator->
    //
    // not sure this works, but I never implemented the access operator.
    T *operator->()
    {
        return get();
    }

    //---------------------------------------------------------------------
    // get
    //
    // access shared memory object.
    T * get()
    {
        assert(m_sContainerName.size() > 0);
        assert(getShmemManager());
        T *pV = getShmemManager()->template find<T>(m_sContainerName.c_str()).first;
        if (nullptr == pV)
        {
            pV = getShmemManager()->template construct<T>(m_sContainerName.c_str())();
        }
        assert(pV);
        return pV;

    }

    int allocSize()
    {
        return ((sizeof(T) + 4));
    }

    // helper function to return the boost shared memory manager.
    boost::interprocess::managed_shared_memory *getShmemManager() { return getShmemManagerInstance(); }

private:
    std::string m_sContainerName;

};

//---------------------------------------------------------------------
// ShmemSignal
//
// over ride for semaphore.  semaphore constructor takes a parameter
// and mutex doesn't.  There must be a better way to handle this????
template<>
class ShmemSignal <ShmemSemaphore>
{
public:
    ShmemSignal(const char *pContainer)
            : m_sContainerName(pContainer)
    {
    }
    ShmemSignal(const std::string &s)
            : m_sContainerName(s)
    {}

    void setContainerName(const char *p)
    {
        m_sContainerName = p;
    }

    void setContainerName(const std::string &s)
    {
        m_sContainerName = s;
    }

    //-----------------------------------------------------------------
    // get
    //
    // access the shared memory semaphone
    ShmemSemaphore * get()
    {
        assert(m_sContainerName.size() > 0);
        assert(getShmemManager());
        ShmemSemaphore *pV = getShmemManager()->template find<ShmemSemaphore>(m_sContainerName.c_str()).first;
        if (nullptr == pV)
        {
            pV = getShmemManager()->template construct<ShmemSemaphore>(m_sContainerName.c_str())(0);
        }
        assert(pV);
        return pV;

    }

    //-----------------------------------------------------------------
    // timed_wait
    //
    // wait for signal or timeout.
    bool timed_wait(int nSeconds)
    {
        boost::posix_time::time_duration td = boost::posix_time::seconds(nSeconds);

        ShmemSemaphore *p = get();
        assert(p);
        bool b = p->timed_wait(boost::get_system_time() + td);
        m_nMyCnt--;
        return b;
    }

    //-----------------------------------------------------------------
    // try_wait
    //
    // just try to lock teh semaphone, but if it's locked don't wait.
    bool try_wait()
    {
        ShmemSemaphore *p = get();
        assert(p);
        bool b = p->try_wait();
        m_nMyCnt--;
        return b;
    }

    //-----------------------------------------------------------------
    // wait
    //
    // wait for a signal
    void wait()
    {
        auto *p = get();
        assert(p);
        p->wait();
        m_nMyCnt--;
    }

    //-----------------------------------------------------------------
    // post
    //
    // set the signal
    void post()
    {
        m_nMyCnt++;
        _TRACE("Posting semaphore");
        auto *p = get();
        assert(p);
        p->post();
    }

    boost::interprocess::managed_shared_memory *getShmemManager() { return getShmemManagerInstance(); }

private:
    std::string m_sContainerName;
    int m_nMyCnt = 0;
};
}

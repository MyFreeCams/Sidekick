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

#include "ShmemDefines.h"
#include "ShmemVector.h"
#include "ShmemSimpleType.h"

namespace MFCIPC
{
//---------------------------------------------------------------------
// ShmemSimpleType
//
// add simple type to shared memory.
template <typename T>
class ShmemSimpleType
{
public:
    ShmemSimpleType(const char *pContainer)
            : m_sContainerName(pContainer)
    {
        m_sContainerMutexName = m_sContainerName + "_MUTEX";

    }

    //---------------------------------------------------------------------
    // getMutex
    //
    // get the mutex for this object.
    ShmemMutex* getMutex()
    {
        assert(getShmemManager());
        ShmemMutex *pMutex = getShmemManager()->template find<ShmemMutex>(m_sContainerMutexName.c_str()).first;
        if (nullptr == pMutex)
        {
            pMutex = getShmemManager()->template construct<ShmemMutex>(m_sContainerMutexName.c_str())();
            assert(pMutex);
        }
        assert(pMutex);
        return pMutex;
    }

    //-----------------------------------------------------------------
    // access member for the type
    void set(T &v)
    {
        assert(getMutex());
        ShmemLockGuard lock(*getMutex());
        assert(getShmemManager());
        T *pV = getShmemManager()->template find<T>(m_sContainerName.c_str()).first;
        if (nullptr == pV)
        {
            pV = getShmemManager()->template construct<T>(m_sContainerName.c_str())();
        }

        assert(pV);
        *pV = v;
    }

    T  get()
    {
        assert(getMutex());
        ShmemLockGuard lock(*getMutex());
        assert(getShmemManager());
        T *pV = getShmemManager()->template find<T>(m_sContainerName.c_str()).first;
        if (nullptr == pV)
        {
            pV = getShmemManager()->template construct<T>(m_sContainerName.c_str())();
        }
        assert(pV);
        return *pV;

    }

    boost::interprocess::managed_shared_memory *getShmemManager() { return getShmemManagerInstance(); }

private:
    std::string m_sContainerName;
    std::string m_sContainerMutexName;
};

}


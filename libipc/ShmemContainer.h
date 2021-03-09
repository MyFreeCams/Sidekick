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

#include <boost/interprocess/containers/string.hpp>

namespace MFCIPC
{
//---------------------------------------------------------------------
// ShmemStringContainer
//
// shared memory string container.
class ShmemStringContainer
{

public:
    ShmemStringContainer(const char *pContainer, const char *pInitialValue)
            : m_sContainerName(pContainer)
            , m_sInitialValue(pInitialValue)
    {
        m_sContainerMutexName = m_sContainerName + "_MUTEX";

    }

    //---------------------------------------------------------------------
    // getMutex
    //
    // get the shared memory mutex
    ShmemMutex *getMutex()
    {
        ShmemMutex *pMutex = getShmemManager()->template find<ShmemMutex>(m_sContainerMutexName.c_str()).first;
        if (nullptr == pMutex)
        {
            pMutex = getShmemManager()->template construct<ShmemMutex>(m_sContainerMutexName.c_str())();
            assert(pMutex);
        }
        assert(pMutex);
        return pMutex;
    }

    //---------------------------------------------------------------------
    // set
    //
    // set the value into shared memory
    void set(const char *p)
    {
        assert(getMutex());
        ShmemLockGuard lock(*getMutex());
        assert(getShmemManager());
        ShmemString *pV = getShmemManager()->template find<ShmemString>(m_sContainerName.c_str()).first;
        if (nullptr == pV)
        {
            pV = getShmemManager()->template construct<ShmemString>(m_sContainerName.c_str())("1.0.0.0",getShmemManager()->get_segment_manager());
            assert(pV);
            *pV = m_sInitialValue.c_str();
        }
        assert(pV);
        *pV = p;
    }

    //---------------------------------------------------------------------
    // get
    //
    // read the value from shared memory
    std::string get()
    {
        assert(getMutex());
        ShmemLockGuard lock(*getMutex());
        assert(getShmemManager());
        auto *pV = getShmemManager()->template find<ShmemString>(m_sContainerName.c_str()).first;
        if (nullptr == pV)
        {
            pV = getShmemManager()->template construct<ShmemString>(m_sContainerName.c_str())("1.0.0.0",getShmemManager()->get_segment_manager());
            assert(pV);
            *pV = m_sInitialValue.c_str();
        }
        assert(pV);
        std::string s = pV->c_str();
        return s;

    }

    boost::interprocess::managed_shared_memory *getShmemManager() { return getShmemManagerInstance(); }

private:
    std::string m_sContainerName;
    std::string m_sContainerMutexName;
    std::string m_sInitialValue;

};


}

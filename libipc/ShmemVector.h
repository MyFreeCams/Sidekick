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

#include "../libfcs/Log.h"
#include "IPCUtil.h"
#include "ShmemDefines.h"

namespace MFCIPC
{
//---------------------------------------------------------------------
// shared memory vector for storing events and processes.
template <typename T>
class ShmemVector
{

    /*struct _payloadXXX {
        uint64_t m_nKey;
        bool m_bIsInUse;
        boost::posix_time::ptime m_tmCreateDate;
        boost::posix_time::ptime m_tmUpdateTime;
        T user;
    };
*/

    typedef boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>	_ShmemVectorAllocator;
    typedef boost::interprocess::vector<T, _ShmemVectorAllocator>	_ShmemVector;
public:
    ShmemVector(const char *pContainer
            , int nQueSize)
            : m_sContainer(pContainer)
            , m_nQueSize(nQueSize)

    {
        m_sMutexName = m_sContainer + "_MUTEX";
        m_sCtrName = m_sContainer + "_COUNTER";
        m_sCtrMutexName = m_sCtrName + "_MUTEX";
    }

    //-----------------------------------------------------------------
    // getContainerName
    //
    // name of the shared memory container.
    const char *getContainerName() { return m_sContainer.c_str(); }

    //-----------------------------------------------------------------
    // getMutexName
    //
    // name of the shared memory mutex container.
    const char *getMutexName() { return m_sMutexName.c_str(); }

    //-----------------------------------------------------------------
    // getCounterName
    //
    // name of the shared memory block used to generate unique keys.
    const char *getCounterName() { return m_sCtrName.c_str(); }

    //-----------------------------------------------------------------
    // getCounterMutex
    //
    // name of the shared memory block used to generate unique keys.
    const char *getCounterMutexName() { return m_sCtrMutexName.c_str(); }

    int getQueSize()  { return m_nQueSize; }


    //-----------------------------------------------------------------
    // getMutex
    //
    // get the shared memory mutex for this object.
    ShmemMutex* getMutex()
    {

        assert(getShmemManager());


        ShmemMutex *pMutex = getShmemManager()->template find<ShmemMutex>(getMutexName()).first;
        if (! pMutex)
        {
            pMutex = getShmemManager()->template construct<ShmemMutex>(getMutexName())();
        }
#ifndef NDEBUG_CCC
        pMutex->unlock();
#endif
        assert(pMutex);
        return pMutex;
    }

    //-----------------------------------------------------------------
    // getShmemVector
    //
    // get the shared memory based std::vector object.
    _ShmemVector *getShmemVector()
    {
        assert(getShmemManager());

        uint64_t nFree = getShmemManager()->get_free_memory();
        int nNeeded = (MFC_MAX_EVENT_QUE_SIZE * (sizeof(T) + 4));
        assert(nFree > (MFC_MAX_EVENT_QUE_SIZE * (sizeof(T) + 4)) && "You're running out of memory increase the segment size");
        if (nNeeded > (nFree - 4048))
        {
            _TRACE("You're running out of memory increase the segment size. Needed: %d Remaining %d",nNeeded, nFree);
        }
        _ShmemVector *pList = getShmemManager()->template find<_ShmemVector>(m_sContainer.c_str()).first;
        if (nullptr == pList)
        {
            pList = getShmemManager()->template construct<_ShmemVector>(getContainerName())(
                    m_nQueSize
                    , getShmemManager()->get_segment_manager());

            assert(pList);
        }

        assert(pList);
        return pList;
    }

    //---------------------------------------------------------------------
    //  getMessageCtrMutex
    //
    // get key counter mutex
    ShmemMutex* getMessageCtrMutex()
    {
        assert(getShmemManager());
        ShmemMutex *pMutex = getShmemManager()->template find<ShmemMutex>(getCounterMutexName()).first;
        if (nullptr == pMutex)
        {
            pMutex = getShmemManager()->template construct<ShmemMutex>(getCounterMutexName())();
        }
#ifndef NDEBUG_XX
      else
        {
            pMutex->unlock();
        }
#endif

        assert(pMutex);
        return pMutex;
    }

    //-----------------------------------------------------------------
    // getIPCMessageCtr
    //
    // get the key counter from shared memory.
    uint64_t *getIPCMessageCtr()
    {
        assert(getShmemManager());
        uint64_t *pCtr = getShmemManager()->template find<uint64_t>(m_sCtrName.c_str()).first;
        if (nullptr == pCtr)
        {
            pCtr = getShmemManager()->template construct<uint64_t>(m_sCtrName.c_str())();
            *pCtr = 1;
        }
        assert(pCtr);
        return pCtr;
    }

    //---------------------------------------------------------------------
    // genertateKey
    //
    // generate a new key
    uint64_t generateKey()
    {
        assert(getMessageCtrMutex());
        ShmemLockGuard guard(*getMessageCtrMutex());

        uint64_t *pC = getIPCMessageCtr();
        uint64_t n = (*pC)++;

        return n;
    }

    //---------------------------------------------------------------------
    // for_each_all
    //
    // call the lambda for each record in the vector even if the record
    // is not in use.
    //
    // Sshared memory vector data will be locked.
    void for_each_all(std::function<void(T &)> func)
    {

        assert(getMutex());
        ShmemLockGuard guard(*getMutex());
        _ShmemVector *pSCL = getShmemVector();
        std::for_each(pSCL->begin(),pSCL->end(),[&func](T &sc)
        {
             func(sc);
        });

    }

    //---------------------------------------------------------------------
    // for_each
    //
    // call the lambda for each record in the vector that is inuse
    // is not in use.
    //
    // Sshared memory vector data will be locked.
    void for_each(std::function<void(T &)> func)
    {
        assert(getShmemManager());

        assert(getMutex());
        ShmemLockGuard guard(*getMutex());
        _ShmemVector *pSCL = getShmemVector();
        for(auto sc : *pSCL)
        {
            if (sc.m_bIsInUse)
                func(sc);
        }
        /*
        std::for_each(pSCL->begin(),pSCL->end(),[&func](T &sc)
        {
            if (sc.m_bIsInUse)
                func(sc);
        });
*/
    }

    //-----------------------------------------------------------------
    // update
    //
    // update the record.  return false if the key doesn't exist.
    bool update(T &src)
    {
        assert(getShmemVector());
        ShmemLockGuard guard(*getMutex());
        _ShmemVector *pSCL = getShmemVector();
        assert(pSCL);
#ifdef _DEBUG
        uint64_t nSize = pSCL->size();
#endif
        bool bDone = false;
        for (typename _ShmemVector::iterator itr = pSCL->begin(); !bDone && itr != pSCL->end(); itr++)
        {
            T &msg = *itr;
            if (src.getKey() == msg.m_nKey)
            {
                src.m_tmUpdateTime = IPCUtil::Now();
                src.m_bIsInUse = true;
                msg = src;
                bDone = true;

                msg.Log("Updating");

            }
        }
        if (! bDone)
        {
            _TRACE("Vector Slot for Key %d not found.  Not updated",src.m_nKey);
            src.Log("Dropped");
            return false;
        }
        return true;
    }


    //-------------------------------------------------------------------
    // update_add
    //
    // update the vectory recrod and if the record is not found (by key)
    // add it.
    //
    // if we run out of free vector records, reuse the oldest record.
    void update_add(T &src)
    {

        assert(getShmemVector());
        ShmemLockGuard guard(*getMutex());
        _ShmemVector *pSCL = getShmemVector();
        assert(pSCL);
#ifdef _DEBUG
        uint64_t nSize = getShmemManager()->get_size();
        uint64_t nFree = getShmemManager()->get_free_memory();
        uint64_t nListSize = getShmemVector()->size();
#endif
        assert(getShmemManager()->check_sanity());
        assert(nFree > 2047 && "You're running out of memory increase the segment size");

        bool bFnd = false;

        typename _ShmemVector::iterator itrOldest = pSCL->end();
        typename _ShmemVector::iterator itrFirstNotInUse = pSCL->end();
        bool bDone = false;
        for (typename _ShmemVector::iterator itr = pSCL->begin()
                ; itr != pSCL->end() && !bFnd && ! bDone
                ; itr++)
        {

            T &msg = *itr;
            if (msg.m_bIsInUse)
            {
                if (pSCL->end() == itrOldest)
                {
                    itrOldest = itr;
                }
                else if ((*itrOldest).m_tmCreateDate < msg.m_tmCreateDate)
                {
                    itrOldest = itr;
                }

                if (src.m_nKey == msg.m_nKey)
                {
                    src.m_bIsInUse = true;
                    src.m_tmUpdateTime = IPCUtil::Now();
                    msg = src;
                    msg.Log("Updated");
                    bFnd = true;
                }
            }
            else if (pSCL->end() == itrFirstNotInUse)
            {
                itrFirstNotInUse = itr;
                if (src.m_nKey == 0)
                {
                    // it's new bail
                    bDone = true;
                    bFnd = false;

                }
            }
        }

        // first see if this message topic already exists.

        if (!bFnd)
        {
            assert(getShmemVector());
            if (pSCL->end() != itrFirstNotInUse)
            {
                T &msg = *itrFirstNotInUse;
                src.m_bIsInUse = true;
                src.m_tmUpdateTime = IPCUtil::Now();
                src.m_nKey = generateKey();
                src.m_tmCreateDate = src.m_tmUpdateTime = IPCUtil::Now();
                msg = src;
                msg.Log("Added");
            }
            else
            {
                // not empty slots!  use the oldest
                T &msg = *itrOldest;
                src.m_bIsInUse = true;
                src.m_tmUpdateTime = IPCUtil::Now();
                src.m_nKey = generateKey();
                src.m_tmCreateDate = src.m_tmUpdateTime = IPCUtil::Now();
                msg.Log("Dropped");
                msg = src;
                msg.Log("Replacement");
            }

        }

    }

    //-----------------------------------------------------------------
    // remove
    //
    // remove the record with match key.  Call the lambda callback
    // after removing.
    //
    // the lambda function CAN override the remove by setting m_bIsInUse
    // to true;/
    bool remove(uint64_t nKey,std::function<void(T &)> func)
    {
        assert(getShmemVector());
        ShmemLockGuard guard(*getMutex());
        _ShmemVector *pL = getShmemVector();
        typename _ShmemVector::iterator itr = std::find_if(pL->begin(),pL->end(),[nKey](T &sc){ return nKey == sc.m_nKey; });
        if (itr != pL->end())
        {
            T &evt = *itr;
            evt.Log("Removed");
            evt.m_bIsInUse = false;
            func(evt);

            return true;
        }
        return false;
    }

    //-----------------------------------------------------------------
    // log
    //
    // log the contents of the vector
    void log(const char *pMsg)
    {
        _ShmemVector *pL = getShmemVector();
        _TRACE("%s event count: %d",pMsg, pL->size());
        for(auto &e : *pL)
        {
            if (e.m_bIsInUse)
                e.Log("");
        }

    }

    boost::interprocess::managed_shared_memory *getShmemManager() { return getShmemManagerInstance(); }
    
private:
    std::string m_sContainer;
    std::string m_sMutexName;
    std::string m_sCtrName;
    std::string m_sCtrMutexName;


    int m_nQueSize;

};
}
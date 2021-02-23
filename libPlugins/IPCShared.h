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

#ifndef IPC_SHARED_H_
#define IPC_SHARED_H_

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include <string>
#include <cstdlib>
#include <ctime>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/lock_guard.hpp>
//#include <boost/thread/thread.hpp>

#define BUF_SIZE                        1024
#define ADDR_BUF_SIZE                   32

// names of shared memory objects.
#define SHARED_MEMNAME                  "MFCSharedMemoryYYXX"
#define CONTAINER_NAME                  "MyBoostList"
#define MFC_SEMA_NAME                   "fcsSEMA"
#define MFC_CLIENT_MUTEX_NAME           "fcsClientMutex"
#define MFC_SERVER_MUTEX_NAME           "fcsServerMutex"
#define MAX_QUE_SIZE                    20

#define ADDR_FCSLOGIN                   "fcsLoginCEF"
#define ADDR_OBS_BROADCAST_Plugin       "obsBPlugin"
#define ADDR_CEF_JSEXTENSION            "CefJSExtension"

#define MSG_TYPE_PING                   1
#define MSG_TYPE_START                  2
#define MSG_TYPE_SHUTDOWN               3
#define MSG_TYPE_DOLOGIN                4
#define MSG_TYPE_LOGIN_DENY             5
#define MSG_TYPE_LOGIN_AUTH             6
#define MSG_TYPE_SET_MSK                7
#define MSG_TYPE_DOCREDENTIALS          8
#define MSG_TYPE_LOG                    9

using namespace boost::interprocess;
using std::make_unique;
using std::string;
using std::unique_ptr;

namespace MFC_Shared_Mem {

// helper class so the mutex doesn't get stuck
class CSharedMemMsg
{
public:
    CSharedMemMsg()
    {
        memset(m_szBuffer,  '\0', BUF_SIZE);
        memset(m_szTo,      '\0', ADDR_BUF_SIZE);
        memset(m_szFrom,    '\0', ADDR_BUF_SIZE);
        m_nMsgID = 0;
    }

    CSharedMemMsg(const char* pTo, const char* pFrom, int nMsgID, const char* pBuf)
    {
        m_nMsgID = nMsgID;
        strncpy(m_szTo,     pTo,    ADDR_BUF_SIZE);
        strncpy(m_szFrom,   pFrom,  ADDR_BUF_SIZE);
        strncpy(m_szBuffer, pBuf,   BUF_SIZE);
    }

    CSharedMemMsg(const CSharedMemMsg* pSrc)
    {
        operator=(pSrc);
    }

    const CSharedMemMsg& operator=(const char* pBuf)
    {
        strncpy(m_szBuffer, pBuf, BUF_SIZE);
        return *this;
    }

    const CSharedMemMsg& operator=(const CSharedMemMsg* pSrc)
    {
        strncpy(m_szBuffer, pSrc->m_szBuffer,   BUF_SIZE);
        strncpy(m_szTo,     pSrc->m_szTo,       ADDR_BUF_SIZE);
        strncmp(m_szFrom,   pSrc->m_szFrom,     ADDR_BUF_SIZE);
        m_nMsgID = pSrc->m_nMsgID;
        return *this;
    }

    bool operator==(const char* pBuf)  // is this operator needed?  Why would we compare?
    {
        return (strncmp(m_szBuffer, pBuf, BUF_SIZE) == 0);
    }

    // Get the return addr of this package
    char* getFrom() { return m_szFrom; }
    void setFrom(const char* p) { strncpy(m_szFrom, p, ADDR_BUF_SIZE); }

    // Get the address of this memory package.
    const char* getTo() const { return m_szTo; }
    void setTo(const char* p) { strncpy(m_szTo, p, ADDR_BUF_SIZE); }

    // get the messge payload.
    char* getMessage() { return m_szBuffer; }
    void setMessage(const char* p) { strncpy(m_szBuffer, p, ADDR_BUF_SIZE); }

    int getID() { return m_nMsgID; }
    void setID(int n) { m_nMsgID = n; }

private:
    // no std::strings, this object lives in shared memory.
    char m_szTo[ADDR_BUF_SIZE];
    char m_szFrom[ADDR_BUF_SIZE];
    char m_szBuffer[BUF_SIZE];
    int  m_nMsgID;
};


class CDoLoginMessage : public CSharedMemMsg
{
public:
    CDoLoginMessage(const char* pTo, const char* pFrom, const char* pUser, const char* pPwd)
    {
        setTo(pTo);
        setFrom(pFrom);
        setID(MSG_TYPE_DOLOGIN);
        string s = pUser;
        s += ",";
        s += pPwd;
        setMessage(s.c_str());
    }

    CDoLoginMessage(CSharedMemMsg& src)
        : CSharedMemMsg(src)
    {}

    const CDoLoginMessage& operator=(CSharedMemMsg& src)
    {
        CSharedMemMsg::operator=(src);
        return *this;
    }

    string getUser()
    {
        string s = getMessage();
        size_t nComma = s.find(',');
        string sName = "";
        if (nComma != string::npos)
            sName = s.substr(0, nComma);
        return sName;
    }

    string getPwd()
    {
        string s = getMessage();
        size_t nComma = s.find(',');
        string sPwd = "";
        if (nComma != string::npos)
            sPwd = s.substr(nComma + 1);
        return sPwd;
    }
};


class CDoCredentialsMessage : public CSharedMemMsg
{
public:
    CDoCredentialsMessage(const char* pJson)
    {
        setMessage(pJson);
    }

    CDoCredentialsMessage(CSharedMemMsg& src)
        : CSharedMemMsg(src)
    {}

    const CDoCredentialsMessage& operator=(CSharedMemMsg& src)
    {
        CSharedMemMsg::operator=(src);
        return *this;
    }

    string getCredentials()
    {
        string s = getMessage();
        return s;
    }
};


// boost typedefs for readability
typedef boost::interprocess::allocator<CSharedMemMsg, managed_shared_memory::segment_manager>   ShmemListAllocator;
typedef boost::interprocess::list<CSharedMemMsg, ShmemListAllocator>                            ShmemList;
typedef boost::interprocess::interprocess_mutex                                                 ShmemMutex;
typedef boost::lock_guard<ShmemMutex>                                                           ShmemLockGuard;
typedef unique_ptr<boost::interprocess::managed_shared_memory>                                  ManagedShmemPtr;


// this class handles access to the message queue between the processes.
class CMessageManager
{
public:
    CMessageManager()
        : m_pList(nullptr)
        , m_pSegment(nullptr)
        , m_pIPCMutex(nullptr)
        , m_pIPCServerMutex(nullptr)
        , m_pIPCClientMutex(nullptr)
    {}

    ~CMessageManager()
    {
        m_pList = nullptr; // m_pList doesn't belong to us, let boost handle it.
        m_pSegment = nullptr;
        if (isServer())
        {
            // if server, remove the object and start over.
            // commented out because remove() throws an exception in Windows
            // the shared memory object is removed in init prior to creation
            //shared_memory_object::remove(SHARED_MEMNAME);
        }
    }

    bool isInitialized()
    {
        return nullptr != m_pList;
    }

    // Initialize the shared memory objects.
    virtual bool init(bool bServer)
    {
        setServer(bServer);
        bool bRv = true;
        string s;
        if (bServer)
        {
            // if server, remove the object and start over.
            shared_memory_object::remove(SHARED_MEMNAME);
            m_pSegment = make_unique<managed_shared_memory>(open_or_create, SHARED_MEMNAME, 65536);
        }
        else
        {
            try
            {
                m_pSegment = make_unique<managed_shared_memory>(open_only, SHARED_MEMNAME);
            }
            catch (interprocess_exception& e)
            {
                // MFCCefLogin Helper has a security issue on Mac!!!
                s = e.what();
                //_TRACE("Error creating shared memory file: %s", s.c_str());
                m_pSegment = nullptr;
                bRv = false;
                // todo: we need _TRACE!!
            }
        }

        // create or open the shared memory shared memory.
        if (bRv)
        {
            if (bServer)
            {
                // create mutex in shared memory.
                if (m_pSegment->find<ShmemMutex>(MFC_SEMA_NAME).first)
                    m_pSegment->destroy<ShmemMutex>(MFC_SEMA_NAME);
                // note, we don't have to send the memory manager in the second set of ()!!!!!
                // the mutex doesn't need to alloc any memory!
                m_pIPCMutex = m_pSegment->construct<ShmemMutex>(MFC_SEMA_NAME)();
                m_pIPCServerMutex = m_pSegment->construct<ShmemMutex>(MFC_SERVER_MUTEX_NAME)();
                m_pIPCClientMutex = m_pSegment->construct<ShmemMutex>(MFC_CLIENT_MUTEX_NAME)();
            }
            else
            {
                // create mutex in shared memory.
                if (m_pSegment->find<ShmemMutex>(MFC_SEMA_NAME).first)
                {
                    m_pIPCMutex = m_pSegment->find<ShmemMutex>(MFC_SEMA_NAME).first;
                    m_pIPCServerMutex = m_pSegment->find<ShmemMutex>(MFC_SERVER_MUTEX_NAME).first;
                    m_pIPCClientMutex = m_pSegment->find<ShmemMutex>(MFC_CLIENT_MUTEX_NAME).first;
                }
                else
                {
                    //_TRACE("Couldn't find %s to create shared memory mutex", MFC_SEMA_NAME);
                    bRv = false;
                }
            }

            if (bRv)
            {
                // now that we have the mutex, lock it.
                assert(getMutex());
                ShmemLockGuard guard(*(getMutex()));

                // create boost list container object in shared memory.
                m_pList = nullptr;
                if (m_pSegment->find<ShmemList>(CONTAINER_NAME).first)
                    m_pList = m_pSegment->find<ShmemList>(CONTAINER_NAME).first;
                else
                    m_pList = m_pSegment->construct<ShmemList>(CONTAINER_NAME)(m_pSegment->get_segment_manager());
            }
        }
        //_TRACE("CMemoryManager returned %d", bRv);
        return bRv;
    }

    // remove all members from the list.
    void clear()
    {
        assert(m_pList);
        ShmemLockGuard guard(*getMutex());
        m_pList->clear();
    }

#ifdef UNUSED_CODE
    // this just blindly returns the next message.  This is a test only function.
    bool getNextMessage(CSharedMemMsg* pMsg)
    {
        assert(m_pList);
        ShmemLockGuard guard(*getMutex());
        ShmemList::iterator itr = m_pList->begin();
        if (itr != m_pList->end())
        {
            *pMsg = *itr;
            m_pList->pop_front();
            return true;
        }
        return false;
    }
#endif

    // get the next message that is addressed to me.
    bool getNextMessage(CSharedMemMsg* pMsg, const char* pTo)
    {
        bool bDone = false;
        if (m_pList)
        {
            assert(m_pList);
            ShmemLockGuard guard(*getMutex());
            string sTo = boost::to_upper_copy<string>(pTo);

            // walk the list looking for the first message with a matching address.
            for (ShmemList::iterator itr = m_pList->begin(); !bDone && itr != m_pList->end(); itr++)
            {
                CSharedMemMsg msg = *itr;
                string sAddressedTo = boost::to_upper_copy<string>(msg.getTo());
                if (sAddressedTo == sTo)
                {
                    *pMsg = msg;
                    m_pList->erase(itr);
                    bDone = true;
                }
            }
        }
        return bDone;
    }

    // Counts the number of messages in the qeuue addressed to me
    int dump(const char* pTo)
    {
        assert(m_pList);
        ShmemLockGuard guard(*getMutex());
        string sTo = boost::to_upper_copy<string>(pTo);

        int nCnt = 0;
        // walk the list looking for the first message with a matching address.
        for (ShmemList::iterator itr = m_pList->begin(); itr != m_pList->end(); itr++)
        {
            CSharedMemMsg msg = *itr;
            string sAddressedTo = boost::to_upper_copy<string>(msg.getTo());
            if (sAddressedTo == sTo)
            {
                nCnt++;
            }
        }
        return nCnt;
    }

    void sendMessage(const char* pTo, const char* pFrom, int nType, const char* pFormat, ...)
    {
        char buffer[1024] = { '\0' };
        va_list args;
        va_start(args, pFormat);
        vsnprintf(buffer, 1023, pFormat, args);
        va_end(args);
        CSharedMemMsg msg(pTo, pFrom, nType, buffer);
        sendMessage(msg);
    }

    // add a message to the queue
    void sendMessage(const CSharedMemMsg& msg)
    {
        if (m_pList)
        {
            {
                ShmemLockGuard guard(*getMutex());
                if (m_pList->size() > MAX_QUE_SIZE)
                {
                    m_pList->pop_front();
                }
                m_pList->push_back(msg);
            }
            string sTo = msg.getTo();
            if (sTo == ADDR_OBS_BROADCAST_Plugin)
            {
                // this signals the broadcast plugin that a message is pending in the que.
                getBroadcastPluginMutex()->unlock();
            }
            else getClientMutex()->unlock();
        }
    }

    ShmemMutex* getMutex()                  { return m_pIPCMutex;       }
    ShmemMutex* getClientMutex()            { return m_pIPCClientMutex; }
    ShmemMutex* getBroadcastPluginMutex()   { return m_pIPCServerMutex; }

    bool isServer()                         { return m_bIsServer;       }
    void setServer(bool b)                  { m_bIsServer = b;          }

private:
    bool                    m_bIsServer;
    ShmemList*              m_pList;
    ManagedShmemPtr         m_pSegment;

    ShmemMutex*             m_pIPCMutex;
    ShmemMutex*             m_pIPCServerMutex;  // signal the server that there are waiting messages
    ShmemMutex*             m_pIPCClientMutex;  // signal the client there are waiting messages.
};

};  // namespace MFC_Shared_Mem

#endif  // IPC_SHARED_H_

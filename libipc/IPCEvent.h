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

// boost time includes. 
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>

// project includes
#include "EvtDefines.h"
#include "ShmemBase.h"

namespace MFCIPC
{


class CIPCEvent : public SHMEM_BASE
{
    friend class CRouter;
public:
    CIPCEvent()
    {
        clear();
    }

    CIPCEvent(const char *pTopic, const char *pTo, const char *pFrom, int nType, const char *pBuf, boost::posix_time::ptime timeExpiration)
    {
        clear();
        m_nType = nType;
        strncpy(m_szTo,		pTo,	ADDR_BUF_SIZE);
        strncpy(m_szFrom,	pFrom,	ADDR_BUF_SIZE);
        strncpy(m_szBuffer,	pBuf,	EVT_PAYLOAD_SIZE);
        strncpy(m_szTopic, pTopic, TOPIC_BUF_SIZE);

        m_tmExpirationDate = timeExpiration;
        m_bIsInUse = false;

    }

    CIPCEvent(const char *pTopic, const char *pTo, const char *pFrom, int nType)
    {
        clear();
        m_nType = nType;
        strncpy(m_szTo,		pTo,	ADDR_BUF_SIZE);
        strncpy(m_szFrom,	pFrom,	ADDR_BUF_SIZE);
        strncpy(m_szTopic, pTopic, TOPIC_BUF_SIZE);

        m_bIsInUse = false;
    }

    CIPCEvent(const char *pTopic, const char *pTo, const char *pFrom, int nType, boost::posix_time::time_duration tmDuration, const char *pFormat, ...);


    CIPCEvent(const CIPCEvent &src)
    {
        operator=(src);
    }

    virtual ~CIPCEvent()
    {

    }


    //-----------------------------------------------------------------
    // operator=
    //
    // copy the event.
    const CIPCEvent &operator=(const CIPCEvent &src)
    {
        clear();
        m_nKey = src.m_nKey;
        strncpy(m_szTopic, src.m_szTopic,TOPIC_BUF_SIZE);
        strncpy(m_szTo, src.m_szTo,ADDR_BUF_SIZE);
        strncpy(m_szFrom,src.m_szFrom,ADDR_BUF_SIZE);
        strncpy(m_szBuffer,src.m_szBuffer,EVT_PAYLOAD_SIZE);
        m_nType = src.m_nType;
        m_tmCreateDate = src.m_tmCreateDate;
        m_tmExpirationDate = src.m_tmExpirationDate;
        m_tmReadDate = src.m_tmReadDate;
        m_tmUpdateTime = src.m_tmUpdateTime;
        m_bHasBeenRead = src.m_bHasBeenRead;
        m_bIsInUse = src.m_bIsInUse;
        strncpy(m_szReadByBuffer,src.m_szReadByBuffer,READ_BY_BUFFER_SIZE);
        m_nReadFrequency = src.m_nReadFrequency;
        return *this;
    }

    //-----------------------------------------------------------------
    // clear
    //
    // initializse the event contents to blank
    void clear()
    {
        m_nKey = 0;
        m_nType = 0;
        m_bHasBeenRead = false;

        memset(m_szTopic,'\0', TOPIC_BUF_SIZE);
        memset(m_szTo,'\0', ADDR_BUF_SIZE);
        memset(m_szFrom,'\0',ADDR_BUF_SIZE);
        memset(m_szBuffer,'\0',EVT_PAYLOAD_SIZE);
        memset(m_szReadByBuffer,'\0', READ_BY_BUFFER_SIZE);

        m_tmCreateDate = IPCUtil::Now();
        m_tmExpirationDate = IPCUtil::Now();
        m_tmReadDate = boost::posix_time::min_date_time;
        m_tmUpdateTime = boost::posix_time::min_date_time;
        m_bIsInUse = false;
        m_nReadFrequency = EVT_READ_ONCE;

    }


    //-----------------------------------------------------------------
    // From
    //
    //
    // access member for sender of event.
    char* getFrom()
    {
        return m_szFrom;
    }

    void setFrom(const char *p)
    {
        strncpy(m_szFrom, p, ADDR_BUF_SIZE);
    }

    //-----------------------------------------------------------------
    // To
    //
    // access members for address to field.
    const char* getTo() const
    {
        return m_szTo;
    }

    void setTo(const char *p)
    {
        strncpy(m_szTo, p, ADDR_BUF_SIZE);
    }

    //-----------------------------------------------------------------
    // setTopic
    //
    // access member for publist/subscribe topic.
    const char *getTopic()  { return m_szTopic;}
    void setTopic(const char *p)
    {
        memset(m_szTopic,0,TOPIC_BUF_SIZE + 1);
        strncpy(m_szTopic,p,TOPIC_BUF_SIZE);
    }

    //-----------------------------------------------------------------
    // CreateDate
    //
    // access member for time event was created.
    boost::posix_time::ptime getCreateDate() { return m_tmCreateDate; }
    void setCreateDate() { m_tmCreateDate = boost::posix_time::second_clock::local_time();}
    void setCreateDate(boost::posix_time::ptime pt) { m_tmCreateDate = pt;}

    //-----------------------------------------------------------------
    // ExpirationDate
    //
    // access members for event expiration time.
    //
    // expiration time is when maintenance will delete the record
    //
    boost::posix_time::ptime getExpirationDate() { return m_tmExpirationDate; }
    void setExpirationDate() { m_tmExpirationDate = boost::posix_time::second_clock::local_time();}
    void setExpirationDate(boost::posix_time::ptime pt) { m_tmExpirationDate = pt;}


    void setExpirationInSeconds(int nSeconds)
    {
        setExpirationDate(IPCUtil::Now() + boost::posix_time::seconds(nSeconds));
    }

    void setExpirationInHours(int nHours)
    {
        setExpirationDate(IPCUtil::Now() + boost::posix_time::hours(nHours));
    }
    void setExpirationInDays(int nDays)
    {
        setExpirationDate(IPCUtil::Now() + boost::posix_time::hours(nDays * 24));

    }

    //-----------------------------------------------------------------
    // setExpirationDate
    //
    // update expiration date using boost time duration string
    // Expected format for string is "[-]h[h][:mm][:ss][.fff]".
    // A negative duration will be created if the first character in
    // string is a '-', all other '-' will be treated as delimiters.
    // Accepted delimiters are "-:,.".
    //
    void setExpirationDate(const char *p)
    {
        std::string s = p;
        setExpirationDate(s);
    }

    void setExpirationDate(const std::string sDuration)
    {
        boost::posix_time::time_duration td = boost::posix_time::duration_from_string(sDuration);
        m_tmExpirationDate = IPCUtil::Now() + td;
    }
    
    void setExpirationDate(boost::posix_time::time_duration td)
    {
        setExpirationDate(IPCUtil::Now() + td);
    }

    //-----------------------------------------------------------------
    // ReadTime
    //
    // access member for time message was first read.
    boost::posix_time::ptime getReadTime() { return m_tmReadDate; }
    void setReadTime() { m_tmReadDate = IPCUtil::Now();}
    void setReadTime(boost::posix_time::ptime pt) { m_tmReadDate = pt;}

    //-----------------------------------------------------------------
    // UpdateTime
    //
    // Access members for time event was last updated.
    boost::posix_time::ptime getUpdateTime() { return m_tmUpdateTime; }
    void setUpdateTime() { m_tmUpdateTime = IPCUtil::Now();}
    void setUpdateTime(boost::posix_time::ptime pt) { m_tmUpdateTime = pt;}

    //-----------------------------------------------------------------
    // Type
    //
    // access members for message type
    void setType(int nType) { m_nType = nType;}
    int getType() { return m_nType;}

    //-----------------------------------------------------------------
    //  isRead
    //
    // return true if message is read
    //
    // this is for non-publish/subscribe messages.
    void isRead(bool b)
    {
        m_bHasBeenRead = b;
        if (b)
        {
            m_tmReadDate = IPCUtil::Now();
        }

    }
    bool isRead() { return m_bHasBeenRead; }

    //-----------------------------------------------------------------
    // is Expired
    //
    // return true if message is expired.
    bool isExpired()
    {

        boost::posix_time::ptime now = IPCUtil::Now();
        std::string sNow = boost::posix_time::to_simple_string(now);
        
        boost::posix_time::ptime exp = getExpirationDate() + boost::posix_time::seconds( MFC_IPC_EVENT_DELETE_AGE_SECS );;
        std::string sExp = boost::posix_time::to_simple_string(exp);
        
        if (exp < now )
        {
            return true;
        }

        return false;
    }

    //-----------------------------------------------------------------
    // Payload
    //
    // Access members for message body
    void setPayload(const char *pFormat, ...);
    const char *getPayload() const { return m_szBuffer;}

    //-----------------------------------------------------------------
    // Key
    //
    // Access members for generated message key.
    uint64_t getKey() { return m_nKey; }
    void setKey(uint64_t n)  { m_nKey = n; }

    //-----------------------------------------------------------------
    // isInUse
    //
    // Access member for record inuse flag.
    void isInUse(bool b) { m_bIsInUse = b; }
    bool isInUse() { return m_bIsInUse; }

    //-----------------------------------------------------------------
    // update
    //
    // Write the event record back to shared memory.
    void update(bool bTrigger = true);

    //-----------------------------------------------------------------
    // isReadBy
    //
    // return true if the passed id has read the message.
    bool isReadBy(const char *p)
    {
        return isReadBy(std::string(p));
    }
    bool isReadBy(std::string s);

    //-----------------------------------------------------------------
    // getReadBy
    //
    // return the read by list as a json string.
    std::string getReadBy()
    {
        return std::string(m_szReadByBuffer);
    }

    //-----------------------------------------------------------------
    // markAsRead
    //
    // mark the message as read, right now.
    void markAsRead(const std::string &s)
    {
        markAsRead(s, IPCUtil::Now());

    }

    void markAsRead(const char *pID)
    {
        markAsRead(std::string(pID),IPCUtil::Now());
    }
    
    void markAsRead(std::string sID,boost::posix_time::ptime tm);
    void clearIsRead()
    {
        memset(m_szReadByBuffer,READ_BY_BUFFER_SIZE, '\0');
    }

    //-----------------------------------------------------------------
    // getTimeOfLastRead
    //
    // get the time the client last read this message.
    boost::posix_time::ptime getTimeOfLastRead(const char *pID);
    

    //-----------------------------------------------------------------
    // getReadFrequency
    //
    // how often should this message be sent (for postit messages)
    int getReadFrequency() { return m_nReadFrequency; }
    void setReadFrequency(int n) { m_nReadFrequency = n; }

    void Log(const char *p)
    {
        LOG_EVT(p,(*this));
    }

private:
    char m_szTopic[TOPIC_BUF_SIZE + 1];
    char m_szTo[ADDR_BUF_SIZE+1];
    char m_szFrom[ADDR_BUF_SIZE+1];
    char m_szBuffer[EVT_PAYLOAD_SIZE+1];
    char m_szReadByBuffer[READ_BY_BUFFER_SIZE];

    int  m_nType;

    boost::posix_time::ptime m_tmExpirationDate;
    boost::posix_time::ptime m_tmReadDate;

    bool m_bHasBeenRead;
    int m_nReadFrequency;

private:
    // no std::strings, this object lives in shared memory.

};


//---------------------------------------------------------------------
// CMaintenanceEvent
//
// Helper class for creating maitenance events
class CMaintenanceEvent : public CIPCEvent
{
    typedef CIPCEvent _myBase;
public:
    CMaintenanceEvent(const char *pTopic, int nEventType, const char *pDuration,  const char *pFormat, ...);

    const CMaintenanceEvent &operator=(const CIPCEvent &src)
    {
        return static_cast<const CMaintenanceEvent &>(_myBase::operator=(src));
    }

};

}



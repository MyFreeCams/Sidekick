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


//#include <string>

#include <mfc_ipc.h>

#include <nlohmann/json.hpp>

using namespace nlohmann;

namespace MFCIPC
{

CIPCEvent::CIPCEvent(const char *pTopic, const char *pTo, const char *pFrom, int nType, boost::posix_time::time_duration tmDuration, const char *pFormat, ...)
{
    clear();

    char buffer[EVT_PAYLOAD_SIZE + 1] = { '\0' };
    va_list args;
    va_start(args, pFormat);
    vsnprintf(buffer,EVT_PAYLOAD_SIZE, pFormat, args);
    va_end(args);

    m_nType = nType;
    strncpy(m_szTo,		pTo,	ADDR_BUF_SIZE);
    strncpy(m_szFrom,	pFrom,	ADDR_BUF_SIZE);
    strncpy(m_szBuffer,	buffer,	EVT_PAYLOAD_SIZE);
    strncpy(m_szTopic, pTopic, TOPIC_BUF_SIZE);

    m_tmExpirationDate = IPCUtil::Now() + tmDuration;
    m_bIsInUse = false;
}

void CIPCEvent::setPayload(const char *pFormat, ...)
{

    char buffer[EVT_PAYLOAD_SIZE + 1] = { '\0' };
    va_list args;
    va_start(args, pFormat);
    vsnprintf(buffer,EVT_PAYLOAD_SIZE, pFormat, args);
    va_end(args);
    strncpy(m_szBuffer, buffer, EVT_PAYLOAD_SIZE);
}

void CIPCEvent::update(bool bTrigger)
{
    auto *pRTR = CRouter::getInstance();
    pRTR->sendEvent(*this, bTrigger);
}


//-----------------------------------------------------------------
// isReadBy
//
// return true if the passed id has read the message.
bool CIPCEvent::isReadBy(std::string s)
{
    bool bRv = false;
    if (! s.empty() && strlen(m_szReadByBuffer) > 0 )
    {
        json js = {};
        assert(!s.empty());
        if (m_szReadByBuffer[0] != '\0')
            js = json::parse(m_szReadByBuffer);

        if (js.contains(s.c_str()))
        {

            switch (getReadFrequency())
            {
                case EVT_READ_ONCE:
                    bRv = true;
                    break;
                case EVT_READ_HOURLY:
                {
                    time_t t = static_cast<time_t>(js[s.c_str()].get<int>());
                    boost::posix_time::ptime tm = boost::posix_time::from_time_t(t);
                    boost::posix_time::time_duration evtAge = IPCUtil::Now() - tm;
                    if (evtAge.total_seconds() > 60 * 60)
                        bRv = true;
                    else
                        bRv = false;
                    break;

                }
                case EVT_READ_DAILY:
                {
                    time_t t = static_cast<time_t>(js[s.c_str()].get<int>());
                    boost::posix_time::ptime tm = boost::posix_time::from_time_t(t);
                    boost::posix_time::time_duration evtAge = IPCUtil::Now() - tm;
                    if (evtAge.total_seconds() > 60 * 60 * 24)
                        bRv = true;
                    else
                        bRv = false;

                    break;
                }

                case EVT_READ_LOGIN:
                {
                    CRouter *pRTR = CRouter::getInstance();
                    boost::posix_time::ptime tmRegister = pRTR->getRegisterTime();

                    time_t t = static_cast<time_t>(js[s.c_str()].get<int>());
                    boost::posix_time::ptime tm = boost::posix_time::from_time_t(t);
                    if (tmRegister > tm)
                        bRv = true;
                    else
                        bRv = false;

                    break;
                }

            }
        }

    }

    return bRv;
}


//-----------------------------------------------------------------
// markAsRead
//
//
void CIPCEvent::markAsRead(std::string sID, boost::posix_time::ptime tm)
{
    json js = {};
    
    if (strlen(m_szReadByBuffer) > 0)
        js = json::parse(m_szReadByBuffer);
                         
    time_t t = boost::posix_time::to_time_t(tm);
    
    js[sID] = static_cast<uint64_t>(t);
    assert(js.contains(sID));

    std::string s = js.dump();
    assert(strlen(m_szReadByBuffer) + s.size() < READ_BY_BUFFER_SIZE);
    strncpy(m_szReadByBuffer,s.c_str(),READ_BY_BUFFER_SIZE);
}

//---------------------------------------------------------------------
// getTimeOfLastRead
//
// get time event was last read by this client.
boost::posix_time::ptime CIPCEvent::getTimeOfLastRead(const char *pID)
{
    json js = {};
    boost::posix_time::ptime tm = boost::posix_time::min_date_time;
    if (strlen(m_szReadByBuffer) > 0)
        js = json::parse(m_szReadByBuffer);
                         
    if (js.contains(pID))
    {
        time_t t = static_cast<time_t>(js[pID]);
        tm = boost::posix_time::from_time_t(t);
        
    }
    return tm;
}


//---------------------------------------------------------------------
// CMaintenanceEvent
//
// helper class for creating maintenance events. (Because I'm lazy)
CMaintenanceEvent::CMaintenanceEvent(const char *pTopic, int nType, const char *pDuration, const char *pFormat, ...)
        : _myBase(pTopic , MAINT_TO, MAINT_FROM, nType)
{
    char buffer[EVT_PAYLOAD_SIZE + 1] = { '\0' };
    va_list args;
    va_start(args, pFormat);
    vsnprintf(buffer,EVT_PAYLOAD_SIZE, pFormat, args);
    va_end(args);

    setPayload(buffer);
    setExpirationDate(pDuration);
}

}

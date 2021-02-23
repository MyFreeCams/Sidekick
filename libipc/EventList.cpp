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

#include <list>

#ifdef _WIN32
#include <stdio.h>
#endif

#include <Log.h>
#include <fcslib_string.h>
#include "Router.h"
#include "ShmemManager.h"

namespace MFCIPC
{


const CIPCEventList &CIPCEventList::operator=(const CIPCEventList &src)
{
    for(auto e : src)
    {
        push_back(e);
    }
    m_filters = src.m_filters;
    return *this;
}

//-----------------------------------------------------------------
// check
//
// determine if this events should be added to the list.
bool CFilter::check(CIPCEvent &evt)
{
    std::string sClientID = this->getID();
    if (m_key.check(evt.getKey()) >= 0
        && m_topic.check(std::string(evt.getTopic())) >= 0
        && m_to.check(std::string(evt.getTo())) >= 0
        && m_from.check(std::string(evt.getFrom())) >= 0
        && m_isread.check(evt.isReadBy(evt.getTo())) >= 0
        && m_isexpired.check(evt.isExpired()) >= 0
        && m_eventtype.check(evt.getType()) >= 0
        && m_payload.check(std::string(evt.getPayload())) >= 0
            )
    {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------
// fetch
//
// get events from shared mem
 void CIPCEventList::fetch(bool bRemoveAllEvents /* = false */)
{
    if (bRemoveAllEvents)
    {
        erase(begin(), end());
    }
 
    CShmemManager &mman = CShmemManager::getInstance();
    mman.getEventsVectors().for_each([this](CIPCEvent &sc)
        {
            CIPCEvent evt = sc;
            if (getFilters().check(evt))
            {
                if (!findEvent(evt))
                {
                    push_back(evt);
                }
            }
        });

}

//-----------------------------------------------------------------
//
//
//
void CIPCEventList::update(bool bTrigger)
{
    for(CIPCEvent &e : *this)
    {
        e.update(false);
    }
    if (bTrigger)
    {
        CRouter *pR = CRouter::getInstance();
        pR->triggerProcesses();
    }
}

//-----------------------------------------------------------------
// findEvent
//
// helper function to determine if an event is already in this list
bool CIPCEventList::findEvent(CIPCEvent &sc)
{
    // New events will have a key of zero.  Allow them to dup.
    if (sc.getKey() > 0)
    {
        for (auto &e : *this)
        {
            if (e.getKey() > 0 && e.getKey() == sc.getKey())
            {
                return true;
            }
        }
    }
    return false;
}

//-----------------------------------------------------------------
// log
//
// log the events in the list.
void CIPCEventList::log(const char *pMsg)
{
    _TRACE("%s has %d records: ", pMsg, this->size());
    for(auto &e : *this)
    {
        LOG_EVT("",e);
    }
}

}

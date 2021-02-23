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
//#pragma optimize("",off)

#include <string>

#include "../libfcs/Log.h"
#include "../libfcs/fcslib_string.h"
#include <regex>

namespace MFCIPC
{
#define FLT_PARAM_NOT_SET 0
#define FLT_PARAM_ALLOW 1
#define FLT_PARAM_DENY -1


//#include "EvtDefines.h"

template<typename T>
class CFilterParameter
{

public:
    CFilterParameter() {};
    CFilterParameter(const CFilterParameter<T>&src)
    {
        operator=(src);
    }

    ~CFilterParameter() = default;

    bool isSet() { return m_bFlag; }
    T get() { return m_value; }
    void set(T v) { m_value = v; m_bFlag = true; }
    void clear() { m_bFlag = false; }
    int check(T v)
    {
        int nRv = FLT_PARAM_NOT_SET;
        if (isSet())
        {    if (get() == v)
            {
                nRv = FLT_PARAM_ALLOW;
            }
            else
            {
                nRv = FLT_PARAM_DENY;
            }
        }
        return nRv;
    }


    CFilterParameter<T>&operator=(const CFilterParameter<T>&src)
    {
        m_bFlag = src.m_bFlag;
        m_value = src.m_value;
        return *this;
    }

    void log(const char *pMsg)
    {
        std::stringstream ss;
        if (m_bFlag)
        {
            ss << pMsg << " Value: " << m_value;
        }
        else
        {
            ss << pMsg << "Value: NOT SET";

        }
        std::string s = ss.str();
        _TRACE(s.c_str());
    }

private:
    bool m_bFlag = false;
    T m_value;

};

// specialization of template for std::string to support regex.
//
// todo:  there must be a better way to do this. 
template<>
class CFilterParameter <std::string>
{
public:
       CFilterParameter() {};
        CFilterParameter(const CFilterParameter<std::string>&src)
        {
            operator=(src);
        }

        ~CFilterParameter() = default;

        bool isSet() { return m_bFlag; }
        std::string get() { return m_value; }
        void set(std::string v) { m_value = v; m_bFlag = true; }
        void clear() { m_bFlag = false; }
        int check(std::string v)
        {
            int nRv = FLT_PARAM_NOT_SET;
            if (m_bFlag)
            {
                std::string v2 = get();
                int nOffset = static_cast<int>(m_value.find("/:"));
                if (nOffset != -1)
                {
                    std::string s = m_value.substr(nOffset + 2);
                    std::regex re(s.c_str());
                    if (std::regex_match(v,re))
                        nRv = FLT_PARAM_ALLOW;
                    else
                        nRv = FLT_PARAM_DENY;
                }
                else if (IPCUtil::isEqual(v2,v))
                {
                    nRv = FLT_PARAM_ALLOW;
                }
                else
                {
                    nRv = FLT_PARAM_DENY;
                }
            }
            return nRv;
        }

        CFilterParameter<std::string>&operator=(const CFilterParameter<std::string>&src)
        {
            m_bFlag = src.m_bFlag;
            m_value = src.m_value;
            return *this;
        }

        void log(const char *pMsg)
        {
            std::stringstream ss;
            if (m_bFlag)
            {
                ss << pMsg << " Value: " << m_value;
            }
            else
            {
                ss << pMsg << "Value: NOT SET";

            }
            std::string s = ss.str();
            _TRACE(s.c_str());
        }

        private:
        bool m_bFlag = false;
        std::string m_value;

};



class CIPCEvent;
class CFilter
{
public:
    CFilter()
    {}

    ~CFilter() = default;

    void setClientID(const char *p)
    {
        m_sID = p;
    }

    void setClientID(std::string s)
    {
        m_sID = s;
    }

    // because I'm lazy
    void addKey(uint64_t n) { m_key.set(n);}
    void addTopic(const char *p) { m_topic.set(std::string(p)); }
    void addTopic(std::string s) { m_topic.set(s); }

    void addTo(const char *p) { m_to.set(std::string(p)); }
    void addTo(std::string s) { m_to.set(s); }

    void addFrom(const char *p) { m_from.set(std::string(p));}
    void addFrom(std::string s) { m_from.set(s); }

    void addIsRead(bool n) { m_isread.set(n);}
    void addIsExpired(bool n) { m_isexpired.set(n);}
    void addEventType(int n) { m_eventtype.set(n);}

    void addPayload(const char *p) { m_payload.set(std::string(p));}
    void addPayload(std::string s) { m_payload.set(s); }


    CFilterParameter<uint64_t>&getKey() { return m_key; }
    CFilterParameter<std::string>&getTopic() { return  m_topic; }
    CFilterParameter<std::string>&getTo() { return  m_to; }
    CFilterParameter<std::string>&getFrom() { return  m_from; }
    CFilterParameter<bool>&getIsRead() { return  m_isread; }
    CFilterParameter<bool>&getIsExpired() { return  m_isexpired; }
    CFilterParameter<int>&getEventType() { return  m_eventtype; }
    CFilterParameter<std::string>&getPayload() { return  m_payload; }

    CFilter &operator=(const CFilter &src)
    {
        m_key = src.m_key;
        m_topic = src.m_topic;
        m_to = src.m_to;
        m_from = src.m_from;
        m_isread = src.m_isread;
        m_isexpired = src.m_isexpired;
        m_eventtype = src.m_eventtype;
        m_payload = src.m_payload;

        return *this;
    }

    bool check(CIPCEvent &evt);

    std::string getID() { return m_sID; }
    void log(const char *pMsg)
    {
        _TRACE("Filter: %s", pMsg);
        _TRACE("id: %s", m_sID.c_str());
        m_key.log("Key: ");
        m_topic.log("Topic: ");
        m_to.log("To: ");
        m_isread.log("IsRead: ");
        m_isexpired.log("IsExpired: ");
        m_eventtype.log("EventType: ");
    }
private:
    CFilterParameter<uint64_t> m_key;
    CFilterParameter<std::string> m_topic;
    CFilterParameter<std::string> m_to;
    CFilterParameter<std::string> m_from;
    CFilterParameter<bool> m_isread;
    CFilterParameter<bool> m_isexpired;
    CFilterParameter<int> m_eventtype;
    CFilterParameter<std::string>m_payload;

    std::string m_sID;
};

//-----------------------------------------------------------------
// CFilters
//
// filter container.
class CFilters : public std::list<CFilter>
{
public:
    CFilters()
    {
    }
    CFilters(const CFilters &src)
    {
        operator=(src);
    }
    ~CFilters() = default;

    const CFilters &operator=(const CFilters &src)
    {
        for(auto f : src)
        {
            push_back(f);
        }
        return *this;
    }

    //-----------------------------------------------------------------
    // check
    //
    // check if this event meets the criteria of this list.
    bool check(CIPCEvent &evt)
    {
        if (size() > 0)
        {
            for(auto &flt : *this)
            {
                if (flt.check(evt))
                    return true;
            }
            return false;
        }
        return true;
    }
    //-----------------------------------------------------------------
    // addKey
    //
    // filter contents by key.
    void addKey(uint64_t n)
    {
        CFilter flt;
        flt.addKey(n);
        push_back(flt);
    }

    //-----------------------------------------------------------------
    // addTopic
    //
    // filter by topic
    void addTopic(const char *p) { addTopic(std::string(p)); }
    void addTopic(std::string s)
    {
        CFilter flt;
        flt.addTopic(s);
        push_back(flt);
    }

    //-----------------------------------------------------------------
    // addTo
    //
    // filter events by to address
    void addTo(const char *p) { addTo(std::string(p)); }
    void addTo(std::string s)
    {
        CFilter flt;
        flt.addTo(s);
        push_back(flt);
    }

    //-----------------------------------------------------------------
    // addFrom
    //
    // filter events by addFrom address
    void addFrom(const char *p) { addTo(std::string(p)); }
    void addFrom(std::string s)
    {
        CFilter flt;
        flt.addFrom(s);
        push_back(flt);
    }

    //-----------------------------------------------------------------
    // addPayload
    //
    // filter events by addFrom address
    void addPayload(const char *p) { addTo(std::string(p)); }
    void addPayload(std::string s)
    {
        CFilter flt;
        flt.addPayload(s);
        push_back(flt);
    }

    //-----------------------------------------------------------------
    // addIsRead
    //
    // filter events read by client id
    void addIsRead(bool n,const char *pID = "")
    {
        CFilter flt;
        flt.setClientID(pID);
        flt.addIsRead(n);
        push_back(flt);
    }

    //-----------------------------------------------------------------
    // addIsExpired
    //
    // filter by expiration
    void addIsExpired(bool n)
    {
        CFilter flt;
        flt.addIsExpired(n);
        push_back(flt);
    }

    //-----------------------------------------------------------------
    // addEventType
    //
    // filter by event type
    void addEventType(int n)
    {
        CFilter flt;
        flt.addEventType(n);
        push_back(flt);
    }

    //-----------------------------------------------------------------
    // addPendingMaintenanceEvents
    //
    // pending maintenance events NOT ready by this client.
    void addPendingMaintenanceEvents()
    {
        CFilter flt;
        flt.addIsRead(false);
        flt.addEventType(EVT_MAINTENANCE_START);
        push_back(flt);

        flt.addEventType(EVT_MAINTENANCE_END);
        push_back(flt);

        flt.addEventType(EVT_MAINTENANCE_STATUS);
        push_back(flt);
    }

    //------------------------------------------------------------------
    // log
    //
    // log this filter.
    void log(const char *)
    {
        int nCnt = 0;
        for(auto &flt : *this)
        {
            std::string s = stdprintf("Filter: %d",++nCnt);
            flt.log(s.c_str());
        }
    }

};

//---------------------------------------------------------------------
// CIPCEventList
//
// container of events.
class CIPCEventList : public std::list<CIPCEvent>
{
public:
    CIPCEventList()
        : m_filters()
    {}

    CIPCEventList(const CIPCEventList &src)
    {
        operator=(src);
    }

    const CIPCEventList &operator=(const CIPCEventList &src);


    //-----------------------------------------------------------------
    // addFilter
    //
    // add a new filter
    void addFilter(CFilter &flt)
    {
        m_filters.push_back(flt);
    }

    //-----------------------------------------------------------------
    // reset
    //
    // reset the filter list and clear any events.
    void reset()
    {
        m_filters.clear();
        clear();
    }

    //
    // filter functions
    //

    //-----------------------------------------------------------------
    // addKey
    //
    // filter contents by key.
    void addKey(uint64_t n)
    {
        CFilter flt;
        flt.addKey(n);
        m_filters.push_back(flt);
    }

    //-----------------------------------------------------------------
    // addTopic
    //
    // filter by topic
    void addTopic(const char *p) { addTopic(std::string(p)); }
    void addTopic(std::string s)
    {
        m_filters.addTopic(s);
    }

    //-----------------------------------------------------------------
    // addTo
    //
    // filter events by to address
    void addTo(const char *p) { addTo(std::string(p)); }
    void addTo(std::string s)
    {
        m_filters.addTo(s);
    }

    //-----------------------------------------------------------------
    // addFrom
    //
    // filter events by to address
    void addFrom(const char *p) { addFrom(std::string(p)); }
    void addFrom(std::string s)
    {
        m_filters.addFrom(s);
    }

    //-----------------------------------------------------------------
    // addPayload
    //
    // filter events by to address
    void addPayload(const char *p) { addPayload(std::string(p)); }
    void addPayload(std::string s)
    {
        m_filters.addPayload(s);
    }


    //-----------------------------------------------------------------
    // addIsRead
    //
    // filter events read by this client.
    void addIsRead(bool n, const char *pClientID = "")
    {
        m_filters.addIsRead(n, pClientID);
    }

    //-----------------------------------------------------------------
    // addIsExpired
    //
    // filter by expiration
    void addIsExpired(bool n)
    {
        m_filters.addIsExpired(n);
    }

    //-----------------------------------------------------------------
    // addEventType
    //
    // filter by event type
    void addEventType(int n)
    {
        m_filters.addEventType(n);
    }


    //-----------------------------------------------------------------
    // addPendingEvents
    //
    // Filter to events unread by client
    void addPendingEvents(const std::string &s)
    {
        CFilter flt;
        flt.addTo(s.c_str());
        flt.addIsRead(false);
        addFilter(flt);
    }

    void addPendingEvents(const char *pID)
    {
        CFilter flt;
        flt.addTo(pID);
        flt.addIsRead(false);
        size_t max = m_filters.max_size();
        size_t sz = m_filters.size();
        addFilter(flt);
    }

    //-----------------------------------------------------------------
    // addPendingMaintenanceEvents
    //
    // pending maintenance events NOT ready by this client.
    void addPendingMaintenanceEvents()
    {
        m_filters.addPendingMaintenanceEvents();
    }


    //-----------------------------------------------------------------
    // addFilter
    //
    // add a custom filter.
    void addFiter(CFilter &f)
    {
        m_filters.push_back(f);
    }

    //-----------------------------------------------------------------
    // appendFilters
    //
    // append a filter list.  probably a subsription list from a client.
    void appendFilters(CFilters &src)
    {
        for(CFilter &flt : src)
        {
            m_filters.push_back(flt);
        }
    }

    //-----------------------------------------------------------------
    // update
    //
    // trigger the router to process events.
    void update(bool trigger);

    //-----------------------------------------------------------------
    // fetch
    //
    // get events filtered.
    void fetch(bool bRemoveAllEvents = false);

    //-----------------------------------------------------------------
    // log
    //
    // log the events in the list
    void log(const char *);

    CFilters &getFilters() { return m_filters; }

    //---------------------------------------------------------------
    // findEvent
    //
    // find the attached event
    bool findEvent(CIPCEvent &sc);

private:
    CFilters m_filters;
};

}

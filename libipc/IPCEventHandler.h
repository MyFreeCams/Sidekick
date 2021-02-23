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


// boost
#include <boost/shared_ptr.hpp>


#include <regex>
#include <list>


#include "EventList.h"

class CProcessRecord;
class CIPCEventList;
class CIPCEvent;
class CFilter;
class CFilters;

namespace MFCIPC
{
//---------------------------------------------------------------------
// CIPCEventHandlerBase
//
// base class for event handler.
class CIPCEventHandlerBase
{
    friend class CRouter;

public:

    CIPCEventHandlerBase(const char *pID)
    : m_sID(pID)
    {}

    virtual ~CIPCEventHandlerBase() = default;


    //
    // each event handler has an id.  Messages are sent
    // to and from event handlers.
    void setID(const char *p) { m_sID = p;}
    const std::string &getID() { return m_sID; }


    // event handler publish/subscribe
    CFilters &getSubscriptions() { return m_listSubscriptions; }

    //
    // really not used any more, if we create a threaded event handler
    // we might need to start it's back ground thread.
    virtual bool start() { return false;};
    virtual bool stop() { return false;}

    //-----------------------------------------------------------------
    // event handler call backs.
    virtual void ping() {}
    virtual void onIncoming(CIPCEvent &) {}
    virtual void onIncoming(CIPCEventList &) {}

    virtual void onRemoved(CIPCEvent &) {}
    virtual void onRemoved(CIPCEventList &) {}
    virtual void onAttachProcess(CProcessRecord &) { }
    virtual void onDetachProcess(CProcessRecord &) {}

    //
    // lambda functions to handle the call backs.
    //
    // (because I'm lazy and really like lambda functions)
    void setIncomingFunc(std::function<void(CIPCEvent &)>f) { m_funcIncoming = f;}
    void setIncomingFunc(std::function<void(CIPCEventList &)>f) { m_funcIncomingList = f;}

    void setRemoveFunc(std::function<void(CIPCEvent &)>f) { m_funcRemoved = f;}
    void setRemoveFunc(std::function<void(CIPCEventList &)>f) { m_funcRemovedList = f; }

    void setAttachProcessFunc(std::function<void(CProcessRecord &)>f) { m_funcAttachProcess = f;}
    void setDetachProcessFunc(std::function<void(CProcessRecord &)>f) { m_funcDetachProcess = f;}

    //
    // get subscribed events
    //
    // fetch our subscribed events
    void getSubscribedEvents(CIPCEventList &el)
    {
        el.appendFilters(getSubscriptions());
        el.fetch();
    }
protected:

    //
    // helper functions to post events to the call backs.
    // these are called by the router when it wants to
    // post events.
    virtual void postIncomingEvent(CIPCEvent &evt) = 0;
    virtual void postIncomingEvent(CIPCEventList &que) = 0;
    virtual void postRemoveEvent(CIPCEventList &el) = 0;
    virtual void postRemoveEvent(CIPCEvent &evt) = 0;
    virtual void postAttachProcess(CProcessRecord &r) = 0;
    virtual void postDetachProcess(CProcessRecord &r) = 0;

    std::function<void(CIPCEventList &)>m_funcIncomingList = nullptr;
    std::function<void(CIPCEventList &)>m_funcRemovedList = nullptr;
    std::function<void(CIPCEvent &)>m_funcIncoming = nullptr;
    std::function<void(CIPCEvent &)>m_funcRemoved = nullptr;
    std::function<void(CProcessRecord &)>m_funcAttachProcess = nullptr;
    std::function<void(CProcessRecord &)>m_funcDetachProcess = nullptr;

    std::string m_sID;                              // id of this thread
    CFilters m_listSubscriptions;          // list of evt subscriptions.
};

//---------------------------------------------------------------------
// CSimpleEventHandler
//
// Non-thread implementation of event handler.
class CSimpleEventHandler : public CIPCEventHandlerBase
{
    friend class CRouter;
    typedef CIPCEventHandlerBase _myBase;
public:
    CSimpleEventHandler(const char *p);
    ~CSimpleEventHandler();

protected:

    virtual void postIncomingEvent(CIPCEvent &evt) override;
    virtual void postIncomingEvent(CIPCEventList &que) override;
    virtual void postRemoveEvent(CIPCEventList &el) override;
    virtual void postRemoveEvent(CIPCEvent &evt) override;
    virtual void postAttachProcess(CProcessRecord &r) override;
    virtual void postDetachProcess(CProcessRecord &r) override;


};

}



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

#include <string>


#include "mfc_ipc.h"
#include "ShmemDefines.h"
#include "ShmemVector.h"
#include "ShmemSimpleType.h"
#include "ShmemContainer.h"
#include "ShmemSignal.h"
#include "Router.h"

namespace MFCIPC
{
//---------------------------------------------------------------------
// CSimpleEventHandler
//
//
CSimpleEventHandler::CSimpleEventHandler(const char *p)
: _myBase(p)
{
    CRouter *pR = CRouter::getInstance();
    pR->registerEvtHandler(this);

}

CSimpleEventHandler::~CSimpleEventHandler()
{
    CRouter *pR = CRouter::getInstance();
    pR->deregisterEvtHandler(this);
}

//---------------------------------------------------------------------
// postIncomingEvent
//
// post incoming event list to the call back functions/lambda.
void CSimpleEventHandler::postIncomingEvent(CIPCEventList &el)
{
    if (nullptr == m_funcIncomingList)
    {
        onIncoming(el);
    }
    else
    {
        m_funcIncomingList(el);

    }

    for(auto &e : el)
    {
        e.isRead(true);
        e.markAsRead(getID());
        e.update(false);
        if (nullptr == m_funcIncoming)
            onIncoming(e);
        else
            m_funcIncoming(e);
    }

}

//---------------------------------------------------------------------
// postIncomingEvent
//
// post incoming event to the call back function/lambda
void CSimpleEventHandler::postIncomingEvent(CIPCEvent &e)
{
    if (nullptr == m_funcIncoming)
        onIncoming(e);
    else
        m_funcIncoming(e);
}

//---------------------------------------------------------------------
// postRemoveEvent
//
// post remove event list to the callback function/lambda.
void CSimpleEventHandler::postRemoveEvent(CIPCEventList &el)
{
    if (nullptr == m_funcRemovedList)
    {
        onRemoved(el);
    }
    else
    {
        m_funcRemovedList(el);

    }

    for(auto &e : el)
    {
        if (nullptr == m_funcIncoming)
            onRemoved(e);
        else
            m_funcRemoved(e);
    }

}

//---------------------------------------------------------------------
// postRemoveEvent
//
// post remove event to the call functionj/lambda
void CSimpleEventHandler::postRemoveEvent(CIPCEvent &e)
{
    if (nullptr == m_funcRemoved)
        onRemoved(e);
    else
        m_funcRemoved(e);
}

//---------------------------------------------------------------------
// postAttachEvent
//
// post remove event to the call functionj/lambda
void CSimpleEventHandler::postAttachProcess(CProcessRecord &e)
{
    if (nullptr == m_funcAttachProcess)
        onAttachProcess(e);
    else
        m_funcAttachProcess(e);
}

//---------------------------------------------------------------------
// postDeattachEvent
//
// post remove event to the call functionj/lambda
void CSimpleEventHandler::postDetachProcess(CProcessRecord &e)
{
    if (nullptr == m_funcDetachProcess)
        onDetachProcess(e);
    else
        m_funcDetachProcess(e);
}

}


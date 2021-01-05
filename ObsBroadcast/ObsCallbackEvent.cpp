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

// system includes
#include <mutex>

// solution includes
#include "ObsCallbackEvent.h"

std::recursive_mutex g_mutex;
CEventList g_evt;

//---------------------------------------------------------------------------
// getEventList
//
// event list is a singleton
CEventList& CEventList::getEventList()
{
    return g_evt;
}

//---------------------------------------------------------------------------
// push
//
// push an event to the end of the list.
void CEventList::push(CObsCallBackEvent& evt)
{
    std::lock_guard<std::recursive_mutex>guard(g_mutex);
    m_list.push_back(evt);
}

//---------------------------------------------------------------------------
// pop
//
// remove and return the first event in the list.  return false if the list
// is empty
bool CEventList::pop(CObsCallBackEvent* pevt)
{
    std::lock_guard<std::recursive_mutex>guard(g_mutex);
    if (!m_list.empty())
    {
        CObsCallBackEvent e = m_list.front();
        m_list.pop_front();
        *pevt = e;
        return true;
    }
    return false;
}

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

#include <list>

//---------------------------------------------------------------------------
// Obs call back event.
class CObsCallBackEvent
{
public:
    CObsCallBackEvent() = default;

#ifdef _WIN32
    explicit CObsCallBackEvent(enum obs_frontend_event event)
#else
    explicit CObsCallBackEvent(int event)
#endif
    {
        m_event = event;
    }

    CObsCallBackEvent(const CObsCallBackEvent& src)
    {
        operator=(src);
    }

    const CObsCallBackEvent &operator=(const CObsCallBackEvent& src)
    {
        m_event = src.m_event;
        return *this;
    }

    int getEvent() { return m_event; }

private:
#ifdef _WIN32
    enum obs_frontend_event m_event;
#else
    int m_event;
#endif
};


//---------------------------------------------------------------------------
// event list.
class CEventList
{
public:
    CEventList() = default;

    void push(CObsCallBackEvent&);
    bool pop(CObsCallBackEvent* pE);
    static CEventList& getEventList();

private:
    std::list< CObsCallBackEvent> m_list;
};

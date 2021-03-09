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


#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include "IPCSemaphore.h"


namespace MFCIPC
{
CSemaphore::CSemaphore (int n /*= 0*/)
    : m_nCnt(n)
    , m_spMutex(new boost::mutex)
    , m_spCond(new boost::condition_variable)
{}

CSemaphore::CSemaphore ()
    : m_nCnt(0)
    , m_spMutex(new boost::mutex)
    , m_spCond(new boost::condition_variable)
{}

CSemaphore::~CSemaphore()
{

}

void CSemaphore::post()
{
    boost::mutex::scoped_lock lock(*m_spMutex);
    m_nCnt++;
    m_spCond->notify_one();
}

void CSemaphore::wait()
{
    boost::mutex::scoped_lock  lock(*m_spMutex);
    while(m_nCnt == 0)
    {
        
        m_spCond->wait(lock);
    }
    m_nCnt--;
}

bool CSemaphore::timed_wait(int nMilliseconds)
{
    bool bRv = false;
    boost::mutex::scoped_lock lock(*m_spMutex);
    if (m_nCnt == 0)
    {
        if (m_spCond->timed_wait(lock,boost::posix_time::milliseconds(nMilliseconds)))
        {
            m_nCnt--;
            bRv = true;
        }
    }
    return bRv;
}

void CSemaphore::setCount(int nCnt)
{
     boost::mutex::scoped_lock lock(*m_spMutex);
     m_nCnt = nCnt;
}





CIPCSemaphore::CIPCSemaphore (int n /*= 0*/)
    : m_nCnt(n)
    , m_spMutex(new boost::mutex)
    , m_spCond(new boost::condition_variable)
{}

CIPCSemaphore::CIPCSemaphore ()
    : m_nCnt(0)
    , m_spMutex(new boost::mutex)
    , m_spCond(new boost::condition_variable)
{}

CIPCSemaphore::~CIPCSemaphore()
{

}

void CIPCSemaphore::post()
{
  boost::mutex::scoped_lock lock(*m_spMutex);
  m_nCnt++;
  m_spCond->notify_one();
}

void CIPCSemaphore::wait()
{
  boost::mutex::scoped_lock  lock(*m_spMutex);
  while(m_nCnt == 0)
  {

    m_spCond->wait(lock);
  }
  m_nCnt--;
}

bool CIPCSemaphore::timed_wait(int nMilliseconds)
{
  bool bRv = false;
  boost::mutex::scoped_lock lock(*m_spMutex);
  if (m_nCnt == 0)
  {
    if (m_spCond->timed_wait(lock,boost::posix_time::milliseconds(nMilliseconds)))
    {
      m_nCnt--;
      bRv = true;
    }
  }
  return bRv;
}

void CIPCSemaphore::setCount(int nCnt)
{
  boost::mutex::scoped_lock lock(*m_spMutex);
  m_nCnt = nCnt;
}
}
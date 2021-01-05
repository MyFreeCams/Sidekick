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

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <vector>
#include <string>


using namespace std;

#include "fcslib_string.h"

#ifndef _WIN32
#include <sys/time.h>
#else
#include "UtilCommon.h"
#endif

class MfcTimer
{
public:
	MfcTimer()
	{
		m_tvStart.tv_sec = 0, m_tvStart.tv_usec = 0;
		m_tvStop = m_tvStart;
		m_nDiff = 0;
		m_dSeconds = 0;
		//m_nObjectTm = time(NULL);
	}

	MfcTimer(bool fStart)
	{
		m_tvStart.tv_sec = 0, m_tvStart.tv_usec = 0;
		m_tvStop = m_tvStart;
		m_nDiff = 0;
		m_dSeconds = 0;

		if (fStart)
			Start();
	}

	virtual ~MfcTimer()
	{
	}


    static time_t Now(time_t* pNow)
    {
        gettimeofday(&sm_tvNow, NULL);
#ifdef WIN32
		localtime_s(&sm_ctNow, (const time_t*)&sm_tvNow.tv_sec);
#else
		sm_ctNow = *(localtime((const time_t*)&sm_tvNow.tv_sec));
#endif
        *pNow = sm_tvNow.tv_sec;
        return sm_tvNow.tv_sec;
    }
    static struct timeval Now(struct timeval* pTmVal)
    {
        gettimeofday(&sm_tvNow, NULL);
#ifdef WIN32
		localtime_s(&sm_ctNow, (const time_t*)&sm_tvNow.tv_sec);
#else
		sm_ctNow = *(localtime((const time_t*)&sm_tvNow.tv_sec));
#endif
        *pTmVal = sm_tvNow;
        return sm_tvNow;
    }
    static struct tm Date(void)
    {
        gettimeofday(&sm_tvNow, NULL);
#ifdef WIN32
		localtime_s(&sm_ctNow, (const time_t*)&sm_tvNow.tv_sec);
#else
		sm_ctNow = *(localtime((const time_t*)&sm_tvNow.tv_sec));
#endif
        return sm_ctNow;
    }
    static time_t   Now(void)           { return sm_tvNow.tv_sec;           }
    static int      Year(void)          { return sm_ctNow.tm_year + 1900;   }
    static int      Month(void)         { return sm_ctNow.tm_mon + 1;       }
    static int      Day(void)           { return sm_ctNow.tm_mday;          }
    static int      Hour(void)          { return sm_ctNow.tm_hour;          }
    static int      Min(void)           { return sm_ctNow.tm_min;           }
    static int      Sec(void)           { return sm_ctNow.tm_sec;           }


	void Start(void)
    {
        MfcTimer::Now(&m_tvStart);
    }

	double Stop(void)
	{
		gettimeofday(&m_tvStop, NULL);
        sm_tvNow = m_tvStart;
        m_nDiff = DiffMicro(m_tvStop, m_tvStart);
		return (m_dSeconds = (double)m_nDiff / (double)1000000);
	}

	double Restart(void)
	{
		double dRet = Stop();
		Start();
		return dRet;
	}

    //
    // Static helper functions that can work from any two timevals to calculate elapsed time
    //

    // is tvFirst >= tvSecond?
    static bool LaterThan(const struct timeval& tvFirst, const struct timeval& tvSecond)
    {
        return (tvFirst.tv_sec > tvSecond.tv_sec || (tvFirst.tv_sec == tvSecond.tv_sec && tvFirst.tv_usec >= tvSecond.tv_usec));
    }

    // returns the number of seconds, in double floating point percision, between start and stop timevals
    static double DiffTime(const struct timeval& tvStop, const struct timeval& tvStart)
    {
        return (((double)DiffMicro(tvStop, tvStart)) / 1000000.0);
    }

    // returns the number of microseconds between start and stop timevals
    static uint64_t DiffMicro(const struct timeval& tvStop, const struct timeval& tvStart)
    {
        return (uint64_t)((tvStop.tv_sec - tvStart.tv_sec) * 1000000) + (tvStop.tv_usec - tvStart.tv_usec);
    }


	double Seconds(void) { return m_dSeconds; }

	struct timeval m_tvStart;
	struct timeval m_tvStop;
	uint64_t m_nDiff;
	double m_dSeconds;

    static struct timeval sm_tvNow;
    static struct tm sm_ctNow;
};

/*
class ProfTimer :
    public MfcTimer
{
public:
    ProfTimer() : MfcTimer()
    {
        m_pCurSeg = NULL;
        m_nProfDiff = 0;
        m_dProfSeconds = 0;
        m_dProfRatio = 0;
    }

    ProfTimer(bool start) : MfcTimer(start)
    {
        m_pCurSeg = NULL;
        m_nProfDiff = 0;
        m_dProfSeconds = 0;
        m_dProfRatio = 0;
    }

    virtual ~ProfTimer()
    {
        clear();
    }

    void Restart(void)
    {
        clear();
        Start();
    }

    bool SegStart(void)
    {
        bool retVal = false;

        if (m_pCurSeg == NULL)
        {
            m_pCurSeg = new MfcTimer(true);
            sm_stActiveObjs.insert(this);
            retVal = true;
        }

        return retVal;
    }

    double SegStop(void)
    {
        double retVal = -1;
        
        if (m_pCurSeg)
        {
            // Stop this segment
            retVal = m_pCurSeg->Stop();

            // Remove ourself from the shared set of instances that are currently open (leaving just our parents)
            sm_stActiveObjs.erase(this);

            // Adjust any parents with open spans so as not to account for our time spent in their reporting
            for (set< ProfTimer* >::iterator i = sm_stActiveObjs.begin(); i != sm_stActiveObjs.end(); ++i)
            {
                if (LaterThan(m_pCurSeg->m_tvStart, (*i)->m_tvStart))
                {
                    (*i)->m_vAdjustments.push_back(m_pCurSeg->m_nDiff);
                    sm_nOverlaps++;
                }
                else
                {
                    (*i)->m_vAdjustments.push_back(DiffMicro(m_pCurSeg->m_tvStop, (*i)->m_tvStart));
                    sm_nUnalignedOverlaps++;
                }
            }

            // Save this segment of time to profile times vector, and set current segment to null so as to 
            // mark ourself as inactive currently
            m_vSegments.push_back(m_pCurSeg);
            m_pCurSeg = NULL;
        }

        return retVal;
    }

    double Stop(void)
    {
        SegStop();
        MfcTimer::Stop();

        m_nProfDiff = 0;
        m_dProfRatio = 0;
        m_dProfSeconds = 0;

        if (m_nDiff > 0)
        {
            // Build the specific times profiles now
            for (size_t n = 0; n < m_vSegments.size(); n++)
            {
                m_nProfDiff += m_vSegments[n]->m_nDiff;
                m_dProfSeconds += m_vSegments[n]->m_dSeconds;
            }

            // Adjust for any timers running overlapping our segments
            uint64_t nAdjust = 0;
            for (size_t n = 0; n < m_vAdjustments.size(); n++)
                nAdjust += m_vAdjustments[n];

            if (m_nProfDiff > nAdjust)
            {
                m_nProfDiff -= nAdjust;
                m_dProfSeconds -= ((double)nAdjust / 1000000.0);
            }

            // Calculate the % of time spent in profiled segments compared to overall span from start to stop
            if (m_nDiff > m_nProfDiff)
                m_dProfRatio = ((double)m_nProfDiff * 100) / (double)m_nDiff;
        }

        clearSegments();

        return m_dSeconds;
    }

    void clear(void)
    {
        m_nProfDiff = 0;
        m_dProfSeconds = 0;
        m_dProfRatio = 0;

        clearSegments();
    }

    void clearSegments(void)
    {
        // We shouldnt be clearing ourself if we're actively profiling, but if we do - we still need to remove this object instance
        // from sm_stActiveObjs, if its there, and delete the current segment (discarding it, since SegStop() wasn't called first)
        if (m_pCurSeg)
        {
            sm_stActiveObjs.erase(this);
            delete m_pCurSeg;
            m_pCurSeg = NULL;
        }

        // Free up any segments, since we're discarding any data we may have collected
        for (size_t n = 0; n < m_vSegments.size(); n++)
            delete m_vSegments[n];
        m_vSegments.clear();
        m_vAdjustments.clear();
    }


    MfcTimer* m_pCurSeg;                            // Created by SegOpen()
	vector< MfcTimer* > m_vSegments;                // Any previous segments already stopped with SegStop() before final Stop() call
    vector< uint64_t > m_vAdjustments;              // Saves any adjustments to be applied to m_pCurSeg once stopped

    uint64_t m_nProfDiff;
    double m_dProfSeconds;
    double m_dProfRatio;

    static set< ProfTimer* > sm_stActiveObjs;       // Contains instances of any open segments so adjustments can be made for overlaps
    static size_t sm_nOverlaps;                     // Counter for how many times a Segment overlaps another evenly
    static size_t sm_nUnalignedOverlaps;            // Counter for how many times a segment partially overlaps another
};

*/

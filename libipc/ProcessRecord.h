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

#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include "ShmemBase.h"

namespace MFCIPC
{
//---------------------------------------------------------------------
// CProcessRecord
//
// Shared memory process object.
class CProcessRecord : public SHMEM_BASE
{
    friend class CRouter;
public:
    CProcessRecord()
    {
        clear();
    }
    CProcessRecord(const char *pID)
    {
        clear();
        setID(pID);
    }

    virtual ~CProcessRecord() {}

    //-----------------------------------------------------------------
    // operator=
    //
    // copy operator.
    CProcessRecord &operator=(CProcessRecord &src)
    {
        strncpy(pID,src.pID, ADDR_BUF_SIZE);
        strncpy(m_pMutexName,src.m_pMutexName,ADDR_BUF_SIZE);
        tmRegister = src.tmRegister;
        tmLastCheckin = src.tmLastCheckin;
        m_tmUpdateTime = src.m_tmUpdateTime;
        m_tmLastMaintenance = src.m_tmLastMaintenance;
        m_nMaintenanceBid = src.m_nMaintenanceBid;
        m_bIsInUse = src.m_bIsInUse;
        m_nKey = src.m_nKey;
        m_tmCreateDate = m_tmCreateDate;
        return *this;

    }

    //-------------------------------------------------------------------
    // clear
    //
    // zero all the fields.
    void clear()
    {

        memset(pID,'\0', ADDR_BUF_SIZE);
        memset(m_pMutexName,'\0',ADDR_BUF_SIZE);

        tmRegister = boost::posix_time::min_date_time;
        tmLastCheckin = boost::posix_time::min_date_time;
        m_tmUpdateTime = boost::posix_time::min_date_time;
        m_tmLastMaintenance = boost::posix_time::min_date_time;
        m_nMaintenanceBid = 0;
        m_bIsInUse = false;
        m_nKey = 0;
        m_tmCreateDate = boost::posix_time::min_date_time;
    }

    //-----------------------------------------------------------------
    // isActive
    //
    // Using last update time, determine if this process is active.
    inline bool isActve()
    {
        boost::posix_time::ptime tmNow = boost::posix_time::second_clock::local_time();
        boost::posix_time::time_duration tmDiff = tmNow - tmLastCheckin;

        if (tmDiff.total_seconds() < MFC_IPC_INACTIVE_TIMEOUT_SECS)
            return true;
        return false;
    }

    //-----------------------------------------------------------------
    // getID
    //
    // each process has an id assign to it at compile time. .
    const char *getID() const { return pID; }
    void setID(const char *p) { strncpy(pID,p,ADDR_BUF_SIZE); }
    void setID(const std::string &s) { setID(s.c_str()); }

    //-----------------------------------------------------------------
    // getMutexName
    //
    // Mutex that triggers the process to process messages.
    // It is the process name with "_mutex__" appended.
    // see createMutexName
    const char *getMutexName() const { return m_pMutexName; }
    void setMutexName(const char *p) { strncpy(m_pMutexName,p,ADDR_BUF_SIZE); }
    void setMutexName(const std::string &s) { setMutexName(s.c_str()); }

    //-----------------------------------------------------------------
    // getRegisterTime
    //
    // get the time the process registered.  (started)
    const boost::posix_time::ptime getRegisterTime() const { return tmRegister; }
    void setRegisterTime(const boost::posix_time::ptime tm) { tmRegister = tm; }

    //-----------------------------------------------------------------
    // getLastCheckInTime
    //
    // get the last time the process recorded a ping.
    const boost::posix_time::ptime getLastCheckinTime() const  { return tmLastCheckin;}
    void setLastCheckinTime(const boost::posix_time::ptime tm) { tmLastCheckin = tm; }

    //-----------------------------------------------------------------
    // getLastUpdateTime
    //
    // last time the process record was updated.
    const boost::posix_time::ptime &getLastUpdateTime() const { return m_tmUpdateTime;  }
    void setLastUpdateTime(const boost::posix_time::ptime mTmUpdateTime) { m_tmUpdateTime = mTmUpdateTime;  }

    //-----------------------------------------------------------------
    // getLastMaintenanceTime
    //
    // last time this process performed maitenance
    const boost::posix_time::ptime getLastMaintenanceTime() const  { return m_tmLastMaintenance;}
    void setLastMaintenanceTime(const boost::posix_time::ptime tm) { m_tmLastMaintenance = tm; }

    //-----------------------------------------------------------------
    // getMaintenanceBid
    //
    // Process with the highest maintenance bid performs maintenance.
    // assigned at compile time by developers.
    int getMaintenanceBid() { return m_nMaintenanceBid; }
    void setMaintenanceBid(int n) { m_nMaintenanceBid = n; }

    //-----------------------------------------------------------------
    // isInUse
    //
    // return true if this maintenance record is in use.
    bool isInUse() { return m_bIsInUse; }
    void isInUse(bool b)  { m_bIsInUse = b; }

    //-----------------------------------------------------------------
    // getKey
    //
    // shared memory record primary key assigned automatically by
    // ShmemVector when adding new records.
    uint64_t getKey() { return m_nKey; }
    void setKey(uint64_t n) { m_nKey = n; }



    void Log(const char *p)
    {
        return;
        _TRACE("%s Key: %d ID: %s Checkin %s Update: %s Register: %s MutexName: %s"
        , p
        , getKey()
        , getID()
        , boost::posix_time::to_simple_string(getLastCheckinTime()).c_str()
        , boost::posix_time::to_simple_string(getLastUpdateTime()).c_str()
        , boost::posix_time::to_simple_string(getRegisterTime()).c_str()
        , getMutexName()
        );
    }

protected:
    //-----------------------------------------------------------------
    // createMutexName
    //
    // create unique mutex name.
    const std::string createMutexName() {
        std::string s = getID();
        s += "_mutex__";
        return s;
    }

private:
    char pID[ADDR_BUF_SIZE+1];
    boost::posix_time::ptime tmRegister;
    boost::posix_time::ptime tmLastCheckin;
    boost::posix_time::ptime m_tmLastMaintenance;
    char m_pMutexName[ADDR_BUF_SIZE + 1];
    int m_nMaintenanceBid;


};

}

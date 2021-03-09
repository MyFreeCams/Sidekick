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

#include <atomic>
#include <libobs/util/threading.h>
#include "../libipc/mfc_ipc.h"


#define THREADCMD_NONE          0       // null/empty thread cmd
#define THREADCMD_PAUSE         1       // request to pause http thread polling
#define THREADCMD_RESUME        2       // request to resume http thread polling
#define THREADCMD_STREAMSTART   3       // request to resume http thread polling
#define THREADCMD_STREAMSTOP    4       // request to resume http thread polling
#define THREADCMD_PROFILE       5       // notify thread that profile (and g_ctx) changed
#define THREADCMD_SHUTDOWN      255     // request to stop any http polling and end thread

class CBroadcastCtx;

namespace MFCIPC {
class CSemaphore;
}
class CHttpThread
{
public:
    CHttpThread();
    ~CHttpThread();

    const static int PING_MSG_INTERVAL = 120; // every 2 minutes we send a ping msg over the shared memory segment

    bool Start();
    bool Stop(int);
    void Process();


    void setServicesFilename(const std::string& sFile)
    {
        m_sServicesFilename = sFile;
    }
    string getServicesFilename() { return m_sServicesFilename; }


    // thread-safe methods for setting and getting the current thread cmd
    static uint32_t                         getCmd(void);
    static void                             setCmd(uint32_t dwCmd);

    static void*                            startProcessThread(void* pCtx);


    static atomic< uint32_t >               sm_dwThreadCmd;


    std::string                             m_sServicesFilename;

private:
    set< int >                              m_workerPids;
    pthread_t                               m_thread;
    std::unique_ptr<MFCIPC::CSemaphore>             m_pSema;
#ifdef _DEBUG
public:
    void orig_process(); // original implementation of Process()
#endif

};

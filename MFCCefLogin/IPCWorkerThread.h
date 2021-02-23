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

#include <thread>
#include "MFCCefEventHandler.h"

#include "mfc_ipc.h"

namespace boost {
class mutex;
class condition_variable;
}
class CSemaphore;

typedef CMFCCefApp<CMFCCefEventHandler> MFC_LOGIN_APP;
#ifdef _USE_OLD_MEMMANAGER
class CIPCWorkerThread
{
public:

    CIPCWorkerThread(CMFCCefApp<CMFCCefEventHandler> &);

    ~CIPCWorkerThread();

    int Process();
    bool init();
    bool Start();
    bool End();
    bool Stop();

    bool isStopFlag() ;
    void setStopFlag(bool b);

    CefRefPtr<CefBrowser> getBrowser();

   // MFC_Shared_Mem::CMessageManager& getSharedMemManager() { return m_msgManager;  }
    MFC_LOGIN_APP &getCefApp() { return m_App; }
    void closeAllBrowsers();

private:
    std::thread* m_pThread;
    std::unique_ptr<boost::mutex> m_spMutexFlags;
    std::unique_ptr<boost::mutex> m_spMutexApp;
    std::unique_ptr<CSemaphore>m_spSemaClient;
    MFC_LOGIN_APP& m_App;
    bool m_bStop;
  // MFC_Shared_Mem::CMessageManager m_msgManager;
};
#endif

class CIPCWorkerEventHandler : public MFCIPC::CSimpleEventHandler
{
    friend class CRouter;
    typedef CSimpleEventHandler _myBase;
public:
    CIPCWorkerEventHandler(const char *pm, CMFCCefApp<CMFCCefEventHandler> &);
    ~CIPCWorkerEventHandler();

    CefRefPtr<CefBrowser>getBrowser();
    MFC_LOGIN_APP &getCefApp() { return m_App; }
    void closeAllBrowsers();

    virtual void onIncoming(MFCIPC::CIPCEvent &);
    virtual void onAttachProcess(MFCIPC::CProcessRecord &);
    virtual void onDetachProcess(MFCIPC::CProcessRecord &);
protected:
    
private:
    MFC_LOGIN_APP& m_App;
    std::unique_ptr<boost::mutex> m_spMutexFlags;
    std::unique_ptr<boost::mutex> m_spMutexApp;
};

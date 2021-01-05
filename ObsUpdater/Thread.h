#pragma once

// used by DownloadCache.h
#define WM_USER_DL_ERRAPI				(WM_USER + 2005)
#define WM_USER_DL_DONE					(WM_USER + 2006)
#define WM_USER_DL_404					(WM_USER + 2007)
#define WM_USER_DL_LOG					(WM_USER + 2008)

// used by chat windows
#define WM_USER_DISCONNECT				(WM_USER + 2599)
#define WM_USER_CONNECT					(WM_USER + 2600)
#define WM_USER_PACKET					(WM_USER + 2602)
#define WM_USER_RELOGIN					(WM_USER + 2603)
#define WM_USER_MUTE					(WM_USER + 2604)
#define WM_USER_AUDIOLEVEL				(WM_USER + 2605)
#define WM_USER_CLEANUP					(WM_USER + 2606)
#define WM_USER_WARNDISCONNECT		    (WM_USER + 2607)
#define WM_USER_UPGRADE					(WM_USER + 2609)
#define WM_USER_CONNECTING				(WM_USER + 2610)
#define WM_USER_BANDWIDTHLEVEL		    (WM_USER + 2611)
#define WM_USER_BWPROFILE				(WM_USER + 2613)
#define WM_USER_NAMELOOKUP				(WM_USER + 2614)

// used by WebHost.h
#define WM_USER_DOCUMENTCOMPLETE		(WM_USER + 9874)
#define WM_USER_NAVIGATEERROR			(WM_USER + 9875)


#define THREADCMD_NONE					0
#define THREADCMD_SENDQ					1
#define THREADCMD_SLEEPON				2
#define THREADCMD_SLEEPOFF				3
#define THREADCMD_DISCONNECT			4
#define THREADCMD_SHUTDOWN				14
// ... More thread commands here

template<class T> class Thread
{
	DWORD m_dwThreadId;
	HANDLE m_hThread;
	T *m_pThis;
	PUCHAR (T::*m_pmfnThreadProc)();

	PUCHAR _threadProc() throw()
	{
        try
        {
		    PUCHAR pRet = NULL;

		    if(NULL != m_pThis && NULL != m_pmfnThreadProc)
		    {
			    pRet = (m_pThis->*m_pmfnThreadProc)();
		    }

		    return pRet;
        }
        catch(...)
        {
            return NULL;
        }
	}

	static PUCHAR WINAPI _threadProcStatic(LPVOID lpParameter) throw()
	{
        try
        {
		    Thread<T>* pThis = NULL;
		    PUCHAR pRet = NULL;

		    pThis = reinterpret_cast< Thread< T >* >(lpParameter);

		    if(NULL != pThis)
		    {
			    pRet = pThis->_threadProc();
		    }

		    return pRet;
        }
        catch(...)
        {
            return NULL;
        }
	}

public:
    /* TODO:
    Thread() throw()
    try : m_hThread(NULL), m_dwThreadId(0), m_pThis(NULL), m_pmfnThreadProc(NULL)
    {
    }
    catch(...)
    {
    }*/

	Thread() throw() : m_hThread(NULL), m_dwThreadId(0), m_pThis(NULL), m_pmfnThreadProc(NULL)
	{
	}

	virtual ~Thread() throw()
	{
        try
        {
    		close();
        }
        catch(...)
        {
        }
	}

	bool start(T* pThis, PUCHAR (T::*pmfnThreadProc)()) throw()
	{
        try
        {
		    bool fRet = false;

		    if(NULL == m_hThread)
		    {
			    m_pThis = pThis;
			    m_pmfnThreadProc = pmfnThreadProc;

                m_hThread = ::CreateThread(NULL, 0, reinterpret_cast< LPTHREAD_START_ROUTINE >(_threadProcStatic), this, 0, &m_dwThreadId);

			    fRet = (NULL != m_hThread);
		    }

		    return fRet;
        }
        catch(...)
        {
            return false;
        }
	}

	HANDLE handle() const throw()
	{
		try
		{
			return m_hThread;
		}
		catch(...)
		{
			return 0;
		}
	}

	DWORD id() const throw()
	{
        try
        {
    		return m_dwThreadId;
        }
        catch(...)
        {
            return 0;
        }
	}

	bool wait(DWORD dwTimeout = INFINITE) throw()
	{
        try
        {
		    bool fRet = false;

		    if(NULL != m_hThread)
		    {
			    DWORD dwResult;

                dwResult = ::WaitForSingleObject(m_hThread, dwTimeout);

			    fRet = (WAIT_OBJECT_0 == dwResult);
		    }

		    return fRet;
        }
        catch(...)
        {
            return false;
        }
	}

	bool close() throw()
	{
        try
        {
		    bool fRet = false;

		    if(NULL != m_hThread)
		    {
			    if(::CloseHandle(m_hThread))
			    {
					fRet = true;
				    m_hThread = NULL;
				    m_dwThreadId = 0;
				    m_pThis = NULL;
				    m_pmfnThreadProc = NULL;
			    }
		    }

		    return fRet;
        }
        catch(...)
        {
            return false;
        }
	}
} ;


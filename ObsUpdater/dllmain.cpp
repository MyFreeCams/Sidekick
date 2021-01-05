// dllmain.cpp : Defines the entry point for the DLL application.
#include "ObsUpdater.h"

#ifdef _WIN32
#define UNUSED_PARAMETER(param) (void)param

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
	UNUSED_PARAMETER(lpReserved);
	UNUSED_PARAMETER(hModule);

     switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#endif

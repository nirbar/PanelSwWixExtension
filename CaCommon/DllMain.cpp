
#include "stdafx.h"
#include <Windows.h>
#include <procutil.h>

// DllMain - Initialize and cleanup WiX custom action utils.
PROC_FILESYSTEMREDIRECTION _gFsRedirect;
extern "C" BOOL DllMain(
	__in HINSTANCE hInst,
	__in ULONG ulReason,
	__in LPVOID
	)
{
	BOOL bWow64 = FALSE;
	HANDLE hProc = NULL;
	HRESULT hr = S_OK;
	
	switch(ulReason)
	{
	case DLL_PROCESS_ATTACH:
		WcaGlobalInitialize(hInst);
		
		_gFsRedirect.fDisabled = FALSE;
		hProc = ::GetCurrentProcess();
		hr = ProcWow64( hProc, &bWow64);
		if( SUCCEEDED( hr) &&  bWow64)
		{
			hr = ProcDisableWowFileSystemRedirection( &_gFsRedirect);
			if( FAILED( hr))
			{
				_gFsRedirect.fDisabled = FALSE;
				WcaLogError( E_FAIL, "ProcDisableWowFileSystemRedirection failed");
			}
		}

		break;

	case DLL_PROCESS_DETACH:
		WcaGlobalFinalize();

		if( _gFsRedirect.fDisabled)
		{
			hr = ProcRevertWowFileSystemRedirection( &_gFsRedirect);
			if( FAILED( hr))
			{
				WcaLogError( E_FAIL, "Wow64RevertWow64FsRedirection failed");
			}
		}

		break;
	}

	return TRUE;
}

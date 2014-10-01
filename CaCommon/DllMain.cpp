
#include "stdafx.h"
#include <Windows.h>

// DllMain - Initialize and cleanup WiX custom action utils.
PVOID _gFsRedirect = NULL;
extern "C" BOOL WINAPI DllMain(
	__in HINSTANCE hInst,
	__in ULONG ulReason,
	__in LPVOID
	)
{
	switch(ulReason)
	{
	case DLL_PROCESS_ATTACH:
		WcaGlobalInitialize(hInst);
		
		if( ! ::Wow64DisableWow64FsRedirection( &_gFsRedirect))
		{
			_gFsRedirect = NULL;
			WcaLogError( E_FAIL, "Wow64DisableWow64FsRedirection failed");
		}

		break;

	case DLL_PROCESS_DETACH:
		WcaGlobalFinalize();

		if(( _gFsRedirect != NULL) && ! ::Wow64RevertWow64FsRedirection ( _gFsRedirect))
		{
			WcaLogError( E_FAIL, "Wow64RevertWow64FsRedirection failed");
		}

		break;
	}

	return TRUE;
}

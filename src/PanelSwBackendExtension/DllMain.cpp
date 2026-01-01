#include "pch.h"

HINSTANCE g_hInstance = NULL;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ulReason, LPVOID)
{
	HRESULT hr = S_OK;

	switch (ulReason)
	{
	case DLL_PROCESS_ATTACH:
#ifdef DEBUGGABLE_RELEASE
		if (::GetEnvironmentVariableW(L"DEBUG_PSW", nullptr, 0) > 0)
		{
			::MessageBoxW(NULL, L"DEBUG_PSW environment variable is set. You may attach a debugger now.\nTo disable this message, delete the environment variable 'DEBUG_PSW'", L"DEBUG_PSW", MB_OK);
		}
#endif
		g_hInstance = hInst;
		break;
	case DLL_PROCESS_DETACH:
		g_hInstance = NULL;
		break;
	}

	return TRUE;
}

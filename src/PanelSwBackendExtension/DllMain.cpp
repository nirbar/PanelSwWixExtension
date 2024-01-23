#include "pch.h"

HINSTANCE g_hInstance = NULL;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ulReason, LPVOID)
{
	HRESULT hr = S_OK;

	switch (ulReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hInstance = hInst;
		break;
	case DLL_PROCESS_DETACH:
		g_hInstance = NULL;
		break;
	}

	return TRUE;
}

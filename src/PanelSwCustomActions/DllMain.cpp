#include "pch.h"

PROC_FILESYSTEMREDIRECTION _gFsRedirect;
extern "C" BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ulReason, LPVOID)
{
	BOOL bWow64 = FALSE;
	HANDLE hProc = NULL;
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

		WcaGlobalInitialize(hInst);
		_gFsRedirect.fDisabled = FALSE;

#if !defined(_WIN64)
		hProc = ::GetCurrentProcess();
		hr = ProcWow64(hProc, &bWow64);
		if (SUCCEEDED(hr) && bWow64)
		{
			hr = ProcDisableWowFileSystemRedirection(&_gFsRedirect);
			if (FAILED(hr))
			{
				_gFsRedirect.fDisabled = FALSE;
			}
		}
#endif
		break;

	case DLL_PROCESS_DETACH:
		WcaGlobalFinalize();

		if (_gFsRedirect.fDisabled)
		{
			hr = ProcRevertWowFileSystemRedirection(&_gFsRedirect);
		}

		break;
	}

	return TRUE;
}

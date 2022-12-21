#include "pch.h"
#include <Windows.h>
#include <procutil.h>
#include "DeferredActionBase.h"

PROC_FILESYSTEMREDIRECTION _gFsRedirect;
extern "C" BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ulReason, LPVOID)
{
	BOOL bWow64 = FALSE;
	HANDLE hProc = NULL;
	HRESULT hr = S_OK;

	switch (ulReason)
	{
	case DLL_PROCESS_ATTACH:
		WcaGlobalInitialize(hInst);

		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Checking if file system redirection can be disabled");
		_gFsRedirect.fDisabled = FALSE;
		hProc = ::GetCurrentProcess();
		hr = ProcWow64(hProc, &bWow64);
		if (SUCCEEDED(hr) && bWow64)
		{
			CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Disabling file system redirection");
			hr = ProcDisableWowFileSystemRedirection(&_gFsRedirect);
			if (FAILED(hr))
			{
				_gFsRedirect.fDisabled = FALSE;
				CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"ProcDisableWowFileSystemRedirection failed with code %u", hr);
			}
		}

		break;

	case DLL_PROCESS_DETACH:
		WcaGlobalFinalize();

		if (_gFsRedirect.fDisabled)
		{
			hr = ProcRevertWowFileSystemRedirection(&_gFsRedirect);
			if (FAILED(hr))
			{
				CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Wow64RevertWow64FsRedirection failed");
			}
		}

		break;
	}

	return TRUE;
}
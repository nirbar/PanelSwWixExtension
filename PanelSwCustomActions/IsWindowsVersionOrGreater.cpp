#include "stdafx.h"
#include <WixString.h>
#include <pathutil.h>
#include <fileutil.h>

extern "C" UINT __stdcall IsWindowsVersionOrGreater(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	HMODULE hKernel32 = NULL;
	CWixString szKernel32Path, szKernel32Version;
	ULARGE_INTEGER ullKernelVersion;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_IsWindowsVersionOrGreater");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_IsWindowsVersionOrGreater'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_IsWindowsVersionOrGreater'. Have you authored 'PanelSw:IsWindowsVersionOrGreater' entries in WiX code?");
	
	hKernel32 = ::GetModuleHandleW(L"kernel32");
	ExitOnNullWithLastError(hKernel32, hr, "Failed to get module handle for kernel32.");

	hr = PathForCurrentProcess((LPWSTR*)szKernel32Path, hKernel32);
	ExitOnFailure(hr, "Failed to get full path of Kernel32.dll");

	hr = FileVersion((LPCWSTR)szKernel32Path, &ullKernelVersion.HighPart, &ullKernelVersion.LowPart);
	ExitOnFailure(hr, "Failed to get version of Kernel32.dll");

	hr = FileVersionToStringEx(ullKernelVersion.QuadPart, (LPWSTR*)szKernel32Version);
	ExitOnFailure(hr, "Failed to parse version of Kernel32.dll");
	WcaLog(LOGMSG_STANDARD, "Detected version '%ls' of '%ls'", (LPCWSTR)szKernel32Version, (LPCWSTR)szKernel32Path);

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Property_`, `Version` FROM `PSW_IsWindowsVersionOrGreater`", &hView);
	ExitOnFailure(hr, "Failed to execute MSI SQL query");

	// Loop
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");
		
		CWixString szProperty, szMinVersion;
		ULARGE_INTEGER ullMinVersion;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szProperty);
		ExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szMinVersion);
		ExitOnFailure(hr, "Failed to get Version.");

		hr = FileVersionFromString((LPCWSTR)szMinVersion, &ullMinVersion.HighPart, &ullMinVersion.LowPart);
		ExitOnFailure(hr, "Failed to parse minimal version '%ls' for '%ls'.", (LPCWSTR)szMinVersion, (LPCWSTR)szProperty);

		if (ullKernelVersion.QuadPart >= ullMinVersion.QuadPart)
		{
			hr = WcaSetIntProperty((LPCWSTR)szProperty, 1);
			ExitOnFailure(hr, "Failed to set property.");
		}
	}
	hr = S_OK;

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

#include "stdafx.h"
#include <WixString.h>
#include <pathutil.h>
#include <fileutil.h>
#include <memutil.h>

static HRESULT DAPI GetKernel32ProductVersion(ULARGE_INTEGER* pullKernelProductVersion);

extern "C" UINT __stdcall IsWindowsVersionOrGreater(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CWixString szKernel32Version;
	ULARGE_INTEGER ullKernelVersion = {};

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_IsWindowsVersionOrGreater");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_IsWindowsVersionOrGreater'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_IsWindowsVersionOrGreater'. Have you authored 'PanelSw:IsWindowsVersionOrGreater' entries in WiX code?");
	
	hr = GetKernel32ProductVersion(&ullKernelVersion);
	ExitOnFailure(hr, "Failed to get product version of Kernel32.dll");

	hr = FileVersionToStringEx(ullKernelVersion.QuadPart, (LPWSTR*)szKernel32Version);
	ExitOnFailure(hr, "Failed to parse product version of Kernel32.dll");
	WcaLog(LOGMSG_STANDARD, "Detected product version '%ls' of Kernel32.dll", (LPCWSTR)szKernel32Version);

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Property_`, `MinVersion`, `MaxVersion` FROM `PSW_IsWindowsVersionOrGreater`", &hView);
	ExitOnFailure(hr, "Failed to execute MSI SQL query");

	// Loop
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");
		
		CWixString szProperty, szMinVersion, szMaxVersion;
		ULARGE_INTEGER ullMinVersion = {}, ullMaxVersion = {};

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szProperty);
		ExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szMinVersion);
		ExitOnFailure(hr, "Failed to get Version.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)szMaxVersion);
		ExitOnFailure(hr, "Failed to get Version.");

		hr = FileVersionFromString((LPCWSTR)szMinVersion, &ullMinVersion.HighPart, &ullMinVersion.LowPart);
		ExitOnFailure(hr, "Failed to parse minimal version '%ls' for '%ls'.", (LPCWSTR)szMinVersion, (LPCWSTR)szProperty);

		if (!szMaxVersion.IsNullOrEmpty())
		{
			hr = FileVersionFromString((LPCWSTR)szMaxVersion, &ullMaxVersion.HighPart, &ullMaxVersion.LowPart);
			ExitOnFailure(hr, "Failed to parse maximal version '%ls' for '%ls'.", (LPCWSTR)szMinVersion, (LPCWSTR)szProperty);
		}

		if ((ullKernelVersion.QuadPart >= ullMinVersion.QuadPart) && (szMaxVersion.IsNullOrEmpty() || (ullKernelVersion.QuadPart <= ullMaxVersion.QuadPart)))
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

static HRESULT DAPI GetKernel32ProductVersion(ULARGE_INTEGER* pullKernelProductVersion)
{
	HRESULT hr = S_OK;
	UINT cbVerBuffer = 0;
	LPVOID pVerBuffer = nullptr;
	VS_FIXEDFILEINFO* pvsFileInfo = nullptr;
	UINT cbFileInfo = 0;
	BOOL bRes = TRUE;

	cbVerBuffer = ::GetFileVersionInfoSize(L"Kernel32", nullptr);
	ExitOnNullWithLastError(cbVerBuffer, hr, "Failed to get Kernel32.dll version info size");

	pVerBuffer = ::MemAlloc(cbVerBuffer, TRUE);
	ExitOnNullWithLastError(pVerBuffer, hr, "Failed to allocate memory");

	bRes = ::GetFileVersionInfo(L"Kernel32", 0, cbVerBuffer, pVerBuffer);
	ExitOnNullWithLastError(bRes, hr, "Failed to get Kernel32.dll version info");

	bRes = ::VerQueryValue(pVerBuffer, L"\\", (void**)&pvsFileInfo, &cbFileInfo);
	ExitOnNullWithLastError(bRes, hr, "Failed to get Kernel32.dll version");

	pullKernelProductVersion->HighPart = pvsFileInfo->dwProductVersionMS;
	pullKernelProductVersion->LowPart = pvsFileInfo->dwProductVersionLS;

LExit:
	ReleaseMem(pVerBuffer);

	return hr;
}

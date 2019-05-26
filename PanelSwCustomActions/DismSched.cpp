#include "stdafx.h"
#include <wcautil.h>
#include <strutil.h>
#include <list>
#include <string>
#include "..\CaCommon\WixString.h"
#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")
using namespace std;

/* CustomActionData fields: 
- Log file
- Per feature:
	- Enable regex
	- Exclude regex
	- Package
	- Cost
	- Error handling
*/ 
extern "C" UINT __stdcall DismSched(MSIHANDLE hInstall)
{
	UINT er = ERROR_SUCCESS;
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	LPWSTR szMsiLog = nullptr;
	WCHAR szDismLog[MAX_PATH];
	PMSIHANDLE hView, hRecord;
	LPWSTR szCustomActionData = nullptr;
	int nVersionNT = 0;
	UINT nTotalCost = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaGetIntProperty(L"VersionNT", &nVersionNT);
	ExitOnFailure(hr, "Failed to get VersionNT");

	// DISM log file
	hr = WcaGetProperty(L"MsiLogFileLocation", &szMsiLog);
	ExitOnFailure(hr, "Failed to get MsiLogFileLocation");

	// Are we logging?
	szDismLog[0] = NULL;
	if (szMsiLog && (::wcslen(szMsiLog) <= (MAX_PATH - 6)))
	{
		::wcscpy_s(szDismLog, szMsiLog);

		bRes = ::PathRenameExtension(szDismLog, L".dism.log");
		ExitOnNullWithLastError(bRes, hr, "Failed renaming file extension: '%ls'", szMsiLog);
	}
	hr = WcaWriteStringToCaData(szDismLog, &szCustomActionData);
	ExitOnFailure(hr, "Failed adding log path to CustomActionData");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `Component_`, `EnableFeatures`, `ExcludeFeatures`, `PackagePath`, `Cost`, `ErrorHandling` FROM `PSW_Dism`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString id, exclude, include, package, component;
		int nCost = 0, nErrorHandling = 0;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)id);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)component);
		ExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)include);
		ExitOnFailure(hr, "Failed to get EnableFeatures.");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)exclude);
		ExitOnFailure(hr, "Failed to get ExcludeFeatures.");
		hr = WcaGetRecordFormattedString(hRecord, 5, (LPWSTR*)package);
		ExitOnFailure(hr, "Failed to get PackagePath.");
		hr = WcaGetRecordInteger(hRecord, 6, &nCost);
		ExitOnFailure(hr, "Failed to get Cost.");
		hr = WcaGetRecordInteger(hRecord, 7, &nErrorHandling);
		ExitOnFailure(hr, "Failed to get ErrorHandling.");

		compAction = WcaGetComponentToDo((LPCWSTR)component);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
		case WCA_TODO::WCA_TODO_REINSTALL:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will enable features matching pattern '%ls', excluding pattern '%ls' on component '%ls'", (LPCWSTR)include, (LPCWSTR)exclude, (LPCWSTR)component);
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping DISM for feature '%ls' as component '%ls' is not installed or repaired", (LPCWSTR)id, (LPCWSTR)component);
			continue;
		}

		// Sanity
		ExitOnNull(!include.IsNullOrEmpty(), hr, E_INVALIDARG, "EnableFeatures is empty");
		if (nVersionNT <= 601)
		{
			ExitOnFailure(hr = E_NOTIMPL, "PanelSwWixExtension Dism is only supported on Windows 8 / Windows Server 2008 R2 or newer operating systems");
		}

		nTotalCost += nCost;

		hr = WcaWriteStringToCaData((LPCWSTR)include, &szCustomActionData);
		ExitOnFailure(hr, "Failed appending field to CustomActionData");

		hr = WcaWriteStringToCaData((LPCWSTR)exclude, &szCustomActionData);
		ExitOnFailure(hr, "Failed appending field to CustomActionData");

		hr = WcaWriteStringToCaData((LPCWSTR)package, &szCustomActionData);
		ExitOnFailure(hr, "Failed appending field to CustomActionData");

		hr = WcaWriteIntegerToCaData(nCost, &szCustomActionData);
		ExitOnFailure(hr, "Failed appending field to CustomActionData");

		hr = WcaWriteIntegerToCaData(nErrorHandling, &szCustomActionData);
		ExitOnFailure(hr, "Failed appending field to CustomActionData");
	}
	hr = S_OK; // We're only getting here on hr = E_NOMOREITEMS.

	// Since Dism API takes long, we only want to execute it if there's something to do. Conditioning the Dism deferred CA with the property existance will save us time.
	if (WcaCountOfCustomActionDataRecords(szCustomActionData) > 1)
	{
		hr = WcaSetProperty(L"DismX86", szCustomActionData);
		ExitOnFailure(hr, "Failed setting CustomActionData");

		hr = WcaSetProperty(L"DismX64", szCustomActionData);
		ExitOnFailure(hr, "Failed setting CustomActionData");

		if (nTotalCost)
		{
			hr = WcaProgressMessage(nTotalCost, TRUE);
			ExitOnFailure(hr, "Failed perdicting ticks");
		}
	}

LExit:
	ReleaseStr(szMsiLog);
	ReleaseStr(szCustomActionData);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

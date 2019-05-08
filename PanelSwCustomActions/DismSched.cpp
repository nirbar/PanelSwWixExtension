#include "stdafx.h"
#include <wcautil.h>
#include <strutil.h>
#include <list>
#include <string>
#include "..\CaCommon\WixString.h"
#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")
using namespace std;

#define DismLogPrefix		L"DismLog="
#define IncludeFeatures		L"IncludeFeatures="
#define ExcludeFeatures		L"ExcludeFeatures="
#define Packages			L"Packages="
#define TOTOAL_TICKS			(1024.0 * 1024 * 1024) // Same value as in Dism.cpp

// Immediate custom action
extern "C" UINT __stdcall DismSched(MSIHANDLE hInstall)
{
	UINT er = ERROR_SUCCESS;
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	LPWSTR szMsiLog = nullptr;
	WCHAR szDismLog[MAX_PATH];
	PMSIHANDLE hView, hRecord;
	CWixString allInclude, allExclude, allPackages, cad;
	int nVersionNT = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaGetIntProperty(L"VersionNT", &nVersionNT);
	ExitOnFailure(hr, "Failed to get VersionNT");

	// DISM log file
	hr = WcaGetProperty(L"MsiLogFileLocation", &szMsiLog);
	ExitOnFailure(hr, "Failed to get MsiLogFileLocation");

	// Are we logging?
	if (szMsiLog && (::wcslen(szMsiLog) <= (MAX_PATH - 6)))
	{
		::wcscpy_s(szDismLog, szMsiLog);

		bRes = ::PathRenameExtension(szDismLog, L".dism.log");
		ExitOnNullWithLastError(bRes, hr, "Failed renaming file extension: '%ls'", szMsiLog);

		hr = cad.Format(DismLogPrefix L"%s;", szDismLog);
		ExitOnFailure(hr, "Failed formatting string for DISM log file");
	}

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `Component_`, `EnableFeatures`, `ExcludeFeatures`, `PackagePath` FROM `PSW_Dism`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString id, exclude, include, package, component;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)id);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)component);
		ExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)include);
		ExitOnFailure(hr, "Failed to get EnableFeatures.");
		ExitOnNull(!include.IsNullOrEmpty(), hr, E_INVALIDARG, "EnableFeatures is empty");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)exclude);
		ExitOnFailure(hr, "Failed to get ExcludeFeatures.");
		hr = WcaGetRecordFormattedString(hRecord, 5, (LPWSTR*)package);
		ExitOnFailure(hr, "Failed to get PackagePath.");

		compAction = WcaGetComponentToDo((LPCWSTR)component);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
		case WCA_TODO::WCA_TODO_REINSTALL:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will enable features matching pattern '%ls', excluding pattern '%ls' on component '%ls'", (LPCWSTR)include, (LPCWSTR)exclude, (LPCWSTR)component);

			if (nVersionNT <= 601)
			{
				ExitOnFailure(hr = E_NOTIMPL, "PanelSwWixExtension Dism is only supported on Windows 8 / Windows Server 2008 R2 or newer operating systems");
			}

			hr = allInclude.AppnedFormat(L"%s;", (LPCWSTR)include);
			ExitOnFailure(hr, "Failed appending formatted string");

			if (!exclude.IsNullOrEmpty())
			{
				hr = allExclude.AppnedFormat(L"%s;", (LPCWSTR)exclude);
				ExitOnFailure(hr, "Failed appending formatted string");
			}

			if (!package.IsNullOrEmpty())
			{
				hr = allPackages.AppnedFormat(L"%s;", (LPCWSTR)package);
				ExitOnFailure(hr, "Failed appending formatted string");
			}
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping DISM for feature '%ls' as component '%ls' is not installed or repaired", (LPCWSTR)id, (LPCWSTR)component);
			break;
		}
	}
	hr = S_OK; // We're only getting here on hr = E_NOMOREITEMS.

	// Since Dism API takes long, we only want to execute it if there's something to do. Conditioning the Dism deferred CA with the property existance will save us time.
	if (!allInclude.IsNullOrEmpty())
	{
		hr = cad.AppnedFormat(IncludeFeatures L"%s", (LPCWSTR)allInclude);
		ExitOnFailure(hr, "Failed appending formatted string");

		if (!allExclude.IsNullOrEmpty())
		{
			hr = cad.AppnedFormat(ExcludeFeatures L"%s", (LPCWSTR)allExclude);
			ExitOnFailure(hr, "Failed appending formatted string");
		}

		if (!allPackages.IsNullOrEmpty())
		{
			hr = cad.AppnedFormat(Packages L"%s", (LPCWSTR)allPackages);
			ExitOnFailure(hr, "Failed appending formatted string");
		}

		hr = WcaSetProperty(L"DismX86", (LPCWSTR)cad);
		ExitOnFailure(hr, "Failed setting CustomActionData");

		hr = WcaSetProperty(L"DismX64", (LPCWSTR)cad);
		ExitOnFailure(hr, "Failed setting CustomActionData");

		hr = WcaProgressMessage(TOTOAL_TICKS, TRUE);
		ExitOnFailure(hr, "Failed perdicting ticks");
	}

LExit:
	ReleaseStr(szMsiLog);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

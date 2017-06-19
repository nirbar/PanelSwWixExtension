#include "stdafx.h"
#include <wcautil.h>
#include <strutil.h>
#include <DismApi.h>
#include <Shlwapi.h>
#include <list>
#include <string>
#include <regex>
#pragma comment (lib, "dismapi.lib")
#pragma comment (lib, "Shlwapi.lib")
using namespace std;

static LPCWSTR DismStateString(DismPackageFeatureState state);

#define Dism_QUERY L"SELECT `Id`, `Component_`, `EnableFeatures` FROM `PSW_Dism`"
enum DismQuery { Id = 1, Component = 2, EnableFeatures = 3 };

#define DismLogPrefix		L"DismLog="

// Immediate custom action
extern "C" __declspec(dllexport) UINT DismSched(MSIHANDLE hInstall)
{
	UINT er = ERROR_SUCCESS;
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	LPWSTR szMsiLog = NULL;
	LPWSTR szCAD = NULL;
	WCHAR szDismLog[MAX_PATH];
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPWSTR szId = NULL;
	LPWSTR szComponent = NULL;
	LPWSTR szFeature = NULL;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// DISM log file
	hr = WcaGetProperty(L"MsiLogFileLocation", &szMsiLog);
	ExitOnFailure(hr, "Failed to get MsiLogFileLocation");

	// Are we logging?
	if (szMsiLog && (::wcslen(szMsiLog) <= (MAX_PATH - 6)))
	{
		::wcscpy(szDismLog, szMsiLog);

		bRes = PathRenameExtension(szDismLog, L".dism.log");
		ExitOnNullWithLastError1(bRes, hr, "Failed renaming file extension: '%ls'", szMsiLog);

		hr = StrAllocFormatted(&szCAD, DismLogPrefix L"%s", szDismLog);
		ExitOnFailure(hr, "Failed formatting string for DISM log file");
	}

	// Execute view
	hr = WcaOpenExecuteView(Dism_QUERY, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", Dism_QUERY);

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		PMSIHANDLE hExitCodeView;
		PMSIHANDLE hExitCodeRecord;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		DWORD_PTR szStrLen = 0;

		hr = WcaGetRecordString(hRecord, DismQuery::Id, &szId);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, DismQuery::Component, &szComponent);
		ExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, DismQuery::EnableFeatures, &szFeature);
		ExitOnFailure(hr, "Failed to get EnableFeatures.");

		compAction = WcaGetComponentToDo(szComponent);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
		case WCA_TODO::WCA_TODO_REINSTALL:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will enable features matching pattern '%ls' on component '%ls'", szFeature, szComponent);

			szStrLen = ::wcslen(szCAD);
			hr = StrAllocConcat(&szCAD, L";", szStrLen);
			ExitOnFailure(hr, "Failed to build CustomActionData");

			++szStrLen;
			hr = StrAllocConcat(&szCAD, szFeature, szStrLen);
			ExitOnFailure(hr, "Failed to build CustomActionData");
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping DISM for feature '%ls' as component '%ls' is not installed or repaired", szId, szComponent);
			break;
		}

		// Clean for next iteration
		ReleaseStr(szId);
		ReleaseStr(szComponent);
		ReleaseStr(szFeature);
		szId = NULL;
		szComponent = NULL;
		szFeature = NULL;
	}

	hr = WcaSetProperty(L"Dism", szCAD);
	ExitOnFailure(hr, "Failed setting CustomActionData");

	hr = WcaSetProperty(L"Dism64", szCAD);
	ExitOnFailure(hr, "Failed setting CustomActionData");

LExit:

	ReleaseStr(szMsiLog);
	ReleaseStr(szCAD);
	ReleaseStr(szId);
	ReleaseStr(szComponent);
	ReleaseStr(szFeature);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}



extern "C" __declspec(dllexport) UINT Dism(MSIHANDLE hInstall)
{
	UINT er = ERROR_SUCCESS;
	HRESULT hr = S_OK;
	DismSession hSession = DISM_SESSION_DEFAULT;
	DismFeature *pFeatures = NULL;
	DismString *pErrorString = NULL;
	BOOL bDismInit = FALSE;
	BOOL bRes = TRUE;
	UINT uFeatureNum = 0;
	list<wregex> enableFaetures;
	LPWSTR szCAD = NULL;
	LPCWSTR szDismLog = NULL;
	LPCWSTR szTok = NULL;
	LPWSTR szTokData = NULL;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	hr = WcaGetProperty(L"CustomActionData", &szCAD);
	ExitOnFailure(hr, "Failed getting CustomActionData");

	// Parse CAD
	for (szTok = ::wcstok_s(szCAD, L";", &szTokData); szTok != NULL; szTok = ::wcstok_s(NULL, L";", &szTokData))
	{
		// Dism log?
		if ((szDismLog == NULL) && (::wcsstr(szTok, DismLogPrefix) != NULL))
		{
			szDismLog = szTok + ::wcslen(DismLogPrefix);
			continue;
		}

		// Next feature
		enableFaetures.push_back(wregex(szTok, std::regex_constants::syntax_option_type::optimize));
	}

	hr = ::DismInitialize(DismLogErrorsWarningsInfo, szDismLog, NULL);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		ExitOnFailure(hr, "Failed initializing DISM. %ls", (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}
	bDismInit = TRUE;

	hr = ::DismOpenSession(DISM_ONLINE_IMAGE, NULL, NULL, &hSession);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		ExitOnFailure(hr, "Failed opening DISM online session. %ls",  (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}

	// Enumerate features
	hr = ::DismGetFeatures(hSession, NULL, DismPackageNone, &pFeatures, &uFeatureNum);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		ExitOnFailure(hr, "Failed querying DISM features. %ls",  (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}

	for (UINT i = 0; i < uFeatureNum; ++i)
	{
		// Print feature state
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Feature '%ls', state=%u", pFeatures[i].FeatureName, DismStateString(pFeatures[i].State));

		switch (pFeatures[i].State)
		{
		case DismPackageFeatureState::DismStateNotPresent:
		case DismPackageFeatureState::DismStateStaged:
		case DismPackageFeatureState::DismStateRemoved:
		case DismPackageFeatureState::DismStatePartiallyInstalled:
			// Test if feature matches any regex
			for (const wregex& rx : enableFaetures)
			{
				match_results<LPCWSTR> results;

				bRes = regex_search(pFeatures[i].FeatureName, results, rx);
				if (!bRes || (results.length() <= 0))
				{
					continue;
				}

				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Enabling feature '%ls'", pFeatures[i].FeatureName);
				hr = ::DismEnableFeature(hSession, pFeatures[i].FeatureName, NULL, DismPackageNone, FALSE, NULL, 0, TRUE, NULL, NULL, NULL);
				if (HRESULT_CODE(hr) == ERROR_SUCCESS_REBOOT_REQUIRED)
				{
					hr = S_OK;
					WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Enabled feature '%ls'. However, it requires reboot to complete", pFeatures[i].FeatureName);
					WcaDeferredActionRequiresReboot();
				}

				if (FAILED(hr))
				{
					DismGetLastErrorMessage(&pErrorString);
					ExitOnFailure(hr, "Failed enabling feature '%ls'. %ls", pFeatures[i].FeatureName, (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
				}
				break;
			}
			break;

		case DismPackageFeatureState::DismStateUninstallPending:
		case DismPackageFeatureState::DismStateSuperseded:
		case DismPackageFeatureState::DismStateInstalled:
		case DismPackageFeatureState::DismStateInstallPending:
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Skipping feature '%ls' with state '%ls'", pFeatures[i].FeatureName, DismStateString(pFeatures[i].State));
			break;
			
		default:
			hr = E_INVALIDSTATE;
			ExitOnFailure(hr, "Illegal feature state %i for '%ls'", pFeatures[i].State, pFeatures[i].FeatureName);
			break;
		}
	}

LExit:

	ReleaseStr(szCAD);
	if (pFeatures != NULL)
	{
		::DismDelete(pFeatures);
	}
	if (pErrorString != NULL) 
	{
		::DismDelete(pErrorString);
	}
	if (hSession != DISM_SESSION_DEFAULT)
	{
		::DismCloseSession(hSession);
	}
	if (bDismInit)
	{
		::DismShutdown();
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static LPCWSTR DismStateString(DismPackageFeatureState state)
{
	static LPCWSTR NotPresent = L"Absent";
	static LPCWSTR UninstallPending = L"Pending Remove";
	static LPCWSTR Staged = L"Staged";
	static LPCWSTR Removed = L"Removed";
	static LPCWSTR Installed = L"Installed";
	static LPCWSTR InstallPending = L"Pending Install";
	static LPCWSTR Superseded = L"Superseded";
	static LPCWSTR PartiallyInstalled = L"Partially Installed";
	static LPCWSTR Invalid = L"Invalid State";

	switch (state)
	{
	case DismPackageFeatureState::DismStateNotPresent:
		return NotPresent;
	case DismPackageFeatureState::DismStateUninstallPending:
		return UninstallPending;
	case DismPackageFeatureState::DismStateStaged:
		return Staged;
	case DismPackageFeatureState::DismStateRemoved:
		return Removed;
	case DismPackageFeatureState::DismStateInstalled:
		return Installed;
	case DismPackageFeatureState::DismStateInstallPending:
		return InstallPending;
	case DismPackageFeatureState::DismStateSuperseded:
		return Superseded;
	case DismPackageFeatureState::DismStatePartiallyInstalled:
		return PartiallyInstalled;
	
	default:
		return Invalid;
	}
}

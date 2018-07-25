#include "stdafx.h"
#include <wcautil.h>
#include <strutil.h>
#include <DismApi.h>
#include <list>
#include <string>
#include <regex>
#pragma comment (lib, "dismapi.lib")
using namespace std;

static LPCWSTR DismStateString(DismPackageFeatureState state);
static void ProgressCallback(UINT Current, UINT Total, PVOID UserData);

#define DismLogPrefix		L"DismLog="

extern "C" UINT __stdcall Dism(MSIHANDLE hInstall)
{
	UINT er = ERROR_SUCCESS;
	HRESULT hr = S_OK;
	DismSession hSession = DISM_SESSION_DEFAULT;
	DismFeature *pFeatures = nullptr;
	DismString *pErrorString = nullptr;
	BOOL bDismInit = FALSE;
	BOOL bRes = TRUE;
	UINT uFeatureNum = 0;
	list<wregex> enableFaetures;
	LPWSTR szCAD = nullptr;
	LPCWSTR szDismLog = nullptr;
	LPCWSTR szTok = nullptr;
	LPWSTR szTokData = nullptr;
	HANDLE hCancel = NULL;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from Dism " FullVersion);

	hr = WcaGetProperty(L"CustomActionData", &szCAD);
	ExitOnFailure(hr, "Failed getting CustomActionData");

	// Parse CAD
	for (szTok = ::wcstok_s(szCAD, L";", &szTokData); szTok; szTok = ::wcstok_s(nullptr, L";", &szTokData))
	{
		// Dism log?
		if (!szDismLog && ::wcsstr(szTok, DismLogPrefix))
		{
			szDismLog = szTok + ::wcslen(DismLogPrefix);
			continue;
		}

		// Next feature
		enableFaetures.push_back(wregex(szTok, std::regex_constants::syntax_option_type::optimize));
	}

	hCancel = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	ExitOnNullWithLastError(hCancel, hr, "Failed creating event");

	hr = ::DismInitialize(DismLogErrorsWarningsInfo, szDismLog, nullptr);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		ExitOnFailure(hr, "Failed initializing DISM. %ls", (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}
	bDismInit = TRUE;

	hr = ::DismOpenSession(DISM_ONLINE_IMAGE, nullptr, nullptr, &hSession);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		ExitOnFailure(hr, "Failed opening DISM online session. %ls",  (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}

	// Enumerate features
	hr = ::DismGetFeatures(hSession, nullptr, DismPackageNone, &pFeatures, &uFeatureNum);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		ExitOnFailure(hr, "Failed querying DISM features. %ls",  (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}

	for (UINT i = 0; i < uFeatureNum; ++i)
	{
		// Print feature state
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Feature '%ls', state=%ls", pFeatures[i].FeatureName, DismStateString(pFeatures[i].State));

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
				hr = ::DismEnableFeature(hSession, pFeatures[i].FeatureName, nullptr, DismPackageNone, FALSE, nullptr, 0, TRUE, hCancel, ProgressCallback, hCancel);
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
	if (hCancel)
	{
		::CloseHandle(hCancel);
	}
	if (pFeatures)
	{
		::DismDelete(pFeatures);
	}
	if (pErrorString) 
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

static void ProgressCallback(UINT Current, UINT Total, PVOID UserData)
{
	HRESULT hr = S_OK;
	PMSIHANDLE hRec;
	HANDLE hCancel = (HANDLE)UserData;

	hr = WcaProgressMessage(0, FALSE);
	if (hr == S_FALSE)
	{
		::SetEvent(hCancel);
	}
}
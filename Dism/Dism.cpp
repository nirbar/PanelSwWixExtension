#include "stdafx.h"
#include <wcautil.h>
#include <strutil.h>
#include <shlwapi.h>
#include <DismApi.h>
#include <list>
#include <string>
#include <regex>
#pragma comment (lib, "dismapi.lib")
#pragma comment (lib, "Shlwapi.lib")
using namespace std;

static LPCWSTR DismStateString(DismPackageFeatureState state);
static void CALLBACK ProgressCallback(UINT Current, UINT Total, PVOID UserData);

// Will report 1GB for all features.
struct ProgressReportState
{
	HANDLE hCancel_ = NULL;
	
	// Number of features to be enabled.
	UINT nFeatureCount_ = 0;

	// Number of ticks so far reported
	ULONGLONG nTotalTicksReported_ = 0;

	// Number of ticks so far reported for current feature
	ULONGLONG nTicksReportedInFeature_ = 0;

	static const UINT TOTOAL_TICKS = 1073741824; // 1GB

	UINT TickPerFeatureAllowance()
	{
		return (nFeatureCount_ == 0) ? 0 : (TOTOAL_TICKS / nFeatureCount_);
	}
};

#define DismLogPrefix		L"DismLog="
#define IncludeFeatures		L"IncludeFeatures="
#define ExcludeFeatures		L"ExcludeFeatures="
#define Packages			L"Packages="

enum CadToken
{
	None,
	CadToken_Include_Feature,
	CadToken_Exclude_Feature,
	CadToken_Package
};

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
	list<wregex> excludeFaetures;
	list<LPCWSTR> packages;
	list<DismFeature*> resolvedFeatures; // Features that actually need to be enabled.
	LPWSTR szCAD = nullptr;
	LPCWSTR szDismLog = nullptr;
	LPCWSTR szTok = nullptr;
	LPWSTR szTokData = nullptr;
	CadToken tokenType = CadToken::None;
	ProgressReportState state;

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
		else if (::wcsstr(szTok, IncludeFeatures))
		{
			tokenType = CadToken::CadToken_Include_Feature;
			szTok += ::wcslen(IncludeFeatures);
		}
		else if (::wcsstr(szTok, ExcludeFeatures))
		{
			tokenType = CadToken::CadToken_Exclude_Feature;
			szTok += ::wcslen(ExcludeFeatures);
		}
		else if (::wcsstr(szTok, Packages))
		{
			tokenType = CadToken::CadToken_Package;
			szTok += ::wcslen(Packages);
		}

		if (!(szTok && *szTok)) // Skip empty.
		{
			continue;
		}

		switch (tokenType)
		{
		case CadToken::CadToken_Include_Feature:
			enableFaetures.push_back(wregex(szTok, std::regex_constants::syntax_option_type::optimize));
			break;

		case CadToken::CadToken_Exclude_Feature:
			excludeFaetures.push_back(wregex(szTok, std::regex_constants::syntax_option_type::optimize));
			break;

		case CadToken::CadToken_Package:
			packages.push_back(szTok);
			break;
		}
	}

	state.hCancel_ = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	ExitOnNullWithLastError(state.hCancel_, hr, "Failed creating event");

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

	// Enumerate features and evaluate include/exclude regex.
	hr = ::DismGetFeatures(hSession, nullptr, DismPackageNone, &pFeatures, &uFeatureNum);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		ExitOnFailure(hr, "Failed querying DISM features. %ls", (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
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

				// Feature included. Was it explictly excluded?
				bool excluded = false;
				for (const wregex& exRx : excludeFaetures)
				{
					match_results<LPCWSTR> exResults;

					bRes = regex_search(pFeatures[i].FeatureName, exResults, exRx);
					if (bRes && (exResults.length() > 0))
					{
						excluded = true;
						break;
					}
				}
				if (excluded)
				{
					WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Feature '%ls' excluded", pFeatures[i].FeatureName);
					continue;
				}

				resolvedFeatures.push_back(pFeatures + i);
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

	state.nFeatureCount_ = resolvedFeatures.size() + packages.size();
	for (LPCWSTR pkgPath : packages)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Adding package '%ls'", pkgPath);
		bRes = ::PathFileExists(pkgPath);
		ExitOnNullWithLastError(bRes, hr, "DISM package file not found: '%ls'", pkgPath);

		hr = ::DismAddPackage(hSession, pkgPath, FALSE, FALSE, state.hCancel_, ProgressCallback, &state);
		if (HRESULT_CODE(hr) == ERROR_SUCCESS_REBOOT_REQUIRED)
		{
			hr = S_OK;
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Added package '%ls'. However, it requires reboot to complete", pkgPath);
			WcaDeferredActionRequiresReboot();
		}
		if (FAILED(hr))
		{
			DismGetLastErrorMessage(&pErrorString);
			ExitOnFailure(hr, "Failed adding DISM package '%ls'. %ls", pkgPath, (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
		}

		// Cancelled?
		if (::WaitForSingleObject(state.hCancel_, 0) == WAIT_OBJECT_0)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Seems like DISM was canceled.");
			hr = S_FALSE;
			ExitFunction();
		}

		// Report unused ticks for this package
		if (state.nTicksReportedInFeature_ < state.TickPerFeatureAllowance())
		{
			UINT delta = (state.TickPerFeatureAllowance() - state.nTicksReportedInFeature_);
			hr = WcaProgressMessage(delta, FALSE);
			if (hr == S_FALSE)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Seems like DISM was canceled.");
				ExitFunction();
			}
			state.nTotalTicksReported_ += delta;
			state.nTicksReportedInFeature_ += delta;
		}

		state.nTicksReportedInFeature_ = 0;
	}

	for (const DismFeature* pFtr : resolvedFeatures)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Enabling feature '%ls'", pFtr->FeatureName);
		hr = ::DismEnableFeature(hSession, pFtr->FeatureName, nullptr, DismPackageNone, FALSE, nullptr, 0, TRUE, state.hCancel_, ProgressCallback, &state);
		if (HRESULT_CODE(hr) == ERROR_SUCCESS_REBOOT_REQUIRED)
		{
			hr = S_OK;
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Enabled feature '%ls'. However, it requires reboot to complete", pFtr->FeatureName);
			WcaDeferredActionRequiresReboot();
		}

		if (FAILED(hr))
		{
			DismGetLastErrorMessage(&pErrorString);
			ExitOnFailure(hr, "Failed enabling feature '%ls'. %ls", pFtr->FeatureName, (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
		}

		// Cancelled?
		if (::WaitForSingleObject(state.hCancel_, 0) == WAIT_OBJECT_0)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Seems like DISM was canceled.");
			hr = S_FALSE;
			ExitFunction();
		}

		// Report unused ticks for this feature
		if (state.nTicksReportedInFeature_ < state.TickPerFeatureAllowance())
		{
			UINT delta = (state.TickPerFeatureAllowance() - state.nTicksReportedInFeature_);
			hr = WcaProgressMessage(delta, FALSE);
			if (hr == S_FALSE)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Seems like DISM was canceled.");
				ExitFunction();
			}
			state.nTotalTicksReported_ += delta;
			state.nTicksReportedInFeature_ += delta;
		}

		state.nTicksReportedInFeature_ = 0;
	}

	// Report any left-over ticks.
	if (state.nTotalTicksReported_ < ProgressReportState::TOTOAL_TICKS)
	{
		WcaProgressMessage(ProgressReportState::TOTOAL_TICKS - state.nTotalTicksReported_, FALSE);
	}

LExit:

	ReleaseStr(szCAD);
	if (state.hCancel_)
	{
		::CloseHandle(state.hCancel_);
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

static void CALLBACK ProgressCallback(UINT Current, UINT Total, PVOID UserData)
{
	HRESULT hr = S_OK;
	PMSIHANDLE hRec;
	ProgressReportState *state = (ProgressReportState*)UserData;

	double tickDelta = ((((double)(Current - state->nTicksReportedInFeature_)) / Total) * state->TickPerFeatureAllowance());
	state->nTicksReportedInFeature_ = Current; // Tick reported for this feature
	state->nTotalTicksReported_ += tickDelta; // Ticks reported to MSI, normalized to 1GB total.

	hr = WcaProgressMessage(tickDelta, FALSE);
	if (state->hCancel_ && (hr == S_FALSE))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Cancelling DISM.");
		::SetEvent(state->hCancel_);
	}
}
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

// Will report 1GB for all features.
#define TOTOAL_TICKS			(1024.0 * 1024 * 1024) // Same value as in DismShced.cpp
struct ProgressReportState
{
	HANDLE hCancel_ = NULL;
	
	// Number of features to be enabled.
	UINT nFeatureCount_ = 0;

	// Index of currently executing feature.
	UINT nCurrentFeature_ = 0;

	// Number of ticks so far reported
	UINT nTotalTicksReported_ = 0;

	// Number of ticks so far reported for current feature
	UINT nTicksReportedInFeature_ = 0;
};

#define DismLogPrefix		L"DismLog="
#define IncludeFeatures		L"IncludeFeatures="
#define ExcludeFeatures		L"ExcludeFeatures="

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
	list<DismFeature*> resolvedFeatures; // Features that actually need to be enabled.
	LPWSTR szCAD = nullptr;
	LPCWSTR szDismLog = nullptr;
	LPCWSTR szTok = nullptr;
	LPWSTR szTokData = nullptr;
	bool bInclude = true;
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
			bInclude = true;
			szTok += ::wcslen(IncludeFeatures);
		}
		else if (::wcsstr(szTok, ExcludeFeatures))
		{
			bInclude = false;
			szTok += ::wcslen(ExcludeFeatures);
		}

		// Next feature
		if (bInclude)
		{
			enableFaetures.push_back(wregex(szTok, std::regex_constants::syntax_option_type::optimize));
		}
		else
		{
			excludeFaetures.push_back(wregex(szTok, std::regex_constants::syntax_option_type::optimize));
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

				// Feature included. Was it explictly excluded?
				bool excluded = false;
				for (const wregex& exRx : excludeFaetures)
				{
					match_results<LPCWSTR> exResults;

					bRes = regex_search(pFeatures[i].FeatureName, exResults, exRx);
					if (bRes && (results.length() > 0))
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

	state.nFeatureCount_ = resolvedFeatures.size();
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
		if (::WaitForSingleObject(state.hCancel_, 0) != WAIT_OBJECT_0)
		{
			break;
		}

		++state.nCurrentFeature_;
		state.nTicksReportedInFeature_ = 0;
	}

	// Report any left-over ticks.
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "DISM reported %u ticks", state.nTotalTicksReported_);
	if (state.nTotalTicksReported_ < TOTOAL_TICKS)
	{
		WcaProgressMessage(TOTOAL_TICKS - state.nTotalTicksReported_, FALSE);
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

static void WINAPI ProgressCallback(UINT Current, UINT Total, PVOID UserData)
{
	HRESULT hr = S_OK;
	PMSIHANDLE hRec;
	ProgressReportState *state = (ProgressReportState*)UserData;

	double featurePortion = TOTOAL_TICKS / state->nFeatureCount_;
	double tickDelta = ((1.0 * Current - state->nTicksReportedInFeature_) / Total) * featurePortion;
	state->nTicksReportedInFeature_ = Current; // Tick reported for this feature
	state->nTotalTicksReported_ += tickDelta; // Ticks reported to MSI, normalized to 1GB total.

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "DISM progress report: Feature #%u / %u, Current=%u, Total=%u, tickDelta=%f", state->nCurrentFeature_, state->nFeatureCount_, Current, Total, tickDelta);
	hr = WcaProgressMessage(tickDelta, FALSE);
	if (state->hCancel_ && (hr == S_FALSE))
	{
		::SetEvent(state->hCancel_);
	}
}
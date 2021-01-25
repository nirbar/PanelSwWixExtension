#include "stdafx.h"
#include <fileutil.h>
#include <wcautil.h>
#include <strutil.h>
#include <memutil.h>
#include <shlwapi.h>
#include <DismApi.h>
#include <regex>
#pragma comment (lib, "dismapi.lib")
#pragma comment (lib, "Shlwapi.lib")
using namespace std;

#define E_RETRY		HRESULT_FROM_WIN32(ERROR_RETRY)
enum ErrorHandling
{
	fail = 0,
	ignore = 1,
	prompt = 2
};

#define ERROR_ID_PACKAGE	27003
#define ERROR_ID_FEATURE	27004

static LPCWSTR DismStateString(DismPackageFeatureState state);
static void ProgressCallback(UINT Current, UINT Total, PVOID UserData);
static HRESULT HandleError(ErrorHandling nErrorHandling, UINT nErrorId, LPCWSTR szFeature, LPCWSTR szErrorMsg);

static ULONGLONG nMsiTicksReported_ = 0;
static HANDLE hCancel_ = NULL;

// Work around DISM API which in some versions is __stdcall and in others __cdecl.
static DISM_PROGRESS_CALLBACK _pfProgressCallback = [](UINT Current, UINT Total, PVOID UserData) -> void { ProgressCallback(Current, Total, UserData); };

// Will report 1GB for all features.
struct ProgressReportState
{
	// Number of ticks so far reported for current feature
	ULONGLONG nDismTicksReported = 0;

	LPWSTR szInclude = nullptr;
	LPWSTR szExclude = nullptr;
	LPWSTR szPackage = nullptr;
	int nMsiCost = 0;
	ErrorHandling eErrorHandling = ErrorHandling::fail;
	
	// Features resolved to be enabled
	DismFeature **pFeatures = nullptr;
	DWORD dwFeatureNum = 0;
};

/* CustomActionData fields:
- Log file
- Per feature:
	- Enable regex
	- Exclude regex
	- Package
	- Cost
	- Error handling
*/
extern "C" UINT __stdcall Dism(MSIHANDLE hInstall)
{
	UINT er = ERROR_SUCCESS;
	HRESULT hr = S_OK;
	DismSession hSession = DISM_SESSION_DEFAULT;
	UINT uFeatureNum = 0;
	DismFeature *pFeatures = nullptr;
	DismString *pErrorString = nullptr;
	BOOL bDismInit = FALSE;
	BOOL bRes = TRUE;
	LPWSTR szCAD = nullptr;
	LPWSTR szDismLog = nullptr;
	DWORD dwStateNum = 0;
	ProgressReportState *pStates = nullptr;
	ProgressReportState currState;
	errno_t err = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from Dism " FullVersion);

	hr = WcaGetProperty(L"CustomActionData", &szCAD);
	ExitOnFailure(hr, "Failed getting CustomActionData");

	hr = WcaReadStringFromCaData(&szCAD, &szDismLog);
	ExitOnFailure(hr, "Failed getting CustomActionData field");

	// Parse CAD
	while ((hr = WcaReadStringFromCaData(&szCAD, &currState.szInclude)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed getting CustomActionData field");

		hr = WcaReadStringFromCaData(&szCAD, &currState.szExclude);
		ExitOnFailure(hr, "Failed getting CustomActionData field");

		hr = WcaReadStringFromCaData(&szCAD, &currState.szPackage);
		ExitOnFailure(hr, "Failed getting CustomActionData field");

		hr = WcaReadIntegerFromCaData(&szCAD, &currState.nMsiCost);
		ExitOnFailure(hr, "Failed getting CustomActionData field");

		hr = WcaReadIntegerFromCaData(&szCAD, (int*)&currState.eErrorHandling);
		ExitOnFailure(hr, "Failed getting CustomActionData field");

		hr = MemInsertIntoArray((void**)&pStates, 0, 1, dwStateNum, sizeof(ProgressReportState), 1);
		ExitOnFailure(hr, "Failed inserting state to array");

		err = ::memcpy_s(pStates, sizeof(ProgressReportState), &currState, sizeof(currState));
		ExitOnNull((err == 0), hr, E_FAIL, "Failed copying memory. Error %i", err);

		++dwStateNum;
		ZeroMemory(&currState, sizeof(currState));
	}
	hr = S_OK;

	hCancel_ = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	ExitOnNullWithLastError(hCancel_, hr, "Failed creating event");

	hr = ::DismInitialize(DismLogErrorsWarningsInfo, szDismLog, nullptr);
	if (FAILED(hr))
	{
		::DismGetLastErrorMessage(&pErrorString);
		ExitOnFailure(hr, "Failed initializing DISM. %ls", (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}
	bDismInit = TRUE;

	hr = ::DismOpenSession(DISM_ONLINE_IMAGE, nullptr, nullptr, &hSession);
	if (FAILED(hr))
	{
		::DismGetLastErrorMessage(&pErrorString);
		ExitOnFailure(hr, "Failed opening DISM online session. %ls", (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}

	// Enumerate features to evaluate include/exclude regex.
	hr = ::DismGetFeatures(hSession, nullptr, DismPackageNone, &pFeatures, &uFeatureNum);
	if (FAILED(hr))
	{
		::DismGetLastErrorMessage(&pErrorString);
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
			for (DWORD j = 0; j < dwStateNum; ++j)
			{
				match_results<LPCWSTR> results;
				try
				{
					wregex rxInclude(pStates[j].szInclude);
					bRes = regex_search(pFeatures[i].FeatureName, results, rxInclude);
				}
				catch (std::regex_error ex)
				{
					hr = HRESULT_FROM_WIN32(ex.code());
					if (SUCCEEDED(hr))
					{
						hr = E_FAIL;
					}
					ExitOnFailure(hr, "Failed evaluating regular expression. %ls", ex.what());
				}

				if (!bRes || (results.length() <= 0))
				{
					continue;
				}
				// Feature included. Was it explictly excluded?
				if (pStates[j].szExclude && *pStates[j].szExclude)
				{
					try
					{
						wregex rxExclude(pStates[j].szExclude);
						bRes = regex_search(pFeatures[i].FeatureName, results, rxExclude);
					}
					catch (std::regex_error ex)
					{
						hr = HRESULT_FROM_WIN32(ex.code());
						if (SUCCEEDED(hr))
						{
							hr = E_FAIL;
						}
						ExitOnFailure(hr, "Failed evaluating regular expression. %ls", ex.what());
					}

					if (bRes && (results.length() > 0))
					{
						WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Feature '%ls' excluded by regex '%ls'", pFeatures[i].FeatureName, pStates[j].szExclude);
						continue;
					}
				}

				hr = MemInsertIntoArray((void**)&pStates[j].pFeatures, 0, 1, pStates[j].dwFeatureNum, sizeof(DismFeature*), 1);
				ExitOnFailure(hr, "Failed adding feature to array");

				pStates[j].pFeatures[0] = pFeatures + i;
				++pStates[j].dwFeatureNum;

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

	for (DWORD i = 0; i < dwStateNum; ++i)
	{
		if (pStates[i].dwFeatureNum == 0)
		{
			if (pStates[i].nMsiCost) // Report cost for features that were preinstalled.
			{
				WcaProgressMessage(pStates[i].nMsiCost, FALSE);
				nMsiTicksReported_ += pStates[i].nMsiCost;
			}
			continue;
		}

		if (pStates[i].szPackage && *pStates[i].szPackage)
		{
		LRetryPkg:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Adding package '%ls'", pStates[i].szPackage);
			bRes = FileExistsEx(pStates[i].szPackage, nullptr);
			ExitOnNullWithLastError(bRes, hr, "DISM package file not found: '%ls'", pStates[i].szPackage);

			hr = ::DismAddPackage(hSession, pStates[i].szPackage, FALSE, FALSE, hCancel_, _pfProgressCallback, &pStates[i]);
			if (HRESULT_CODE(hr) == ERROR_SUCCESS_REBOOT_REQUIRED)
			{
				hr = S_OK;
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Added package '%ls'. However, it requires reboot to complete", pStates[i].szPackage);
				WcaDeferredActionRequiresReboot();
			}
			if (FAILED(hr))
			{
				::DismGetLastErrorMessage(&pErrorString);
				WcaLogError(hr, "Failed adding DISM package '%ls'. %ls", pStates[i].szPackage, (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");

				hr = HandleError(pStates[i].eErrorHandling, ERROR_ID_PACKAGE, pStates[i].szPackage, (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
				if (hr == E_RETRY)
				{
					hr = S_OK;
					goto LRetryPkg;
				}
				ExitOnFailure(hr, "Failed adding DISM package");
			}

			// Cancelled?
			if (::WaitForSingleObject(hCancel_, 0) == WAIT_OBJECT_0)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Seems like DISM was canceled.");
				hr = S_FALSE;
				ExitFunction();
			}
		}

		for (DWORD j = 0; j < pStates[i].dwFeatureNum; ++j)
		{
		LRetryFtr:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Enabling feature '%ls'", pStates[i].pFeatures[j]->FeatureName);
			hr = ::DismEnableFeature(hSession, pStates[i].pFeatures[j]->FeatureName, nullptr, DismPackageNone, FALSE, nullptr, 0, TRUE, hCancel_, _pfProgressCallback, &pStates[i]);
			if (HRESULT_CODE(hr) == ERROR_SUCCESS_REBOOT_REQUIRED)
			{
				hr = S_OK;
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Enabled feature '%ls'. However, it requires reboot to complete", pStates[i].pFeatures[j]->FeatureName);
				WcaDeferredActionRequiresReboot();
			}

			if (FAILED(hr))
			{
				::DismGetLastErrorMessage(&pErrorString);
				WcaLogError(hr, "Failed enabling feature '%ls'. %ls", pStates[i].pFeatures[j]->FeatureName, (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");

				hr = HandleError(pStates[i].eErrorHandling, ERROR_ID_FEATURE, pStates[i].pFeatures[j]->FeatureName, (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
				if (hr == E_RETRY)
				{
					hr = S_OK;
					goto LRetryFtr;
				}
				ExitOnFailure(hr, "Failed enabling feature");
			}

			// Cancelled?
			if (::WaitForSingleObject(hCancel_, 0) == WAIT_OBJECT_0)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Seems like DISM was canceled.");
				hr = S_FALSE;
				ExitFunction();
			}
		}
	}

LExit:
	ULONGLONG nMsiRequestedTicks = 0;
	for (DWORD i = 0; i < dwStateNum; ++i)
	{
		nMsiRequestedTicks += pStates[i].nMsiCost;
		ReleaseNullStr(pStates[i].szExclude);
		ReleaseNullStr(pStates[i].szInclude);
		ReleaseNullStr(pStates[i].szPackage);
		ReleaseNullMem(pStates[i].pFeatures);
	}

	// Report any left-over ticks.
	if (nMsiRequestedTicks > nMsiTicksReported_)
	{
		WcaProgressMessage(nMsiRequestedTicks - nMsiTicksReported_, FALSE);
	}

	ReleaseMem(pStates);
	ReleaseStr(szCAD);
	ReleaseStr(szDismLog);

	if (hCancel_)
	{
		::CloseHandle(hCancel_);
	}
	if (pFeatures)
	{
		::DismDelete(pFeatures); //TODO Free each feature in the array?
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
	ProgressReportState *state = (ProgressReportState*)UserData;

	double ftrCnt = state->dwFeatureNum;
	if (state->szPackage && *state->szPackage)
	{
		++ftrCnt;
	}
	double ftrPortion = state->nMsiCost / ftrCnt;
	double tickDelta = ((((double)(Current - state->nDismTicksReported)) / Total) * ftrPortion);
	state->nDismTicksReported = Current; // Tick reported for this feature
	nMsiTicksReported_ += tickDelta; // Ticks reported to MSI

	hr = WcaProgressMessage(tickDelta, FALSE);
	if (hCancel_ && (hr == S_FALSE))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Cancelling DISM.");
		::SetEvent(hCancel_);
	}
}

static HRESULT HandleError(ErrorHandling nErrorHandling, UINT nErrorId, LPCWSTR szFeature, LPCWSTR szErrorMsg)
{
	HRESULT hr = S_OK;

	switch (nErrorHandling)
	{
	case ErrorHandling::fail:
	default:
		hr = E_FAIL;
		break;

	case ErrorHandling::ignore:
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Ignoring Dism failure");
		hr = S_OK;
		break;

	case ErrorHandling::prompt:
	{
		PMSIHANDLE hRec;
		UINT promptResult = IDOK;

		hRec = ::MsiCreateRecord(3);
		ExitOnNull(hRec, hr, E_FAIL, "Failed creating record");

		hr = WcaSetRecordInteger(hRec, 1, nErrorId);
		ExitOnFailure(hr, "Failed setting record integer");

		hr = WcaSetRecordString(hRec, 2, szFeature);
		ExitOnFailure(hr, "Failed setting record string");

		hr = WcaSetRecordString(hRec, 3, szErrorMsg);
		ExitOnFailure(hr, "Failed setting record string");

		promptResult = WcaProcessMessage((INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_ERROR | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 | MB_ICONERROR), hRec);
		switch (promptResult)
		{
		case IDABORT:
		case IDCANCEL:
		case 0: // Probably silent (result 0)
		default:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User aborted on DISM failure.");
			hr = E_FAIL;
			break;

		case IDRETRY:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User chose to retry on DISM failure.");
			hr = E_RETRY;
			break;

		case IDIGNORE:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User ignored DISM failure.");
			hr = S_OK;
			break;
		}
		break;
	}
	}

LExit:
	return hr;
}
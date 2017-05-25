#include "ExecOnComponent.h"
#include "../CaCommon/WixString.h"
#include <wcautil.h>
#include <DismApi.h>
#pragma comment (lib, "dismapi.lib")

static LPCWSTR DismStateString(DismPackageFeatureState state);

extern "C" __declspec(dllexport) UINT Dism(MSIHANDLE hInstall)
{
	UINT er = ERROR_SUCCESS;
	HRESULT hr = S_OK;
	DismSession hSession = DISM_SESSION_DEFAULT;
	DismFeature *pFeatures = NULL;
	DismString *pErrorString = NULL;
	BOOL bDismInit = FALSE;
	UINT uFeatureNum = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	hr = ::DismInitialize(DismLogErrorsWarningsInfo, NULL, NULL);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		BreakExitOnFailure1(hr, "Failed initializing DISM. %ls", (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}
	bDismInit = TRUE;

	hr = ::DismOpenSession(DISM_ONLINE_IMAGE, NULL, NULL, &hSession);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		BreakExitOnFailure1(hr, "Failed opening DISM online session. %ls",  (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}

	// Enumerate features
	hr = ::DismGetFeatures(hSession, NULL, DismPackageNone, &pFeatures, &uFeatureNum);
	if (FAILED(hr))
	{
		DismGetLastErrorMessage(&pErrorString);
		BreakExitOnFailure1(hr, "Failed querying DISM features. %ls",  (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
	}

	for (UINT i = 0; i < uFeatureNum; ++i)
	{
		// Print feature state
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Feature '%ls', state=%u", pFeatures[i].FeatureName, DismStateString(pFeatures[i].State));

		// Enable IIS features
		if ((::wcsncmp(L"IIS", pFeatures[i].FeatureName, 3) == 0) 
			&& (pFeatures[i].State != DismStateInstalled)
			&& (pFeatures[i].State != DismStateInstallPending)
			&& (pFeatures[i].State != DismStateSuperseded))
		{
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
				BreakExitOnFailure1(hr, "Failed enabling feature '%ls'. %ls", pFeatures[i].FeatureName, (pErrorString && pErrorString->Value) ? pErrorString->Value : L"");
			}
		}
	}

LExit:

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

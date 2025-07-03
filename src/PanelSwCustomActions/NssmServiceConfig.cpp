#include "pch.h"
#include "RegistryOperations.h"

extern "C" UINT __stdcall NssmServiceConfigSched(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CRegistryOperations deferredCAD;
	CRegistryOperations rollbackCAD;
	PMSIHANDLE hView;
	PMSIHANDLE hRec;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_RemoveRegistryValue exists.
	hr = WcaTableExists(L"PSW_NssmServiceConfig");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_NssmServiceConfig'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_NssmServiceConfig'. Have you authored 'PanelSw:NssmServiceConfig' entries in WiX code?");

	hr = WcaOpenExecuteView(L"SELECT `Component_`, `ServiceName`, `Application`, `AppDirectory`, `AppParameters`, `AppExit` FROM `PSW_NssmServiceConfig`", &hView);
	ExitOnFailure(hr, "Failed to execute view");

	while (E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
	{
		ExitOnFailure(hr, "Failed to fetch record");

		// Get record.
		CWixString szComponent, szServiceName, szApplication, szAppDirectory, szAppParameters, szAppExit, szServiceReg, szParametersReg, szAppExitReg;

		hr = WcaGetRecordString(hRec, 1, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component");
		hr = WcaGetRecordFormattedString(hRec, 2, (LPWSTR*)szServiceName);
		ExitOnFailure(hr, "Failed to get ServiceName");
		hr = WcaGetRecordFormattedString(hRec, 3, (LPWSTR*)szApplication);
		ExitOnFailure(hr, "Failed to get Application");
		hr = WcaGetRecordFormattedString(hRec, 4, (LPWSTR*)szAppDirectory);
		ExitOnFailure(hr, "Failed to get AppDirectory");
		hr = WcaGetRecordFormattedString(hRec, 5, (LPWSTR*)szAppParameters);
		ExitOnFailure(hr, "Failed to get AppParameters");
		hr = WcaGetRecordString(hRec, 6, (LPWSTR*)szAppExit);
		ExitOnFailure(hr, "Failed to get AppExit");

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Configuring NSSM service '%ls'", (LPCWSTR)szServiceName);

		hr = szServiceReg.Format(L"System\\CurrentControlSet\\Services\\%ls", (LPCWSTR)szServiceName);
		ExitOnFailure(hr, "Failed to format string");

		hr = szParametersReg.Format(L"%ls\\Parameters", (LPCWSTR)szServiceReg);
		ExitOnFailure(hr, "Failed to format string");

		hr = szAppExitReg.Format(L"%ls\\AppExit", (LPCWSTR)szParametersReg);
		ExitOnFailure(hr, "Failed to format string");

		hr = rollbackCAD.AddRecreateHierarchy(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg);
		ExitOnFailure(hr, "Failed to prepare rollback information for NSSM service '%ls'", (LPCWSTR)szServiceName);

		WCA_TODO todo = WcaGetComponentToDo(szComponent);
		if ((todo != WCA_TODO::WCA_TODO_INSTALL) && (todo != WCA_TODO::WCA_TODO_REINSTALL))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skip NSSM service '%ls' configuration because component '%ls' isn't (re)installed", (LPCWSTR)szServiceName, (LPCWSTR)szComponent);
			continue;
		}

		hr = deferredCAD.AddCreateKey(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg);
		ExitOnFailure(hr, "Failed to schedule registry operation");

		hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg, L"Application", REG_EXPAND_SZ, (LPCBYTE)(LPCWSTR)szApplication, (szApplication.StrLen() + 1) * sizeof(WCHAR));
		ExitOnFailure(hr, "Failed to schedule registry operation");

		hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg, L"AppDirectory", REG_EXPAND_SZ, (LPCBYTE)(LPCWSTR)szAppDirectory, (szAppDirectory.StrLen() + 1) * sizeof(WCHAR));
		ExitOnFailure(hr, "Failed to schedule registry operation");

		hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg, L"AppParameters", REG_EXPAND_SZ, (LPCBYTE)(LPCWSTR)szAppParameters, (szAppParameters.StrLen() + 1) * sizeof(WCHAR));
		ExitOnFailure(hr, "Failed to schedule registry operation");

		hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szAppExitReg, nullptr, REG_SZ, (LPCBYTE)(LPCWSTR)szAppExit, (szAppExit.StrLen() + 1) * sizeof(WCHAR));
		ExitOnFailure(hr, "Failed to schedule registry operation");
	}

	hr = rollbackCAD.SetCustomActionData(L"PSW_NssmServiceConfigRlbk");
	ExitOnFailure(hr, "Failed to set rollback action data");

	hr = deferredCAD.SetCustomActionData(L"PSW_NssmServiceConfigExec");
	ExitOnFailure(hr, "Failed to set deferred action data");

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

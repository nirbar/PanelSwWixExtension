#include "pch.h"
#include "RegistryOperations.h"

extern "C" UINT __stdcall NssmServiceConfigSched(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CRegistryOperations deferredCAD;
	CRegistryOperations rollbackCAD;
	PMSIHANDLE hView, hRec, hChildView, hChildExecRec;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_NssmServiceConfig exists.
	hr = WcaTableExists(L"PSW_NssmServiceConfig");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_NssmServiceConfig'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_NssmServiceConfig'. Have you authored 'PanelSw:NssmServiceConfig' entries in WiX code?");
	hr = WcaTableExists(L"PSW_NssmServiceConfig_Property");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_NssmServiceConfig_Property'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_NssmServiceConfig_Property'. Have you authored 'PanelSw:NssmServiceConfig' entries in WiX code?");

	hr = WcaOpenExecuteView(L"SELECT `Id`, `Component_`, `ServiceName`, `Application`, `AppDirectory`, `AppParameters`, `AppExit`, `AppRestartDelay` FROM `PSW_NssmServiceConfig`", &hView);
	ExitOnFailure(hr, "Failed to execute view");
	hr = WcaOpenView(L"SELECT `Name`, `Value`, `DataType` FROM `PSW_NssmServiceConfig_Property` WHERE `PSW_NssmServiceConfig_` = ?", &hChildView);
	ExitOnFailure(hr, "Failed to execute view");
	hChildExecRec = ::MsiCreateRecord(1);
	ExitOnNullWithLastError(hChildExecRec, hr, "Failed to create record");

	while (E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
	{
		ExitOnFailure(hr, "Failed to fetch record");

		// Get record.
		CWixString szId, szComponent, szServiceName, szApplication, szAppDirectory, szAppParameters, szAppExit, szServiceReg, szParametersReg, szAppExitReg;
		PMSIHANDLE hChildRec;
		int nAppRestartDelay = -1;

		hr = WcaGetRecordString(hRec, 1, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id");
		hr = WcaGetRecordString(hRec, 2, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component");
		hr = WcaGetRecordFormattedString(hRec, 3, (LPWSTR*)szServiceName);
		ExitOnFailure(hr, "Failed to get ServiceName");
		hr = WcaGetRecordFormattedString(hRec, 4, (LPWSTR*)szApplication);
		ExitOnFailure(hr, "Failed to get Application");
		hr = WcaGetRecordFormattedString(hRec, 5, (LPWSTR*)szAppDirectory);
		ExitOnFailure(hr, "Failed to get AppDirectory");
		hr = WcaGetRecordFormattedString(hRec, 6, (LPWSTR*)szAppParameters);
		ExitOnFailure(hr, "Failed to get AppParameters");
		hr = WcaGetRecordString(hRec, 7, (LPWSTR*)szAppExit);
		ExitOnFailure(hr, "Failed to get AppExit");
		hr = WcaGetRecordInteger(hRec, 8, &nAppRestartDelay);
		ExitOnFailure(hr, "Failed to get AppRestartDelay");

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

		// Mandatory fields
		hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg, L"Application", REG_EXPAND_SZ, (LPCBYTE)(LPCWSTR)szApplication, (szApplication.StrLen() + 1) * sizeof(WCHAR));
		ExitOnFailure(hr, "Failed to schedule registry operation");

		hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg, L"AppDirectory", REG_EXPAND_SZ, (LPCBYTE)(LPCWSTR)szAppDirectory, (szAppDirectory.StrLen() + 1) * sizeof(WCHAR));
		ExitOnFailure(hr, "Failed to schedule registry operation");

		hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg, L"AppParameters", REG_EXPAND_SZ, (LPCBYTE)(LPCWSTR)szAppParameters, (szAppParameters.StrLen() + 1) * sizeof(WCHAR));
		ExitOnFailure(hr, "Failed to schedule registry operation");

		hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szAppExitReg, nullptr, REG_SZ, (LPCBYTE)(LPCWSTR)szAppExit, (szAppExit.StrLen() + 1) * sizeof(WCHAR));
		ExitOnFailure(hr, "Failed to schedule registry operation");

		if (nAppRestartDelay >= 0)
		{
			hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg, L"AppRestartDelay", REG_DWORD, (LPCBYTE)&nAppRestartDelay, sizeof(nAppRestartDelay));
			ExitOnFailure(hr, "Failed to schedule registry operation");
		}

		// Optional fields
		hr = WcaSetRecordString(hChildExecRec, 1, (LPCWSTR)szId);
		ExitOnFailure(hr, "Failed to set record");

		hr = WcaExecuteView(hChildView, hChildExecRec);
		ExitOnFailure(hr, "Failed to execute view");

		while (E_NOMOREITEMS != (hr = WcaFetchRecord(hChildView, &hChildRec)))
		{
			ExitOnFailure(hr, "Failed to fetch record");
		
			CWixString szName, szValue;
			int nDataType = 0;
			int nNumericValue = 0;
			BYTE* pbData = nullptr;
			DWORD cbData = 0;

			hr = WcaGetRecordString(hChildRec, 1, (LPWSTR*)szName);
			ExitOnFailure(hr, "Failed to get Name");
			hr = WcaGetRecordFormattedString(hChildRec, 2, (LPWSTR*)szValue);
			ExitOnFailure(hr, "Failed to get Value");
			hr = WcaGetRecordInteger(hChildRec, 3, &nDataType);
			ExitOnFailure(hr, "Failed to get DataType");

			switch (nDataType)
			{
			case REG_SZ:
			case REG_EXPAND_SZ:
				pbData = (LPBYTE)(LPCWSTR)szValue;
				cbData = (szValue.StrLen() + 1) * sizeof(WCHAR);
				break;
			case REG_DWORD:
				nNumericValue = StrToInt((LPCWSTR)szValue);
				pbData = (LPBYTE)&nNumericValue;
				cbData = sizeof(nNumericValue);
				break;
			default:
				hr = E_INVALIDARG;
				ExitOnFailure(hr, "Unsupported NSSM registry data type %i", nDataType);
				break;
			}

			hr = deferredCAD.AddCreateValue(msidbRegistryRootLocalMachine, KEY_WOW64_64KEY, (LPCWSTR)szParametersReg, (LPCWSTR)szName, nDataType, pbData, cbData);
			ExitOnFailure(hr, "Failed to schedule registry operation");
		}
		hr = S_OK;
	}
	hr = S_OK;

	hr = rollbackCAD.SetCustomActionData(L"PSW_NssmServiceConfigRlbk");
	ExitOnFailure(hr, "Failed to set rollback action data");

	hr = deferredCAD.SetCustomActionData(L"PSW_NssmServiceConfigExec");
	ExitOnFailure(hr, "Failed to set deferred action data");

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

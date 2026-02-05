#include "pch.h"
#include "RegistryOperations.h"
#include "..\CaCommon\SummaryStream.h"

static HRESULT GetRegistryFlags(DWORD* pdwBitness, HKEY *phkRoot, msidbRegistryRoot* pnRoot, LPWSTR *pszRegistryKey);

extern "C" UINT __stdcall PropertyPersist(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRec;
	DWORD dwBitness = 0;
	msidbRegistryRoot nRoot = msidbRegistryRootLocalMachine;
	CWixString szRegistryKey;
	HKEY hkRoot = NULL;
	HKEY hkReg = NULL;

	hr = WcaInitialize(hInstall, "PropertyPersist");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_PropertyPersist");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_PropertyPersist'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_PropertyPersist'. Have you authored 'PanelSw:PropertyPersist' entries in WiX code?");

	hr = GetRegistryFlags(&dwBitness, &hkRoot, &nRoot, (LPWSTR*)szRegistryKey);
	ExitOnFailure(hr, "Failed to determine package bitness and context");

	hr = RegOpen(hkRoot, szRegistryKey, dwBitness | GENERIC_READ, &hkReg);
	if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
	{
		hr = S_OK;
		ExitFunction();
	}
	ExitOnFailure(hr, "Failed to open registry key '%ls'", (LPCWSTR)szRegistryKey);

	hr = WcaOpenExecuteView(L"SELECT `Id` FROM `PSW_PropertyPersist`", &hView);
	ExitOnFailure(hr, "Failed to execute view");

	while (E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
	{
		ExitOnFailure(hr, "Failed to fetch record");
		CWixString szId, szValue;

		hr = WcaGetRecordString(hRec, 1, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id");

		hr = RegReadString(hkReg, szId, (LPWSTR*)szValue);
		if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
		{
			hr = S_OK;
			continue;
		}
		ExitOnFailure(hr, "Failed to read value '%ls' from registry", (LPCWSTR)szId);

		if ((LPCWSTR)szValue == nullptr) 
		{
			hr = szValue.Copy(L"");
			ExitOnFailure(hr, "Failed to copy string");
		}

		hr = WcaSetProperty(szId, szValue);
		ExitOnFailure(hr, "Failed to set property '%ls'", (LPCWSTR)szId);
	}
	hr = S_OK;

LExit:
	ReleaseRegKey(hkReg);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

extern "C" UINT __stdcall PropertyPersistSched(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRec;
	DWORD dwBitness = 0;
	msidbRegistryRoot nRoot = msidbRegistryRootLocalMachine;
	CWixString szRegistryKey;
	HKEY hkRoot = NULL;
	HKEY hkReg = NULL;
	CRegistryOperations deferredCAD, rollbackCAD;

	hr = WcaInitialize(hInstall, "PropertyPersistSched");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_PropertyPersist");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_PropertyPersist'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_PropertyPersist'. Have you authored 'PanelSw:Persist' entries in WiX code?");

	hr = GetRegistryFlags(&dwBitness, &hkRoot, &nRoot, (LPWSTR*)szRegistryKey);
	ExitOnFailure(hr, "Failed to determine package bitness and context");

	hr = RegOpen(hkRoot, szRegistryKey, dwBitness | GENERIC_READ, &hkReg);
	if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
	{
		hr = S_OK;
	}
	ExitOnFailure(hr, "Failed to open registry key '%ls'", (LPCWSTR)szRegistryKey);

	hr = WcaOpenExecuteView(L"SELECT `Id` FROM `PSW_PropertyPersist`", &hView);
	ExitOnFailure(hr, "Failed to execute view");

	while (E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
	{
		ExitOnFailure(hr, "Failed to fetch record");
		CWixString szId, szCurrValue, szNewValue;
		BOOL bHasValue = FALSE;

		hr = WcaGetRecordString(hRec, 1, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id");

		if (hkReg)
		{
			hr = RegReadString(hkReg, szId, (LPWSTR*)szCurrValue);
			if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
			{
				hr = S_OK;
				bHasValue = FALSE;
			}
			else
			{
				ExitOnFailure(hr, "Failed to get read value '%ls' from registry", (LPCWSTR)szId);

				hr = rollbackCAD.AddCreateValue(nRoot, dwBitness, szRegistryKey, szId, REG_SZ, (LPCBYTE)(LPCWSTR)szCurrValue, sizeof(WCHAR) * (1 + szCurrValue.StrLen()));
				ExitOnFailure(hr, "Failed to add rollback data");
				bHasValue = TRUE;
			}
		}
		if (!bHasValue)
		{
			hr = rollbackCAD.AddDeleteValue(nRoot, dwBitness, szRegistryKey, szId);
			ExitOnFailure(hr, "Failed to add rollback data");
		}

		hr = WcaGetProperty(szId, (LPWSTR*)szNewValue);
		ExitOnFailure(hr, "Failed to get property");

		if (szNewValue.IsNullOrEmpty())
		{
			hr = deferredCAD.AddCreateValue(nRoot, dwBitness, szRegistryKey, szId, REG_SZ, (LPCBYTE)L"", sizeof(WCHAR));
			ExitOnFailure(hr, "Failed to add rollback data");
		}
		else
		{
			hr = deferredCAD.AddCreateValue(nRoot, dwBitness, szRegistryKey, szId, REG_SZ, (LPCBYTE)(LPCWSTR)szNewValue, sizeof(WCHAR) * (1 + szNewValue.StrLen()));
			ExitOnFailure(hr, "Failed to add rollback data");
		}
	}

	hr = rollbackCAD.DoDeferredAction(L"PropertyPersistRollback");
	ExitOnFailure(hr, "Failed to schedule rollback action");

	hr = deferredCAD.DoDeferredAction(L"PropertyPersistExec");
	ExitOnFailure(hr, "Failed to schedule action");

LExit:
	ReleaseRegKey(hkReg);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT GetRegistryFlags(DWORD* pdwBitness, HKEY* phkRoot, msidbRegistryRoot* pnRoot, LPWSTR* pszRegistryKey)
{
	HRESULT hr = S_OK;
	CWixString szUpgradeCode;

	hr = CSummaryStream::IsPackageX64();
	ExitOnFailure(hr, "Failed to determine package bitness");
	*pdwBitness = (hr == S_OK) ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;

	hr = CSummaryStream::IsUserContext();
	ExitOnFailure(hr, "Failed to determine package context");
	*pnRoot = (hr == S_OK) ? msidbRegistryRootCurrentUser : msidbRegistryRootLocalMachine;
	*phkRoot = (hr == S_OK) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

	hr = WcaGetProperty(L"UpgradeCode", (LPWSTR*)szUpgradeCode);
	ExitOnFailure(hr, "Failed to get UpgradeCode");

	hr = StrAllocFormatted(pszRegistryKey, L"SOFTWARE\\PanelSwWixExtension\\%ls\\Properties", (LPCWSTR)szUpgradeCode);
	ExitOnFailure(hr, "Failed to format string");

LExit:
	return hr;
}

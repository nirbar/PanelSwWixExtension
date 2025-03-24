#include "pch.h"
#include "RegistryOperations.h"
#include "..\CaCommon\SummaryStream.h"

static HRESULT String2Data(DWORD dwType, LPCWSTR szData, LPBYTE* ppbData, SIZE_T* pcbData);

extern "C" UINT __stdcall CustomUninstallKeySched(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRec;
	CRegistryOperations deferredCAD;
	CRegistryOperations rollbackCAD;
	LPBYTE pbData = nullptr;
	SIZE_T cbData = 0;
	DWORD dwBitness = 0;
	msidbRegistryRoot nRoot = msidbRegistryRootLocalMachine;

	hr = WcaInitialize(hInstall, "CustomUninstallKeySched");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = CSummaryStream::IsPackageX64();
	ExitOnFailure(hr, "Failed to determine package bitness");
	dwBitness = (hr == S_OK) ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;

	hr = CSummaryStream::IsUserContext();
	ExitOnFailure(hr, "Failed to determine package context");
	nRoot = (hr == S_OK) ? msidbRegistryRootCurrentUser : msidbRegistryRootLocalMachine;

	hr = WcaTableExists(L"PSW_CustomUninstallKey");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_CustomUninstallKey'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_CustomUninstallKey'. Have you authored 'PanelSw:CustomUninstallKey' entries in WiX code?");

	hr = WcaOpenExecuteView(L"SELECT `Id`, `ProductCode`, `Name`, `Data`, `DataType`, `Condition` FROM `PSW_CustomUninstallKey`", &hView);
	ExitOnFailure(hr, "Failed to execute view");

	while (E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
	{
		ExitOnFailure(hr, "Failed to fetch record");
		ReleaseNullMem(pbData);
		cbData = 0;

		CWixString szId, szProductCode, szName, szData, szCondition;
		CWixString szUninstallKey;
		int nDataType = 0;

		hr = WcaGetRecordString(hRec, 1, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id");
		hr = WcaGetRecordFormattedString(hRec, 2, (LPWSTR*)szProductCode);
		ExitOnFailure(hr, "Failed to get ProductCode");
		hr = WcaGetRecordString(hRec, 3, (LPWSTR*)szName);
		ExitOnFailure(hr, "Failed to get Name");
		hr = WcaGetRecordString(hRec, 4, (LPWSTR*)szData);
		ExitOnFailure(hr, "Failed to get Data");
		hr = WcaGetRecordInteger(hRec, 5, &nDataType);
		ExitOnFailure(hr, "Failed to get DataType");
		hr = WcaGetRecordString(hRec, 6, (LPWSTR*)szCondition);
		ExitOnFailure(hr, "Failed to get Condition");

		MSICONDITION cond = ::MsiEvaluateCondition(hInstall, (LPCWSTR)szCondition);
		switch (cond)
		{
		case MSICONDITION::MSICONDITION_NONE:
		case MSICONDITION::MSICONDITION_TRUE:
			break;
		case MSICONDITION::MSICONDITION_FALSE:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Condition evaluated false for %ls", (LPCWSTR)szId);
			continue;
		case MSICONDITION::MSICONDITION_ERROR:
			hr = E_FAIL;
			ExitOnFailure(hr, "Failed to evaluate condition");
		}

		if (szProductCode.IsNullOrEmpty())
		{
			hr = WcaGetProperty(L"ProductCode", (LPWSTR*)szProductCode);
			ExitOnFailure(hr, "Failed to get ProductCode");
		}

		hr = szUninstallKey.Format(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%ls", (LPCWSTR)szProductCode);
		ExitOnFailure(hr, "Failed to format string");

		hr = String2Data(nDataType, (LPCWSTR)szData, &pbData, &cbData);
		ExitOnFailure(hr, "Failed to serialize data");

		hr = deferredCAD.AddCreateValue(nRoot, dwBitness, (LPCWSTR)szUninstallKey, (LPCWSTR)szName, nDataType, pbData, cbData);
		ExitOnFailure(hr, "Failed to add rollback data");
	}

	hr = rollbackCAD.DoDeferredAction(L"CustomUninstallKeyRollback");
	ExitOnFailure(hr, "Failed to schedule rollback action");

	hr = deferredCAD.DoDeferredAction(L"CustomUninstallKeyExec");
	ExitOnFailure(hr, "Failed to schedule deferred action");

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT String2Data(DWORD dwType, LPCWSTR szData, LPBYTE* ppbData, SIZE_T* pcbData)
{
	HRESULT hr = S_OK;
	LPBYTE pbData = nullptr;
	DWORD cbData = 0;
	errno_t err = 0;
	ULARGE_INTEGER ull = {};
	LPWSTR sz = nullptr;

	switch (dwType)
	{
	case REG_SZ:
	case REG_EXPAND_SZ:
		cbData = sizeof(WCHAR) * (1 + wcslen(szData));
		pbData = (LPBYTE)MemAlloc(cbData, FALSE);
		ExitOnNull(pbData, hr, E_OUTOFMEMORY, "Failed to allocate memory");
		err = memcpy_s(pbData, cbData, szData, cbData);
		ExitOnWin32Error(err, hr, "Failed to mem-copy string");
		break;

	case REG_DWORD:
		cbData = sizeof(ull.LowPart);
		pbData = (LPBYTE)MemAlloc(cbData, FALSE);
		ExitOnNull(pbData, hr, E_OUTOFMEMORY, "Failed to allocate memory");

		ull.LowPart = wcstoul(szData, &sz, 10);
		ExitOnWin32Error(errno, hr, "Failed to parse string");

		err = memcpy_s(pbData, cbData, &ull.LowPart, cbData);
		ExitOnWin32Error(err, hr, "Failed to mem-copy string");
		break;

	case REG_QWORD:
		cbData = sizeof(ull.QuadPart);
		pbData = (LPBYTE)MemAlloc(cbData, FALSE);
		ExitOnNull(pbData, hr, E_OUTOFMEMORY, "Failed to allocate memory");

		ull.QuadPart = wcstoull(szData, &sz, 10);
		ExitOnWin32Error(errno, hr, "Failed to parse string");

		err = memcpy_s(pbData, cbData, &ull.QuadPart, cbData);
		ExitOnWin32Error(err, hr, "Failed to mem-copy string");
		break;

	case REG_NONE:
		ExitFunction();
	default:
		hr = E_INVALIDARG;
		ExitOnFailure(hr, "Unsupported data type 0x%08X", dwType);
		break;
	}

	*ppbData = pbData;
	*pcbData = cbData; 
	pbData = nullptr;

LExit:
	ReleaseMem(pbData);

	return hr;
}

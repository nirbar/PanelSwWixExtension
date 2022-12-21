#include "pch.h"
#include <errno.h>

extern "C" UINT __stdcall ReadIniValues(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = WcaInitialize(hInstall, "ReadIniValues");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_ReadIniValues exists.
	hr = WcaTableExists(L"PSW_ReadIniValues");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_ReadIniValues'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_ReadIniValues'. Have you authored 'PanelSw:ReadIniValues' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `FilePath`, `Section`, `Key`, `DestProperty`, `Attributes`, `Condition` FROM `PSW_ReadIniValues`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'ReadIniValues'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szFilePath, szSection, szKey, szDestProperty, szCondition, szValue(1024);
		DWORD dwRes = 0;
		int bIgnoreErrors = 0;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)szFilePath);
		ExitOnFailure(hr, "Failed to get FilePath.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)szSection);
		ExitOnFailure(hr, "Failed to get Section.");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)szKey);
		ExitOnFailure(hr, "Failed to get Key.");
		hr = WcaGetRecordString(hRecord, 5, (LPWSTR*)szDestProperty);
		ExitOnFailure(hr, "Failed to get DestProperty.");
		hr = WcaGetRecordInteger(hRecord, 6, &bIgnoreErrors);
		ExitOnFailure(hr, "Failed to get Attributes.");
		hr = WcaGetRecordString(hRecord, 7, (LPWSTR*)szCondition);
		ExitOnFailure(hr, "Failed to get Condition.");

		// Test condition
		MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, (LPCWSTR)szCondition);
		switch (condRes)
		{
		case MSICONDITION::MSICONDITION_NONE:
		case MSICONDITION::MSICONDITION_TRUE:
			WcaLog(LOGMSG_STANDARD, "Condition evaluated to true / none.");
			break;

		case MSICONDITION::MSICONDITION_FALSE:
			WcaLog(LOGMSG_STANDARD, "Skipping. Condition evaluated to false");
			continue;

		case MSICONDITION::MSICONDITION_ERROR:
			hr = E_FAIL;
			ExitOnFailure(hr, "Bad Condition field");
		}

		// Get the value.
		while ((dwRes = ::GetPrivateProfileStringW((LPCWSTR)szSection, (LPCWSTR)szKey, nullptr, (LPWSTR)szValue, szValue.Capacity(), (LPCWSTR)szFilePath)) == (szValue.Capacity() - 1))
		{
			hr = szValue.Allocate(2 * szValue.Capacity());
			ExitOnFailure(hr, "Failed allocating memory");
		}

		// Error?
		if (dwRes == 0)
		{
			if (bIgnoreErrors || ((dwRes = ::GetLastError()) == ERROR_SUCCESS))
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Couldn't read value from file '%ls', section '%ls', key '%ls'. Error 0x%08X, ignore = %i", (LPCWSTR)szFilePath, (LPCWSTR)szSection, (LPCWSTR)szKey, dwRes, bIgnoreErrors);
				continue;
			}
			ExitOnNullWithLastError(0, hr, "Failed reading value from '%ls'.", (LPCWSTR)szFilePath);
		}

		hr = WcaSetProperty((LPCWSTR)szDestProperty, (LPCWSTR)szValue);
		ExitOnFailure(hr, "Failed to set property.");
	}
	hr = S_OK;

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

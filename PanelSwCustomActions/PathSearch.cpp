#include "stdafx.h"
#include "../CaCommon/WixString.h"

extern "C" UINT __stdcall PathSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_PathSearch");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_PathSearch'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_PathSearch'. Have you authored 'PanelSw:PathSearch' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `FileName`, `Property_` FROM `PSW_PathSearch`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_PathSearch'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szFileName, szProperty, szFullPath;
		DWORD nBuffSize = 0;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)szFileName);
		ExitOnFailure(hr, "Failed to get FileName.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szProperty);
		ExitOnFailure(hr, "Failed to get Property_.");

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Searching '%ls' on PATH. Setting result in property '%ls'", (LPCWSTR)szFileName, (LPCWSTR)szProperty);

		while ((nBuffSize = ::SearchPath(nullptr, szFileName, nullptr, szFullPath.Capacity(), (LPWSTR)szFullPath, nullptr)) > szFullPath.Capacity())
		{
			hr = szFullPath.Allocate(++nBuffSize);
			ExitOnFailure(hr, "Failed allocating buffer");
		}
		if (nBuffSize == 0)
		{
			DWORD dwRes = ::GetLastError();
			if ((dwRes == ERROR_FILE_NOT_FOUND) || (dwRes == ERROR_PATH_NOT_FOUND))
			{
				continue;
			}
			ExitOnWin32Error(dwRes, hr, "Failed searching for file '%ls' on PATH", (LPCWSTR)szFileName);
		}

		hr = WcaSetProperty(szProperty, szFullPath);
		ExitOnFailure(hr, "Failed setting property");
	}
	hr = S_OK;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}
#include "..\CaCommon\WixString.h"
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

extern "C" UINT __stdcall DiskSpace(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	DWORD dwRes = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_DiskSpace");
	BreakExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_DiskSpace'. Have you authored 'PanelSw:DiskSpace' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Directory_` FROM `PSW_DiskSpace`", &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString dirName, dirPath;
		ULARGE_INTEGER diskSpace = { 0 };
		WCHAR ullBuff[70];
		errno_t errn = 0;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)dirName);
		BreakExitOnFailure(hr, "Failed to get Directory_ ID.");
		ExitOnNull(!dirName.IsNullOrEmpty(), hr, E_INVALIDARG, "Directory ID is empty");

		hr = WcaGetProperty(dirName, (LPWSTR*)dirPath);
		BreakExitOnFailure(hr, "Failed getting value of '%ls' directory", (LPCWSTR)dirName);
		ExitOnNull(!dirPath.IsNullOrEmpty(), hr, E_INVALIDARG, "Directory path is empty");

		dwRes = ::PathStripToRoot((LPWSTR)dirPath);
		BreakExitOnNullWithLastError(dwRes, hr, "Failed getting disk root from folder '%ls'", (LPCWSTR)dirPath);

		dwRes = ::GetDiskFreeSpaceEx((LPCWSTR)dirPath, nullptr, nullptr, &diskSpace);
		BreakExitOnNullWithLastError(dwRes, hr, "Failed getting disk free space for '%ls'", (LPCWSTR)dirPath);

		errn = ::_ui64tow_s(diskSpace.QuadPart, ullBuff, sizeof(ullBuff), 10);
		BreakExitOnNull(!errn, hr, E_FAIL, "Failed converting disk free space to string for directory '%ls'", (LPCWSTR)dirPath);

		hr = dirName.AppnedFormat(L"_DISK_FREE_SPACE");
		BreakExitOnFailure(hr, "Failed appending string");

		hr = WcaSetProperty(dirName, ullBuff);
		BreakExitOnFailure(hr, "Failed setting property");
	}
	hr = S_OK;

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

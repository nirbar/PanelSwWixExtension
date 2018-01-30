#include "FileOperations.h"
#include <strutil.h>
#include "../CaCommon/WixString.h"
#include <Shellapi.h>
#include <Shlwapi.h>
#include "FileOperations.h"
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

#define BackupAndRestore_QUERY L"SELECT `Id`, `Component_`, `Path`, `Flags` FROM `PSW_BackupAndRestore`"
enum BackupAndRestoreQuery { Id = 1, Component, Path, Flags, Condition };

extern "C" __declspec(dllexport) UINT BackupAndRestore(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CFileOperations rollbackCAD;
	CFileOperations deferredCAD;
	CFileOperations commitCAD;
	WCHAR shortTempPath[MAX_PATH + 1];
	WCHAR longTempPath[MAX_PATH + 1];
	DWORD dwRes = 0;
	LPWSTR szTempFilesSpec = nullptr;
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_BackupAndRestore");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_BackupAndRestore'. Have you authored 'PanelSw:BackupAndRestore' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(BackupAndRestore_QUERY, &hView);
	BreakExitOnFailure1(hr, "Failed to execute SQL query '%ls'.", BackupAndRestore_QUERY);
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Get temporay folder
	dwRes = ::GetTempPath(MAX_PATH, shortTempPath);
	BreakExitOnNullWithLastError(dwRes, hr, "Failed getting temporary folder");
	BreakExitOnNull((dwRes <= MAX_PATH), hr, E_FAIL, "Temporary folder path too long");

	dwRes = ::GetLongPathName(shortTempPath, longTempPath, MAX_PATH + 1);
	BreakExitOnNullWithLastError(dwRes, hr, "Failed expanding temporary folder");
	BreakExitOnNull((dwRes <= MAX_PATH), hr, E_FAIL, "Temporary folder expanded path too long");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szPath, szComponent;
		WCHAR szTempFile[MAX_PATH + 1];
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		int flags = 0;
		bool bIgnoreMissing;

		hr = WcaGetRecordString(hRecord, BackupAndRestoreQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, BackupAndRestoreQuery::Path, (LPWSTR*)szPath);
		BreakExitOnFailure(hr, "Failed to get Path.");
		hr = WcaGetRecordInteger(hRecord, BackupAndRestoreQuery::Flags, &flags);
		BreakExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordString(hRecord, BackupAndRestoreQuery::Component, (LPWSTR*)szComponent);
		BreakExitOnFailure(hr, "Failed to get Condition.");
		bIgnoreMissing = flags & CFileOperations::FileOperationsAttributes::IgnoreMissingPath;

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		if ((compAction == WCA_TODO::WCA_TODO_UNINSTALL) || (compAction == WCA_TODO::WCA_TODO_UNKNOWN))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping backup and restore '%ls' since component is neither installed nor repaired", (LPCWSTR)szId);
			continue;
		}

		// Generate temp file name.
		dwRes = ::GetTempFileName(longTempPath, L"BNR", 0, szTempFile);
		BreakExitOnNullWithLastError(dwRes, hr, "Failed getting temporary file name");

		hr = deferredCAD.CopyPath(szPath, szTempFile, false, bIgnoreMissing, flags & CFileOperations::FileOperationsAttributes::IgnoreErrors);
		BreakExitOnFailure(hr, "Failed backing up '%ls'", (LPCWSTR)szPath);
		if (hr == S_FALSE)
		{
			deferredCAD.DeletePath(szTempFile, true, true);
			continue;
		}

		// For folders, we'll copy the contents to target folder. Otherwise, the temp folder will be copied with the content
		if (::PathIsDirectory(szTempFile))
		{
			hr = StrAllocFormatted(&szTempFilesSpec, L"%s\\*", szTempFile);
			BreakExitOnFailure(hr, "Failed formatting string.");

			hr = rollbackCAD.AddMoveFile(szTempFilesSpec, szPath, flags);
			BreakExitOnFailure(hr, "Failed creating custom action data for rollback action.");

			hr = rollbackCAD.AddDeleteFile(szTempFile, flags);
			BreakExitOnFailure(hr, "Failed creating custom action data for rollbak action.");
		}
		else
		{
			hr = rollbackCAD.AddMoveFile(szTempFile, szPath, flags);
			BreakExitOnFailure(hr, "Failed creating custom action data for rollback action.");
		}

		hr = deferredCAD.AddCopyFile(szTempFilesSpec, szPath, flags);
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred action.");

		hr = commitCAD.AddDeleteFile(szTempFile, flags);
		BreakExitOnFailure(hr, "Failed creating custom action data for commit action.");
	}

	hr = rollbackCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback action.");
	hr = WcaSetProperty(L"BackupAndRestore_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = commitCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for commit action.");
	hr = WcaSetProperty(L"BackupAndRestore_commit", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting commit action data.");

	ReleaseNullStr(szCustomActionData);
	hr = deferredCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaSetProperty(L"BackupAndRestore_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");
	hr = WcaProgressMessage(deferredCAD.GetCost(), TRUE);
	BreakExitOnFailure(hr, "Failed extending progress bar.");

LExit:
	ReleaseStr(szCustomActionData);
	ReleaseStr(szTempFilesSpec);
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

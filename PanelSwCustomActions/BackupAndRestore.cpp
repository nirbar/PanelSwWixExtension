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

extern "C" UINT __stdcall BackupAndRestore(MSIHANDLE hInstall) noexcept
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
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_BackupAndRestore");
	ExitOnFailure(hr, "Table does not exist 'PSW_BackupAndRestore'. Have you authored 'PanelSw:BackupAndRestore' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(BackupAndRestore_QUERY, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", BackupAndRestore_QUERY);

	// Get temporay folder
	dwRes = ::GetTempPath(MAX_PATH, shortTempPath);
	ExitOnNullWithLastError(dwRes, hr, "Failed getting temporary folder");
	ExitOnNull((dwRes <= MAX_PATH), hr, E_FAIL, "Temporary folder path too long");

	dwRes = ::GetLongPathName(shortTempPath, longTempPath, MAX_PATH + 1);
	ExitOnNullWithLastError(dwRes, hr, "Failed expanding temporary folder");
	ExitOnNull((dwRes <= MAX_PATH), hr, E_FAIL, "Temporary folder expanded path too long");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szPath, szComponent;
		WCHAR szTempFile[MAX_PATH + 1];
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		int flags = 0;
		bool bIgnoreMissing;
		bool bIsFolder = false;

		hr = WcaGetRecordString(hRecord, BackupAndRestoreQuery::Id, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, BackupAndRestoreQuery::Path, (LPWSTR*)szPath);
		ExitOnFailure(hr, "Failed to get Path.");
		hr = WcaGetRecordInteger(hRecord, BackupAndRestoreQuery::Flags, &flags);
		ExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordString(hRecord, BackupAndRestoreQuery::Component, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Condition.");
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
		ExitOnNullWithLastError(dwRes, hr, "Failed getting temporary file name");

		bIsFolder = ::PathIsDirectory(szPath);
		if (bIsFolder) // For folders, we'll delete the file that holds the same name. For files, we'll simply overwrite.
		{
			hr = deferredCAD.DeletePath(szTempFile, true, true); // Delete the temporay file we created now as we're not going to need it.
			ExitOnFailure(hr, "Failed deleting file");
		}

		hr = deferredCAD.CopyPath(szPath, szTempFile, false, bIgnoreMissing, flags & CFileOperations::FileOperationsAttributes::IgnoreErrors);
		ExitOnFailure(hr, "Failed backing up '%ls'", (LPCWSTR)szPath);
		if (hr == S_FALSE)
		{
			deferredCAD.DeletePath(szTempFile, true, true); // Delete the temporay file we created now as we're not going to need it.
			continue;
		}

		// For folders, we'll copy the contents to target folder. Otherwise, the temp folder will be copied with the content
		if (bIsFolder)
		{
			hr = rollbackCAD.AddDeleteFile(szPath, flags);
			ExitOnFailure(hr, "Failed creating custom action data for rollback action.");

			hr = rollbackCAD.AddMoveFile(szTempFile, szPath, flags);
			ExitOnFailure(hr, "Failed creating custom action data for rollback action.");
		
			hr = deferredCAD.AddDeleteFile(szPath, flags);
			ExitOnFailure(hr, "Failed creating custom action data for deferred action.");

			hr = deferredCAD.AddCopyFile(szTempFile, szPath, flags);
			ExitOnFailure(hr, "Failed creating custom action data for deferred action.");
		}
		else
		{
			hr = rollbackCAD.AddMoveFile(szTempFile, szPath, flags);
			ExitOnFailure(hr, "Failed creating custom action data for rollback action.");

			hr = deferredCAD.AddCopyFile(szTempFile, szPath, flags);
			ExitOnFailure(hr, "Failed creating custom action data for deferred action.");
		}

		hr = commitCAD.AddDeleteFile(szTempFile, flags);
		ExitOnFailure(hr, "Failed creating custom action data for commit action.");
	}

	hr = rollbackCAD.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback action.");
	hr = WcaSetProperty(L"BackupAndRestore_rollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = commitCAD.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for commit action.");
	hr = WcaSetProperty(L"BackupAndRestore_commit", szCustomActionData);
	ExitOnFailure(hr, "Failed setting commit action data.");

	ReleaseNullStr(szCustomActionData);
	hr = deferredCAD.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaSetProperty(L"BackupAndRestore_deferred", szCustomActionData);
	ExitOnFailure(hr, "Failed setting deferred action data.");
	hr = WcaProgressMessage(deferredCAD.GetCost(), TRUE);
	ExitOnFailure(hr, "Failed extending progress bar.");

LExit:
	ReleaseStr(szCustomActionData);
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

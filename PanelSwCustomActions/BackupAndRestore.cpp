#include "FileOperations.h"
#include <strutil.h>
#include <pathutil.h>
#include "../CaCommon/WixString.h"
#include <Shellapi.h>
#include <Shlwapi.h>
#include "FileOperations.h"
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

#define BackupAndRestore_QUERY L"SELECT `Id`, `Component_`, `Path`, `Flags` FROM `PSW_BackupAndRestore`"
enum BackupAndRestoreQuery { Id = 1, Component, Path, Flags, Condition };

extern "C" UINT __stdcall BackupAndRestore(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CFileOperations rollbackCAD;
	CFileOperations deferredCAD;
	CFileOperations commitCAD;
	DWORD dwRes = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_BackupAndRestore");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_BackupAndRestore'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_BackupAndRestore'. Have you authored 'PanelSw:BackupAndRestore' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(BackupAndRestore_QUERY, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", BackupAndRestore_QUERY);

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szPath, szComponent;
		CWixString szTempFile;
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

		// Generate temp file or folder name.
		bIsFolder = ::PathIsDirectory(szPath);
		hr = CFileOperations::MakeTemporaryName(szPath, L"BNR%05i.tmp", bIsFolder, (LPWSTR*)szTempFile);
		ExitOnFailure(hr, "Failed getting temporary path for '%ls'", (LPCWSTR)szPath);
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Backing up and '%ls' and will restore it later, using temporary path '%ls'", (LPCWSTR)szPath, (LPCWSTR)szTempFile);

		hr = deferredCAD.CopyPath(szPath, szTempFile, false, bIgnoreMissing, flags & CFileOperations::FileOperationsAttributes::IgnoreErrors, false, false);
		ExitOnFailure(hr, "Failed backing up '%ls'", (LPCWSTR)szPath);
		if (hr == S_FALSE)
		{
			deferredCAD.DeletePath(szTempFile, true, true, false, false); // Delete the temporay file we created now as we're not going to need it.
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

	hr = rollbackCAD.SetCustomActionData(L"BackupAndRestore_rollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	hr = commitCAD.SetCustomActionData(L"BackupAndRestore_commit");
	ExitOnFailure(hr, "Failed setting commit action data.");

	hr = deferredCAD.SetCustomActionData(L"BackupAndRestore_deferred");
	ExitOnFailure(hr, "Failed setting deferred action data.");

	hr = WcaProgressMessage(deferredCAD.GetCost(), TRUE);
	ExitOnFailure(hr, "Failed extending progress bar.");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

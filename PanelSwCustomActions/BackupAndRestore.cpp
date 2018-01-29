#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include <strutil.h>
#include <Shellapi.h>
#include <Shlwapi.h>
#include "FileOperations.h"
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

static HRESULT CopyPath(LPCWSTR szFrom, LPCWSTR szTo, bool bIgnoreMissing, bool bIgnoreErrors);

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
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_DeletePath");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_DeletePath'. Have you authored 'PanelSw:DeletePath' entries in WiX code?");

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
		CWixString tempFile;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		int flags = 0;

		hr = WcaGetRecordString(hRecord, BackupAndRestoreQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, BackupAndRestoreQuery::Path, (LPWSTR*)szPath);
		BreakExitOnFailure(hr, "Failed to get Path.");
		hr = WcaGetRecordInteger(hRecord, BackupAndRestoreQuery::Flags, &flags);
		BreakExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordString(hRecord, BackupAndRestoreQuery::Component, (LPWSTR*)szComponent);
		BreakExitOnFailure(hr, "Failed to get Condition.");

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		if ((compAction == WCA_TODO::WCA_TODO_UNINSTALL) || (compAction == WCA_TODO::WCA_TODO_UNKNOWN))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping backup and restore '%ls' since component is neither installed nor repaired", (LPCWSTR)szId);
			continue;
		}

		// Generate temp file name.
		hr = tempFile.Allocate(MAX_PATH + 1);
		BreakExitOnFailure(hr, "Failed allocating memory");

		dwRes = ::GetTempFileName(longTempPath, L"BNR", 0, (LPWSTR)tempFile);
		BreakExitOnNullWithLastError(dwRes, hr, "Failed getting temporary file name");

		hr = rollbackCAD.AddMoveFile(tempFile, szPath, flags);
		BreakExitOnFailure(hr, "Failed creating custom action data for rollback action.");

		hr = deferredCAD.AddCopyFile(tempFile, szPath, flags);
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred action.");

		hr = commitCAD.AddDeleteFile(tempFile, flags);
		BreakExitOnFailure(hr, "Failed creating custom action data for commit action.");

		// Add deferred data to move file szFilePath -> tempFile.
		hr = CopyPath(szPath, tempFile, flags & CFileOperations::FileOperationsAttributes::IgnoreMissingPath, flags & CFileOperations::FileOperationsAttributes::IgnoreErrors);
		BreakExitOnFailure(hr, "Failed backing up '%ls'", (LPCWSTR)szPath);
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
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT CopyPath(LPCWSTR szFrom, LPCWSTR szTo, bool bIgnoreMissing, bool bIgnoreErrors)
{
	SHFILEOPSTRUCT opInfo;
	HRESULT hr = S_OK;
	INT nRes = ERROR_SUCCESS;
	LPWSTR szFromNull = nullptr;
	LPWSTR szToNull = nullptr;

	if (bIgnoreMissing)
	{
		if (!::PathFileExists(szFrom))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping copy '%ls' as it doesn't exist and marked to ignore missing", szFrom);
			ExitFunction1(hr = S_FALSE);
		}
	}

	hr = StrAllocFormatted(&szFromNull, L"%s%c%c", szFrom, L'\0', L'\0');
	BreakExitOnFailure(hr, "Failed formatting string");

	hr = StrAllocFormatted(&szToNull, L"%s%c%c", szTo, L'\0', L'\0');
	BreakExitOnFailure(hr, "Failed formatting string");

	// Prepare 
	::memset(&opInfo, 0, sizeof(opInfo));
	opInfo.wFunc = FO_COPY;
	opInfo.pFrom = szFromNull;
	opInfo.pTo = szToNull;
	opInfo.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_NO_UI;

	nRes = ::SHFileOperation(&opInfo);
	if (bIgnoreErrors && ((nRes != 0) || opInfo.fAnyOperationsAborted))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Failed Copying '%ls' to '%ls'; Ignoring error (%i)", szFromNull, szToNull, nRes);
		ExitFunction();
	}
	BreakExitOnNull1((nRes == 0), hr, E_FAIL, "Failed copying file '%ls' to '%ls' (Error %i)", szFromNull, szToNull, nRes);
	BreakExitOnNull((!opInfo.fAnyOperationsAborted), hr, E_FAIL, "Failed copying file (operation aborted)");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Copied '%ls' to '%ls'", szFromNull, szToNull);

LExit:
	ReleaseStr(szFromNull);
	ReleaseStr(szToNull);
	return hr;
}

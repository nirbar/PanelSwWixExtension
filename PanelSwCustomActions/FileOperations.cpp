#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include <Shellapi.h>
#include <Shlwapi.h>
#include "fileOperationsDetails.pb.h"
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

#define DeletePath_QUERY L"SELECT `Id`, `Path`, `Flags`, `Condition` FROM `PSW_DeletePath`"
enum DeletePathQuery { Id = 1, Path, Flags, Condition };

extern "C" UINT __stdcall DeletePath(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CFileOperations rollbackCAD;
	CFileOperations deferredFileCAD;
	CFileOperations commitCAD;
	WCHAR shortTempPath[MAX_PATH + 1];
	WCHAR longTempPath[MAX_PATH + 1];
	DWORD dwRes = 0;
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_DeletePath");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_DeletePath'. Have you authored 'PanelSw:DeletePath' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(DeletePath_QUERY, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", DeletePath_QUERY);
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
		CWixString szId, szFilePath, szCondition;
		CWixString tempFile;
		int flags = 0;

		hr = WcaGetRecordString(hRecord, DeletePathQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, DeletePathQuery::Path, (LPWSTR*)szFilePath);
		BreakExitOnFailure(hr, "Failed to get Path.");
		hr = WcaGetRecordInteger(hRecord, DeletePathQuery::Flags, &flags);
		BreakExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordString(hRecord, DeletePathQuery::Condition, (LPWSTR*)szCondition);
		BreakExitOnFailure(hr, "Failed to get Condition.");

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
			BreakExitOnFailure(hr, "Bad Condition field");
		}

		// Generate temp file name.
		hr = tempFile.Allocate(MAX_PATH + 1);
		BreakExitOnFailure(hr, "Failed allocating memory");

		dwRes = ::GetTempFileName(longTempPath, L"DLT", 0, (LPWSTR)tempFile);
		BreakExitOnNullWithLastError(dwRes, hr, "Failed getting temporary file name");

		hr = rollbackCAD.AddDeleteFile((LPCWSTR)szFilePath, flags | CFileOperations::FileOperationsAttributes::IgnoreMissingPath); // Delete the target path. Done for case where the source is folder rather than file.
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred file action.");

		hr = rollbackCAD.AddMoveFile((LPCWSTR)tempFile, (LPCWSTR)szFilePath, flags);
		BreakExitOnFailure(hr, "Failed creating custom action data for rollback action.");

		// Add deferred data to move file szFilePath -> tempFile.
		hr = deferredFileCAD.AddDeleteFile((LPCWSTR)tempFile, flags); // Delete the temporary file. Done for case where the source is folder rather than file.
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred file action.");

		hr = deferredFileCAD.AddMoveFile((LPCWSTR)szFilePath, (LPCWSTR)tempFile, flags);
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred file action.");

		hr = commitCAD.AddDeleteFile((LPCWSTR)tempFile, flags);
		BreakExitOnFailure(hr, "Failed creating custom action data for commit action.");
	}

	hr = rollbackCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"DeletePath_rollback", szCustomActionData, rollbackCAD.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

	ReleaseNullStr(szCustomActionData);
	hr = deferredFileCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"DeletePath_deferred", szCustomActionData, deferredFileCAD.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

	ReleaseNullStr(szCustomActionData);
	hr = commitCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"DeletePath_commit", szCustomActionData, commitCAD.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

LExit:
	ReleaseStr(szCustomActionData);
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CFileOperations::AddCopyFile(LPCWSTR szFrom, LPCWSTR szTo, int flags)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	FileOperationsDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CFileOperations", &pCmd);
	BreakExitOnFailure(hr, "Failed to add command");

	pDetails = new FileOperationsDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_move(false);
	pDetails->set_from(szFrom, WSTR_BYTE_SIZE(szFrom));
	pDetails->set_to(szTo, WSTR_BYTE_SIZE(szTo));

	pDetails->set_ignoreerrors(flags & FileOperationsAttributes::IgnoreErrors);
	pDetails->set_ignoremissing(flags & FileOperationsAttributes::IgnoreMissingPath);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CFileOperations::AddMoveFile(LPCWSTR szFrom, LPCWSTR szTo, int flags)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	FileOperationsDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CFileOperations", &pCmd);
	BreakExitOnFailure(hr, "Failed to add command");

	pDetails = new FileOperationsDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_move(true);
	pDetails->set_from(szFrom, WSTR_BYTE_SIZE(szFrom));
	pDetails->set_to(szTo, WSTR_BYTE_SIZE(szTo));

	pDetails->set_ignoreerrors(flags & FileOperationsAttributes::IgnoreErrors);
	pDetails->set_ignoremissing(flags & FileOperationsAttributes::IgnoreMissingPath);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CFileOperations::AddDeleteFile(LPCWSTR szPath, int flags)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	FileOperationsDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CFileOperations", &pCmd);
	BreakExitOnFailure(hr, "Failed to add command");

	pDetails = new FileOperationsDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_from(szPath, WSTR_BYTE_SIZE(szPath));
	pDetails->set_ignoreerrors(flags & FileOperationsAttributes::IgnoreErrors);
	pDetails->set_ignoremissing(flags & FileOperationsAttributes::IgnoreMissingPath);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CFileOperations::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	FileOperationsDetails details;
	LPCWSTR szFrom = nullptr;
	LPCWSTR szTo = nullptr;

	bRes = details.ParseFromString(command);
	BreakExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking FileOperationsDetails");

	if (details.from().size())
	{
		szFrom = (LPCWSTR)details.from().data();
	}
	BreakExitOnNull(szFrom && *szFrom, hr, E_FAIL, "'From' field is empty");

	if (details.to().size())
	{
		szTo = (LPCWSTR)details.to().data();
	}

	if (szFrom && szTo)
	{
		hr = CopyPath(szFrom, szTo, details.move(), details.ignoremissing(), details.ignoreerrors());
		BreakExitOnFailure(hr, "Failed to copy file");
	}
	else 
	{
		hr = DeletePath(szFrom, details.ignoremissing(), details.ignoreerrors());
		BreakExitOnFailure(hr, "Failed to delete file");
	}

LExit:
	return hr;
}

HRESULT CFileOperations::CopyPath(LPCWSTR szFrom, LPCWSTR szTo, bool bMove, bool bIgnoreMissing, bool bIgnoreErrors)
{
	SHFILEOPSTRUCT opInfo;
	HRESULT hr = S_OK;
	INT nRes = ERROR_SUCCESS;
	LPWSTR szFromNull = nullptr;
	LPWSTR szToNull = nullptr;

	hr = StrAllocFormatted(&szFromNull, L"%s%c%c", szFrom, L'\0', L'\0');
	BreakExitOnFailure(hr, "Failed formatting string");

	hr = StrAllocFormatted(&szToNull, L"%s%c%c", szTo, L'\0', L'\0');
	BreakExitOnFailure(hr, "Failed formatting string");

	// Remove trailing backslashes (fails on Windows XP)
	::PathRemoveBackslash(szFromNull);
	::PathRemoveBackslash(szToNull);

	// Prepare 
	::memset(&opInfo, 0, sizeof(opInfo));
	opInfo.wFunc = bMove ? FO_MOVE : FO_COPY;
	opInfo.pFrom = szFromNull;
	opInfo.pTo = szToNull;
	opInfo.fFlags = FOF_NO_UI;

	nRes = ::SHFileOperation(&opInfo);
	if (bIgnoreMissing && (nRes == ERROR_FILE_NOT_FOUND) || (nRes == ERROR_PATH_NOT_FOUND))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping copy '%ls' as it doesn't exist and marked to ignore missing", szFrom);
		ExitFunction1(hr = S_FALSE);
	}
	if (bIgnoreErrors && ((nRes != 0) || opInfo.fAnyOperationsAborted))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Failed Copying '%ls' to '%ls'; Ignoring error (%i)", szFromNull, szToNull, nRes);
		ExitFunction1(hr = S_FALSE);
	}
	BreakExitOnWin32Error(nRes, hr, "Failed copying file '%ls' to '%ls'", szFromNull, szToNull);
	BreakExitOnNull((!opInfo.fAnyOperationsAborted), hr, E_FAIL, "Failed copying file (operation aborted)");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Copied '%ls' to '%ls'", szFromNull, szToNull);

LExit:
	ReleaseStr(szFromNull);
	ReleaseStr(szToNull);
	return hr;
}

HRESULT CFileOperations::DeletePath(LPCWSTR szFrom, bool bIgnoreMissing, bool bIgnoreErrors)
{
	SHFILEOPSTRUCT opInfo;
	HRESULT hr = S_OK;
	INT nRes = ERROR_SUCCESS;
	LPWSTR szFromNull = nullptr;

	hr = StrAllocFormatted(&szFromNull, L"%s%c%c", szFrom, L'\0', L'\0');
	BreakExitOnFailure(hr, "Failed formatting string");

	// Remove trailing backslashes (fails on Windows XP)
	::PathRemoveBackslash(szFromNull);

	// Prepare 
	::memset(&opInfo, 0, sizeof(opInfo));
	opInfo.wFunc = FO_DELETE;
	opInfo.pFrom = szFromNull;
	opInfo.fFlags = FOF_NO_UI;

	nRes = ::SHFileOperation(&opInfo);
	if (bIgnoreMissing && (nRes == ERROR_FILE_NOT_FOUND) || (nRes == ERROR_PATH_NOT_FOUND))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping deletion of '%ls' as it doesn't exist and marked to ignore missing", szFrom);
		ExitFunction1(hr = S_FALSE);
	}
	if (bIgnoreErrors && ((nRes != 0) || opInfo.fAnyOperationsAborted))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Failed deleting '%ls'; Ignoring error (%i)", szFromNull, nRes);
		ExitFunction1(hr = S_FALSE);
	}
	BreakExitOnNull((nRes == 0), hr, E_FAIL, "Failed deleting '%ls' (Error %i)", szFromNull, nRes);
	BreakExitOnNull((!opInfo.fAnyOperationsAborted), hr, E_FAIL, "Failed deleting file (operation aborted)");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Deleted '%ls'", szFrom);

LExit:
	ReleaseStr(szFromNull);

	return hr;
}
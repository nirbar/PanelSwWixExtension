#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include <fileutil.h>
#include <dirutil.h>
#include <pathutil.h>
#include <Shellapi.h>
#include <Shlwapi.h>
#include "fileOperationsDetails.pb.h"
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

extern "C" UINT __stdcall DeletePath(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CFileOperations commitCAD;
	DWORD dwRes = 0;
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_DeletePath");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_DeletePath'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_DeletePath'. Have you authored 'PanelSw:DeletePath' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Path`, `Flags`, `Condition` FROM `PSW_DeletePath` ORDER BY `Order`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szFilePath, szCondition;
		int flags = 0;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)szFilePath);
		ExitOnFailure(hr, "Failed to get Path.");
		hr = WcaGetRecordInteger(hRecord, 2, &flags);
		ExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)szCondition);
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

		// Trim slashes
		for (size_t i = szFilePath.StrLen() - 1; ((i > 1) && ((szFilePath[i] == L'\\') || (szFilePath[i] == L'/'))); --i)
		{
			szFilePath[i] = NULL;
		}

		hr = commitCAD.AddDeleteFile((LPCWSTR)szFilePath, flags);
		ExitOnFailure(hr, "Failed creating custom action data for commit action.");
	}

	hr = commitCAD.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for commit action.");
	hr = WcaDoDeferredAction(L"DeletePath_commit", szCustomActionData, commitCAD.GetCost());
	ExitOnFailure(hr, "Failed scheduling deferred action.");

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
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new FileOperationsDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_move(false);
	pDetails->set_from(szFrom, WSTR_BYTE_SIZE(szFrom));
	pDetails->set_to(szTo, WSTR_BYTE_SIZE(szTo));

	pDetails->set_ignoreerrors(flags & FileOperationsAttributes::IgnoreErrors);
	pDetails->set_ignoremissing(flags & FileOperationsAttributes::IgnoreMissingPath);
	pDetails->set_onlyifempty(flags & FileOperationsAttributes::OnlyIfEmpty);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

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
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new FileOperationsDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_move(true);
	pDetails->set_from(szFrom, WSTR_BYTE_SIZE(szFrom));
	pDetails->set_to(szTo, WSTR_BYTE_SIZE(szTo));

	pDetails->set_ignoreerrors(flags & FileOperationsAttributes::IgnoreErrors);
	pDetails->set_ignoremissing(flags & FileOperationsAttributes::IgnoreMissingPath);
	pDetails->set_onlyifempty(flags & FileOperationsAttributes::OnlyIfEmpty);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

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
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new FileOperationsDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_from(szPath, WSTR_BYTE_SIZE(szPath));
	pDetails->set_ignoreerrors(flags & FileOperationsAttributes::IgnoreErrors);
	pDetails->set_ignoremissing(flags & FileOperationsAttributes::IgnoreMissingPath);
	pDetails->set_onlyifempty(flags & FileOperationsAttributes::OnlyIfEmpty);
	pDetails->set_allowreboot(flags & FileOperationsAttributes::AllowReboot);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CFileOperations::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	FileOperationsDetails details;
	LPCWSTR szFrom = nullptr;
	LPCWSTR szTo = nullptr;

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking FileOperationsDetails");

	if (details.from().size())
	{
		szFrom = (LPCWSTR)(LPVOID)details.from().data();
	}
	ExitOnNull(szFrom && *szFrom, hr, E_FAIL, "'From' field is empty");

	if (details.to().size())
	{
		szTo = (LPCWSTR)(LPVOID)details.to().data();
	}

	if (szFrom && szTo)
	{
		hr = CopyPath(szFrom, szTo, details.move(), details.ignoremissing(), details.ignoreerrors(), details.onlyifempty(), details.allowreboot());
		ExitOnFailure(hr, "Failed to copy file");
	}
	else 
	{
		hr = DeletePath(szFrom, details.ignoremissing(), details.ignoreerrors(), details.onlyifempty(), details.allowreboot());
		ExitOnFailure(hr, "Failed to delete file");
	}

LExit:
	return hr;
}

HRESULT CFileOperations::CopyPath(LPCWSTR szFrom, LPCWSTR szTo, bool bMove, bool bIgnoreMissing, bool bIgnoreErrors, bool bOnlyIfEmpty, bool bAllowReboot)
{
	SHFILEOPSTRUCT opInfo;
	HRESULT hr = S_OK;
	INT nRes = ERROR_SUCCESS;
	LPWSTR szFromNull = nullptr;
	LPWSTR szToNull = nullptr;
	LPWSTR* pszFiles = nullptr;
	UINT nFiles = 0;

	if (bMove && bOnlyIfEmpty && ::PathIsDirectory(szFrom))
	{
		hr = ListFiles(szFrom, L"*", true, &pszFiles, &nFiles);
		ExitOnFailure(hr, "Failed testing wether folder '%ls' is empty", szFrom);

		if (nFiles > 0)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skip moving folder '%ls' because it contains %u files", szFrom, nFiles);
			ExitFunction();
		}
	}

	hr = StrAllocFormatted(&szFromNull, L"%ls%lc%lc", szFrom, L'\0', L'\0');
	ExitOnFailure(hr, "Failed formatting string");

	hr = StrAllocFormatted(&szToNull, L"%ls%lc%lc", szTo, L'\0', L'\0');
	ExitOnFailure(hr, "Failed formatting string");

	// Remove trailing slashes
	for (size_t i = ::wcslen(szFromNull) - 1; ((i > 1) && ((szFromNull[i] == L'\\') || (szFromNull[i] == L'/'))); --i)
	{
		szFromNull[i] = NULL;
	}
	for (size_t i = ::wcslen(szToNull) - 1; ((i > 1) && ((szToNull[i] == L'\\') || (szToNull[i] == L'/'))); --i)
	{
		szToNull[i] = NULL;
	}

	// Prepare 
	::memset(&opInfo, 0, sizeof(opInfo));
	opInfo.wFunc = bMove ? FO_MOVE : FO_COPY;
	opInfo.pFrom = szFromNull;
	opInfo.pTo = szToNull;
	opInfo.fFlags = FOF_NO_UI;

	LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"%ls '%ls' to '%ls'", bMove ? L"Moving" : L"Copying", szFromNull, szToNull);
	nRes = ::SHFileOperation(&opInfo);
	
	//TODO On Windows XP the error code is generic (0x402) when the source file is absent
	if (bIgnoreMissing && ((nRes == ERROR_FILE_NOT_FOUND) || (nRes == ERROR_PATH_NOT_FOUND)))
	{
		LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Skipping copy '%ls' as it doesn't exist and marked to ignore missing", szFrom);
		ExitFunction1(hr = S_FALSE);
	}
	
	if ((nRes != 0) || opInfo.fAnyOperationsAborted)
	{
		// If file is locked then retry the operation after reboot
		if (bMove && bAllowReboot && ((nRes == ERROR_LOCK_VIOLATION) || (nRes == ERROR_DRIVE_LOCKED) || (nRes == ERROR_SHARING_VIOLATION)))
		{
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Failed moving '%ls' to '%ls' due to a lock on the file(s), so reboot will be required", szFromNull, szToNull);

			::MoveFileEx(szFromNull, szToNull, MOVEFILE_DELAY_UNTIL_REBOOT);
			WcaDeferredActionRequiresReboot();
			ExitFunction1(hr = S_OK);
		}
		if (bIgnoreErrors)
		{
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Failed copying '%ls' to '%ls'; Ignoring error (%i)", szFromNull, szToNull, nRes);
			ExitFunction1(hr = S_FALSE);
		}
	}
	ExitOnWin32Error(nRes, hr, "Failed copying file '%ls' to '%ls'", szFromNull, szToNull);
	ExitOnNull((!opInfo.fAnyOperationsAborted), hr, E_FAIL, "Failed copying file (operation aborted)");

LExit:
	ReleaseStrArray(pszFiles, nFiles);
	ReleaseStr(szFromNull);
	ReleaseStr(szToNull);
	return hr;
}

HRESULT CFileOperations::DeletePath(LPCWSTR szFrom, bool bIgnoreMissing, bool bIgnoreErrors, bool bOnlyIfEmpty, bool bAllowReboot)
{
	SHFILEOPSTRUCT opInfo;
	HRESULT hr = S_OK;
	INT nRes = ERROR_SUCCESS;
	LPWSTR szFromNull = nullptr;
	LPWSTR* pszFiles = nullptr;
	UINT nFiles = 0;

	if (bOnlyIfEmpty && ::PathIsDirectory(szFrom))
	{
		hr = ListFiles(szFrom, L"*", true, &pszFiles, &nFiles);
		ExitOnFailure(hr, "Failed testing wether folder '%ls' is empty", szFrom);

		if (nFiles > 0)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skip deleting folder '%ls' because it contains %u files", szFrom, nFiles);
			ExitFunction();
		}
	}

	hr = StrAllocFormatted(&szFromNull, L"%ls%lc%lc", szFrom, L'\0', L'\0');
	ExitOnFailure(hr, "Failed formatting string");

	// Prepare 
	::memset(&opInfo, 0, sizeof(opInfo));
	opInfo.wFunc = FO_DELETE;
	opInfo.pFrom = szFromNull;
	opInfo.fFlags = FOF_NO_UI;

	LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Deleting '%ls'", szFrom);
	nRes = ::SHFileOperation(&opInfo);
	if (bIgnoreMissing && (nRes == ERROR_FILE_NOT_FOUND) || (nRes == ERROR_PATH_NOT_FOUND))
	{
		LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Skipping deletion of '%ls' as it doesn't exist and marked to ignore missing", szFrom);
		ExitFunction1(hr = S_FALSE);
	}
	if ((nRes != 0) || opInfo.fAnyOperationsAborted)
	{
		// If file is locked then retry the operation after reboot
		if (bAllowReboot && ((nRes == ERROR_LOCK_VIOLATION) || (nRes == ERROR_DRIVE_LOCKED) || (nRes == ERROR_SHARING_VIOLATION)))
		{
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Failed deleting '%ls' due to a lock on file(s), so reboot will be required", szFromNull);

			// MoveFileEx can delete empty folder only, so we must explictly delete files first
			ReleaseStrArray(pszFiles, nFiles);
			hr = ListFiles(szFrom, L"*", true, &pszFiles, &nFiles);
			ExitOnFailure(hr, "Failed listing files in folder '%ls'", szFrom);

			for (UINT i = 0; i < nFiles; ++i)
			{
				::MoveFileEx(pszFiles[i], nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
			}

			::MoveFileEx(szFromNull, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
			WcaDeferredActionRequiresReboot();
			ExitFunction1(hr = S_OK);
		}
		if (bIgnoreErrors)
		{
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Failed deleting '%ls'; Ignoring error (%i)", szFromNull, nRes);
			ExitFunction1(hr = S_FALSE);
		}
	}
	ExitOnNull((nRes == 0), hr, E_FAIL, "Failed deleting '%ls' (Error %i)", szFromNull, nRes);
	ExitOnNull((!opInfo.fAnyOperationsAborted), hr, E_FAIL, "Failed deleting file (operation aborted)");

LExit:
	ReleaseStrArray(pszFiles, nFiles);
	ReleaseStr(szFromNull);

	return hr;
}

FileRegexDetails::FileEncoding CFileOperations::DetectEncoding(const void* pFileContent, DWORD dwSize)
{
	int nTests = IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK | IS_TEXT_UNICODE_NOT_ASCII_MASK;
	HRESULT hr = S_OK;
	FileRegexDetails::FileEncoding eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None;

	if (dwSize < 2)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File is too short to analyze encoding. Assuming multi-byte");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte;
		ExitFunction();
	}

	::IsTextUnicode(pFileContent, dwSize, &nTests);

	// Multi-byte
	if (nTests & IS_TEXT_UNICODE_ILLEGAL_CHARS)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File has illegal UNICODE characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_ODD_LENGTH)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File has odd length so it cannot be UNICODE");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte;
		ExitFunction();
	}

	// Unicode BOM
	if (nTests & IS_TEXT_UNICODE_SIGNATURE)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File has UNICODE BOM");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File has reverse UNICODE BOM");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_ReverseUnicode;
		ExitFunction();
	}

	// Unicode
	if (nTests & IS_TEXT_UNICODE_ASCII16)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File has only ASCII UNICODE characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_CONTROLS)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File has UNICODE control characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}

	// Reverse unicode
	if (nTests & IS_TEXT_UNICODE_REVERSE_ASCII16)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File has only reverse ASCII UNICODE characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_ReverseUnicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_REVERSE_CONTROLS)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File has reverse UNICODE control characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_ReverseUnicode;
		ExitFunction();
	}

	// Stat tests
	if (nTests & IS_TEXT_UNICODE_STATISTICS)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File is probably UNICODE");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_REVERSE_STATISTICS)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File is probably reverse UNICODE");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}

	// Unicode NULL (can't tell if it strait or reverse).
	if (nTests & IS_TEXT_UNICODE_NULL_BYTES)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File has NULL UNICODE characters. Assuming unicode, though, it may as well be reverse unicode");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}

	WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "All tests failed for UNICODE encoding. Assuming multi-byte");
	eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte;

LExit:
	return eEncoding;
}

// static 
HRESULT CFileOperations::PathToDevicePath(LPCWSTR szPath, LPWSTR* pszDevicePath)
{
	HRESULT hr = S_OK;
	CWixString szDevicePath;
	CWixString szDrive;
	LPCWSTR szVolumeEnd = nullptr;
	WCHAR szDosName[MAX_PATH + 1];
	DWORD dwRes = ERROR_SUCCESS;

	szVolumeEnd = wcschr(szPath, L':');
	if ((szVolumeEnd == nullptr) || (szVolumeEnd == szPath))
	{
		hr = StrAllocString(pszDevicePath, szPath, 0);
		ExitOnFailure(hr, "Failed copying string");
		ExitFunction();
	}

	hr = szDrive.Copy(szPath, szVolumeEnd - szPath + 1); // Copy C:
	ExitOnFailure(hr, "Failed copying string");

	::ZeroMemory(szDosName, ARRAYSIZE(szDosName) * sizeof(WCHAR));

	dwRes = ::QueryDosDevice((LPCWSTR)szDrive, szDosName, ARRAYSIZE(szDosName));
	ExitOnNullWithLastError(dwRes, hr, "Failed getting device path for drive '%ls'", (LPCWSTR)szDrive);

	hr = szDevicePath.Format(L"%ls%ls", szDosName, szVolumeEnd + 1);
	ExitOnFailure(hr, "Failed formatting device path");

	*pszDevicePath = szDevicePath.Detach();

LExit:
	return hr;
}

// static 
HRESULT CFileOperations::ListFiles(LPCWSTR szFolder, LPCWSTR szPattern, bool bRecursive, LPWSTR** pszFiles, UINT* pcFiles)
{
	HRESULT hr = S_OK;
	LPWSTR szFullPattern = nullptr;
	LPWSTR szFullFolder = nullptr;
	LPWSTR szCurrFile = nullptr;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	if (IsSymbolicLinkOrMount(szFolder))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Folder '%ls' is a symbolic link or a mount point, so not enumerating its files");
		ExitFunction();
	}

	hr = StrAllocString(&szFullFolder, szFolder, 0);
	ExitOnFailure(hr, "Failed allocating string");

	hr = PathBackslashTerminate(&szFullFolder);
	ExitOnFailure(hr, "Failed allocating string");

	// Start with subfolders, no pattern filtering
	if (bRecursive)
	{
		hr = StrAllocFormatted(&szFullPattern, L"%ls*", szFullFolder);
		ExitOnFailure(hr, "Failed allocating string");

		hFind = ::FindFirstFile(szFullPattern, &FindFileData);
		ExitOnNullWithLastError((hFind != INVALID_HANDLE_VALUE), hr, "Failed searching files in '%ls'", szFullFolder);

		do
		{
			if ((::wcscmp(L".", FindFileData.cFileName) == 0) || (::wcscmp(L"..", FindFileData.cFileName) == 0))
			{
				continue;
			}

			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			{
				ReleaseNullStr(szCurrFile);

				hr = StrAllocFormatted(&szCurrFile, L"%ls%ls", szFullFolder, FindFileData.cFileName);
				ExitOnFailure(hr, "Failed allocating string");

				hr = ListFiles(szCurrFile, szPattern, bRecursive, pszFiles, pcFiles);
				ExitOnFailure(hr, "Failed finding files");
			}

		} while (::FindNextFile(hFind, &FindFileData));
		ExitOnNullWithLastError((::GetLastError() == ERROR_NO_MORE_FILES), hr, "Failed searching files in '%ls'", szFullFolder);

		::FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}

	// Now look for files
	ReleaseNullStr(szFullPattern);
	if (szPattern && *szPattern)
	{
		hr = StrAllocFormatted(&szFullPattern, L"%ls%ls", szFullFolder, szPattern);
		ExitOnFailure(hr, "Failed allocating string");
	}
	else
	{
		hr = StrAllocFormatted(&szFullPattern, L"%ls*", szFullFolder);
		ExitOnFailure(hr, "Failed allocating string");
	}

	hFind = ::FindFirstFile(szFullPattern, &FindFileData);
	ExitOnNullWithLastError((hFind != INVALID_HANDLE_VALUE), hr, "Failed searching files in '%ls'", szFullPattern);

	do
	{
		if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		ReleaseNullStr(szCurrFile);

		hr = StrAllocFormatted(&szCurrFile, L"%ls%ls", szFullFolder, FindFileData.cFileName);
		ExitOnFailure(hr, "Failed allocating string");

		hr = StrArrayAllocString(pszFiles, pcFiles, szCurrFile, 0);
		ExitOnFailure(hr, "Failed allocating string");

	} while (::FindNextFile(hFind, &FindFileData));
	ExitOnNullWithLastError((::GetLastError() == ERROR_NO_MORE_FILES), hr, "Failed searching files in '%ls'", szFullPattern);

LExit:

	ReleaseStr(szFullPattern);
	ReleaseStr(szCurrFile);
	if (hFind && (hFind != INVALID_HANDLE_VALUE))
	{
		::FindClose(hFind);
	}

	return hr;
}

//static 
HRESULT CFileOperations::MakeTemporaryName(LPCWSTR szBackupOf, LPCWSTR szPrefix, bool bIsFolder, LPWSTR* pszTempName)
{
	HRESULT hr = S_OK;
	CWixString szParentFolder;

	if (szBackupOf && *szBackupOf)
	{
		hr = szParentFolder.Copy(szBackupOf);
		ExitOnFailure(hr, "Failed copying string");

		while (!DirExists(szParentFolder, nullptr))
		{
			::PathRemoveBackslashW((LPWSTR)szParentFolder);
			::PathRemoveFileSpecW((LPWSTR)szParentFolder);
			if (szParentFolder.StrLen() < 3)
			{
				szParentFolder.Release();
				break;
			}
		}
	}

LRetry:
	if (bIsFolder)
	{
		hr = PathCreateTempDirectory((LPCWSTR)szParentFolder, szPrefix, INFINITE, pszTempName);
	}
	else
	{
		hr = PathCreateTempFile((LPCWSTR)szParentFolder, szPrefix, INFINITE, FILE_ATTRIBUTE_NORMAL, pszTempName, nullptr);
	}
	if ((hr == E_PATHNOTFOUND) && szParentFolder.StrLen())
	{
		::PathRemoveBackslashW((LPWSTR)szParentFolder);
		::PathRemoveFileSpecW((LPWSTR)szParentFolder);
		if (szParentFolder.StrLen() < 3)
		{
			szParentFolder.Release();
		}
		goto LRetry;
	}
	ExitOnFailure(hr, "Failed getting temporary path in '%ls'", (LPCWSTR)szParentFolder);

LExit:
	return hr;
}

bool CFileOperations::IsSymbolicLinkOrMount(LPCWSTR szPath)
{
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA wfaData;
	HRESULT hr = S_OK;

	hFind = ::FindFirstFile(szPath, &wfaData);
	ExitOnNullWithLastError((hFind && (hFind != INVALID_HANDLE_VALUE)), hr, "Path '%ls' can't be checked for reparse point tag");

LExit:
	if (hFind && (hFind != INVALID_HANDLE_VALUE))
	{
		::FindClose(hFind);
	}

	return SUCCEEDED(hr) ? (((wfaData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT) 
		&& ((wfaData.dwReserved0 == IO_REPARSE_TAG_SYMLINK) || (wfaData.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT))) : false;
}

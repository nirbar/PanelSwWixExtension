#include "FileRegex.h"
#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include <regex>
#include <memutil.h>
using namespace std;
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

#define FileRegex_QUERY L"SELECT `Id`, `FilePath`, `Regex`, `Replacement`, `IgnoreCase`, `Encoding`, `Condition` FROM `PSW_FileRegex` ORDER BY `Order`"
enum FileRegexQuery { Id = 1, FilePath = 2, Regex = 3, Replacement = 4, IgnoreCase = 5, Encoding = 6, Condition = 7 };

extern "C" __declspec(dllexport) UINT FileRegex(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CFileRegex oDeferredFileRegex;
	CFileOperations rollbackCAD;
	CFileOperations deferredFileCAD;
	CFileOperations commitCAD;
	WCHAR shortTempPath[MAX_PATH + 1];
	WCHAR longTempPath[MAX_PATH + 1];
	LPWSTR szCustomActionData = nullptr;
	DWORD dwRes = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_FileRegex exists.
	hr = WcaTableExists(L"PSW_FileRegex");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_FileRegex'. Have you authored 'PanelSw:FileRegex' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(FileRegex_QUERY, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", FileRegex_QUERY);
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Get temporary folder
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
		CWixString szId, szFilePath, szRegex, szReplacement, szCondition;
		CWixString tempFile;
		int nIgnoreCase = 0;
		int nEncoding = 0;

		hr = WcaGetRecordString(hRecord, FileRegexQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, FileRegexQuery::FilePath, (LPWSTR*)szFilePath);
		BreakExitOnFailure(hr, "Failed to get FilePath.");
		hr = WcaGetRecordFormattedString(hRecord, FileRegexQuery::Regex, (LPWSTR*)szRegex);
		BreakExitOnFailure(hr, "Failed to get Regex.");
		hr = WcaGetRecordFormattedString(hRecord, FileRegexQuery::Replacement, (LPWSTR*)szReplacement);
		BreakExitOnFailure(hr, "Failed to get Replacement.");
		hr = WcaGetRecordInteger(hRecord, FileRegexQuery::IgnoreCase, &nIgnoreCase);
		BreakExitOnFailure(hr, "Failed to get IgnoreCase.");
		hr = WcaGetRecordInteger(hRecord, FileRegexQuery::Encoding, &nEncoding);
		BreakExitOnFailure(hr, "Failed to get Encoding.");
		hr = WcaGetRecordString(hRecord, FileRegexQuery::Condition, (LPWSTR*)szCondition);
		BreakExitOnFailure(hr, "Failed to get Condition.");

		// Test condition
		MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, szCondition);
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

		dwRes = ::GetTempFileName(longTempPath, L"RGX", 0, (LPWSTR)tempFile);
		BreakExitOnNullWithLastError( dwRes, hr, "Failed getting temporary file name");

		hr = rollbackCAD.AddMoveFile( (LPCWSTR)tempFile, szFilePath);
		BreakExitOnFailure(hr, "Failed creating custom action data for rollback action.");

		// Add deferred data to copy file szFilePath -> tempFile.
		hr = deferredFileCAD.AddCopyFile(szFilePath, (LPCWSTR)tempFile);
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred file action.");

		hr = oDeferredFileRegex.AddFileRegex(szFilePath, szRegex, szReplacement, (FileRegexDetails::FileEncoding)nEncoding, nIgnoreCase != 0);
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred action.");

		hr = commitCAD.AddDeleteFile( (LPCWSTR)tempFile);
		BreakExitOnFailure(hr, "Failed creating custom action data for commit action.");
	}

	hr = rollbackCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"FileRegex_rollback", szCustomActionData, rollbackCAD.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredFileRegex.Prepend(&deferredFileCAD);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred file actions.");
	hr = oDeferredFileRegex.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"FileRegex_deferred", szCustomActionData, oDeferredFileRegex.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

	ReleaseNullStr(szCustomActionData);
	hr = commitCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"FileRegex_commit", szCustomActionData, commitCAD.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

LExit:
	ReleaseStr(szCustomActionData);
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CFileRegex::AddFileRegex(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	FileRegexDetails *pDetails = nullptr;
	Any *pAny = nullptr;

	hr = AddCommand("CFileRegex", &pCmd);
	BreakExitOnFailure(hr, "Failed to add XML element");

	pDetails = new FileRegexDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_file(szFilePath, WSTR_BYTE_SIZE(szFilePath));
	pDetails->set_expression(szRegex, WSTR_BYTE_SIZE(szRegex));
	pDetails->set_replacement(szReplacement, WSTR_BYTE_SIZE(szReplacement));
	pDetails->set_encoding(eEncoding);
	pDetails->set_ignorecase(bIgnoreCase);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	pAny->PackFrom(*pDetails);

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CFileRegex::DeferredExecute(const ::google::protobuf::Any* pCommand)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	FileRegexDetails details;
	LPCWSTR szFile = nullptr;
	LPCWSTR szExpression = nullptr;
	LPCWSTR szReplacement = nullptr;

	BreakExitOnNull(pCommand->Is<FileRegexDetails>(), hr, E_INVALIDARG, "Expected command to be FileRegexDetails");
	bRes = pCommand->UnpackTo(&details);
	BreakExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking FileOperationsDetails");

	szFile = (LPCWSTR)details.file().data();
	szExpression = (LPCWSTR)details.expression().data();
	szReplacement = (LPCWSTR)details.replacement().data();

	hr = Execute(szFile, szExpression, szReplacement, details.encoding(), details.ignorecase());
	BreakExitOnFailure(hr, "Failed to execute file regular expression");

LExit:
	return hr;
}

HRESULT CFileRegex::Execute(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase)
{
    HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	DWORD dwBytesRead = 0;
	DWORD dwFileSize = 0;
	DWORD dwFileAttr = FILE_ATTRIBUTE_NORMAL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	void* pFileContents = nullptr;
	FileRegexDetails::FileEncoding eDetectedEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None;

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Replacing matches of regex '%ls' with '%ls' on file '%ls'", szRegex, szReplacement, szFilePath);

	hFile = ::CreateFile(szFilePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed opening file");

	dwFileSize = ::GetFileSize(hFile, nullptr);
	pFileContents = MemAlloc(dwFileSize + 2, FALSE);
	ExitOnNull(pFileContents, hr, E_FAIL, "Failed allocating memory");
	
	// Terminate with ascii/wchar NULL.
	((BYTE*)pFileContents)[dwFileSize] = NULL;
	((BYTE*)pFileContents)[dwFileSize + 1] = NULL;

	bRes = ::ReadFile(hFile, pFileContents, dwFileSize, &dwBytesRead, nullptr);
	ExitOnNullWithLastError(bRes, hr, "Failed reading file");
	ExitOnNull((dwFileSize == dwBytesRead), hr, E_FAIL, "Failed reading file. Read %i/%i bytes", dwBytesRead, dwFileSize);

	::CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	eDetectedEncoding = DetectEncoding(pFileContents, dwFileSize);
	if (eEncoding != FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None)
	{
		if (eDetectedEncoding != eEncoding)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Overriding detected encoding with author-specified encoding");
			eDetectedEncoding = eEncoding;
		}
	}

	if (eDetectedEncoding == FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte)
	{
		hr = ExecuteMultibyte(szFilePath, (LPCSTR)pFileContents, szRegex, szReplacement, bIgnoreCase);
		ExitOnFailure(hr, "Failed executing regular expression");
	}
	else
	{
		hr = ExecuteUnicode(szFilePath, (LPCWSTR)pFileContents, szRegex, szReplacement, bIgnoreCase);
		ExitOnFailure(hr, "Failed executing regular expression");
	}

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}
	if (pFileContents)
	{
		MemFree(pFileContents);
	}

    return hr;
}

HRESULT CFileRegex::ExecuteMultibyte(LPCWSTR szFilePath, LPCSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	DWORD dwBytesWritten = 0;
	DWORD dwSize = 0;
	DWORD dwFileAttr = FILE_ATTRIBUTE_NORMAL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	LPSTR szRegexMB = nullptr;
	LPSTR szReplacementMB = nullptr;
	regex rx;
	string szContent;
	regex_constants::syntax_option_type syntax = std::regex_constants::syntax_option_type::ECMAScript;

	// Convert szRegex to multi-byte.
	dwSize = ::WideCharToMultiByte(CP_UTF8, 0, szRegex, -1, nullptr, 0, nullptr, nullptr);
	ExitOnNullWithLastError(dwSize, hr, "Failed converting WCHAR string to multi-byte");

	szRegexMB = (LPSTR)MemAlloc(dwSize, FALSE);
	ExitOnNullWithLastError(szRegexMB, hr, "Failed allocating memory");

	dwSize = ::WideCharToMultiByte(CP_UTF8, 0, szRegex, -1, szRegexMB, dwSize, nullptr, nullptr);
	ExitOnNullWithLastError(dwSize, hr, "Failed converting WCHAR string to multi-byte");

	// Convert szReplacement to multi-byte.
	dwSize = ::WideCharToMultiByte(CP_UTF8, 0, szReplacement, -1, nullptr, 0, nullptr, nullptr);
	ExitOnNullWithLastError(dwSize, hr, "Failed converting WCHAR string to multi-byte");

	szReplacementMB = (LPSTR)MemAlloc(dwSize, FALSE);
	ExitOnNullWithLastError(szReplacementMB, hr, "Failed allocating memory");

	dwSize = ::WideCharToMultiByte(CP_UTF8, 0, szReplacement, -1, szReplacementMB, dwSize, nullptr, nullptr);
	ExitOnNullWithLastError(dwSize, hr, "Failed converting WCHAR string to multi-byte");

	if (bIgnoreCase)
	{
		syntax |= std::regex_constants::syntax_option_type::icase;
	}
	rx.assign(szRegexMB, syntax);

	szContent = std::regex_replace(szFileContent, rx, szReplacementMB);
	dwSize = szContent.length();

	dwFileAttr = ::GetFileAttributes(szFilePath);

	hFile = ::CreateFile(szFilePath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, dwFileAttr, nullptr);
	ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed creating file");

	bRes = ::WriteFile(hFile, szContent.c_str(), dwSize, &dwBytesWritten, nullptr);
	ExitOnNullWithLastError(bRes, hr, "Failed writing file");
	ExitOnNull((dwSize == dwBytesWritten), hr, E_FAIL, "Failed writing file. Wrote %i/%i bytes", dwBytesWritten, dwSize);

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}
	if (szRegexMB)
	{
		MemFree(szRegexMB);
	}
	if (szReplacementMB)
	{
		MemFree(szReplacementMB);
	}

	return hr;
}

HRESULT CFileRegex::ExecuteUnicode(LPCWSTR szFilePath, LPCWSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	DWORD dwBytesWritten = 0;
	DWORD dwFileSize = 0;
	DWORD dwFileAttr = FILE_ATTRIBUTE_NORMAL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	wregex rx;
	wstring szContent;
	regex_constants::syntax_option_type syntax = std::regex_constants::syntax_option_type::ECMAScript;

	if (bIgnoreCase)
	{
		syntax |= std::regex_constants::syntax_option_type::icase;
	}
	rx.assign(szRegex, syntax);

	szContent = std::regex_replace(szFileContent, rx, szReplacement);
	dwFileSize = szContent.length();
	dwFileSize *= sizeof(wchar_t);

	dwFileAttr = ::GetFileAttributes(szFilePath);

	hFile = ::CreateFile(szFilePath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, dwFileAttr, nullptr);
	ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed creating file");

	bRes = ::WriteFile(hFile, szContent.c_str(), dwFileSize, &dwBytesWritten, nullptr);
	ExitOnNullWithLastError(bRes, hr, "Failed writing file");
	ExitOnNull((dwFileSize == dwBytesWritten), hr, E_FAIL, "Failed writing file. Wrote %i/%i bytes", dwBytesWritten, dwFileSize);

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}

	return hr;
}

FileRegexDetails::FileEncoding CFileRegex::DetectEncoding(const void* pFileContent, DWORD dwSize)
{
	int nTests = IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK | IS_TEXT_UNICODE_NOT_ASCII_MASK;
	HRESULT hr = S_OK;
	FileRegexDetails::FileEncoding eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None;

	if (dwSize < 2)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File is too short to analyze encoding. Assuming multi-byte");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte;
		ExitFunction();
	}

	::IsTextUnicode(pFileContent, dwSize, &nTests);

	// Multi-byte
	if (nTests & IS_TEXT_UNICODE_ILLEGAL_CHARS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has illegal UNICODE characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_ODD_LENGTH)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has odd length so it cannot be UNICODE");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte;
		ExitFunction();
	}

	// Unicode BOM
	if (nTests & IS_TEXT_UNICODE_SIGNATURE)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has UNICODE BOM");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has reverse UNICODE BOM");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_ReverseUnicode;
		ExitFunction();
	}

	// Unicode
	if (nTests & IS_TEXT_UNICODE_ASCII16)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has only ASCII UNICODE characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_CONTROLS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has UNICODE control characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}

	// Reverse unicode
	if (nTests & IS_TEXT_UNICODE_REVERSE_ASCII16)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has only reverse ASCII UNICODE characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_ReverseUnicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_REVERSE_CONTROLS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has reverse UNICODE control characters");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_ReverseUnicode;
		ExitFunction();
	}

	// Stat tests
	if (nTests & IS_TEXT_UNICODE_STATISTICS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File is probably UNICODE");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_REVERSE_STATISTICS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File is probably reverse UNICODE");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}

	// Unicode NULL (can't tell if it strait or reverse).
	if (nTests & IS_TEXT_UNICODE_NULL_BYTES)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has NULL UNICODE characters. Assuming unicode, though, it may as well be reverse unicode");
		eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode;
		ExitFunction();
	}

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "All tests failed for UNICODE encoding. Assuming multi-byte");
	eEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte;

LExit:
	return eEncoding;
}
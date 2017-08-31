#include "FileRegex.h"
#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include <regex>
#include <memutil.h>
using namespace std;

#define FileRegex_QUERY L"SELECT `Id`, `FilePath`, `Regex`, `Replacement`, `IgnoreCase`, `Encoding`, `Condition` FROM `PSW_FileRegex`"
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
	CComBSTR szCustomActionData;
	DWORD dwRes = 0;
	DWORD dwUnique = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_FileRegex exists.
	hr = WcaTableExists(L"PSW_FileRegex");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_FileRegex'. Have you authored 'PanelSw:FileRegex' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(FileRegex_QUERY, &hView);
	BreakExitOnFailure1(hr, "Failed to execute SQL query '%ls'.", FileRegex_QUERY);
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

		dwRes = ::GetTempFileName(longTempPath, L"RGX", ++dwUnique, (LPWSTR)tempFile);
		BreakExitOnNullWithLastError( dwRes, hr, "Failed getting temporary file name");

		hr = rollbackCAD.AddMoveFile( (LPCWSTR)tempFile, szFilePath);
		BreakExitOnFailure(hr, "Failed creating custom action data for rollback action.");

		// Add deferred data to copy file szFilePath -> tempFile.
		hr = deferredFileCAD.AddCopyFile(szFilePath, (LPCWSTR)tempFile);
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred file action.");

		hr = oDeferredFileRegex.AddFileRegex(szFilePath, szRegex, szReplacement, (CFileRegex::FileEncoding)nEncoding, nIgnoreCase != 0);
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred action.");

		hr = commitCAD.AddDeleteFile( (LPCWSTR)tempFile);
		BreakExitOnFailure(hr, "Failed creating custom action data for commit action.");
	}

	hr = rollbackCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"FileRegex_rollback", szCustomActionData, rollbackCAD.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

	szCustomActionData.Empty();
	hr = oDeferredFileRegex.Prepend(&deferredFileCAD);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred file actions.");
	hr = oDeferredFileRegex.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"FileRegex_deferred", szCustomActionData, oDeferredFileRegex.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

	szCustomActionData.Empty();
	hr = commitCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"FileRegex_commit", szCustomActionData, commitCAD.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CFileRegex::AddFileRegex(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, FileEncoding eEncoding, bool bIgnoreCase)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"FileRegex", L"CFileRegex", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("FilePath"), CComVariant(szFilePath));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'FilePath'");

	hr = pElem->setAttribute(CComBSTR("Regex"), CComVariant(szRegex));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Regex'");

	hr = pElem->setAttribute(CComBSTR("Replacement"), CComVariant(szReplacement));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Replacement'");

	hr = pElem->setAttribute(CComBSTR("Encoding"), CComVariant(eEncoding));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Encoding'");

	hr = pElem->setAttribute(CComBSTR("IgnoreCase"), CComVariant(bIgnoreCase ? 1 : 0));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'IgnoreCase'");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CFileRegex::DeferredExecute(IXMLDOMElement* pElem)
{
	HRESULT hr = S_OK;
	CComVariant vFilePath;
	CComVariant vRegex;
	CComVariant vReplacement;
	CComVariant vIgnoreCase;
	CComVariant vEncoding;
	int nIgnoreCase;
	int nEncoding;

	// Get Parameters:
	hr = pElem->getAttribute(CComBSTR(L"FilePath"), &vFilePath);
	BreakExitOnFailure(hr, "Failed to get FilePath");

	hr = pElem->getAttribute(CComBSTR(L"Regex"), &vRegex);
	BreakExitOnFailure(hr, "Failed to get Regex");

	hr = pElem->getAttribute(CComBSTR(L"Replacement"), &vReplacement);
	BreakExitOnFailure(hr, "Failed to get Replacement");

	hr = pElem->getAttribute(CComBSTR(L"IgnoreCase"), &vIgnoreCase);
	BreakExitOnFailure(hr, "Failed to get IgnoreCase");
	nIgnoreCase = ::_wtoi(vIgnoreCase.bstrVal);

	hr = pElem->getAttribute(CComBSTR(L"Encoding"), &vEncoding);
	BreakExitOnFailure(hr, "Failed to get Encoding");
	nEncoding = ::_wtoi(vEncoding.bstrVal);

	hr = Execute(vFilePath.bstrVal, vRegex.bstrVal, vReplacement.bstrVal, (FileEncoding)nEncoding, (nIgnoreCase != 0));
	BreakExitOnFailure(hr, "Failed to execute file regular expression");

LExit:
	return hr;
}

HRESULT CFileRegex::Execute(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, FileEncoding eEncoding, bool bIgnoreCase)
{
    HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	DWORD dwBytesRead = 0;
	DWORD dwFileSize = 0;
	DWORD dwFileAttr = FILE_ATTRIBUTE_NORMAL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	void* pFileContents = NULL;
	FileEncoding eDetectedEncoding = FileEncoding::None;

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Replacing matches of regex '%ls' with '%ls' on file '%ls'", szRegex, szReplacement, szFilePath);

	hFile = ::CreateFile(szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed opening file");

	dwFileSize = ::GetFileSize(hFile, NULL);
	pFileContents = MemAlloc(dwFileSize + 2, FALSE);
	ExitOnNull(pFileContents, hr, E_FAIL, "Failed allocating memory");
	
	// Terminate with ascii/wchar NULL.
	((BYTE*)pFileContents)[dwFileSize] = NULL;
	((BYTE*)pFileContents)[dwFileSize + 1] = NULL;

	bRes = ::ReadFile(hFile, pFileContents, dwFileSize, &dwBytesRead, NULL);
	ExitOnNullWithLastError(bRes, hr, "Failed reading file");
	ExitOnNull((dwFileSize == dwBytesRead), hr, E_FAIL, "Failed reading file. Read %i/%i bytes", dwBytesRead, dwFileSize);

	::CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	eDetectedEncoding = DetectEncoding(pFileContents, dwFileSize);
	if (eEncoding != FileEncoding::None)
	{
		if (eDetectedEncoding != eEncoding)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Overriding detected encoding with author-specified encoding");
			eDetectedEncoding = eEncoding;
		}
	}

	if (eDetectedEncoding == FileEncoding::MultiByte)
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
	LPSTR szRegexMB = NULL;
	LPSTR szReplacementMB = NULL;
	regex rx;
	string szContent;
	regex_constants::syntax_option_type syntax = std::regex_constants::syntax_option_type::ECMAScript;

	// Convert szRegex to multi-byte.
	dwSize = ::WideCharToMultiByte(CP_UTF8, 0, szRegex, -1, NULL, 0, NULL, NULL);
	ExitOnNullWithLastError(dwSize, hr, "Failed converting WCHAR string to multi-byte");

	szRegexMB = (LPSTR)MemAlloc(dwSize, FALSE);
	ExitOnNullWithLastError(szRegexMB, hr, "Failed allocating memory");

	dwSize = ::WideCharToMultiByte(CP_UTF8, 0, szRegex, -1, szRegexMB, dwSize, NULL, NULL);
	ExitOnNullWithLastError(dwSize, hr, "Failed converting WCHAR string to multi-byte");

	// Convert szReplacement to multi-byte.
	dwSize = ::WideCharToMultiByte(CP_UTF8, 0, szReplacement, -1, NULL, 0, NULL, NULL);
	ExitOnNullWithLastError(dwSize, hr, "Failed converting WCHAR string to multi-byte");

	szReplacementMB = (LPSTR)MemAlloc(dwSize, FALSE);
	ExitOnNullWithLastError(szReplacementMB, hr, "Failed allocating memory");

	dwSize = ::WideCharToMultiByte(CP_UTF8, 0, szReplacement, -1, szReplacementMB, dwSize, NULL, NULL);
	ExitOnNullWithLastError(dwSize, hr, "Failed converting WCHAR string to multi-byte");

	if (bIgnoreCase)
	{
		syntax |= std::regex_constants::syntax_option_type::icase;
	}
	rx.assign(szRegexMB, syntax);

	szContent = std::regex_replace(szFileContent, rx, szReplacementMB);
	dwSize = szContent.length();

	dwFileAttr = ::GetFileAttributes(szFilePath);

	hFile = ::CreateFile(szFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, dwFileAttr, NULL);
	ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed creating file");

	bRes = ::WriteFile(hFile, szContent.c_str(), dwSize, &dwBytesWritten, NULL);
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

	hFile = ::CreateFile(szFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, dwFileAttr, NULL);
	ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed creating file");

	bRes = ::WriteFile(hFile, szContent.c_str(), dwFileSize, &dwBytesWritten, NULL);
	ExitOnNullWithLastError(bRes, hr, "Failed writing file");
	ExitOnNull((dwFileSize == dwBytesWritten), hr, E_FAIL, "Failed writing file. Wrote %i/%i bytes", dwBytesWritten, dwFileSize);

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}

	return hr;
}

CFileRegex::FileEncoding CFileRegex::DetectEncoding(const void* pFileContent, DWORD dwSize)
{
	int nTests = IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK | IS_TEXT_UNICODE_NOT_ASCII_MASK;
	HRESULT hr = S_OK;
	FileEncoding eEncoding = FileEncoding::None;

	if (dwSize < 2)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File is too short to analyze encoding. Assuming multi-byte");
		eEncoding = FileEncoding::MultiByte;
		ExitFunction();
	}

	::IsTextUnicode(pFileContent, dwSize, &nTests);

	// Multi-byte
	if (nTests & IS_TEXT_UNICODE_ILLEGAL_CHARS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has ilegal UNICODE characters");
		eEncoding = FileEncoding::MultiByte;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_ODD_LENGTH)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has odd length so it cannot be UNICODE");
		eEncoding = FileEncoding::MultiByte;
		ExitFunction();
	}

	// Unicode
	if (nTests & IS_TEXT_UNICODE_SIGNATURE)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has UNICODE BOM");
		eEncoding = FileEncoding::Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_ASCII16)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has only ASCII UNICODE characters");
		eEncoding = FileEncoding::Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_CONTROLS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has UNICODE control characters");
		eEncoding = FileEncoding::Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_NULL_BYTES)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has NULL UNICODE characters");
		eEncoding = FileEncoding::Unicode;
		ExitFunction();
	}

	// Reverse unicode
	if (nTests & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has reverse UNICODE BOM");
		eEncoding = FileEncoding::ReverseUnicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_REVERSE_ASCII16)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has only reverse ASCII UNICODE characters");
		eEncoding = FileEncoding::ReverseUnicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_REVERSE_CONTROLS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File has reverse UNICODE control characters");
		eEncoding = FileEncoding::ReverseUnicode;
		ExitFunction();
	}

	// Stat tests
	if (nTests & IS_TEXT_UNICODE_STATISTICS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File is probably UNICODE");
		eEncoding = FileEncoding::Unicode;
		ExitFunction();
	}
	if (nTests & IS_TEXT_UNICODE_REVERSE_STATISTICS)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File is probably reverse UNICODE");
		eEncoding = FileEncoding::Unicode;
		ExitFunction();
	}

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "All tests failed for UNICODE encoding. Assuming multibyte");
	eEncoding = FileEncoding::MultiByte;

LExit:
	return eEncoding;
}
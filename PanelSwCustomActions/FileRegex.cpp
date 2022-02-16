#include "FileRegex.h"
#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include <regex>
#include <dictutil.h>
#include <memutil.h>
#include <pathutil.h>
using namespace std;
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

static HRESULT IsInstall(LPCWSTR szComponent, LPCWSTR szCondition);

extern "C" UINT __stdcall FileRegex(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	STRINGDICT_HANDLE hPrevFiles = nullptr;
	CFileRegex oDeferredFileRegex;
	CFileOperations rollbackCAD;
	CFileOperations deferredFileCAD;
	CFileOperations commitCAD;
	LPWSTR szCustomActionData = nullptr;
	DWORD dwRes = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_FileRegex exists.
	hr = WcaTableExists(L"PSW_FileRegex");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_FileRegex'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_FileRegex'. Have you authored 'PanelSw:FileRegex' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Component_`, `File_`, `FilePath`, `Regex`, `Replacement`, `IgnoreCase`, `Encoding`, `Condition` FROM `PSW_FileRegex` ORDER BY `Order`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query.");

	hr = DictCreateStringList(&hPrevFiles, 100, DICT_FLAG::DICT_FLAG_CASEINSENSITIVE);
	ExitOnFailure(hr, "Failed to create string-dictionary.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szComponent, szFileId, szFileFormat, szFilePath, szRegexUnformatted, szReplacementUnformatted, szCondition;
		CWixString szRegex, szRegexObfuscated, szReplacement, szReplacementObfuscated;
		int nIgnoreCase = 0;
		int nEncoding = 0;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szFileId);
		ExitOnFailure(hr, "Failed to get File_.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)szFileFormat);
		ExitOnFailure(hr, "Failed to get FilePath.");
		hr = WcaGetRecordString(hRecord, 4, (LPWSTR*)szRegexUnformatted);
		ExitOnFailure(hr, "Failed to get Regex.");
		hr = WcaGetRecordString(hRecord, 5, (LPWSTR*)szReplacementUnformatted);
		ExitOnFailure(hr, "Failed to get Replacement.");
		hr = WcaGetRecordInteger(hRecord, 6, &nIgnoreCase);
		ExitOnFailure(hr, "Failed to get IgnoreCase.");
		hr = WcaGetRecordInteger(hRecord, 7, &nEncoding);
		ExitOnFailure(hr, "Failed to get Encoding.");
		hr = WcaGetRecordString(hRecord, 8, (LPWSTR*)szCondition);
		ExitOnFailure(hr, "Failed to get Condition.");

		// Test condition(s)
		hr = IsInstall(szComponent, szCondition);
		ExitOnFailure(hr, "Failed to evaluate conditions.");
		if (hr == S_FALSE)
		{
			continue;
		}

		// Sanity
		ExitOnNull((szFileId.IsNullOrEmpty() != szFileFormat.IsNullOrEmpty()), hr, E_INVALIDARG, "Both FilePath and File_ have values or both empty");

		// Parse file path
		if (!szFileId.IsNullOrEmpty())
		{
			hr = szFileFormat.Format(L"[#%s]", (LPCWSTR)szFileId);
			ExitOnFailure(hr, "Failed formatting string");
		}

		hr = szFilePath.MsiFormat(szFileFormat);
		ExitOnFailure(hr, "Failed MSI-formatting string");

		if (szFilePath.IsNullOrEmpty())
		{
			CFileRegex::LogUnformatted(LOGMSG_STANDARD, "Will skip regex for file '%ls'.", (LPCWSTR)szFileFormat);
			continue;
		}

		hr = szRegex.MsiFormat((LPCWSTR)szRegexUnformatted, (LPWSTR*)szRegexObfuscated);
		ExitOnFailure(hr, "Failed formatting string");

		hr = szReplacement.MsiFormat((LPCWSTR)szReplacementUnformatted, (LPWSTR*)szReplacementObfuscated);
		ExitOnFailure(hr, "Failed formatting string");

		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, "Will replace matches of regex '%ls' with '%ls' on file '%ls'", (LPCWSTR)szRegexObfuscated, (LPCWSTR)szReplacementObfuscated, (LPCWSTR)szFilePath);

		hr = oDeferredFileRegex.AddFileRegex(szFilePath, szRegex, szReplacement, szRegexObfuscated, szReplacementObfuscated, (FileRegexDetails::FileEncoding)nEncoding, nIgnoreCase != 0);
		ExitOnFailure(hr, "Failed creating custom action data for deferred action.");

		// Once per file: Backup it up before applying replacements; Restore on failure; Delete on commit.
		hr = DictKeyExists(hPrevFiles, szFilePath);
		if (hr != E_NOTFOUND)
		{
			ExitOnFailure(hr, "Failed searching file in index");
		}
		else
		{
			CWixString tempFile;

			hr = PathCreateTempFile(nullptr, L"RGX%05i.tmp", INFINITE, FILE_ATTRIBUTE_NORMAL, (LPWSTR*)tempFile, nullptr);
			ExitOnFailure(hr, "Failed getting temporary file name");

			hr = rollbackCAD.AddMoveFile((LPCWSTR)tempFile, szFilePath);
			ExitOnFailure(hr, "Failed creating custom action data for rollback action.");

			hr = deferredFileCAD.AddCopyFile(szFilePath, (LPCWSTR)tempFile);
			ExitOnFailure(hr, "Failed creating custom action data for deferred file action.");

			hr = commitCAD.AddDeleteFile((LPCWSTR)tempFile);
			ExitOnFailure(hr, "Failed creating custom action data for commit action.");

			hr = DictAddKey(hPrevFiles, szFilePath);
			ExitOnFailure(hr, "Failed indexing file path");
		}
	}

	hr = rollbackCAD.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"FileRegex_rollback", szCustomActionData, rollbackCAD.GetCost());
	ExitOnFailure(hr, "Failed scheduling deferred action.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredFileRegex.Prepend(&deferredFileCAD);
	ExitOnFailure(hr, "Failed getting custom action data for deferred file actions.");
	hr = oDeferredFileRegex.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"FileRegex_deferred", szCustomActionData, oDeferredFileRegex.GetCost());
	ExitOnFailure(hr, "Failed scheduling deferred action.");

	ReleaseNullStr(szCustomActionData);
	hr = commitCAD.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"FileRegex_commit", szCustomActionData, commitCAD.GetCost());
	ExitOnFailure(hr, "Failed scheduling deferred action.");

LExit:
	ReleaseStr(szCustomActionData);
	DictDestroy(hPrevFiles);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT IsInstall(LPCWSTR szComponent, LPCWSTR szCondition)
{
	HRESULT hr = S_OK;

	if (szComponent && *szComponent)
	{
		WCA_TODO compToDo = WCA_TODO::WCA_TODO_UNKNOWN;

		compToDo = WcaGetComponentToDo(szComponent);
		switch (compToDo)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
		case WCA_TODO::WCA_TODO_REINSTALL:
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
		case WCA_TODO::WCA_TODO_UNKNOWN:
		default:
			WcaLog(LOGMSG_STANDARD, "Will skip regex for component '%ls'.", (LPCWSTR)szComponent);
			hr = S_FALSE;
			ExitFunction();
		}
	}

	if (szCondition && *szCondition)
	{
		MSICONDITION condRes = MSICONDITION::MSICONDITION_NONE;

		condRes = ::MsiEvaluateCondition(WcaGetInstallHandle(), szCondition);
		ExitOnNullWithLastError((condRes != MSICONDITION::MSICONDITION_ERROR), hr, "Failed evaluating condition '%ls'", szCondition);

		hr = (condRes == MSICONDITION::MSICONDITION_FALSE) ? S_FALSE : S_OK;
		WcaLog(LOGMSG_STANDARD, "Condition evaluated to %i", (1 - (int)hr));
	}

LExit:
	return hr;
}

HRESULT CFileRegex::AddFileRegex(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, LPCWSTR szRegexObfuscated, LPCWSTR szReplacementObfuscated, FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	FileRegexDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CFileRegex", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new FileRegexDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_file(szFilePath, WSTR_BYTE_SIZE(szFilePath));
	pDetails->set_expression(szRegex, WSTR_BYTE_SIZE(szRegex));
	pDetails->set_expressionobfuscated(szRegexObfuscated, WSTR_BYTE_SIZE(szRegexObfuscated));
	pDetails->set_replacement(szReplacement, WSTR_BYTE_SIZE(szReplacement));
	pDetails->set_replacementobfuscated(szReplacementObfuscated, WSTR_BYTE_SIZE(szReplacementObfuscated));
	pDetails->set_encoding(eEncoding);
	pDetails->set_ignorecase(bIgnoreCase);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CFileRegex::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	FileRegexDetails details;
	LPCWSTR szFile = nullptr;
	LPCWSTR szExpression = nullptr;
	LPCWSTR szReplacement = nullptr;
	LPCWSTR szRegexObfuscated = nullptr;
	LPCWSTR szReplacementObfuscated = nullptr;

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking FileOperationsDetails");

	szFile = (LPCWSTR)(LPVOID)details.file().data();
	szExpression = (LPCWSTR)(LPVOID)details.expression().data();
	szReplacement = (LPCWSTR)(LPVOID)details.replacement().data();
	szRegexObfuscated = (LPCWSTR)(LPVOID)details.expressionobfuscated().data();
	szReplacementObfuscated = (LPCWSTR)(LPVOID)details.replacementobfuscated().data();

	hr = Execute(szFile, szExpression, szReplacement, szRegexObfuscated, szReplacementObfuscated, details.encoding(), details.ignorecase());
	ExitOnFailure(hr, "Failed to execute file regular expression");

LExit:
	return hr;
}

HRESULT CFileRegex::Execute(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, LPCWSTR szRegexObfuscated, LPCWSTR szReplacementObfuscated, FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	DWORD dwBytesRead = 0;
	DWORD dwFileSize = 0;
	DWORD dwFileAttr = FILE_ATTRIBUTE_NORMAL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	void* pFileContents = nullptr;
	FileRegexDetails::FileEncoding eDetectedEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None;
	PMSIHANDLE hActionData;

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Replacing regex '%ls' matches with '%ls' in file '%ls'", szRegexObfuscated, szReplacementObfuscated, szFilePath);

	// ActionData: "Replacing '[1]' with '[2]' in [3]"
	hActionData = ::MsiCreateRecord(3);
	if (hActionData 
		&& SUCCEEDED(WcaSetRecordString(hActionData, 1, szRegexObfuscated))
		&& SUCCEEDED(WcaSetRecordString(hActionData, 2, szReplacementObfuscated))
		&& SUCCEEDED(WcaSetRecordString(hActionData, 3, szFilePath))
		)
	{
		WcaProcessMessage(INSTALLMESSAGE::INSTALLMESSAGE_ACTIONDATA, hActionData);
	}

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

	eDetectedEncoding = CFileOperations::DetectEncoding(pFileContents, dwFileSize);
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

	try
	{
		regex rx(szRegexMB, syntax);
		szContent = std::regex_replace(szFileContent, rx, szReplacementMB);
	}
	catch (std::regex_error ex)
	{
		hr = HRESULT_FROM_WIN32(ex.code());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
		ExitOnFailure(hr, "Failed evaluating regular expression. %s", ex.what());
	}

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
		::FlushFileBuffers(hFile);
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
	wstring szContent;
	regex_constants::syntax_option_type syntax = std::regex_constants::syntax_option_type::ECMAScript;

	if (bIgnoreCase)
	{
		syntax |= std::regex_constants::syntax_option_type::icase;
	}
	try
	{
		wregex rx(szRegex, syntax);
		szContent = std::regex_replace(szFileContent, rx, szReplacement);
	}
	catch (std::regex_error ex)
	{
		hr = HRESULT_FROM_WIN32(ex.code());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
		ExitOnFailure(hr, "Failed evaluating regular expression. %s", ex.what());
	}

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
		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
	}

	return hr;
}

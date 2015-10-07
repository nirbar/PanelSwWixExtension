#include "FileRegex.h"
#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include <regex>
#include <sstream>
#include <fstream>
#include <codecvt>
using namespace std;

#define FileRegex_QUERY L"SELECT `Id`, `FilePath`, `Regex`, `Replacement`, `IgnoreCase`, `Condition` FROM `PSW_FileRegex`"
enum FileRegexQuery { Id = 1, FilePath = 2, Regex = 3, Replacement = 4, IgnoreCase = 5, Condition = 6 };

enum FileRegexFlags
{
	None = 0,
	OnExecute = 1,
	OnCommit = 2,
	OnRollback = 4
};

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
	CWixString tempPath;
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

	// Get temporay folder
	dwRes = ::GetTempPath(dwRes, (LPWSTR)tempPath);
	BreakExitOnNullWithLastError( dwRes, hr, "Failed getting temporary folder");

	hr = tempPath.Allocate(dwRes + 1);
	BreakExitOnFailure(hr, "Failed allocating memory");

	dwRes = ::GetTempPath(dwRes + 1, (LPWSTR)tempPath);
	BreakExitOnNullWithLastError(dwRes, hr, "Failed getting temporary folder");
	BreakExitOnNull( (dwRes < tempPath.Capacity()), hr, E_FAIL, "Failed getting temporary folder");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szFilePath, szRegex, szReplacement, szCondition;
		CWixString tempFile;
		int nIgnoreCase = 0;

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

		dwRes = ::GetTempFileName((LPCWSTR)tempPath, L"RGX", ++dwUnique, (LPWSTR)tempFile);
		BreakExitOnNullWithLastError( dwRes, hr, "Failed getting temporary file name");

		hr = rollbackCAD.AddMoveFile( (LPCWSTR)tempFile, szFilePath);
		BreakExitOnFailure(hr, "Failed creating custom action data for rollback action.");

		// Add deferred data to copy file szFilePath -> tempFile.
		hr = deferredFileCAD.AddCopyFile(szFilePath, (LPCWSTR)tempFile);
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred file action.");

		hr = oDeferredFileRegex.AddFileRegex(szFilePath, szRegex, szReplacement, nIgnoreCase != 0);
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

HRESULT CFileRegex::AddFileRegex(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase)
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
	int nIgnoreCase;

	// Get Parameters:
	hr = pElem->getAttribute(CComBSTR(L"FilePath"), &vFilePath);
	BreakExitOnFailure(hr, "Failed to get FilePath");

	// Get Args
	hr = pElem->getAttribute(CComBSTR(L"Regex"), &vRegex);
	BreakExitOnFailure(hr, "Failed to get Regex");

	// Get Args
	hr = pElem->getAttribute(CComBSTR(L"Replacement"), &vReplacement);
	BreakExitOnFailure(hr, "Failed to get Replacement");

	// Get Args
	hr = pElem->getAttribute(CComBSTR(L"IgnoreCase"), &vIgnoreCase);
	BreakExitOnFailure(hr, "Failed to get IgnoreCase");
	
	nIgnoreCase = ::_wtoi(vIgnoreCase.bstrVal);
	
	hr = Execute(vFilePath.bstrVal, vRegex.bstrVal, vReplacement.bstrVal, (nIgnoreCase != 0));
	BreakExitOnFailure(hr, "Failed to execute file regular expression");

LExit:
	return hr;
}

HRESULT CFileRegex::Execute(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase)
{
    HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	DWORD dwBytesRead = 0;
	wstring content;
	wifstream fileRead;
	wofstream fileWrite;
	wstringstream fileStream;
	wregex rx;
	regex_constants::syntax_option_type syntax = std::regex_constants::syntax_option_type::ECMAScript;

	WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Replacing matches of regex '%ls' with '%ls' on file '%ls'", szRegex, szReplacement, szFilePath);

	fileRead.open(szFilePath);
	BreakExitOnNull(fileRead.good(), hr, E_FAIL, "Failed openning file for reading");

	fileRead.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
	fileStream << fileRead.rdbuf();
	content = fileStream.str();

	if (bIgnoreCase)
	{
		syntax |= std::regex_constants::syntax_option_type::icase;
	}
	rx.assign(szRegex, syntax);

	content = std::regex_replace(content.c_str(), rx, szReplacement);

	// Truncate file & write
	fileWrite.open(szFilePath, ios_base::trunc);
	BreakExitOnNull(fileWrite.good(), hr, E_FAIL, "Failed openning file for writing");

	fileWrite.write(content.c_str(), content.length());
	BreakExitOnNull(fileWrite.good(), hr, E_FAIL, "Failed writing to file");

LExit:

    return hr;
}
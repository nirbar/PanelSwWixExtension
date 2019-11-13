#include "stdafx.h"
#include "../CaCommon/WixString.h"
#include <regex>
#include <fileutil.h>
#include <memutil.h>
#include "FileOperations.h"
#include "fileRegexDetails.pb.h"

using namespace std;
using namespace ::com::panelsw::ca;
#define RegularExpressionQuery L"SELECT `Id`, `FilePath`, `Input`, `Expression`, `Replacement`, `DstProperty_`, `Flags`, `Condition` FROM `PSW_RegularExpression` ORDER BY `Order`"
enum eRegularExpressionQuery { Id = 1, FilePath, Input, Expression, Replacement, DstProperty, Flags, Condition };
enum SearchFlags
{
	Search = 0
	, Replace = 1
};

enum ResultFlags
{
	MustMatch = 1
};

enum MatchFlags
{
	IgnoreCare = 1
	, Extended = 2
};

union RegexFlags
{
	unsigned int u;
	struct
	{
		SearchFlags search : 1;
		ResultFlags result : 1;
		MatchFlags match : 2;
		unsigned dontCare : 4;
	} s;
};

static HRESULT SearchUnicode(LPCWSTR szProperty_, LPCWSTR szExpression, LPCWSTR szInput, RegexFlags flags, std::regex_constants::syntax_option_type syntax);
static HRESULT SearchMultibyte(LPCWSTR szProperty_, LPCWSTR szExpression, LPCSTR szInput, RegexFlags flags, std::regex_constants::syntax_option_type syntax);
static HRESULT SearchInFile(LPCWSTR szProperty_, LPCWSTR szExpression, LPCWSTR szFilePath, RegexFlags flags, std::regex_constants::syntax_option_type syntax);

extern "C" UINT __stdcall RegularExpression(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_RegularExpression");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_RegularExpression'. Have you authored 'PanelSw:RegularExpression' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(RegularExpressionQuery, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query on 'PSW_RegularExpression'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szExpressionFormat, szExpression, szObfuscatedExpression;
		CWixString sId, sFilePath, sInput, sReplace, sDstProperty, sCondition;
		int iFlags = 0;
		RegexFlags flags;
		std::regex_constants::syntax_option_type syntax = (std::regex_constants::syntax_option_type)0;

		hr = WcaGetRecordString(hRecord, eRegularExpressionQuery::Id, (LPWSTR*)sId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::FilePath, (LPWSTR*)sFilePath);
		BreakExitOnFailure(hr, "Failed to get FilePath.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::Input, (LPWSTR*)sInput);
		BreakExitOnFailure(hr, "Failed to get Input.");
		hr = WcaGetRecordString(hRecord, eRegularExpressionQuery::Expression, (LPWSTR*)szExpressionFormat);
		BreakExitOnFailure(hr, "Failed to get Expression.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::Replacement, (LPWSTR*)sReplace);
		BreakExitOnFailure(hr, "Failed to get Replacement.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::DstProperty, (LPWSTR*)sDstProperty);
		BreakExitOnFailure(hr, "Failed to get DstProperty_.");
		hr = WcaGetRecordInteger(hRecord, eRegularExpressionQuery::Flags, &iFlags);
		BreakExitOnFailure(hr, "Failed to get Flags.");
		flags.u = *reinterpret_cast<unsigned int*>(&iFlags);
		hr = WcaGetRecordString(hRecord, eRegularExpressionQuery::Condition, (LPWSTR*)sCondition);
		BreakExitOnFailure(hr, "Failed to get Condition.");

		hr = szExpression.MsiFormat((LPCWSTR)szExpressionFormat, (LPWSTR*)szObfuscatedExpression);
		BreakExitOnFailure(hr, "Failed to format Expression.");

		// Test condition
		if (!sCondition.IsNullOrEmpty())
		{
			MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, (LPCWSTR)sCondition);
			switch (condRes)
			{
			case MSICONDITION::MSICONDITION_NONE:
			case MSICONDITION::MSICONDITION_TRUE:

				WcaLog(LOGMSG_STANDARD, "Condition evaluated to true / none for %ls", (LPCWSTR)sId);
				break;

			case MSICONDITION::MSICONDITION_FALSE:

				WcaLog(LOGMSG_STANDARD, "Skipping. Condition evaluated to false for %ls", (LPCWSTR)sId);
				continue;

			case MSICONDITION::MSICONDITION_ERROR:

				hr = E_FAIL;
				BreakExitOnFailure(hr, "Bad Condition field");
			}
		}
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, "Executing regular expression query '%ls'", (LPCWSTR)szObfuscatedExpression);

		// Syntax flags
		if (flags.s.match & MatchFlags::IgnoreCare)
		{
			syntax |= std::regex_constants::syntax_option_type::icase;
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Ignoring case", (LPCWSTR)sId);
		}
		if (flags.s.match & MatchFlags::Extended)
		{
			syntax |= std::regex_constants::syntax_option_type::extended;
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Extended regex syntax", (LPCWSTR)sId);
		}

		// Match in file
		if (!sFilePath.IsNullOrEmpty())
		{
			hr = SearchInFile(sDstProperty, szExpression, sFilePath, flags, syntax);
			BreakExitOnFailure(hr, "Failed executing search");
		}
		// Match in property
		else if (flags.s.search == SearchFlags::Search)
		{
			hr = SearchUnicode(sDstProperty, szExpression, sInput, flags, syntax);
			BreakExitOnFailure(hr, "Failed executing search");
		}
		// Replace
		else
		{
			wregex rx((LPCWSTR)szExpression, syntax);

			std::wstring rep = regex_replace((LPCWSTR)sInput, rx, (LPCWSTR)sReplace);
			
			hr = WcaSetProperty(sDstProperty, rep.c_str());
			BreakExitOnFailure(hr, "Failed setting target property");
		}
	}

	hr = ERROR_SUCCESS;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT SearchUnicode(LPCWSTR szProperty_, LPCWSTR szExpression, LPCWSTR szInput, RegexFlags flags, std::regex_constants::syntax_option_type syntax)
{
	HRESULT hr = S_OK;
	bool bRes = true;
	wregex rx(szExpression, syntax);
	match_results<LPCWSTR> results;
	CWixString sPropName;
	match_results<LPCWSTR>::const_iterator curIt, endIt;

	bRes = regex_search(szInput, results, rx);
	BreakExitOnNull((bRes || ((flags.s.result & ResultFlags::MustMatch) == 0)), hr, E_FAIL, "Regex returned no matches");
	if (!bRes)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No matches");
		ExitFunction1(hr = S_FALSE);
	}

	hr = sPropName.Format(L"%s_COUNT", szProperty_);
	BreakExitOnFailure(hr, "Failed formatting string");

	hr = WcaSetIntProperty((LPCWSTR)sPropName, results.size());
	BreakExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)sPropName);

	// Iterate results
	curIt = results.begin();
	endIt = results.end();
	for (size_t i = 0; curIt != endIt; ++i, ++curIt)
	{
		hr = sPropName.Format(L"%s_%Iu", szProperty_, i);
		BreakExitOnFailure(hr, "Failed formatting string");

		hr = WcaSetProperty((LPCWSTR)sPropName, curIt->str().c_str());
		BreakExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)sPropName);
	}

LExit:
	return hr;
}

static HRESULT SearchMultibyte(LPCWSTR szProperty_, LPCWSTR szExpression, LPCSTR szInput, RegexFlags flags, std::regex_constants::syntax_option_type syntax)
{
	HRESULT hr = S_OK;
	LPSTR szPropertyA = nullptr;
	LPSTR szExpressionA = nullptr;

	hr = StrAnsiAllocString(&szExpressionA, szExpression, 0, CP_UTF8);
	BreakExitOnFailure(hr, "Failed converting string to multibyte");

	// New scope to construct regex in
	{
		bool bRes = true;
		regex rx(szExpressionA, syntax);
		match_results<LPCSTR> results;
		CWixString szCntProp;

		bRes = regex_search(szInput, results, rx);
		BreakExitOnNull((bRes || ((flags.s.result & ResultFlags::MustMatch) == 0)), hr, E_FAIL, "Regex returned no matches");
		if (!bRes)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No matches");
			ExitFunction1(hr = S_FALSE);
		}

		hr = szCntProp.Format(L"%s_COUNT", szProperty_);
		BreakExitOnFailure(hr, "Failed formatting string");

		hr = ::WcaSetIntProperty(szCntProp, results.size());
		BreakExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)szCntProp);

		// Iterate results
		match_results<LPCSTR>::const_iterator curIt = results.begin();
		match_results<LPCSTR>::const_iterator endIt = results.end();
		for (size_t i = 0; curIt != endIt; ++i, ++curIt)
		{
			hr = StrAnsiAllocFormatted(&szPropertyA, "%ls_%Iu", szProperty_, i);
			BreakExitOnFailure(hr, "Failed formatting ansi string");

			hr = ::MsiSetPropertyA(WcaGetInstallHandle(), szPropertyA, curIt->str().c_str());
			BreakExitOnFailure(hr, "Failed setting property '%s'", szPropertyA);

			ReleaseNullMem(szPropertyA);
		}
	}

LExit:
	ReleaseMem(szExpressionA);
	ReleaseMem(szPropertyA);
	return hr;
}


static HRESULT SearchInFile(LPCWSTR szProperty_, LPCWSTR szExpression, LPCWSTR szFilePath, RegexFlags flags, std::regex_constants::syntax_option_type syntax)
{
	HRESULT hr = S_OK;
	bool bRes = true;
	FileRegexDetails::FileEncoding eDetectedEncoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	void* pFileContents = nullptr;
	DWORD dwFileSize = 0;
	DWORD dwBytesRead = 0;

	if (flags.s.search == SearchFlags::Replace)
	{
		hr = E_INVALIDARG;
		ExitOnFailure(hr, "Can't perform Regex replace in immediate custom action. Use FileRegex for that");
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

	eDetectedEncoding = CFileOperations::DetectEncoding(pFileContents, dwFileSize);
	if (eDetectedEncoding == FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte)
	{
		hr = SearchMultibyte(szProperty_, szExpression, (LPCSTR)pFileContents, flags, syntax);
		ExitOnFailure(hr, "Failed executing multibyte regular expression");
	}
	else
	{
		hr = SearchUnicode(szProperty_, szExpression, (LPCWSTR)pFileContents, flags, syntax);
		ExitOnFailure(hr, "Failed executing unicode regular expression");
	}

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}
	ReleaseMem(pFileContents);

	return hr;
}

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

static HRESULT SearchUnicode(LPCWSTR szProperty, LPCWSTR szExpression, LPCWSTR szInput, RegexFlags flags, std::regex_constants::syntax_option_type syntax);
static HRESULT SearchMultibyte(LPCWSTR szProperty, LPCWSTR szExpression, LPCSTR szInput, RegexFlags flags, std::regex_constants::syntax_option_type syntax);
static HRESULT SearchInFile(LPCWSTR szProperty, LPCWSTR szExpression, LPCWSTR szFilePath, RegexFlags flags, std::regex_constants::syntax_option_type syntax);

extern "C" UINT __stdcall RegularExpression(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_RegularExpression");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_RegularExpression'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_RegularExpression'. Have you authored 'PanelSw:RegularExpression' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(RegularExpressionQuery, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_RegularExpression'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szExpressionFormat, szExpression;
		CWixString sId, sFilePath, sInput, sReplace, sDstProperty, sCondition;
		int iFlags = 0;
		RegexFlags flags;
		std::regex_constants::syntax_option_type syntax = (std::regex_constants::syntax_option_type)0;

		hr = WcaGetRecordString(hRecord, eRegularExpressionQuery::Id, (LPWSTR*)sId);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::FilePath, (LPWSTR*)sFilePath);
		ExitOnFailure(hr, "Failed to get FilePath.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::Input, (LPWSTR*)sInput);
		ExitOnFailure(hr, "Failed to get Input.");
		hr = WcaGetRecordString(hRecord, eRegularExpressionQuery::Expression, (LPWSTR*)szExpressionFormat);
		ExitOnFailure(hr, "Failed to get Expression.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::Replacement, (LPWSTR*)sReplace);
		ExitOnFailure(hr, "Failed to get Replacement.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::DstProperty, (LPWSTR*)sDstProperty);
		ExitOnFailure(hr, "Failed to get DstProperty_.");
		hr = WcaGetRecordInteger(hRecord, eRegularExpressionQuery::Flags, &iFlags);
		ExitOnFailure(hr, "Failed to get Flags.");
		flags.u = *reinterpret_cast<unsigned int*>(&iFlags);
		hr = WcaGetRecordString(hRecord, eRegularExpressionQuery::Condition, (LPWSTR*)sCondition);
		ExitOnFailure(hr, "Failed to get Condition.");

		hr = szExpression.MsiFormat((LPCWSTR)szExpressionFormat);
		ExitOnFailure(hr, "Failed to format Expression.");

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
				ExitOnFailure(hr, "Bad Condition field");
			}
		}
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Executing regular expression query '%ls'", szExpression.Obfuscated());

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
			ExitOnFailure(hr, "Failed executing search");
		}
		// Match in property
		else if (flags.s.search == SearchFlags::Search)
		{
			hr = SearchUnicode(sDstProperty, szExpression, sInput, flags, syntax);
			ExitOnFailure(hr, "Failed executing search");
		}
		// Replace
		else
		{
			try
			{
				wregex rx((LPCWSTR)szExpression, syntax);
				std::wstring rep = regex_replace((LPCWSTR)sInput, rx, (LPCWSTR)sReplace);

				hr = WcaSetProperty(sDstProperty, rep.c_str());
				ExitOnFailure(hr, "Failed setting target property");
			}
			catch (std::regex_error ex)
			{
				hr = HRESULT_FROM_WIN32(ex.code());
				if (SUCCEEDED(hr))
				{
					hr = E_FAIL;
				}
				ExitOnFailure(hr, "Failed evaluating regular expression. %hs", ex.what());
			}
		}
	}

	hr = ERROR_SUCCESS;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT SearchUnicode(LPCWSTR szProperty, LPCWSTR szExpression, LPCWSTR szInput, RegexFlags flags, std::regex_constants::syntax_option_type syntax)
{
	HRESULT hr = S_OK;
	bool bRes = true;
	match_results<LPCWSTR> results;
	CWixString sPropName;

	try
	{
		wregex rx(szExpression, syntax);
		bRes = regex_search(szInput, results, rx);
		ExitOnNull((bRes || ((flags.s.result & ResultFlags::MustMatch) == 0)), hr, E_FAIL, "Regex returned no matches");
	}
	catch (std::regex_error ex)
	{
		hr = HRESULT_FROM_WIN32(ex.code());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
		ExitOnFailure(hr, "Failed evaluating regular expression. %hs", ex.what());
	}

	if (!bRes)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No matches");
		ExitFunction1(hr = S_FALSE);
	}

	hr = sPropName.Format(L"%ls_COUNT", szProperty);
	ExitOnFailure(hr, "Failed formatting string");

	hr = WcaSetIntProperty(szProperty, results.size());
	ExitOnFailure(hr, "Failed setting property '%ls'", szProperty);

	hr = WcaSetIntProperty((LPCWSTR)sPropName, results.size());
	ExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)sPropName);

	// Iterate results
	for (size_t i = 0; i < results.size(); ++i)
	{
		const std::sub_match<LPCWSTR>& match = results[i];

		hr = sPropName.Format(L"%ls_%Iu", szProperty, i);
		ExitOnFailure(hr, "Failed formatting string");

		hr = WcaSetProperty((LPCWSTR)sPropName, match.str().c_str());
		ExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)sPropName);
	}

LExit:
	return hr;
}

static HRESULT SearchMultibyte(LPCWSTR szProperty, LPCWSTR szExpression, LPCSTR szInput, RegexFlags flags, std::regex_constants::syntax_option_type syntax)
{
	HRESULT hr = S_OK;
	LPSTR szPropertyA = nullptr;
	LPSTR szExpressionA = nullptr;
	match_results<LPCSTR> results;
	bool bRes = true;
	CWixString szCntProp;

	hr = StrAnsiAllocString(&szExpressionA, szExpression, 0, CP_UTF8);
	ExitOnFailure(hr, "Failed converting string to multibyte");

	// New scope to construct regex in
	try
	{
		regex rx(szExpressionA, syntax);

		bRes = regex_search(szInput, results, rx);
		ExitOnNull((bRes || ((flags.s.result & ResultFlags::MustMatch) == 0)), hr, E_FAIL, "Regex returned no matches");
	}
	catch (std::regex_error ex)
	{
		hr = HRESULT_FROM_WIN32(ex.code());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
		ExitOnFailure(hr, "Failed evaluating regular expression. %hs", ex.what());
	}

	if (!bRes)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No matches");
		ExitFunction1(hr = S_FALSE);
	}

	hr = szCntProp.Format(L"%ls_COUNT", szProperty);
	ExitOnFailure(hr, "Failed formatting string");

	hr = WcaSetIntProperty(szProperty, results.size());
	ExitOnFailure(hr, "Failed setting property '%ls'", szProperty);

	hr = ::WcaSetIntProperty(szCntProp, results.size());
	ExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)szCntProp);

	// Iterate results	
	for (size_t i = 0; i < results.size(); ++i)
	{
		const std::sub_match<LPCSTR>& match = results[i];

		hr = StrAnsiAllocFormatted(&szPropertyA, "%ls_%Iu", szProperty, i);
		ExitOnFailure(hr, "Failed formatting ansi string");

		hr = ::MsiSetPropertyA(WcaGetInstallHandle(), szPropertyA, match.str().c_str());
		ExitOnFailure(hr, "Failed setting property '%hs'", szPropertyA);

		ReleaseNullMem(szPropertyA);
	}

LExit:
	ReleaseMem(szExpressionA);
	ReleaseMem(szPropertyA);
	return hr;
}


static HRESULT SearchInFile(LPCWSTR szProperty, LPCWSTR szExpression, LPCWSTR szFilePath, RegexFlags flags, std::regex_constants::syntax_option_type syntax)
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
		hr = SearchMultibyte(szProperty, szExpression, (LPCSTR)pFileContents, flags, syntax);
		ExitOnFailure(hr, "Failed executing multibyte regular expression");
	}
	else
	{
		hr = SearchUnicode(szProperty, szExpression, (LPCWSTR)pFileContents, flags, syntax);
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

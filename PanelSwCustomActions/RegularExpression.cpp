#include "stdafx.h"
#include "../CaCommon/WixString.h"
#include <regex>

using namespace std;
#define RegularExpressionQuery L"SELECT `Id`, `Input`, `Expression`, `Replacement`, `DstProperty_`, `Flags`, `Condition` FROM `PSW_RegularExpression` ORDER BY `Order`"
enum eRegularExpressionQuery { Id = 1, Input, Expression, Replacement, DstProperty, Flags, Condition };
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
		CWixString sId, sInput, sExpression, sReplace, sDstProperty, sCondition;
		int iFlags = 0;
		RegexFlags flags;
		std::regex_constants::syntax_option_type syntax = (std::regex_constants::syntax_option_type)0;
		bool bRes = true;

		hr = WcaGetRecordString(hRecord, eRegularExpressionQuery::Id, (LPWSTR*)sId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::Input, (LPWSTR*)sInput);
		BreakExitOnFailure(hr, "Failed to get Input.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::Expression, (LPWSTR*)sExpression);
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

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing regular expression query '%ls'", (LPCWSTR)sId);

		// Syntax flags
		if (flags.s.match & MatchFlags::IgnoreCare)
		{
			syntax |= std::regex_constants::syntax_option_type::icase;
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Ignoring case", (LPCWSTR)sId);
		}
		if (flags.s.match & MatchFlags::Extended)
		{
			syntax |= std::regex_constants::syntax_option_type::extended;
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Extended regex syntax", (LPCWSTR)sId);
		}

		wregex rx((LPCWSTR)sExpression, syntax);
		match_results<LPCWSTR> results;

		// Match:
		if (flags.s.search == SearchFlags::Search)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Matching regex '%ls' on input '%ls'", (LPCWSTR)sExpression, (LPCWSTR)sInput);
		
			bRes = regex_search((LPCWSTR)sInput, results, rx);
			BreakExitOnNull((bRes || ((flags.s.result & ResultFlags::MustMatch) == 0)), hr, E_FAIL, "Regex returned no matches");
			if (!bRes)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No matches");
				continue;
			}

			// Iterate results
			match_results<LPCWSTR>::const_iterator curIt = results.begin();
			match_results<LPCWSTR>::const_iterator endIt = results.end();
			for (size_t i = 0; curIt != endIt; ++i, ++curIt)
			{
				CWixString sPropName;

				hr = sPropName.Format(L"%s_%Iu", (LPCWSTR)sDstProperty, i);
				BreakExitOnFailure(hr, "Failed formatting string");

				hr = WcaSetProperty((LPCWSTR)sPropName, curIt->str().c_str());
				BreakExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)sPropName);
			}
		}
		// Replace
		else
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Replacing regex '%ls' with '%ls' on input '%ls'", (LPCWSTR)sExpression, (LPCWSTR)sReplace, (LPCWSTR)sInput);
	
			std::wstring rep = regex_replace((LPCWSTR)sInput, rx, (LPCWSTR)sReplace);
			
			hr = WcaSetProperty(sDstProperty, rep.c_str());
			BreakExitOnFailure(hr, "Failed setting target property");
		}

	}

	hr = ERROR_SUCCESS;
	WcaLog(LOGMSG_STANDARD, "Done.");

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}


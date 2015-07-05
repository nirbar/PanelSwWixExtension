#include "stdafx.h"
#include "../CaCommon/WixString.h"
#include <regex>

using namespace std;
#define RegularExpressionQuery L"SELECT `Id`, `Input`, `Expression`, `Replacement`, `PropertyFormat`, `Flags`, `Condition` FROM `PSW_RegularExpression`"
enum eRegularExpressionQuery { Id = 1, Input, Expression, Replacement, PropertyFormat, Flags, Condition };
enum SearchFlags
{
	Search = 0
	, Replace = 1
};

enum ResultFlags
{
	None = 0
	, MustMatch = 1
};

union RegexFlags
{
	unsigned int u;
	struct
	{
		SearchFlags search : 1;
		ResultFlags result : 1;
		unsigned dontCare : 6;
	} s;
};

extern "C" __declspec(dllexport) UINT RegularExpression(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_RegularExpression");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_RegularExpression'. Have you authored 'PanelSw:RegularExpression' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(RegularExpressionQuery, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query on 'PSW_RegularExpression'.");
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString sId, sInput, sExpression, sReplace, sPropFormat, sCondition;
		int iFlags = 0;
		RegexFlags flags;
		bool bRes = true;

		hr = WcaGetRecordString(hRecord, eRegularExpressionQuery::Id, (LPWSTR*)sId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::Input, (LPWSTR*)sInput);
		BreakExitOnFailure(hr, "Failed to get Input.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::Expression, (LPWSTR*)sExpression);
		BreakExitOnFailure(hr, "Failed to get Expression.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::Replacement, (LPWSTR*)sReplace);
		BreakExitOnFailure(hr, "Failed to get Replacement.");
		hr = WcaGetRecordFormattedString(hRecord, eRegularExpressionQuery::PropertyFormat, (LPWSTR*)sPropFormat);
		BreakExitOnFailure(hr, "Failed to get PropertyFormat.");
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

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing regular expression query %ls", (LPCWSTR)sId);

		LPCWSTR scFirst = (LPCWSTR)sInput;
		LPCWSTR scLast = scFirst + sInput.StrLength();
		wregex rx((LPCWSTR)sExpression);
		match_results<LPCWSTR> results;

		// Match:
		bRes = regex_match(scFirst, scLast, results, rx, regex_constants::match_flag_type::match_default);
		BreakExitOnNull((bRes || ((flags.s.result & ResultFlags::MustMatch) == 0)), hr, E_FAIL, "Regex returned no matches");

		// Iterate results
		if (bRes)
		{
			match_results<LPCWSTR>::const_iterator curIt = results.begin();
			match_results<LPCWSTR>::const_iterator endIt = results.end();

			for (size_t i = 0; curIt != endIt; ++i, ++curIt)
			{
				CWixString sPropName;

				hr = sPropName.Format((LPCWSTR)sPropFormat, i);
				BreakExitOnFailure1(hr, "Failed formatting property name with '%ls'", (LPCWSTR)sPropFormat);

				hr = WcaSetProperty((LPCWSTR)sPropName, curIt->str().c_str());
				BreakExitOnFailure1(hr, "Failed setting property '%ls'", (LPCWSTR)sPropName);
			}
		}
	}

	hr = ERROR_SUCCESS;
	WcaLog(LOGMSG_STANDARD, "Done.");

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}


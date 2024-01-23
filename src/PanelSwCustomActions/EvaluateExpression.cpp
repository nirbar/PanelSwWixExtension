#include "pch.h"
#pragma push_macro("min")
#define min min
#pragma push_macro("max")
#define max max
#include "../exprtk/exprtk.hpp"
#pragma pop_macro("min")  
#pragma pop_macro("max")  

#define QUERY		L"SELECT `Expression`, `Property_` FROM `PSW_EvaluateExpression` ORDER BY `Order`"

extern "C" UINT __stdcall EvaluateExpression(MSIHANDLE hInstall)
{	
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPSTR szAnsiExpression = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_EvaluateExpression");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_EvaluateExpression'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_EvaluateExpression'. Have you authored 'PanelSw:Evaluate' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(QUERY, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", QUERY);

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");
		CWixString expression, property_, value;
		bool bRes = true;
		exprtk::expression<double> expr;
		exprtk::parser<double> parser;
		exprtk::symbol_table<double> symbols;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)expression);
		ExitOnFailure(hr, "Failed to get expression.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)property_);
		ExitOnFailure(hr, "Failed to get Path.");
		hr = expression.ToAnsiString(&szAnsiExpression);
		ExitOnFailure(hr, "Failed to get expression as ANSI string.");

		if (symbols.add_constants())
		{
			expr.register_symbol_table(symbols);
		}

		bRes = parser.compile(szAnsiExpression, expr);
		ExitOnNull(bRes, hr, E_FAIL, "Failed compiling expression '%hs'. %hs", szAnsiExpression, parser.error().c_str());

		if (expr.value() == (int)expr.value())
		{
			hr = value.Format(L"%i", (int)expr.value());
			ExitOnFailure(hr, "Failed to format result");
		}
		else
		{
			hr = value.Format(L"%f", expr.value());
			ExitOnFailure(hr, "Failed to format result");
		}

		hr = WcaSetProperty(property_, value);
		ExitOnFailure(hr, "Failed to set property.");

		ReleaseNullMem(szAnsiExpression);
	}
	hr = S_OK;

LExit:
	ReleaseMem(szAnsiExpression);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

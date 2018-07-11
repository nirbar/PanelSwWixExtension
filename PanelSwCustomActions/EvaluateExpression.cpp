#include "../CaCommon/WixString.h"
#include <memutil.h>
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
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_BackupAndRestore");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_BackupAndRestore'. Have you authored 'PanelSw:BackupAndRestore' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(QUERY, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", QUERY);

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");
		CWixString expression, property_;
		bool bRes = true;
		double value = 0;
		exprtk::expression<double> expr;
		exprtk::parser<double> parser;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)expression);
		BreakExitOnFailure(hr, "Failed to get expression.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)property_);
		BreakExitOnFailure(hr, "Failed to get Path.");
		hr = expression.ToAnsiString(&szAnsiExpression);
		BreakExitOnFailure(hr, "Failed to get expression as ANSI string.");

		bRes = parser.compile(szAnsiExpression, expr);
		BreakExitOnNull(bRes, hr, E_FAIL, "Failed compiling expression '%s'", szAnsiExpression);

		value = expr.value();

		hr = WcaSetIntProperty(property_, (int)value);
		BreakExitOnFailure(hr, "Failed to set property.");

		ReleaseNullMem(szAnsiExpression);
	}
	hr = S_OK;

LExit:
	ReleaseMem(szAnsiExpression);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

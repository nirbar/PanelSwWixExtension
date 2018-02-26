#include "stdafx.h"
#include "../CaCommon/WixString.h"

#define MsiSqlQueryQuery L"SELECT `Id`, `Query`, `Condition` FROM `PSW_MsiSqlQuery`"
enum eMsiSqlQueryQuery { Id = 1, Query, Condition };

extern "C" UINT __stdcall MsiSqlQuery(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_MsiSqlQuery");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_MsiSqlQuery'. Have you authored 'PanelSw:MsiSqlQuery' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(MsiSqlQueryQuery, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query on 'PSW_MsiSqlQuery'.");
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString sId, sQuery, sCondition;
		PMSIHANDLE hQueryView;

		hr = WcaGetRecordString(hRecord, eMsiSqlQueryQuery::Id, (LPWSTR*)sId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, eMsiSqlQueryQuery::Query, (LPWSTR*)sQuery);
		BreakExitOnFailure(hr, "Failed to get Query.");
		hr = WcaGetRecordString(hRecord, eMsiSqlQueryQuery::Condition, (LPWSTR*)sCondition);
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

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing MSI query: %ls", (LPCWSTR)sQuery);

		hr = WcaOpenExecuteView((LPCWSTR)sQuery, &hQueryView);
		BreakExitOnFailure(hr, "Failed executing MSI SQL query %ls", (LPCWSTR)sId);
	}

	hr = ERROR_SUCCESS;
	WcaLog(LOGMSG_STANDARD, "Done.");

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

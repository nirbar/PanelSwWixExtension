#include "stdafx.h"
#include "../CaCommon/WixString.h"

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
	hr = WcaOpenExecuteView(L"SELECT `Id`, `Property_`, `Query`, `Condition` FROM `PSW_MsiSqlQuery`", &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query on 'PSW_MsiSqlQuery'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString sId, dstProperty, sQuery, sCondition;
		PMSIHANDLE hQueryView;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)sId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)dstProperty);
		BreakExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)sQuery);
		BreakExitOnFailure(hr, "Failed to get Query.");
		hr = WcaGetRecordString(hRecord, 4, (LPWSTR*)sCondition);
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

		// Store result to a property?
		if (!dstProperty.IsNullOrEmpty())
		{
			PMSIHANDLE hQueryRec;
			CWixString value;
			
			hr = WcaFetchRecord(hQueryView, &hQueryRec);
			if (hr == E_NOMOREITEMS)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No results found");
				hr = S_FALSE;
				continue;
			}
			BreakExitOnFailure(hr, "Failed fetching query result");

			hr = WcaGetRecordString(hQueryRec, 1, (LPWSTR*)value);
			BreakExitOnFailure(hr, "Failed to get query result.");

			hr = WcaSetProperty(dstProperty, value);
			BreakExitOnFailure(hr, "Failed to set query result to property.");
		}
	}

	hr = S_OK;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

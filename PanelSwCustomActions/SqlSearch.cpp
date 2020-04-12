#include "stdafx.h"
#include "../CaCommon/WixString.h"
#include "../CaCommon/SqlConnection.h"
#include "../CaCommon/SqlQuery.h"
#include "SqlScript.h"

extern "C" UINT __stdcall SqlSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_SqlSearch");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_SqlSearch'. Have you authored 'PanelSw:SqlSearch' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Property_`, `Server`, `Instance`, `Database`, `Username`, `Password`, `Query`, `Condition`, `Port`, `Encrypted` FROM `PSW_SqlSearch` ORDER BY `Order`", &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query on 'PSW_SqlSearch'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szProperty;
		CWixString szServer;
		CWixString szInstance;
		CWixString szDatabase;
		CWixString szUsername;
		CWixString szPassword;
		CWixString szQuery;
		CWixString szCondition;
		CWixString szResult;
		CWixString szEncrypted;
		int nPort = 0;
		int bEncrypted = 0;
		CSqlConnection sqlConn;
		CSqlQuery sqlQuery;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szProperty);
		BreakExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)szServer);
		BreakExitOnFailure(hr, "Failed to get Server.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)szInstance);
		BreakExitOnFailure(hr, "Failed to get Instance.");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)szDatabase);
		BreakExitOnFailure(hr, "Failed to get Database.");
		hr = WcaGetRecordFormattedString(hRecord, 5, (LPWSTR*)szUsername);
		BreakExitOnFailure(hr, "Failed to get Username.");
		hr = WcaGetRecordFormattedString(hRecord, 6, (LPWSTR*)szPassword);
		BreakExitOnFailure(hr, "Failed to get Password.");
		hr = WcaGetRecordFormattedString(hRecord, 7, (LPWSTR*)szQuery);
		BreakExitOnFailure(hr, "Failed to get Query.");
		hr = WcaGetRecordFormattedString(hRecord, 8, (LPWSTR*)szCondition);
		BreakExitOnFailure(hr, "Failed to get Condition.");
		hr = WcaGetRecordFormattedInteger(hRecord, 9, &nPort);
		BreakExitOnFailure(hr, "Failed to get Port.");
		hr = WcaGetRecordFormattedString(hRecord, 10, (LPWSTR*)szEncrypted);
		BreakExitOnFailure(hr, "Failed to get Encrypted.");
		bEncrypted = (szEncrypted.EqualsIgnoreCase(L"true") || szEncrypted.EqualsIgnoreCase(L"yes") || szEncrypted.Equals(L"1"));

		if (!szCondition.IsNullOrEmpty())
		{
			MSICONDITION condRes = MSICONDITION::MSICONDITION_NONE;

			condRes = ::MsiEvaluateCondition(hInstall, szCondition);
			BreakExitOnNullWithLastError((condRes != MSICONDITION::MSICONDITION_ERROR), hr, "Failed evaluating condition '%ls'", szCondition);

			hr = (condRes == MSICONDITION::MSICONDITION_FALSE) ? S_FALSE : S_OK;
			WcaLog(LOGMSG_STANDARD, "Condition '%ls' evaluated to %i", (LPCWSTR)szCondition, (1 - (int)hr));
			if (hr == S_FALSE)
			{
				continue;
			}
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing SQL query '%ls'. Server='%ls', Instance='%ls', Port=%u, Database='%ls', User='%ls'. Will place results in property '%ls'", (LPCWSTR)szQuery, (LPCWSTR)szServer, (LPCWSTR)szInstance, nPort, (LPCWSTR)szDatabase, (LPCWSTR)szUsername, (LPCWSTR)szProperty);

		hr = sqlConn.Connect((LPCWSTR)szServer, (LPCWSTR)szInstance, nPort, (LPCWSTR)szDatabase, (LPCWSTR)szUsername, (LPCWSTR)szPassword, bEncrypted);
		BreakExitOnFailure(hr, "Failed connecting to database");

		hr = sqlQuery.ExecuteQuery(sqlConn, (LPWSTR)szQuery, (LPWSTR*)szResult);
		BreakExitOnFailure(hr, "Failed excuting query");

		hr = WcaSetProperty((LPCWSTR)szProperty, szResult.IsNullOrEmpty() ? L"" : (LPCWSTR)szResult);
		BreakExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)szProperty);
	}
	hr = S_OK;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}
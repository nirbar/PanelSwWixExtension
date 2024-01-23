#include "pch.h"
#include "../CaCommon/SqlConnection.h"
#include "../CaCommon/SqlQuery.h"
#include "errorHandling.pb.h"
using namespace ::com::panelsw::ca;
#include "SqlScript.h"

static HRESULT ExecuteOne(LPCWSTR szConnectionString, LPCWSTR szServer, LPCWSTR szInstance, int nPort, LPCWSTR szDatabase, LPCWSTR szUsername, LPCWSTR szPassword, bool bEncrypted, ErrorHandling errorHandling, LPCWSTR szPropertyName, LPCWSTR szQuery);

extern "C" UINT __stdcall SqlSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_SqlSearch");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_SqlSearch'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_SqlSearch'. Have you authored 'PanelSw:SqlSearch' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Property_`, `Server`, `Instance`, `Database`, `Username`, `Password`, `Query`, `Condition`, `Port`, `Encrypted`, `ErrorHandling`, `ConnectionString` FROM `PSW_SqlSearch` ORDER BY `Order`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_SqlSearch'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szProperty;
		CWixString szConnectionString;
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
		ErrorHandling nErrorHandling = ErrorHandling::fail;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szProperty);
		ExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)szServer);
		ExitOnFailure(hr, "Failed to get Server.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)szInstance);
		ExitOnFailure(hr, "Failed to get Instance.");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)szDatabase);
		ExitOnFailure(hr, "Failed to get Database.");
		hr = WcaGetRecordFormattedString(hRecord, 5, (LPWSTR*)szUsername);
		ExitOnFailure(hr, "Failed to get Username.");
		hr = WcaGetRecordFormattedString(hRecord, 6, (LPWSTR*)szPassword);
		ExitOnFailure(hr, "Failed to get Password.");
		hr = WcaGetRecordFormattedString(hRecord, 7, (LPWSTR*)szQuery);
		ExitOnFailure(hr, "Failed to get Query.");
		hr = WcaGetRecordString(hRecord, 8, (LPWSTR*)szCondition);
		ExitOnFailure(hr, "Failed to get Condition.");
		hr = WcaGetRecordFormattedInteger(hRecord, 9, &nPort);
		ExitOnFailure(hr, "Failed to get Port.");
		hr = WcaGetRecordFormattedString(hRecord, 10, (LPWSTR*)szEncrypted);
		ExitOnFailure(hr, "Failed to get Encrypted.");
		bEncrypted = (szEncrypted.EqualsIgnoreCase(L"true") || szEncrypted.EqualsIgnoreCase(L"yes") || szEncrypted.Equals(L"1"));
		hr = WcaGetRecordInteger(hRecord, 11, (int*)&nErrorHandling);
		ExitOnFailure(hr, "Failed to get ErrorHandling.");
		hr = WcaGetRecordFormattedString(hRecord, 12, (LPWSTR*)szConnectionString);
		ExitOnFailure(hr, "Failed to get ConnectionString.");

		if (!szCondition.IsNullOrEmpty())
		{
			MSICONDITION condRes = MSICONDITION::MSICONDITION_NONE;

			condRes = ::MsiEvaluateCondition(hInstall, szCondition);
			ExitOnNullWithLastError((condRes != MSICONDITION::MSICONDITION_ERROR), hr, "Failed evaluating condition '%ls'", (LPCWSTR)szCondition);

			hr = (condRes == MSICONDITION::MSICONDITION_FALSE) ? S_FALSE : S_OK;
			WcaLog(LOGMSG_STANDARD, "Condition '%ls' evaluated to %i", (LPCWSTR)szCondition, (1 - (int)hr));
			if (hr == S_FALSE)
			{
				continue;
			}
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing SQL query '%ls'. Will place results in property '%ls'", (LPCWSTR)szQuery, (LPCWSTR)szProperty);

		hr = ExecuteOne((LPCWSTR)szConnectionString, (LPCWSTR)szServer, (LPCWSTR)szInstance, nPort, (LPCWSTR)szDatabase, (LPCWSTR)szUsername, (LPCWSTR)szPassword, bEncrypted, nErrorHandling, szProperty, szQuery);
		ExitOnFailure(hr, "Failed executing SQL search");
	}
	hr = S_OK;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT ExecuteOne(LPCWSTR szConnectionString, LPCWSTR szServer, LPCWSTR szInstance, int nPort, LPCWSTR szDatabase, LPCWSTR szUsername, LPCWSTR szPassword, bool bEncrypted, ErrorHandling errorHandling, LPCWSTR szPropertyName, LPCWSTR szQuery)
{
	HRESULT hr = S_OK;
	CSqlConnection sqlConn;
	CSqlQuery sqlQuery;
	CWixString szResult;
	CWixString szError;
	CErrorPrompter errorPrompter(PSW_MSI_MESSAGES::PSW_MSI_MESSAGES_SQL_SEARCH_ERROR);
	errorPrompter.SetErrorHandling((PSW_ERROR_HANDLING)errorHandling);

LRetry:
	if (szConnectionString && *szConnectionString)
	{
		hr = sqlConn.Connect(szConnectionString, (LPWSTR*)szError);
	}
	else
	{
		hr = sqlConn.Connect(nullptr, szServer, szInstance, nPort, szDatabase, szUsername, szPassword, bEncrypted, (LPWSTR*)szError);
	}
	if (SUCCEEDED(hr))
	{
		hr = sqlQuery.ExecuteQuery(sqlConn, szQuery, (LPWSTR*)szResult, (LPWSTR*)szError);
	}

	if (FAILED(hr))
	{
		WcaLogError(hr, "Failed executing SQL query: %ls", (LPCWSTR)szError);
		hr = errorPrompter.Prompt(szQuery, (LPCWSTR)szError);
		if (hr == E_RETRY)
		{
			hr = S_OK;
			goto LRetry;
		}
	}
	ExitOnFailure(hr, "Failed excuting SQL search");

	hr = WcaSetProperty(szPropertyName, szResult.IsNullOrEmpty() ? L"" : (LPCWSTR)szResult);
	ExitOnFailure(hr, "Failed setting property '%ls'", szPropertyName);

LExit:
	return hr;
}

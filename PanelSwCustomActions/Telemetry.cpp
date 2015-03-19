#include "Telemetry.h"
#include "../CaCommon/WixString.h"
#include <Urlmon.h>
#include <Wininet.h>
#pragma comment (lib, "Wininet.lib")
#pragma comment (lib, "Urlmon.lib")


#define TELEMETRY_QUERY L"SELECT `Id`, `Url`, `Data`, `Flags`, `Condition` FROM `PSW_Telemetry`"
enum TelemetryQuery { Id=1, Url=2, Data=3, Flags=3, Condition=5 };

enum TelemetryFlags
{
	None = 0,
	OnExecute = 1,
	OnCommit = 2,
	OnRollback = 4
};

extern "C" __declspec(dllexport) UINT Telemetry(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CTelemetry oRollbackTelemetry;
	CTelemetry oCommitTelemetry;
	CTelemetry oDeferredTelemetry;
	CComBSTR szCustomActionData;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_Telemetry exists.
	hr = WcaTableExists(L"PSW_Telemetry");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_Telemetry'. Have you authored 'PanelSw:Telemetry' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(TELEMETRY_QUERY, &hView);
	BreakExitOnFailure1(hr, "Failed to execute SQL query '%ls'.", TELEMETRY_QUERY);
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szUrl, szData, szCondition;
		int nFlags;

		hr = WcaGetRecordString(hRecord, TelemetryQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, TelemetryQuery::Url, (LPWSTR*)szUrl);
		BreakExitOnFailure(hr, "Failed to get URL.");
		hr = WcaGetRecordFormattedString(hRecord, TelemetryQuery::Data, (LPWSTR*)szData);
		BreakExitOnFailure(hr, "Failed to get Data.");
		hr = WcaGetRecordInteger(hRecord, TelemetryQuery::Flags, &nFlags);
		BreakExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordString(hRecord, TelemetryQuery::Condition, (LPWSTR*)szCondition);
		BreakExitOnFailure(hr, "Failed to get Condition.");

		// Test condition
		MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, szCondition);
		switch (condRes)
		{
		case MSICONDITION::MSICONDITION_NONE:
		case MSICONDITION::MSICONDITION_TRUE:
			WcaLog(LOGMSG_STANDARD, "Condition evaluated to true / none.");
			break;

		case MSICONDITION::MSICONDITION_FALSE:
			WcaLog(LOGMSG_STANDARD, "Skipping. Condition evaluated to false");
			continue;

		case MSICONDITION::MSICONDITION_ERROR:
			hr = E_FAIL;
			BreakExitOnFailure(hr, "Bad Condition field");
		}

		if ((nFlags & TelemetryFlags::OnExecute) != 0)
		{
			hr = oDeferredTelemetry.AddPost(szUrl, szData);
			BreakExitOnFailure(hr, "Failed creating custom action data for deferred action.");
		}
		if ((nFlags & TelemetryFlags::OnCommit) != 0)
		{
			hr = oCommitTelemetry.AddPost(szUrl, szData);
			BreakExitOnFailure(hr, "Failed creating custom action data for commit action.");
		}
		if ((nFlags & TelemetryFlags::OnRollback) != 0)
		{
			hr = oRollbackTelemetry.AddPost(szUrl, szData);
			BreakExitOnFailure(hr, "Failed creating custom action data for rollback action.");
		}
	}
	
	// Schedule actions.
	hr = oRollbackTelemetry.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback action.");
	hr = WcaDoDeferredAction(L"Telemetry_rollback", szCustomActionData, oRollbackTelemetry.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling rollback action.");

	szCustomActionData.Empty();
	hr = oDeferredTelemetry.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"Telemetry_deferred", szCustomActionData, oDeferredTelemetry.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

	szCustomActionData.Empty();
	hr = oCommitTelemetry.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for commit action.");
	hr = WcaDoDeferredAction(L"Telemetry_commit", szCustomActionData, oCommitTelemetry.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling commit action.");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CTelemetry::AddPost(LPCWSTR szUrl, LPCWSTR szData)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"PostTelemetry", L"CTelemetry", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("Url"), CComVariant(szUrl));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Url'");

	hr = pElem->setAttribute(CComBSTR("Data"), CComVariant(szData));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Data'");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CTelemetry::DeferredExecute(IXMLDOMElement* pElem)
{
	HRESULT hr = S_OK;
	CComVariant vTag;
	CComVariant vUrl;
	CComVariant vData;

	// Get URL
	hr = pElem->getAttribute(CComBSTR(L"Url"), &vUrl);
	BreakExitOnFailure(hr, "Failed to get URL");

	// Get Data
	hr = pElem->getAttribute(CComBSTR(L"Data"), &vData);
	BreakExitOnFailure(hr, "Failed to get Data");

	hr = Post(vUrl.bstrVal, vData.bstrVal);
	BreakExitOnFailure2(hr, "Failed to post Data '%ls' to URL '%ls'", vData.bstrVal, vUrl.bstrVal);

LExit:
	return hr;
}

HRESULT CTelemetry::Post(LPCWSTR szUrl, LPCWSTR szData)
{
	HRESULT hr = S_OK;
	char szHttpUseragent[512];
	DWORD szhttpUserAgent = sizeof(szHttpUseragent);
	HINTERNET hInet = NULL;
	HINTERNET hCnnct = NULL;
	HINTERNET hRequest = NULL;

	hr = ::ObtainUserAgentString(0, szHttpUseragent, &szhttpUserAgent);
	BreakExitOnFailure(hr, "Failed getting user agent");

	// http://msdn.microsoft.com/en-us/library/windows/desktop/aa385096%28v=vs.85%29.aspx
	hInet = ::InternetOpenA(szHttpUseragent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	BreakExitOnNullWithLastError(hInet, hr, "Failed openning internet connection");

	hCnnct = ::InternetConnect(hInet, szUrl, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	BreakExitOnNullWithLastError(hCnnct, hr, "Failed connecting to URL");

	//http://msdn.microsoft.com/en-us/library/windows/desktop/aa384233%28v=vs.85%29.aspx
	hRequest = ::HttpOpenRequest(hCnnct, L"POST", szUrl, L"HTTP/1.1", NULL, NULL,
		INTERNET_FLAG_HYPERLINK | INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
		INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
		INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
		INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
		INTERNET_FLAG_NO_AUTH |
		INTERNET_FLAG_NO_CACHE_WRITE |
		INTERNET_FLAG_NO_UI |
		INTERNET_FLAG_PRAGMA_NOCACHE |
		INTERNET_FLAG_RELOAD, NULL);
	BreakExitOnNullWithLastError(hRequest, hr, "Failed openning request to URL");

	int datalen = 0;
	datalen = ::wcslen(szData);

	//http://msdn.microsoft.com/en-us/library/windows/desktop/aa384247%28v=vs.85%29.aspx
	::HttpSendRequest(hRequest, NULL, 0, (LPVOID)szData, datalen);

LExit:

	if (hInet != NULL)
	{
		::InternetCloseHandle(hInet);
	}
	if (hCnnct != NULL)
	{
		::InternetCloseHandle(hCnnct);
	}
	if (hRequest != NULL)
	{
		InternetCloseHandle(hRequest);
	}

	return hr;
}
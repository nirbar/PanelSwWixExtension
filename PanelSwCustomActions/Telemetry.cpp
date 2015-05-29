#include "Telemetry.h"
#include "../CaCommon/WixString.h"
#include <Winhttp.h>
#pragma comment (lib, "Winhttp.lib")


#define TELEMETRY_QUERY L"SELECT `Id`, `Url`, `Page`, `Method`, `Data`, `Flags`, `Condition` FROM `PSW_Telemetry`"
enum TelemetryQuery { Id=1, Url=2, Page = 3, Method=4, Data=5, Flags=6, Condition=7 };

enum TelemetryFlags
{
	None = 0,
	OnExecute = 1,
	OnCommit = 2,
	OnRollback = 4,

	Secure = 8
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
		CWixString szId, szUrl, szPage, szMethod, szData, szCondition;
		int nFlags = 0;
		BOOL bSecure = FALSE;

		hr = WcaGetRecordString(hRecord, TelemetryQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, TelemetryQuery::Url, (LPWSTR*)szUrl);
		BreakExitOnFailure(hr, "Failed to get URL.");
		hr = WcaGetRecordFormattedString(hRecord, TelemetryQuery::Page, (LPWSTR*)szPage);
		BreakExitOnFailure(hr, "Failed to get Page.");
		hr = WcaGetRecordFormattedString(hRecord, TelemetryQuery::Method, (LPWSTR*)szMethod);
		BreakExitOnFailure(hr, "Failed to get Method.");
		hr = WcaGetRecordFormattedString(hRecord, TelemetryQuery::Data, (LPWSTR*)szData);
		BreakExitOnFailure(hr, "Failed to get Data.");
		hr = WcaGetRecordInteger(hRecord, TelemetryQuery::Flags, &nFlags);
		BreakExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordString(hRecord, TelemetryQuery::Condition, (LPWSTR*)szCondition);
		BreakExitOnFailure(hr, "Failed to get Condition.");

		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Will post telemetry: Id=%ls\nUrl=%ls\nPage=%ls\nData=%ls\nFlags=%i\nCondition=%ls"
			, (LPCWSTR)szId
			, (LPCWSTR)szUrl
			, (LPCWSTR)szPage
			, (LPCWSTR)szData
			, (LPCWSTR)nFlags
			, (LPCWSTR)szCondition
			);

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

		if ((nFlags & TelemetryFlags::Secure) == TelemetryFlags::Secure)
		{
			bSecure = TRUE;
		}

		if ((nFlags & TelemetryFlags::OnExecute) == TelemetryFlags::OnExecute)
		{
			hr = oDeferredTelemetry.AddPost(szUrl, szPage, szMethod, szData, bSecure);
			BreakExitOnFailure(hr, "Failed creating custom action data for deferred action.");
		}
		if ((nFlags & TelemetryFlags::OnCommit) == TelemetryFlags::OnCommit)
		{
			hr = oCommitTelemetry.AddPost(szUrl, szPage, szMethod, szData, bSecure);
			BreakExitOnFailure(hr, "Failed creating custom action data for commit action.");
		}
		if ((nFlags & TelemetryFlags::OnRollback) == TelemetryFlags::OnRollback)
		{
			hr = oRollbackTelemetry.AddPost(szUrl, szPage, szMethod, szData, bSecure);
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

HRESULT CTelemetry::AddPost(LPCWSTR szUrl, LPCWSTR szPage, LPCWSTR szMethod, LPCWSTR szData, BOOL bSecure)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"PostTelemetry", L"CTelemetry", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("Url"), CComVariant(szUrl));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Url'");

	hr = pElem->setAttribute(CComBSTR("Page"), CComVariant(szPage));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Page'");

	hr = pElem->setAttribute(CComBSTR("Method"), CComVariant(szMethod));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Method'");

	hr = pElem->setAttribute(CComBSTR("Data"), CComVariant(szData));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Data'");

	hr = pElem->setAttribute(CComBSTR("Secure"), CComVariant(bSecure));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Secure'");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CTelemetry::DeferredExecute(IXMLDOMElement* pElem)
{
	HRESULT hr = S_OK;
	CComVariant vTag;
	CComVariant vUrl;
	CComVariant vPage;
	CComVariant vData;
	CComVariant vMethod;
	CComVariant vSecure;
	int nSecure = 0;

	// Get URL
	hr = pElem->getAttribute(CComBSTR(L"Url"), &vUrl);
	BreakExitOnFailure(hr, "Failed to get URL");

	// Get URL
	hr = pElem->getAttribute(CComBSTR(L"Page"), &vPage);
	BreakExitOnFailure(hr, "Failed to get Page");

	// Get Method
	hr = pElem->getAttribute(CComBSTR(L"Method"), &vMethod);
	BreakExitOnFailure(hr, "Failed to get Method");

	// Get Data
	hr = pElem->getAttribute(CComBSTR(L"Data"), &vData);
	BreakExitOnFailure(hr, "Failed to get Data");

	// Get Secure
	hr = pElem->getAttribute(CComBSTR(L"Secure"), &vSecure);
	BreakExitOnFailure(hr, "Failed to get Secure");
	nSecure = _ttoi(vSecure.bstrVal);


	WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Posting telemetry: Url=%ls Page=%ls Method=%ls Data=%ls Secure=%ls"
		, vUrl.bstrVal
		, vPage.bstrVal
		, vMethod.bstrVal
		, vData.bstrVal
		, vSecure.bstrVal);

	hr = Post(vUrl.bstrVal, vPage.bstrVal, vMethod.bstrVal, vData.bstrVal, nSecure != 0);
	BreakExitOnFailure2(hr, "Failed to post Data '%ls' to URL '%ls%ls'", vData.bstrVal, vUrl.bstrVal, vPage.bstrVal);

LExit:
	return hr;
}

HRESULT CTelemetry::Post(LPCWSTR szUrl, LPCWSTR szPage, LPCWSTR szMethod, LPCWSTR szData, BOOL bSecure)
{
	HRESULT hr = S_OK;
	DWORD dwSize = 0;
	DWORD dwPrevSize = 0;
	DWORD dwDownloaded = 0;
	LPSTR pszOutBuffer = NULL;
	BOOL  bResults = FALSE;
	HINTERNET hSession = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;

	// Use WinHttpOpen to obtain a session handle.
	hSession = ::WinHttpOpen(L"PanelSwCustomActions/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	BreakExitOnNullWithLastError(hSession, hr, "Failed openning HTTP session");

	// Specify an HTTP server.
	hConnect = ::WinHttpConnect(hSession, szUrl,
		bSecure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
	BreakExitOnNullWithLastError(hConnect, hr, "Failed connecting to URL");

	// Create an HTTP request handle.
	hRequest = ::WinHttpOpenRequest(hConnect, szMethod, szPage, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
		bSecure ? WINHTTP_FLAG_SECURE : 0);
	BreakExitOnNullWithLastError(hRequest, hr, "Failed openning request");

	// Get data size
	if(( szData != NULL) && ((*szData) != NULL))
	{
		dwSize = ::wcslen( szData);
	}
	
	// Send a request.
	bResults = ::WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)szData, dwSize, dwSize, 0);
	BreakExitOnNull(bResults, hr, E_FAIL, "Failed sending HTTP request");

	// End the request.
	bResults = ::WinHttpReceiveResponse(hRequest, NULL);
	BreakExitOnNull(bResults, hr, E_FAIL, "Failed receiving HTTP response");

	// Keep checking for data until there is nothing left.
	do
	{
		// Check for available data.
		dwSize = 0;
		bResults = ::WinHttpQueryDataAvailable(hRequest, &dwSize);
		BreakExitOnNullWithLastError(bResults, hr, "Failed querying available data.");

		// No more data.
		if (dwSize == 0)
		{
			break;
		}

		// Allocate space for the buffer.
		if (dwSize > dwPrevSize)
		{
			// Release previous buffer.
			if( pszOutBuffer != NULL)
			{
				delete[] pszOutBuffer;
				pszOutBuffer = NULL;
			}
		
			pszOutBuffer = new char[dwSize + 1];
			BreakExitOnNull(pszOutBuffer, hr, E_FAIL, "Failed allocating memory");
			dwPrevSize = dwSize;
		}

		// Read the Data.
		::ZeroMemory(pszOutBuffer, dwSize + 1);

		bResults = ::WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded);
		BreakExitOnNullWithLastError(bResults, hr, "Failed reading data");
		BreakExitOnNull(dwDownloaded, hr, E_FAIL,  "Failed reading data (dwDownloaded=0)");
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "%s", pszOutBuffer);

	} while (dwSize > 0);

LExit:
	// Close any open handles.
	if (hRequest != NULL)
	{
		::WinHttpCloseHandle(hRequest);
	}
	if (hConnect != NULL)
	{
		::WinHttpCloseHandle(hConnect);
	}
	if (hSession != NULL)
	{
		::WinHttpCloseHandle(hSession);
	}
	if (pszOutBuffer != NULL)
	{
		delete[] pszOutBuffer;
	}

	return hr;
}
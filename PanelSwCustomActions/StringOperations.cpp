#include "../CaCommon/WixString.h"

#define SplitProp L"PROPERTY_TO_SPLIT"
#define SplitTokenProp L"STRING_SPLIT_TOKEN"

extern "C" UINT __stdcall SplitString(MSIHANDLE hInstall) noexcept
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	CWixString szFullString;
	CWixString szPropName;
	CWixString szDstPropName;
	CWixString szToken;
	LPCWSTR szCurrValue = nullptr;
	size_t i = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");

	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Get property-to-split name
	hr = WcaGetProperty(SplitProp, (LPWSTR*)szPropName);
	ExitOnFailure1(hr, "Failed getting %ls", SplitProp);
	if (szPropName.IsNullOrEmpty())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No property name to split...");
		ExitFunction();
	}
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will split property '%ls'", (LPCWSTR)szPropName);

	// Get string-to-split
	hr = WcaGetProperty((LPCWSTR)szPropName, (LPWSTR*)szFullString);
	ExitOnFailure1(hr, "Failed getting %ls", (LPCWSTR)szPropName);
	if (szFullString.IsNullOrEmpty())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No string to split...");
		ExitFunction();
	}
	WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Will split string '%ls'", (LPCWSTR)szFullString);

	// Get token-to-split-by
	hr = WcaGetProperty(SplitTokenProp, (LPWSTR*)szToken);
	ExitOnFailure1(hr, "Failed getting %ls", SplitTokenProp);
	if (szToken.IsNullOrEmpty())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No token to split by...");
		ExitFunction();
	}
	WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Will split string '%ls' by token '%ls'", (LPCWSTR)szFullString, (LPCWSTR)szToken);

	hr = szDstPropName.Allocate(szPropName.Capacity() + 20);
	ExitOnFailure(hr, "Failed allocating memory");

	for (hr = szFullString.Tokenize((LPCWSTR)szToken, &szCurrValue);
		(SUCCEEDED(hr) && szCurrValue);
		++i, hr = szFullString.NextToken((LPCWSTR)szToken, &szCurrValue))
	{
		hr = szDstPropName.Format(L"%s_%Iu", (LPCWSTR)szPropName, i);
		ExitOnFailure(hr, "Failed formatting string");

		hr = WcaSetProperty((LPCWSTR)szDstPropName, szCurrValue);
		ExitOnFailure1(hr, "Failed setting property '%ls'", (LPCWSTR)szDstPropName);
	}

	if (hr == E_NOMOREITEMS)
	{
		hr = S_OK;
	}

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

#undef SplitProp
#undef SplitTokenProp 

#define TrimProp L"PROPERTY_TO_TRIM"

extern "C" UINT __stdcall TrimString(MSIHANDLE hInstall) noexcept
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	CWixString szPropName;
	CWixString szFullString;
	LPCWSTR pFirst = nullptr;
	size_t i = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");

	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Get property-to-trim name
	hr = WcaGetProperty(TrimProp, (LPWSTR*)szPropName);
	ExitOnFailure1(hr, "Failed getting %ls", TrimProp);
	if (szPropName.IsNullOrEmpty())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No property name to trim...");
		ExitFunction();
	}
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will trim property '%ls'", (LPCWSTR)szPropName);

	// Get string-to-trim
	hr = WcaGetProperty((LPCWSTR)szPropName, (LPWSTR*)szFullString);
	ExitOnFailure1(hr, "Failed getting %ls", (LPCWSTR)szPropName);
	if (szFullString.IsNullOrEmpty())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No string to trim...");
		ExitFunction();
	}
	WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Will trim string '%ls'", (LPCWSTR)szFullString);

	// Trim right (i is unsigned, so after 0 it will be MAX_SIZE)
	for (i = szFullString.StrLen() - 1; i < szFullString.StrLen(); --i)
	{
		switch (((LPCWSTR)szFullString)[i])
		{
		case L' ':
		case L'\r':
		case L'\n':
		case L'\t':
		case L'\v':
			((LPWSTR)szFullString)[i] = NULL;
			continue;

		default:
			break;
		}
		break;
	}

	// Trim left
	for (i = 0; i < szFullString.StrLen(); ++i)
	{
		switch (((LPCWSTR)szFullString)[i])
		{
		case L' ':
		case L'\r':
		case L'\n':
		case L'\t':
		case L'\v':
			continue;

		default:
			break;
		}
		break;
	}

	if (i == szFullString.StrLen())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "%ls is all white-spaces", szPropName);

		hr = WcaSetProperty(szPropName, L"");
		ExitOnFailure1(hr, "Failed setting %ls", (LPCWSTR)szPropName);
	}

	pFirst = i + (LPCWSTR)szFullString;
	hr = WcaSetProperty(szPropName, pFirst);
	ExitOnFailure1(hr, "Failed setting %ls", (LPCWSTR)szPropName);

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

extern "C" UINT __stdcall ToLowerCase(MSIHANDLE hInstall) noexcept
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_ToLowerCase");
	ExitOnFailure(hr, "Table does not exist 'PSW_ToLowerCase'. Have you authored 'PanelSw:ToLowerCase' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Property_` FROM `PSW_ToLowerCase`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_ToLowerCase'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szProperty, szValue;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szProperty);
		ExitOnFailure(hr, "Failed to get Property_.");

		hr = WcaGetProperty(szProperty, (LPWSTR*)szValue);
		ExitOnFailure(hr, "Failed to get property '%ls' value.", (LPCWSTR)szProperty);

		StrStringToLower(szValue);

		hr = WcaSetProperty(szProperty, szValue);
		ExitOnFailure(hr, "Failed setting property");
	}
	hr = ERROR_SUCCESS;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}
#include "../CaCommon/WixString.h"

#define SplitProp L"PROPERTY_TO_SPLIT"
#define SplitTokenProp L"STRING_SPLIT_TOKEN"

extern "C" __declspec(dllexport) UINT SplitString(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	CWixString szFullString;
	CWixString szPropName;
	CWixString szDstPropName;
	CWixString szToken;
	LPCWSTR szCurrValue = NULL;
	size_t i = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");

	WcaLog(LOGMSG_STANDARD, "Initialized.");

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
	BreakExitOnFailure(hr, "Failed allocating memory");

	for (szCurrValue = ::wcstok((LPWSTR)szFullString, (LPCWSTR)szToken);
		szCurrValue != NULL;
		++i, szCurrValue = ::wcstok(NULL, (LPCWSTR)szToken))
	{
		wsprintf((LPWSTR)szDstPropName, L"%s_%Iu", (LPCWSTR)szPropName, i);

		hr = WcaSetProperty((LPCWSTR)szDstPropName, szCurrValue);
		ExitOnFailure1(hr, "Failed setting property '%ls'", (LPCWSTR)szDstPropName);
	}

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

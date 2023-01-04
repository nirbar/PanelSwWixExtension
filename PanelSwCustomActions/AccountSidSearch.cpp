#include "pch.h"
#include <aclutil.h>

extern "C" UINT __stdcall AccountSidSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Property_`, `SystemName`, `AccountName`, `Condition` FROM `PSW_AccountSidSearch`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_AccountSidSearch'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szProperty;
		CWixString szSystemName;
		CWixString szAccountName;
		CWixString szCondition;
		CWixString szSid;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szProperty);
		ExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)szSystemName);
		ExitOnFailure(hr, "Failed to get SystemName.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)szAccountName);
		ExitOnFailure(hr, "Failed to get AccountName.");
		hr = WcaGetRecordString(hRecord, 4, (LPWSTR*)szCondition);
		ExitOnFailure(hr, "Failed to get Condition.");

		// Test condition
		if (!szCondition.IsNullOrEmpty())
		{
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
				ExitOnFailure(hr, "Bad Condition field");
			}
		}

		// Sanity.
		ExitOnNull(!szAccountName.IsNullOrEmpty(), hr, E_INVALIDARG, "Account name is empty");
		ExitOnNull(!szProperty.IsNullOrEmpty(), hr, E_INVALIDARG, "Property name is empty");

		hr = AclGetAccountSidString(szSystemName.IsNullOrEmpty() ? nullptr : (LPCWSTR)szSystemName, (LPCWSTR)szAccountName, (LPWSTR*)szSid);
		if (hr == HRESULT_FROM_WIN32(ERROR_NONE_MAPPED))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Account SID was not found for '%ls'", (LPCWSTR)szAccountName);
			hr = S_FALSE;
			continue;
		}
		ExitOnFailure(hr, "Failed getting account SID for '%ls'", (LPCWSTR)szAccountName);

		hr = WcaSetProperty(szProperty, szSid);
		ExitOnFailure(hr, "Failed set property '%ls' with SID for '%ls'", (LPCWSTR)szProperty, (LPCWSTR)szAccountName);
	}
	hr = S_OK;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

#include "ServiceConfig.h"
#include "RegistryKey.h"
#include "../CaCommon/WixString.h"
#include <wcautil.h>
#include <procutil.h>
#include "google\protobuf\any.h"
using namespace com::panelsw::ca;
using namespace google::protobuf;

extern "C" UINT __stdcall ServiceConfig(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	DWORD dwRes = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPWSTR szCustomActionData = nullptr;
	CServiceConfig oDeferred;
	CServiceConfig oRollback;
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL;
	QUERY_SERVICE_CONFIG *pServiceCfg = nullptr;
	LPWSTR szDependencies = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure tables exist.
	hr = WcaTableExists(L"PSW_ServiceConfig");
	BreakExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_ServiceConfig'. Have you authored 'PanelSw:ServiceConfig' entries in WiX code?");
	hr = WcaTableExists(L"PSW_ServiceConfig_Dependency");
	BreakExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_ServiceConfig_Dependency'. Have you authored 'PanelSw:ServiceConfig' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `Component_`, `ServiceName`, `CommandLine`, `Account`, `Password`, `Start`, `DelayStart`, `LoadOrderGroup`, `ErrorHandling` FROM `PSW_ServiceConfig`", &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query.");

	// Open service.
	hManager = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
	ExitOnNullWithLastError(hManager, hr, "Failed opening service control manager database");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{        
        BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szComponent, szServiceName, szCommand, szCommandFormat, szCommandObfuscated, szAccount, szPassword, szLoadOrderGroupFmt, szLoadOrderGroup;
		CWixString szSubQuery;
		PMSIHANDLE hSubView, hSubRecord;
		int start = -1;
		int nDelayStart = -1;
		int errorHandling = -1;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szComponent);
		BreakExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)szServiceName);
		BreakExitOnFailure(hr, "Failed to get ServiceName.");
		hr = WcaGetRecordString(hRecord, 4, (LPWSTR*)szCommandFormat);
		BreakExitOnFailure(hr, "Failed to get CommandLine.");
		hr = WcaGetRecordFormattedString(hRecord, 5, (LPWSTR*)szAccount);
		BreakExitOnFailure(hr, "Failed to get Account.");
		hr = WcaGetRecordFormattedString(hRecord, 6, (LPWSTR*)szPassword);
		BreakExitOnFailure(hr, "Failed to get Password.");
		hr = WcaGetRecordInteger(hRecord, 7, &start);
		BreakExitOnFailure(hr, "Failed to get Start.");
		hr = WcaGetRecordInteger(hRecord, 8, &nDelayStart);
		BreakExitOnFailure(hr, "Failed to get DelayStart.");
		hr = WcaGetRecordString(hRecord, 9, (LPWSTR*)szLoadOrderGroupFmt);
		BreakExitOnFailure(hr, "Failed to get LoadOrderGroup.");
		hr = WcaGetRecordInteger(hRecord, 10, &errorHandling);
		BreakExitOnFailure(hr, "Failed to get ErrorHandling.");

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		if (compAction != WCA_TODO::WCA_TODO_INSTALL)
		{
			// In case of no-action, we just check if the service is marked for deletion to notify reboot is reqired.
			oDeferred.AddServiceConfig(szServiceName, nullptr, nullptr, nullptr, ServciceConfigDetails::ServiceStart::ServciceConfigDetails_ServiceStart_unchanged, nullptr, nullptr, ErrorHandling::ignore, ServciceConfigDetails_DelayStart::ServciceConfigDetails_DelayStart_unchanged1);

			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping configuration of service '%ls' since component is not installed", (LPCWSTR)szServiceName);
			continue;
		}

		if (!szCommandFormat.IsNullOrEmpty())
		{
			hr = szCommand.MsiFormat(szCommandFormat, (LPWSTR*)szCommandObfuscated);
			ExitOnFailure(hr, "Failed formatting string");

			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will configure service '%ls' to execute command line '%ls'", (LPCWSTR)szServiceName, (LPCWSTR)szCommandObfuscated);
		}
		if (!szAccount.IsNullOrEmpty())
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will change service '%ls' start account to '%ls'", (LPCWSTR)szServiceName, (LPCWSTR)szAccount);
		}
		if (start != ServciceConfigDetails_ServiceStart::ServciceConfigDetails_ServiceStart_unchanged)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will change service '%ls' start type to %i", (LPCWSTR)szServiceName, start);
		}
		if (!szLoadOrderGroupFmt.IsNullOrEmpty())
		{
			hr = szLoadOrderGroup.MsiFormat(szLoadOrderGroupFmt);
			ExitOnFailure(hr, "Failed formatting string");

			if (szLoadOrderGroup.IsNullOrEmpty())
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will clear service '%ls' load order group", (LPCWSTR)szServiceName);
			}
			else
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will change service '%ls' load order group to '%ls'", (LPCWSTR)szServiceName, (LPCWSTR)szLoadOrderGroup);
			}
		}

		// Dependencies
		hr = szSubQuery.Format(L"SELECT `Service`, `Group` FROM `PSW_ServiceConfig_Dependency` WHERE `ServiceConfig_`='%s'", (LPCWSTR)szId);
		BreakExitOnFailure(hr, "Failed to format string");

		hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
		BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

		// Iterate records
		while ((hr = WcaFetchRecord(hSubView, &hSubRecord)) != E_NOMOREITEMS)
		{
			BreakExitOnFailure(hr, "Failed to fetch record.");
			CWixString szServiceFmt, szGroup;

			hr = WcaGetRecordString(hSubRecord, 1, (LPWSTR*)szServiceFmt);
			BreakExitOnFailure(hr, "Failed to get Dependency.");
			hr = WcaGetRecordFormattedString(hSubRecord, 2, (LPWSTR*)szGroup);
			BreakExitOnFailure(hr, "Failed to get Group.");

			if (!szGroup.IsNullOrEmpty())
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will add to service '%ls' dependency on group '%ls'", (LPCWSTR)szServiceName, (LPCWSTR)szGroup);
				if (szDependencies == nullptr)
				{
					hr = StrAllocFormatted(&szDependencies, L"%c%s%c", SC_GROUP_IDENTIFIER, (LPCWSTR)szGroup, NULL);
					BreakExitOnFailure(hr, "Failed creating multstring.");
				}
				else
				{
					CWixString szTmp;

					hr = szTmp.Format(L"%c%s", SC_GROUP_IDENTIFIER, (LPCWSTR)szGroup);
					BreakExitOnFailure(hr, "Failed formatting string");

					hr = MultiSzInsertString(&szDependencies, nullptr, 0, (LPCWSTR)szTmp);
					BreakExitOnFailure(hr, "Failed inserting to multi-string");
				}
			}

			if (!szServiceFmt.IsNullOrEmpty())
			{
				CWixString szService;

				hr = szService.MsiFormat(szServiceFmt, nullptr);
				BreakExitOnFailure(hr, "Failed to format string");
				
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will add to service '%ls' dependency on service '%ls'", (LPCWSTR)szServiceName, (LPCWSTR)szService);
				if (szDependencies == nullptr)
				{
					hr = StrAllocFormatted(&szDependencies, L"%s%c", (LPCWSTR)szService, NULL);
					BreakExitOnFailure(hr, "Failed creating multstring.");
				}
				else
				{
					hr = MultiSzInsertString(&szDependencies, nullptr, 0, (LPCWSTR)szService);
					BreakExitOnFailure(hr, "Failed inserting to multi-string");
				}
			}
		}

		hr = oDeferred.AddServiceConfig(szServiceName, szCommand, szAccount, szPassword, start, szLoadOrderGroup, szDependencies, (ErrorHandling)errorHandling, (ServciceConfigDetails_DelayStart)nDelayStart);
		ExitOnFailure(hr, "Failed creating CustomActionData");

		// Get current service account.
		hService = ::OpenService(hManager, (LPCWSTR)szServiceName, SERVICE_QUERY_CONFIG);
		if (hService) // Won't fail if service doesn't exist
		{
			DWORD dwSize = 0;
			ServciceConfigDetails_DelayStart rlbkDelayStart = ServciceConfigDetails_DelayStart::ServciceConfigDetails_DelayStart_unchanged1;

			dwRes = ::QueryServiceConfig(hService, nullptr, 0, &dwSize);
			ExitOnNullWithLastError((dwRes || (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)) , hr, "Failed querying service '%ls' configuration size", (LPCWSTR)szServiceName);

			pServiceCfg = (QUERY_SERVICE_CONFIG*)new BYTE[dwSize];
			ExitOnNull(pServiceCfg, hr, E_FAIL, "Failed allocating memory");

			dwRes = ::QueryServiceConfig(hService, pServiceCfg, dwSize, &dwSize);
			ExitOnNullWithLastError(dwRes, hr, "Failed querying service '%ls' configuration", (LPCWSTR)szServiceName);

			if (start == ServciceConfigDetails::ServiceStart::ServciceConfigDetails_ServiceStart_unchanged)
			{
				pServiceCfg->dwStartType = SERVICE_NO_CHANGE;
			}
			if (start == ServciceConfigDetails::ServiceStart::ServciceConfigDetails_ServiceStart_auto_)
			{
				SERVICE_DELAYED_AUTO_START_INFO delayStart;
				DWORD dwJunk = 0;

				dwRes = ::QueryServiceConfig2(hService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, (LPBYTE)&delayStart, sizeof(delayStart), &dwJunk);
				ExitOnNullWithLastError(dwRes, hr, "Failed querying service '%ls' delay-start info", (LPCWSTR)szServiceName);

				rlbkDelayStart = delayStart.fDelayedAutostart ? ServciceConfigDetails_DelayStart::ServciceConfigDetails_DelayStart_yes : ServciceConfigDetails_DelayStart::ServciceConfigDetails_DelayStart_no;
			}

			hr = oRollback.AddServiceConfig((LPCWSTR)szServiceName, pServiceCfg->lpBinaryPathName, pServiceCfg->lpServiceStartName, nullptr, pServiceCfg->dwStartType, pServiceCfg->lpLoadOrderGroup, szDependencies, ErrorHandling::ignore, rlbkDelayStart);
			ExitOnFailure(hr, "Failed creating rollback CustomActionData");

			delete[]pServiceCfg;
			pServiceCfg = nullptr;

			::CloseServiceHandle(hService);
			hService = nullptr;

			ReleaseNullStr(szDependencies);
		}
	}

	// Set CAD
	hr = oRollback.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaDoDeferredAction(L"PSW_ServiceConfigRlbk", szCustomActionData, oRollback.GetCost());
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferred.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data.");
	hr = WcaDoDeferredAction(L"PSW_ServiceConfigExec", szCustomActionData, oDeferred.GetCost());
	BreakExitOnFailure(hr, "Failed setting action data.");

LExit:
	ReleaseStr(szCustomActionData);
	ReleaseStr(szDependencies);

	if (pServiceCfg)
	{
		delete[]pServiceCfg;
	}
	if (hService)
	{
		::CloseServiceHandle(hService);
	}
	if (hManager)
	{
		::CloseServiceHandle(hManager);
	}

	dwRes = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(dwRes);
}

HRESULT CServiceConfig::AddServiceConfig(LPCWSTR szServiceName, LPCWSTR szCommandLine, LPCWSTR szAccount, LPCWSTR szPassword, int start, LPCWSTR szLoadOrderGroup, LPCWSTR szDependencies, ErrorHandling errorHandling, ServciceConfigDetails_DelayStart delayStart)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	ServciceConfigDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CServiceConfig", &pCmd);
	BreakExitOnFailure(hr, "Failed to add command");

	pDetails = new ServciceConfigDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_name(szServiceName, WSTR_BYTE_SIZE(szServiceName));
	if (szAccount && *szAccount)
	{
		pDetails->set_account(szAccount, WSTR_BYTE_SIZE(szAccount));
		if (szPassword && *szPassword)
		{
			pDetails->set_password(szPassword, WSTR_BYTE_SIZE(szPassword));
		}
	}
	if (szCommandLine && *szCommandLine)
	{
		pDetails->set_commandline(szCommandLine, WSTR_BYTE_SIZE(szCommandLine));
	}
	if (szLoadOrderGroup) // May be empty
	{
		pDetails->set_loadordergroup(szLoadOrderGroup, WSTR_BYTE_SIZE(szLoadOrderGroup));
	}
	if (szDependencies)
	{
		DWORD dwLen = 0;
		hr = ::MultiSzLen(szDependencies, &dwLen);
		BreakExitOnFailure(hr, "Failed getting multi-string length");
		dwLen *= sizeof(WCHAR);
		
		pDetails->set_dependencies(szDependencies, dwLen);
	}
	pDetails->set_start((ServciceConfigDetails::ServiceStart)start);
	pDetails->set_delaystart(delayStart);
	pDetails->set_errorhandling((ErrorHandling)errorHandling);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CServiceConfig::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	LPCWSTR szServiceName = nullptr;
	LPCWSTR szCommandLine = nullptr;
	LPCWSTR szAccount = nullptr;
	LPCWSTR szPassword = nullptr;
	LPCWSTR szLoadOrderGroup = nullptr;
	LPCWSTR szDependencies = nullptr;
	DWORD bRes = TRUE;
	ServciceConfigDetails details;

	bRes = details.ParseFromString(command);
	BreakExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking ExecOnDetails");

	szServiceName = (LPCWSTR)details.name().data();
	if (details.account().size() > sizeof(WCHAR)) // Larger than NULL
	{
		szAccount = (LPCWSTR)details.account().data();
		if (details.password().size() > sizeof(WCHAR))// Larger than NULL
		{
			szPassword = (LPCWSTR)details.password().data();
		}
	}
	if (details.commandline().size() > sizeof(WCHAR))
	{
		szCommandLine = (LPCWSTR)details.commandline().data();
	}
	if (details.loadordergroup().size() > 0) // May be empty
	{
		szLoadOrderGroup = (LPCWSTR)details.loadordergroup().data();
	}
	if (details.dependencies().size() > 0) // May be empty
	{
		szDependencies = (LPCWSTR)details.dependencies().data();
	}

LRetry:
	hr = ExecuteOne(szServiceName, szCommandLine, szAccount, szPassword, details.start(), szLoadOrderGroup, szDependencies, details.delaystart());
	if (FAILED(hr))
	{
		hr = PromptError(szServiceName, details.errorhandling());
		if (hr == E_RETRY)
		{
			goto LRetry;
		}
	}
	ExitOnFailure(hr, "Failed configuring service '%ls'", szServiceName);

LExit:
	return hr;
}

HRESULT CServiceConfig::ExecuteOne(LPCWSTR szServiceName, LPCWSTR szCommandLine, LPCWSTR szAccount, LPCWSTR szPassword, DWORD dwStart, LPCWSTR szLoadOrderGroup, LPCWSTR szDependencies, ServciceConfigDetails_DelayStart nDelayStart)
{
	HRESULT hr = S_OK;
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL;
	DWORD dwRes = ERROR_SUCCESS;

	// Open service.
	hManager = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	ExitOnNullWithLastError(hManager, hr, "Failed opening service control manager database");

	hService = ::OpenService(hManager, szServiceName, SERVICE_ALL_ACCESS);
	ExitOnNullWithLastError(hService, hr, "Failed opening service '%ls'", szServiceName);

	// Configure.
	dwRes = ::ChangeServiceConfig(hService, SERVICE_NO_CHANGE, dwStart, SERVICE_NO_CHANGE, szCommandLine, szLoadOrderGroup, nullptr, szDependencies, szAccount, szPassword, nullptr);
	if (!dwRes && (::GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Service '%ls' is marked for deletion- reboot is required", szServiceName);
		hr = WcaDeferredActionRequiresReboot();
		ExitOnFailure(hr, "Failed requiring reboot");
		hr = S_FALSE;
		ExitFunction();
	}
	ExitOnNullWithLastError(dwRes, hr, "Failed configuring service '%ls'", szServiceName);

	if ((dwStart == SERVICE_AUTO_START) && (nDelayStart != ServciceConfigDetails_DelayStart::ServciceConfigDetails_DelayStart_unchanged1))
	{
		SERVICE_DELAYED_AUTO_START_INFO delayStart;
		delayStart.fDelayedAutostart = (nDelayStart == ServciceConfigDetails_DelayStart::ServciceConfigDetails_DelayStart_yes) ? TRUE : FALSE;

		dwRes = ::ChangeServiceConfig2(hService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &delayStart);
		ExitOnNullWithLastError(dwRes, hr, "Failed setting service '%ls' delay-start mode", szServiceName);
	}

LExit:

	if (hService)
	{
		::CloseServiceHandle(hService);
	}
	if (hManager)
	{
		::CloseServiceHandle(hManager);
	}
	return hr;
}

HRESULT CServiceConfig::PromptError(LPCWSTR szServiceName, ::com::panelsw::ca::ErrorHandling errorHandling)
{
	HRESULT hr = S_OK;

	switch (errorHandling)
	{
	case ErrorHandling::fail:
	default:
		hr = E_FAIL;
		break;

	case ErrorHandling::ignore:
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Ignoring service configuration failure");
		break;

	case ErrorHandling::prompt:
	{
		PMSIHANDLE hRec;
		UINT promptResult = IDOK;

		hRec = ::MsiCreateRecord(2);
		BreakExitOnNull(hRec, hr, E_FAIL, "Failed creating record");

		hr = WcaSetRecordInteger(hRec, 1, 27002);
		BreakExitOnFailure(hr, "Failed setting record integer");

		hr = WcaSetRecordString(hRec, 2, szServiceName);
		BreakExitOnFailure(hr, "Failed setting record string");

		promptResult = WcaProcessMessage((INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_ERROR | MB_ABORTRETRYIGNORE | MB_DEFBUTTON1 | MB_ICONERROR), hRec);
		switch (promptResult)
		{
		case IDABORT:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User aborted on failure to configure '%ls' service", szServiceName);
			hr = E_FAIL;
			break;

		case IDRETRY:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User chose to retry on failure to configure '%ls' service", szServiceName);
			hr = E_RETRY;
			break;

		case IDIGNORE:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User ignored failure to configure '%ls' service", szServiceName);
			break;

		case IDCANCEL:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User canceled on failure to configure '%ls' service", szServiceName);
			BreakExitOnWin32Error(ERROR_INSTALL_USEREXIT, hr, "Cancelling");
			break;

		default: // Probably silent (result 0)
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Failure to configure '%ls' service. Prompt result is 0x%08X", szServiceName, promptResult);
			hr = E_FAIL;
			break;
		}
		break;
	}
	}

LExit:
	return hr;
}

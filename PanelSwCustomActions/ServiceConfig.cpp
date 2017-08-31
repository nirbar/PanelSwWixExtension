#include "ServiceConfig.h"
#include "RegistryKey.h"
#include "../CaCommon/WixString.h"
#include <wcautil.h>
#include <procutil.h>

#define ServiceConfig_QUERY L"SELECT `Id`, `Component_`, `ServiceName`, `Account`, `Password` FROM `PSW_ServiceConfig`"
enum ServiceConfigQuery { Id = 1, Component, ServiceName, Account, Password };

extern "C" __declspec(dllexport) UINT ServiceConfig(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	DWORD dwRes = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CComBSTR szCustomActionData;
	CServiceConfig oDeferred;
	CServiceConfig oRollback;
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL;
	QUERY_SERVICE_CONFIG *pServiceCfg = NULL;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure tables exist.
	hr = WcaTableExists(L"PSW_ServiceConfig");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_ServiceConfig'. Have you authored 'PanelSw:ServiceConfig' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(ServiceConfig_QUERY, &hView);
	BreakExitOnFailure1(hr, "Failed to execute SQL query '%ls'.", ServiceConfig_QUERY);

	// Open service.
	hManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	ExitOnNullWithLastError(hManager, hr, "Failed opening service control manager database");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{        
        BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szComponent, szServiceName, szAccount, szPassword;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, ServiceConfigQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, ServiceConfigQuery::Component, (LPWSTR*)szComponent);
		BreakExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, ServiceConfigQuery::ServiceName, (LPWSTR*)szServiceName);
		BreakExitOnFailure(hr, "Failed to get ServiceName.");
		hr = WcaGetRecordFormattedString(hRecord, ServiceConfigQuery::Account, (LPWSTR*)szAccount);
		BreakExitOnFailure(hr, "Failed to get Account.");
		hr = WcaGetRecordFormattedString(hRecord, ServiceConfigQuery::Password, (LPWSTR*)szPassword);
		BreakExitOnFailure(hr, "Failed to get Password.");

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		if (compAction != WCA_TODO::WCA_TODO_INSTALL)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping configuration of service '%ls' since component is not installed", (LPCWSTR)szServiceName);
			continue;
		}

		hr = oDeferred.AddServiceConfig(szServiceName, szAccount, szPassword);
		ExitOnFailure(hr, "Failed creating CustomActionData");

		// Get current service account.
		hService = ::OpenService(hManager, (LPCWSTR)szServiceName, SERVICE_QUERY_CONFIG);
		if (hService) // Won't fail if service doesn't exist
		{
			DWORD dwSize = 0;

			dwRes = ::QueryServiceConfig(hService, NULL, 0, &dwSize);
			ExitOnNullWithLastError((dwRes || (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)) , hr, "Failed querying service '%ls' configuration size", (LPCWSTR)szServiceName);

			pServiceCfg = (QUERY_SERVICE_CONFIG*)new BYTE[dwSize];
			ExitOnNull(pServiceCfg, hr, E_FAIL, "Failed allocating memory");

			dwRes = ::QueryServiceConfig(hService, pServiceCfg, dwSize, &dwSize);
			ExitOnNullWithLastError(dwRes, hr, "Failed querying service '%ls' configuration", (LPCWSTR)szServiceName);

			hr = oRollback.AddServiceConfig((LPCWSTR)szServiceName, pServiceCfg->lpServiceStartName, NULL);
			ExitOnFailure(hr, "Failed creating rollback CustomActionData");

			delete[]pServiceCfg;
			pServiceCfg = NULL;

			::CloseServiceHandle(hService);
			hService = NULL;
		}
	}

	// Set CAD
	hr = oRollback.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaDoDeferredAction(L"PSW_ServiceConfigRlbk", szCustomActionData, oRollback.GetCost());
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	szCustomActionData.Empty();
	hr = oDeferred.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data.");
	hr = WcaDoDeferredAction(L"PSW_ServiceConfigExec", szCustomActionData, oDeferred.GetCost());
	BreakExitOnFailure(hr, "Failed setting action data.");

LExit:

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

HRESULT CServiceConfig::AddServiceConfig(LPCWSTR szServiceName, LPCWSTR szAccount, LPCWSTR szPassword)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"ServiceConfig", L"CServiceConfig", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("ServiceName"), CComVariant(szServiceName));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Flags'");

	hr = pElem->setAttribute(CComBSTR("Account"), CComVariant(szAccount));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Account'");

	if (szPassword && *szPassword)
	{
		hr = pElem->setAttribute(CComBSTR("Password"), CComVariant(szPassword));
		BreakExitOnFailure(hr, "Failed to add XML attribute 'Password'");
	}

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CServiceConfig::DeferredExecute(IXMLDOMElement* pElem)
{
	HRESULT hr = S_OK;
	CComVariant szServiceName;
	CComVariant szAccount;
	CComVariant szPassword;
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL;
	DWORD dwRes = ERROR_SUCCESS;	

	hr = pElem->getAttribute(L"ServiceName", &szServiceName);
	BreakExitOnFailure(hr, "Failed to get XML attribute 'ServiceName'");
	ExitOnNull(*szServiceName.bstrVal, hr, E_INVALIDARG, "ServiceName is empty");

	hr = pElem->getAttribute(L"Account", &szAccount);
	BreakExitOnFailure(hr, "Failed to get XML attribute 'Account'");
	ExitOnNull(*szAccount.bstrVal, hr, E_INVALIDARG, "Account is empty");

	hr = pElem->getAttribute(L"Password", &szPassword);
	BreakExitOnFailure(hr, "Failed to get XML attribute 'Password'");

	// No password?
	if (!szPassword.bstrVal || !(*szPassword.bstrVal))
	{
		szPassword.Clear();
	}

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Changing service '%ls' start account to '%ls'", szServiceName.bstrVal, szAccount.bstrVal);

	// Open service.
	hManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	ExitOnNullWithLastError(hManager, hr, "Failed opening service control manager database");

	hService = ::OpenService(hManager, szServiceName.bstrVal, SERVICE_ALL_ACCESS);
	ExitOnNullWithLastError(hService, hr, "Failed opening service '%ls'", szServiceName.bstrVal);

	// Change user.
	dwRes = ::ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, szAccount.bstrVal, szPassword.bstrVal, NULL);
	ExitOnNullWithLastError(dwRes, hr, "Failed changing service '%ls' login account to '%ls'", szServiceName.bstrVal, szAccount.bstrVal);

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

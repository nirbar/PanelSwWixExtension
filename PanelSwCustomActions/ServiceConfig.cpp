#include "ServiceConfig.h"
#include "RegistryKey.h"
#include "../CaCommon/WixString.h"
#include <wcautil.h>
#include <procutil.h>
#include "servciceConfigDetails.pb.h"
#include "google\protobuf\any.h"
using namespace com::panelsw::ca;
using namespace google::protobuf;

#define ServiceConfig_QUERY L"SELECT `Id`, `Component_`, `ServiceName`, `Account`, `Password` FROM `PSW_ServiceConfig`"
enum ServiceConfigQuery { Id = 1, Component, ServiceName, Account, Password };

extern "C" __declspec(dllexport) UINT ServiceConfig(MSIHANDLE hInstall)
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
	hManager = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
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

			dwRes = ::QueryServiceConfig(hService, nullptr, 0, &dwSize);
			ExitOnNullWithLastError((dwRes || (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)) , hr, "Failed querying service '%ls' configuration size", (LPCWSTR)szServiceName);

			pServiceCfg = (QUERY_SERVICE_CONFIG*)new BYTE[dwSize];
			ExitOnNull(pServiceCfg, hr, E_FAIL, "Failed allocating memory");

			dwRes = ::QueryServiceConfig(hService, pServiceCfg, dwSize, &dwSize);
			ExitOnNullWithLastError(dwRes, hr, "Failed querying service '%ls' configuration", (LPCWSTR)szServiceName);

			hr = oRollback.AddServiceConfig((LPCWSTR)szServiceName, pServiceCfg->lpServiceStartName, nullptr);
			ExitOnFailure(hr, "Failed creating rollback CustomActionData");

			delete[]pServiceCfg;
			pServiceCfg = nullptr;

			::CloseServiceHandle(hService);
			hService = nullptr;
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
	::com::panelsw::ca::Command *pCmd = nullptr;
	ServciceConfigDetails *pDetails = nullptr;
	Any *pAny = nullptr;

	hr = AddCommand("CServiceConfig", &pCmd);
	BreakExitOnFailure(hr, "Failed to add XML element");

	pDetails = new ServciceConfigDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pAny = new Any();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	pCmd->set_allocated_details(pAny);
	pDetails->set_name(szServiceName, WSTR_BYTE_SIZE(szServiceName));
	pDetails->set_account(szAccount, WSTR_BYTE_SIZE(szAccount));
	if (szPassword && *szPassword)
	{
		pDetails->set_password(szPassword, WSTR_BYTE_SIZE(szPassword));
	}
	pAny->PackFrom(*pDetails);

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CServiceConfig::DeferredExecute(const ::google::protobuf::Any* pCommand)
{
	HRESULT hr = S_OK;
	LPCWSTR szServiceName = nullptr;
	LPCWSTR szAccount = nullptr;
	LPCWSTR szPassword = nullptr;
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL;
	DWORD dwRes = ERROR_SUCCESS;
	DWORD bRes = TRUE;
	ServciceConfigDetails details;

	BreakExitOnNull(pCommand->Is<ServciceConfigDetails>(), hr, E_INVALIDARG, "Expected command to be ExecOnDetails");
	bRes = pCommand->UnpackTo(&details);
	BreakExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking ExecOnDetails");

	szServiceName = (LPCWSTR)details.name().data();
	szAccount = (LPCWSTR)details.account().data();
	if (!details.password().empty())
	{
		szPassword = (LPCWSTR)details.password().data();
	}

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Changing service '%ls' start account to '%ls'", szServiceName, szAccount);

	// Open service.
	hManager = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	ExitOnNullWithLastError(hManager, hr, "Failed opening service control manager database");

	hService = ::OpenService(hManager, szServiceName, SERVICE_ALL_ACCESS);
	ExitOnNullWithLastError(hService, hr, "Failed opening service '%ls'", szServiceName);

	// Change user.
	dwRes = ::ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, szAccount, szPassword, nullptr);
	ExitOnNullWithLastError(dwRes, hr, "Failed changing service '%ls' login account to '%ls'", szServiceName, szAccount);

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

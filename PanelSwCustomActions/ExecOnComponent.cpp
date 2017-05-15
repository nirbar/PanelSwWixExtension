#include "ExecOnComponent.h"
#include "../CaCommon/WixString.h"
#include <wcautil.h>

#define ExecOnComponent_QUERY L"SELECT `Id`, `Component_`, `Command`, `Flags` FROM `PSW_ExecOnComponent` ORDER BY `Order`"
enum ExecOnComponentQuery { Id = 1, Component = 2, Command = 3, Flags = 4 };

enum Flags
{
	None = 0,

	// Action
	OnInstall = 1,
	OnRemove = 2 * OnInstall,
	OnReinstall = 2 * OnRemove,

	// Action rollback
	OnInstallRollback = 2 * OnReinstall,
	OnRemoveRollback = 2 * OnInstallRollback,
	OnReinstallRollback = 2 * OnRemoveRollback,

	// Schedule
	BeforeStopServices = 2 * OnReinstallRollback,
	AfterStopServices = 2 * BeforeStopServices,
	BeforeStartServices = 2 * AfterStopServices,
	AfterStartServices = 2 * BeforeStartServices,

	// Return
	IgnoreExitCode = 2 * AfterStartServices,

	// Impersonate
	Impersonate = 2 * IgnoreExitCode,
};

static HRESULT ScheduleExecution(LPCWSTR szId, LPCWSTR szCommand, int nFlags, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp);

extern "C" __declspec(dllexport) UINT ExecOnComponent(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CComBSTR szCustomActionData;
	CExecOnComponent oDeferredBeforeStop, oDeferredAfterStop, oDeferredBeforeStart, oDeferredAfterStart;
	CExecOnComponent oRollbackBeforeStop, oRollbackAfterStop, oRollbackBeforeStart, oRollbackAfterStart;
	CExecOnComponent oDeferredBeforeStopImp, oDeferredAfterStopImp, oDeferredBeforeStartImp, oDeferredAfterStartImp;
	CExecOnComponent oRollbackBeforeStopImp, oRollbackAfterStopImp, oRollbackBeforeStartImp, oRollbackAfterStartImp;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_FileRegex exists.
	hr = WcaTableExists(L"PSW_ExecOnComponent");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_ExecOnComponent'. Have you authored 'PanelSw:ExecOnComponent' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(ExecOnComponent_QUERY, &hView);
	BreakExitOnFailure1(hr, "Failed to execute SQL query '%ls'.", ExecOnComponent_QUERY);
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szComponent, szCommand;
		int nFlags = 0;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, ExecOnComponentQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, ExecOnComponentQuery::Component, (LPWSTR*)szComponent);
		BreakExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, ExecOnComponentQuery::Command, (LPWSTR*)szCommand);
		BreakExitOnFailure(hr, "Failed to get Command.");
		hr = WcaGetRecordInteger(hRecord, ExecOnComponentQuery::Flags, &nFlags);
		BreakExitOnFailure(hr, "Failed to get Flags.");

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
			if (nFlags & Flags::OnInstall)
			{
				hr = ScheduleExecution(szId, szCommand, nFlags, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnInstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, nFlags, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_REINSTALL:
			if (nFlags & Flags::OnReinstall)
			{
				hr = ScheduleExecution(szId, szCommand, nFlags, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnReinstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, nFlags, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			if (nFlags & Flags::OnRemove)
			{
				hr = ScheduleExecution(szId, szCommand, nFlags, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnRemoveRollback)
			{
				hr = ScheduleExecution(szId, szCommand, nFlags, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGMSG_STANDARD, "Component '%ls' action is unknown. Skipping execution of '%ls'.", (LPCWSTR)szComponent, (LPCWSTR)szId);
			break;
		}
	}

	// Rollback actions
	hr = oRollbackBeforeStop.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStop_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	szCustomActionData.Empty();
	hr = oRollbackAfterStop.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStop_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	szCustomActionData.Empty();
	hr = oRollbackBeforeStart.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStart_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	szCustomActionData.Empty();
	hr = oRollbackAfterStart.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStart_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	// Deferred actions
	szCustomActionData.Empty();
	hr = oDeferredBeforeStop.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStop_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	szCustomActionData.Empty();
	hr = oDeferredAfterStop.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStop_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	szCustomActionData.Empty();
	hr = oDeferredBeforeStart.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStart_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	szCustomActionData.Empty();
	hr = oDeferredAfterStart.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStart_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	// Rollback actions, impersonated
	szCustomActionData.Empty();
	hr = oRollbackBeforeStopImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStop_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	szCustomActionData.Empty();
	hr = oRollbackAfterStopImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStop_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	szCustomActionData.Empty();
	hr = oRollbackBeforeStartImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStart_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	szCustomActionData.Empty();
	hr = oRollbackAfterStartImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStart_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	// Deferred actions, impersonated
	szCustomActionData.Empty();
	hr = oDeferredBeforeStopImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStop_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	szCustomActionData.Empty();
	hr = oDeferredAfterStopImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStop_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	szCustomActionData.Empty();
	hr = oDeferredBeforeStartImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStart_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	szCustomActionData.Empty();
	hr = oDeferredAfterStartImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStart_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data."); 

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT ScheduleExecution(LPCWSTR szId, LPCWSTR szCommand, int nFlags, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp)
{
	HRESULT hr = S_OK;

	if (nFlags & Flags::BeforeStopServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' before StopServices", (LPCWSTR)szCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStopImp->AddExec(szCommand, nFlags);
		}
		else
		{
			hr = pBeforeStop->AddExec(szCommand, nFlags);
		}
		BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStopServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' after StopServices", (LPCWSTR)szCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStopImp->AddExec(szCommand, nFlags);
		}
		else
		{
			hr = pAfterStop->AddExec(szCommand, nFlags);
		}
		BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::BeforeStartServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' before StartServices", (LPCWSTR)szCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStartImp->AddExec(szCommand, nFlags);
		}
		else
		{
			hr = pBeforeStart->AddExec(szCommand, nFlags);
		}
		BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStartServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' after StartServices", (LPCWSTR)szCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStartImp->AddExec(szCommand, nFlags);
		}
		else
		{
			hr = pAfterStart->AddExec(szCommand, nFlags);
		}
		BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}

LExit:
	return hr;
}

HRESULT CExecOnComponent::AddExec(LPCWSTR szCommand, int nFlags)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"ExecOnComponent", L"CExecOnComponent", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("Command"), CComVariant(szCommand));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Command'");

	hr = pElem->setAttribute(CComBSTR("Flags"), CComVariant(nFlags));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Flags'");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CExecOnComponent::DeferredExecute(IXMLDOMElement* pElem)
{
	HRESULT hr = S_OK;
	CComVariant vCommand;
	CComVariant vFlags;
	int nFlags;

	hr = pElem->getAttribute(L"Command", &vCommand);
	BreakExitOnFailure(hr, "Failed to get XML attribute 'Command'");

	hr = pElem->getAttribute(L"Flags", &vFlags);
	BreakExitOnFailure(hr, "Failed to get XML attribute 'Flags'");
	nFlags = ::_wtoi(vFlags.bstrVal);

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing '%ls'", vCommand.bstrVal);
	hr = QuietExec(vCommand.bstrVal, INFINITE);
	if (FAILED(hr) && (nFlags & Flags::IgnoreExitCode))
	{
		WcaLogError(hr, "Ignoring command '%ls' exit code", vCommand.bstrVal);
		ExitFunction1(hr = S_FALSE);
	}
	BreakExitOnFailure1(hr, "Failed to execute command '%ls'", vCommand.bstrVal);

LExit:
	return hr;
}

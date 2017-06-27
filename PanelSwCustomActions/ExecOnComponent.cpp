#include "ExecOnComponent.h"
#include "RegistryKey.h"
#include "../CaCommon/WixString.h"
#include <wcautil.h>
#include <procutil.h>

#define ExecOnComponent_QUERY L"SELECT `Id`, `Component_`, `Command`, `Flags` FROM `PSW_ExecOnComponent` ORDER BY `Order`"
enum ExecOnComponentQuery { Id = 1, Component = 2, Command = 3, Flags = 4 };

#define ExecOnComponentExitCode_QUERY_Fmt L"SELECT `From`, `To` FROM `PSW_ExecOnComponent_ExitCode` WHERE `ExecOnId_`='%s'"
enum ExecOnComponentExitCodeQuery { From = 1, To = 2 };

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

    // Not waiting
    ASync = 2 * IgnoreExitCode,

	// Impersonate
	Impersonate = 2 * ASync,
};

static HRESULT ScheduleExecution(LPCWSTR szId, LPCWSTR szCommand, CExecOnComponent::ExitCodeMap *pExitCodeMap, int nFlags, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp);
static HRESULT RefreshEnvironment();

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

	// Ensure tables exist.
	hr = WcaTableExists(L"PSW_ExecOnComponent");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_ExecOnComponent'. Have you authored 'PanelSw:ExecOnComponent' entries in WiX code?");
    hr = WcaTableExists(L"PSW_ExecOnComponent_ExitCode");
    BreakExitOnFailure(hr, "Table does not exist 'PSW_ExecOnComponent_ExitCode'. Have you authored 'PanelSw:ExecOnComponent' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(ExecOnComponent_QUERY, &hView);
	BreakExitOnFailure1(hr, "Failed to execute SQL query '%ls'.", ExecOnComponent_QUERY);

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{        
        BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
        PMSIHANDLE hExitCodeView;
        PMSIHANDLE hExitCodeRecord;
		CWixString szId, szComponent, szCommand;
        CWixString szExitCodeQuery;
		int nFlags = 0;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
        CExecOnComponent::ExitCodeMap exitCodeMap;

		hr = WcaGetRecordString(hRecord, ExecOnComponentQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, ExecOnComponentQuery::Component, (LPWSTR*)szComponent);
		BreakExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, ExecOnComponentQuery::Command, (LPWSTR*)szCommand);
		BreakExitOnFailure(hr, "Failed to get Command.");
        hr = WcaGetRecordInteger(hRecord, ExecOnComponentQuery::Flags, &nFlags);
        BreakExitOnFailure(hr, "Failed to get Flags.");

        // Get exit code map (i.e. map exit code 1 to success)
        hr = szExitCodeQuery.Format(ExecOnComponentExitCode_QUERY_Fmt, (LPCWSTR)szId);
        BreakExitOnFailure(hr, "Failed to format string");

        hr = WcaOpenExecuteView((LPCWSTR)szExitCodeQuery, &hExitCodeView);
        BreakExitOnFailure1(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szExitCodeQuery);

        // Iterate records
        while ((hr = WcaFetchRecord(hExitCodeView, &hExitCodeRecord)) != E_NOMOREITEMS)
        {
            BreakExitOnFailure(hr, "Failed to fetch record.");
            int nFrom, nTo;

            hr = WcaGetRecordInteger(hExitCodeRecord, ExecOnComponentExitCodeQuery::From, &nFrom);
            BreakExitOnFailure(hr, "Failed to get From.");
            hr = WcaGetRecordInteger(hExitCodeRecord, ExecOnComponentExitCodeQuery::To, &nTo);
            BreakExitOnFailure(hr, "Failed to get To.");

            exitCodeMap[nFrom] = nTo;
        }

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
			if (nFlags & Flags::OnInstall)
			{
				hr = ScheduleExecution(szId, szCommand, &exitCodeMap, nFlags, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnInstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, &exitCodeMap, nFlags, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_REINSTALL:
			if (nFlags & Flags::OnReinstall)
			{
				hr = ScheduleExecution(szId, szCommand, &exitCodeMap, nFlags, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnReinstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, &exitCodeMap, nFlags, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			if (nFlags & Flags::OnRemove)
			{
				hr = ScheduleExecution(szId, szCommand, &exitCodeMap, nFlags, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnRemoveRollback)
			{
				hr = ScheduleExecution(szId, szCommand, &exitCodeMap, nFlags, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
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

HRESULT ScheduleExecution(LPCWSTR szId, LPCWSTR szCommand, CExecOnComponent::ExitCodeMap* pExitCodeMap, int nFlags, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp)
{
	HRESULT hr = S_OK;

	if (nFlags & Flags::BeforeStopServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' before StopServices", (LPCWSTR)szCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStopImp->AddExec(szCommand, pExitCodeMap, nFlags);
		}
		else
		{
			hr = pBeforeStop->AddExec(szCommand, pExitCodeMap, nFlags);
		}
		BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStopServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' after StopServices", (LPCWSTR)szCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStopImp->AddExec(szCommand, pExitCodeMap, nFlags);
		}
		else
		{
			hr = pAfterStop->AddExec(szCommand, pExitCodeMap, nFlags);
		}
		BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::BeforeStartServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' before StartServices", (LPCWSTR)szCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStartImp->AddExec(szCommand, pExitCodeMap, nFlags);
		}
		else
		{
			hr = pBeforeStart->AddExec(szCommand, pExitCodeMap, nFlags);
		}
		BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStartServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' after StartServices", (LPCWSTR)szCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStartImp->AddExec(szCommand, pExitCodeMap, nFlags);
		}
		else
		{
			hr = pAfterStart->AddExec(szCommand, pExitCodeMap, nFlags);
		}
		BreakExitOnFailure1(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}

LExit:
	return hr;
}

HRESULT CExecOnComponent::AddExec(LPCWSTR szCommand, ExitCodeMap* pExitCodeMap, int nFlags)
{
    HRESULT hr = S_OK;
    CComPtr<IXMLDOMElement> pElem;
    CComPtr<IXMLDOMDocument> pDoc;
    ExitCodeMapItr itCurr, itEnd;

    hr = AddElement(L"ExecOnComponent", L"CExecOnComponent", 1, &pElem);
    BreakExitOnFailure(hr, "Failed to add XML element");

    hr = pElem->setAttribute(CComBSTR("Command"), CComVariant(szCommand));
    BreakExitOnFailure(hr, "Failed to add XML attribute 'Command'");

    hr = pElem->setAttribute(CComBSTR("Flags"), CComVariant(nFlags));
    BreakExitOnFailure(hr, "Failed to add XML attribute 'Flags'");

    hr = pElem->get_ownerDocument(&pDoc);
    BreakExitOnFailure(hr, "Failed to get XML document");

    itEnd = pExitCodeMap->end();
    for (itCurr = pExitCodeMap->begin(); itCurr != itEnd; ++itCurr)
    {
        CComPtr<IXMLDOMElement> child;
        CComPtr<IXMLDOMNode> tmpNode;

        hr = pDoc->createElement(L"ExitCode", &child);
        BreakExitOnFailure(hr, "Failed to create XML element 'ExitCode'");

        hr = child->setAttribute(L"From", CComVariant(itCurr->first));
        BreakExitOnFailure(hr, "Failed to set 'From' attribute");

        hr = child->setAttribute(L"To", CComVariant(itCurr->second));
        BreakExitOnFailure(hr, "Failed to set 'To' attribute");

        hr = pElem->appendChild(child, &tmpNode);
        BreakExitOnFailure(hr, "Failed to append XML element");
    }

LExit:
    return hr;
}

// Execute the command object (XML element)
HRESULT CExecOnComponent::DeferredExecute(IXMLDOMElement* pElem)
{
	HRESULT hr = S_OK;
	CComVariant vCommand;
    CComVariant vFlags;
    CComPtr<IXMLDOMNodeList> childNodes;
    LONG childCount = 0;
    ExitCodeMap exitCodeMap;
	int nFlags;
    DWORD exitCode = 0;

    hr = RefreshEnvironment();
    if (FAILED(hr))
    {
        WcaLogError(hr, "Failed refreshing environment. Ignoring error.");
        hr = S_OK;
    }

	hr = pElem->getAttribute(L"Command", &vCommand);
	BreakExitOnFailure(hr, "Failed to get XML attribute 'Command'");

	hr = pElem->getAttribute(L"Flags", &vFlags);
	BreakExitOnFailure(hr, "Failed to get XML attribute 'Flags'");
	nFlags = ::_wtoi(vFlags.bstrVal);

    hr = pElem->get_childNodes(&childNodes);
    BreakExitOnFailure(hr, "Failed to get XML child nodes");

    hr = childNodes->get_length(&childCount);
    BreakExitOnFailure(hr, "Failed to get child node count");

    for (LONG i = 0; i < childCount; ++i)
    {
        CComPtr<IXMLDOMNode> node;
        CComPtr<IXMLDOMElement> child;
        DOMNodeType nodeType;
        CComVariant vFrom;
        CComVariant vTo;
        CComBSTR chileName;
        DWORD from;
        DWORD to;

        hr = childNodes->get_item(i, &node);
        BreakExitOnFailure(hr, "Failed to get node");
        BreakExitOnNull(node, hr, E_FAIL, "Failed to get node");

        hr = node->get_nodeType(&nodeType);
        BreakExitOnFailure(hr, "Failed to get node type");
        if (nodeType != DOMNodeType::NODE_ELEMENT)
        {
            hr = E_FAIL;
            BreakExitOnFailure(hr, "Expected an element");
        }

        hr = node->QueryInterface(IID_IXMLDOMElement, (void**)&child);
        BreakExitOnFailure(hr, "Failed quering as IID_IXMLDOMElement");
        BreakExitOnNull(child, hr, E_FAIL, "Failed to get IID_IXMLDOMElement");

        hr = child->get_nodeName(&chileName);
        BreakExitOnFailure(hr, "Failed getting child name");

        if (0 != ::wcscmp(L"ExitCode", (LPWSTR)chileName))
        {
            hr = E_INVALIDARG;
            BreakExitOnFailure1(hr, "Unexpected child element '%ls'", (LPWSTR)chileName);
        }

        hr = child->getAttribute(L"From", &vFrom);
        BreakExitOnFailure(hr, "Failed to get XML attribute 'From'");
        from = ::_wtoi(vFrom.bstrVal);

        hr = child->getAttribute(L"To", &vTo);
        BreakExitOnFailure(hr, "Failed to get XML attribute 'To'");
        to = ::_wtoi(vTo.bstrVal);

        exitCodeMap[from] = to;
    }

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing '%ls'", vCommand.bstrVal);
    if ((nFlags & Flags::ASync) == Flags::ASync)
    {
        HANDLE hProc = NULL;

        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Not logging output on async command");

        hr = ProcExecute(vCommand.bstrVal, &hProc, NULL, NULL);
        BreakExitOnFailure1(hr, "Failed to launch command '%ls'", vCommand.bstrVal);
        hr = S_OK;

        if ((hProc != NULL) && (hProc != INVALID_HANDLE_VALUE))
        {
            ::CloseHandle(hProc);
        }
        ExitFunction();
    }
    else
    {
        hr = QuietExec(vCommand.bstrVal, INFINITE);
    }

    // Parse exit code.
    exitCode = HRESULT_CODE(hr);
    if (exitCodeMap.find(exitCode) != exitCodeMap.end())
    {
        exitCode = exitCodeMap[exitCode];
    }
    switch (exitCode)
    {
    case ERROR_SUCCESS_REBOOT_INITIATED:
    case ERROR_SUCCESS_REBOOT_REQUIRED:
    case ERROR_SUCCESS_RESTART_REQUIRED:
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Exit code is %u- reboot is required.", exitCode);
        WcaDeferredActionRequiresReboot();
        hr = S_OK;
        break;

    case ERROR_SUCCESS:
        hr = S_OK;
        break;

    default:
        break;
    }

	if (FAILED(hr) && (nFlags & Flags::IgnoreExitCode))
	{
		WcaLogError(hr, "Ignoring command '%ls' exit code", vCommand.bstrVal);
		ExitFunction1(hr = S_FALSE);
	}
	BreakExitOnFailure1(hr, "Failed to execute command '%ls'", vCommand.bstrVal);

LExit:
	return hr;
}

static HRESULT RefreshEnvironment()
{
    HRESULT hr = S_OK;
    BOOL bRes = TRUE;
    CRegistryKey envKey;
    CWixString szValueName;
    CRegistryKey::RegValueType valueType;

    hr = envKey.Open(CRegistryKey::RegRoot::LocalMachine, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", CRegistryKey::RegArea::X64, CRegistryKey::RegAccess::ReadOnly);
    BreakExitOnFailure(hr, "Failed to open environment registry key");

    for (DWORD dwIndex = 0; S_OK == envKey.EnumValues(dwIndex, (LPWSTR*)szValueName, &valueType); ++dwIndex)
    {
        if ((valueType == CRegistryKey::RegValueType::String) || (valueType == CRegistryKey::RegValueType::Expandable))
        {
            CWixString szValueData;

            hr = envKey.GetStringValue(szValueName, (LPWSTR*)szValueData);
            BreakExitOnFailure1(hr, "Failed to get environment variable '%ls' from registry key", (LPCWSTR)szValueName);

            WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Setting process environment variable '%ls'='%ls'", (LPCWSTR)szValueName, (LPCWSTR)szValueData);

            bRes = ::SetEnvironmentVariable(szValueName, szValueData);
            BreakExitOnNullWithLastError(bRes, hr, "Failed setting environment variable");
        }

        szValueName.Release();
    }
    BreakExitOnFailure(hr, "Failed enumerating environment registry key");

LExit:
    return hr;
}
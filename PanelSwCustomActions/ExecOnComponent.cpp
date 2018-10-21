#include "ExecOnComponent.h"
#include "RegistryKey.h"
#include "../CaCommon/WixString.h"
#include <wcautil.h>
#include <procutil.h>
#include "google\protobuf\any.h"
using namespace com::panelsw::ca;
using namespace google::protobuf;

#define ExecOnComponent_QUERY L"SELECT `Id`, `Component_`, `Command`, `WorkingDirectory`, `Flags` FROM `PSW_ExecOnComponent` ORDER BY `Order`"
enum ExecOnComponentQuery { Id = 1, Component = 2, Command = 3, WorkingDirectory = 4, Flags = 5 };

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

static HRESULT ScheduleExecution(LPCWSTR szId, LPCWSTR szCommand, LPCWSTR szObfuscatedCommand, LPCWSTR szWorkingDirectory, CExecOnComponent::ExitCodeMap *pExitCodeMap, CExecOnComponent::EnvironmentMap *pEnv, int nFlags, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp);

extern "C" UINT __stdcall ExecOnComponent(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPWSTR szCustomActionData = nullptr;
	LPWSTR szObfuscatedCommand = nullptr;
	CExecOnComponent oDeferredBeforeStop, oDeferredAfterStop, oDeferredBeforeStart, oDeferredAfterStart;
	CExecOnComponent oRollbackBeforeStop, oRollbackAfterStop, oRollbackBeforeStart, oRollbackAfterStart;
	CExecOnComponent oDeferredBeforeStopImp, oDeferredAfterStopImp, oDeferredBeforeStartImp, oDeferredAfterStartImp;
	CExecOnComponent oRollbackBeforeStopImp, oRollbackAfterStopImp, oRollbackBeforeStartImp, oRollbackAfterStartImp;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure tables exist.
	hr = WcaTableExists(L"PSW_ExecOnComponent");
	BreakExitOnFailure((hr == S_OK), "Table does not exist 'PSW_ExecOnComponent'. Have you authored 'PanelSw:ExecOnComponent' entries in WiX code?");
    hr = WcaTableExists(L"PSW_ExecOnComponent_ExitCode");
    BreakExitOnFailure((hr == S_OK), "Table does not exist 'PSW_ExecOnComponent_ExitCode'. Have you authored 'PanelSw:ExecOnComponent' entries in WiX code?");
	hr = WcaTableExists(L"PSW_ExecOnComponent_Environment");
	BreakExitOnFailure((hr == S_OK), "Table does not exist 'PSW_ExecOnComponent_Environment'. Have you authored 'PanelSw:ExecOnComponent' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(ExecOnComponent_QUERY, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", ExecOnComponent_QUERY);

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{        
        BreakExitOnFailure(hr, "Failed to fetch record.");
		ReleaseNullStr(szObfuscatedCommand);

		// Get fields
        PMSIHANDLE hSubView;
        PMSIHANDLE hSubRecord;
		CWixString szId, szComponent, szCommand, szCommandFormat, workDir;
        CWixString szSubQuery;
		int nFlags = 0;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		CExecOnComponent::ExitCodeMap exitCodeMap;
		std::map<std::string, std::string> environment;		

		hr = WcaGetRecordString(hRecord, ExecOnComponentQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, ExecOnComponentQuery::Component, (LPWSTR*)szComponent);
		BreakExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordString(hRecord, ExecOnComponentQuery::Command, (LPWSTR*)szCommandFormat);
		BreakExitOnFailure(hr, "Failed to get Command.");
		hr = WcaGetRecordFormattedString(hRecord, ExecOnComponentQuery::WorkingDirectory, (LPWSTR*)workDir);
		BreakExitOnFailure(hr, "Failed to get WorkingDirectory.");
		hr = WcaGetRecordInteger(hRecord, ExecOnComponentQuery::Flags, &nFlags);
        BreakExitOnFailure(hr, "Failed to get Flags.");

		hr = szCommand.MsiFormat((LPCWSTR)szCommandFormat, &szObfuscatedCommand);
		BreakExitOnFailure(hr, "Failed expanding command");

        // Get exit code map (i.e. map exit code 1 to success)
        hr = szSubQuery.Format(ExecOnComponentExitCode_QUERY_Fmt, (LPCWSTR)szId);
        BreakExitOnFailure(hr, "Failed to format string");

        hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
        BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

        // Iterate records
        while ((hr = WcaFetchRecord(hSubView, &hSubRecord)) != E_NOMOREITEMS)
        {
            BreakExitOnFailure(hr, "Failed to fetch record.");
            int nFrom, nTo;

            hr = WcaGetRecordInteger(hSubRecord, ExecOnComponentExitCodeQuery::From, &nFrom);
            BreakExitOnFailure(hr, "Failed to get From.");
            hr = WcaGetRecordInteger(hSubRecord, ExecOnComponentExitCodeQuery::To, &nTo);
            BreakExitOnFailure(hr, "Failed to get To.");

            exitCodeMap[nFrom] = nTo;
        }

		// Custom environment variables
		hr = szSubQuery.Format(L"SELECT `Name`, `Value` FROM `PSW_ExecOnComponent_Environment` WHERE `ExecOnId_`='%s'", (LPCWSTR)szId);
		BreakExitOnFailure(hr, "Failed to format string");

		hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
		BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

		// Iterate records
		while ((hr = WcaFetchRecord(hSubView, &hSubRecord)) != E_NOMOREITEMS)
		{
			BreakExitOnFailure(hr, "Failed to fetch record.");
			CWixString name, value;
			std::string nameA, valueA;

			hr = WcaGetRecordFormattedString(hSubRecord, 1, (LPWSTR*)name);
			BreakExitOnFailure(hr, "Failed to get From.");
			hr = WcaGetRecordFormattedString(hSubRecord, 2, (LPWSTR*)value);
			BreakExitOnFailure(hr, "Failed to get To.");

			nameA.assign((LPCSTR)(LPCWSTR)name, WSTR_BYTE_SIZE((LPCWSTR)name));
			valueA.assign((LPCSTR)(LPCWSTR)value, WSTR_BYTE_SIZE((LPCWSTR)value));
			environment[nameA] = valueA;
		}
		hr = S_OK;

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
			if (nFlags & Flags::OnInstall)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, &exitCodeMap, &environment, nFlags, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnInstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, &exitCodeMap, &environment, nFlags, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_REINSTALL:
			if (nFlags & Flags::OnReinstall)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, &exitCodeMap, &environment, nFlags, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnReinstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, &exitCodeMap, &environment, nFlags, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			if (nFlags & Flags::OnRemove)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, &exitCodeMap, &environment, nFlags, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnRemoveRollback)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, &exitCodeMap, &environment, nFlags, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
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

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackAfterStop.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStop_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackBeforeStart.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStart_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackAfterStart.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStart_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	// Deferred actions
	ReleaseNullStr(szCustomActionData);
	hr = oDeferredBeforeStop.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStop_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredAfterStop.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStop_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredBeforeStart.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStart_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredAfterStart.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStart_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	// Rollback actions, impersonated
	ReleaseNullStr(szCustomActionData);
	hr = oRollbackBeforeStopImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStop_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackAfterStopImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStop_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackBeforeStartImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStart_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackAfterStartImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStart_rollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting rollback action data.");

	// Deferred actions, impersonated
	ReleaseNullStr(szCustomActionData);
	hr = oDeferredBeforeStopImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStop_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredAfterStopImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStop_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredBeforeStartImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStart_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredAfterStartImp.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStart_deferred", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting deferred action data."); 

LExit:
	ReleaseStr(szCustomActionData);
	ReleaseStr(szObfuscatedCommand);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT ScheduleExecution(LPCWSTR szId, LPCWSTR szCommand, LPCWSTR szObfuscatedCommand, LPCWSTR szWorkingDirectory, CExecOnComponent::ExitCodeMap* pExitCodeMap, CExecOnComponent::EnvironmentMap *pEnv, int nFlags, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp)
{
	HRESULT hr = S_OK;

	if (nFlags & Flags::BeforeStopServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' before StopServices", szObfuscatedCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStopImp->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, pExitCodeMap, pEnv, nFlags);
		}
		else
		{
			hr = pBeforeStop->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, pExitCodeMap, pEnv, nFlags);
		}
		BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStopServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' after StopServices", szObfuscatedCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStopImp->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, pExitCodeMap, pEnv, nFlags);
		}
		else
		{
			hr = pAfterStop->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, pExitCodeMap, pEnv, nFlags);
		}
		BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::BeforeStartServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' before StartServices", szObfuscatedCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStartImp->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, pExitCodeMap, pEnv, nFlags);
		}
		else
		{
			hr = pBeforeStart->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, pExitCodeMap, pEnv, nFlags);
		}
		BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStartServices)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' after StartServices", szObfuscatedCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStartImp->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, pExitCodeMap, pEnv, nFlags);
		}
		else
		{
			hr = pAfterStart->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, pExitCodeMap, pEnv, nFlags);
		}
		BreakExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}

LExit:
	return hr;
}

HRESULT CExecOnComponent::AddExec(LPCWSTR szCommand, LPCWSTR szObfuscatedCommand, LPCWSTR szWorkingDirectory, ExitCodeMap* pExitCodeMap, EnvironmentMap *pEnv, int nFlags)
{
    HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	ExecOnDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

    hr = AddCommand("CExecOnComponent", &pCmd);
    BreakExitOnFailure(hr, "Failed to add command");

	pDetails = new ExecOnDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_command(szCommand, WSTR_BYTE_SIZE(szCommand));
	pDetails->set_obfuscatedcommand(szObfuscatedCommand, WSTR_BYTE_SIZE(szObfuscatedCommand));
	if (szWorkingDirectory && *szWorkingDirectory)
	{
		pDetails->set_workingdirectory(szWorkingDirectory, WSTR_BYTE_SIZE(szWorkingDirectory));
	}
	pDetails->set_async(nFlags & Flags::ASync);
	pDetails->set_ignoreerrors(nFlags & Flags::IgnoreExitCode);
	pDetails->mutable_exitcoderemap()->insert(pExitCodeMap->begin(), pExitCodeMap->end());
	pDetails->mutable_environment()->insert(pEnv->begin(), pEnv->end());

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
    return hr;
}

// Execute the command object (XML element)
HRESULT CExecOnComponent::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
    DWORD exitCode = 0;
	BOOL bRes = TRUE;
	ExecOnDetails details;
	LPCWSTR szCommand = nullptr;
	LPCWSTR szObfuscatedCommand = nullptr;
	LPCWSTR szWorkingDirectory = nullptr;

	bRes = details.ParseFromString(command);
	BreakExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking ExecOnDetails");

	szCommand = (LPCWSTR)details.command().c_str();
	szObfuscatedCommand = (LPCWSTR)details.obfuscatedcommand().c_str();
	szWorkingDirectory = (LPCWSTR)details.workingdirectory().c_str();

	hr = SetEnvironment(details.environment());
	if (FAILED(hr))
	{
		WcaLogError(hr, "Failed refreshing environment. Ignoring error.");
		hr = S_OK;
	}

	if (szWorkingDirectory && *szWorkingDirectory)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Setting working directory to '%ls'", szWorkingDirectory);
		::SetCurrentDirectory(szWorkingDirectory);
	}

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing '%ls'", szObfuscatedCommand);
    if (details.async())
    {
        HANDLE hProc = NULL;

        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Not logging output on async command");

        hr = ProcExecute(const_cast<LPWSTR>(szCommand), &hProc, nullptr, nullptr);
        BreakExitOnFailure(hr, "Failed to launch command '%ls'", szCommand);
        hr = S_OK;

        if (hProc && (hProc != INVALID_HANDLE_VALUE))
        {
            ::CloseHandle(hProc);
        }
        ExitFunction();
    }
    else
    {
        hr = QuietExec(const_cast<LPWSTR>(szCommand), INFINITE);
    }

    // Parse exit code.
    exitCode = HRESULT_CODE(hr);
	
    if (details.exitcoderemap().find(exitCode) != details.exitcoderemap().end())
    {
        exitCode = details.exitcoderemap().at(exitCode);
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

	if (FAILED(hr) && details.ignoreerrors())
	{
		WcaLogError(hr, "Ignoring command '%ls' exit code", szCommand);
		ExitFunction1(hr = S_FALSE);
	}
	BreakExitOnFailure(hr, "Failed to execute command '%ls'", szCommand);

LExit:
	return hr;
}

HRESULT CExecOnComponent::SetEnvironment(const ::google::protobuf::Map<std::string, std::string> &customEnv)
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
            BreakExitOnFailure(hr, "Failed to get environment variable '%ls' from registry key", (LPCWSTR)szValueName);

            bRes = ::SetEnvironmentVariable(szValueName, szValueData);
            BreakExitOnNullWithLastError(bRes, hr, "Failed setting environment variable '%ls'", (LPCWSTR)szValueName);
        }
        szValueName.Release();
    }
    BreakExitOnFailure(hr, "Failed enumerating environment registry key");

	for (::google::protobuf::Map<std::string, std::string>::const_iterator itCurr = customEnv.begin(), itEnd = customEnv.end(); itCurr != itEnd; ++itCurr)
	{
		LPCWSTR szName = (LPCWSTR)itCurr->first.c_str();
		LPCWSTR szValue = (LPCWSTR)itCurr->second.c_str();

		if (szName && *szName && szValue && *szValue)
		{
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Setting custom environment variable '%ls'", szName);

			bRes = ::SetEnvironmentVariable(szName, szValue);
			BreakExitOnNullWithLastError(bRes, hr, "Failed setting environment variable '%ls'", (LPCWSTR)szName);
		}
	}

LExit:
    return hr;
}
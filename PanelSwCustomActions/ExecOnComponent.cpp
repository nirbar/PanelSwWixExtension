#include "pch.h"
#include "ExecOnComponent.h"
#include "../CaCommon/RegistryKey.h"
#include "FileOperations.h"
#include <regex>
#include <shlwapi.h>
#include "google\protobuf\any.h"
using namespace std;
using namespace com::panelsw::ca;
using namespace google::protobuf;
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "Rpcrt4.lib")

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

	// Not waiting
	ASync = 2 * AfterStartServices,

	// Impersonate
	Impersonate = 2 * ASync,
};

static HRESULT ScheduleExecution(LPCWSTR szId, const CWixString& szCommand, LPCWSTR szWorkingDirectory, LPCWSTR szDomain, LPCWSTR szUser, LPCWSTR szPassword, CExecOnComponent::ExitCodeMap *pExitCodeMap, std::vector<ConsoleOuputRemap> *pConsoleOuput, CExecOnComponent::EnvironmentMap *pEnv, int nFlags, int errorHandling, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp);

extern "C" UINT __stdcall ExecOnComponent(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CExecOnComponent oDeferredBeforeStop, oDeferredAfterStop, oDeferredBeforeStart, oDeferredAfterStart;
	CExecOnComponent oRollbackBeforeStop, oRollbackAfterStop, oRollbackBeforeStart, oRollbackAfterStart;
	CExecOnComponent oDeferredBeforeStopImp, oDeferredAfterStopImp, oDeferredBeforeStartImp, oDeferredAfterStartImp;
	CExecOnComponent oRollbackBeforeStopImp, oRollbackAfterStopImp, oRollbackBeforeStartImp, oRollbackAfterStartImp;
	CFileOperations rollbackCAD;
	CFileOperations commitCAD;
	BYTE* pbData = nullptr;
	DWORD cbData = 0;
	DWORD dwRes = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure tables exist.
	hr = WcaTableExists(L"PSW_ExecOnComponent");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_ExecOnComponent'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_ExecOnComponent'. Have you authored 'PanelSw:ExecOn' entries in WiX code?");
	hr = WcaTableExists(L"PSW_ExecOnComponent_ExitCode");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_ExecOnComponent_ExitCode'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_ExecOnComponent_ExitCode'. Have you authored 'PanelSw:ExecOn' entries in WiX code?");
	hr = WcaTableExists(L"PSW_ExecOnComponent_Environment");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_ExecOnComponent_Environment'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_ExecOnComponent_Environment'. Have you authored 'PanelSw:ExecOn' entries in WiX code?");
	hr = WcaTableExists(L"PSW_ExecOn_ConsoleOutput");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_ExecOn_ConsoleOutput'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_ExecOn_ConsoleOutput'. Have you authored 'PanelSw:ExecOn' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `Component_`, `Binary_`, `Command`, `WorkingDirectory`, `Flags`, `ErrorHandling`, `User_` FROM `PSW_ExecOnComponent` ORDER BY `Order`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		PMSIHANDLE hSubView;
		PMSIHANDLE hSubRecord;
		CWixString szId, szComponent, szBinary, szCommand, szCommandFormat, workDir;
		CWixString userId, domain, user, password;
		CWixString szSubQuery;
		CWixString szTempFile;
		int nFlags = 0;
		int errorHandling = ErrorHandling::fail;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		CExecOnComponent::ExitCodeMap exitCodeMap;
		std::vector<ConsoleOuputRemap> consoleOutput;
		std::map<std::string, com::panelsw::ca::ObfuscatedString> environment;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)szBinary);
		ExitOnFailure(hr, "Failed to get Binary_.");
		hr = WcaGetRecordString(hRecord, 4, (LPWSTR*)szCommandFormat);
		ExitOnFailure(hr, "Failed to get Command.");
		hr = WcaGetRecordFormattedString(hRecord, 5, (LPWSTR*)workDir);
		ExitOnFailure(hr, "Failed to get WorkingDirectory.");
		hr = WcaGetRecordInteger(hRecord, 6, &nFlags);
		ExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordInteger(hRecord, 7, &errorHandling);
		ExitOnFailure(hr, "Failed to get ErrorHandling.");
		hr = WcaGetRecordString(hRecord, 8, (LPWSTR*)userId);
		ExitOnFailure(hr, "Failed to get User_.");

		// Execute from binary
		if (!szBinary.IsNullOrEmpty())
		{
			CWixString szReplaceMe;
			LPCWSTR szExtension = nullptr;

			hr = szSubQuery.Format(L"SELECT `Data` FROM `Binary` WHERE `Name`='%ls'", (LPCWSTR)szBinary);
			ExitOnFailure(hr, "Failed to format string");

			hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
			ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

			hr = WcaFetchRecord(hSubView, &hSubRecord);
			ExitOnFailure(hr, "Failed to fetch Binary record.");

			hr = WcaGetRecordStream(hSubRecord, 1, &pbData, &cbData);
			ExitOnFailure(hr, "Failed to read Binary.Data for certificate.");

			hSubRecord = NULL;
			hSubView = NULL;

			hr = PathCreateTempFile(nullptr, L"EXE%05i.tmp", INFINITE, L"EXE", FILE_ATTRIBUTE_NORMAL, (LPWSTR*)szTempFile, nullptr);
			ExitOnFailure(hr, "Failed getting temporary file name");

			szExtension = ::PathFindExtension((LPCWSTR)szBinary);
			if (szExtension && *szExtension)
			{
				::DeleteFile((LPCWSTR)szTempFile);
			
				dwRes = ::PathRenameExtension(szTempFile, szExtension);
				ExitOnNullWithLastError(dwRes, hr, "Failed renaming file extension '%ls' to '%ls'", (LPCWSTR)szTempFile, szExtension);
			}

			hr = szReplaceMe.Format(L"{*%ls}", (LPCWSTR)szBinary);
			ExitOnFailure(hr, "Failed to format string");

			hr = szCommandFormat.ReplaceAll((LPCWSTR)szReplaceMe, (LPCWSTR)szTempFile);
			ExitOnFailure(hr, "Failed to replace in string");

			hFile = ::CreateFile((LPCWSTR)szTempFile, GENERIC_ALL, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed opening file");

			dwRes = ::WriteFile(hFile, pbData, cbData, &cbData, nullptr);
			ExitOnNullWithLastError(dwRes, hr, "Failed writing to file");

			// Don't care if this is going to fail- just deleting a temporary file
			rollbackCAD.AddDeleteFile((LPCWSTR)szTempFile, CFileOperations::FileOperationsAttributes::IgnoreErrors | CFileOperations::FileOperationsAttributes::IgnoreMissingPath);
			commitCAD.AddDeleteFile((LPCWSTR)szTempFile, CFileOperations::FileOperationsAttributes::IgnoreErrors | CFileOperations::FileOperationsAttributes::IgnoreMissingPath);

			ReleaseNullMem(pbData);
			cbData = 0;
			if ((hFile != INVALID_HANDLE_VALUE) && (hFile != NULL))
			{
				::FlushFileBuffers(hFile);
				::CloseHandle(hFile);
				hFile = INVALID_HANDLE_VALUE;
			}
		}

		// Impersonate a user?
		if (!userId.IsNullOrEmpty())
		{
			hr = szSubQuery.Format(L"SELECT `Domain`, `Name`, `Password` FROM `Wix4User` WHERE `User`='%ls'", (LPCWSTR)userId);
			ExitOnFailure(hr, "Failed to format string");

			hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
			ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

			hr = WcaFetchRecord(hSubView, &hSubRecord);
			ExitOnFailure(hr, "Failed to fetch User record.");

			hr = WcaGetRecordFormattedString(hSubRecord, 1, (LPWSTR*)domain);
			ExitOnFailure(hr, "Failed to read User.Domain for impersonation");
			hr = WcaGetRecordFormattedString(hSubRecord, 2, (LPWSTR*)user);
			ExitOnFailure(hr, "Failed to read User.Name for impersonation");
			hr = WcaGetRecordFormattedString(hSubRecord, 3, (LPWSTR*)password);
			ExitOnFailure(hr, "Failed to read User.Password for impersonation");

			hSubRecord = NULL;
			hSubView = NULL;
		}

		hr = szCommand.MsiFormat((LPCWSTR)szCommandFormat);
		ExitOnFailure(hr, "Failed expanding command");

		// Get exit code map (i.e. map exit code 1 to success)
		hr = szSubQuery.Format(L"SELECT `From`, `To` FROM `PSW_ExecOnComponent_ExitCode` WHERE `ExecOnId_`='%ls'", (LPCWSTR)szId);
		ExitOnFailure(hr, "Failed to format string");

		hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
		ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

		// Iterate records
		while ((hr = WcaFetchRecord(hSubView, &hSubRecord)) != E_NOMOREITEMS)
		{
			ExitOnFailure(hr, "Failed to fetch record.");
			int nFrom, nTo;

			hr = WcaGetRecordInteger(hSubRecord, 1, &nFrom);
			ExitOnFailure(hr, "Failed to get From.");
			hr = WcaGetRecordInteger(hSubRecord, 2, &nTo);
			ExitOnFailure(hr, "Failed to get To.");

			exitCodeMap[nFrom] = nTo;
		}

		// Custom environment variables
		hr = szSubQuery.Format(L"SELECT `Name`, `Value` FROM `PSW_ExecOnComponent_Environment` WHERE `ExecOnId_`='%ls'", (LPCWSTR)szId);
		ExitOnFailure(hr, "Failed to format string");

		hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
		ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

		// Iterate records
		while ((hr = WcaFetchRecord(hSubView, &hSubRecord)) != E_NOMOREITEMS)
		{
			ExitOnFailure(hr, "Failed to fetch record.");
			CWixString name, valueFormat, value;
			std::string nameA;

			hr = WcaGetRecordFormattedString(hSubRecord, 1, (LPWSTR*)name);
			ExitOnFailure(hr, "Failed to get environment variable name");
			hr = WcaGetRecordString(hSubRecord, 2, (LPWSTR*)valueFormat);
			ExitOnFailure(hr, "Failed to get environment variable value");

			hr = value.MsiFormat((LPCWSTR)valueFormat);
			ExitOnFailure(hr, "Failed to MSI-format string");

			nameA.assign((LPCSTR)(LPCWSTR)name, WSTR_BYTE_SIZE((LPCWSTR)name));
			environment[nameA].set_plain((LPCSTR)(LPCWSTR)value, WSTR_BYTE_SIZE((LPCWSTR)value));
			environment[nameA].set_obfuscated((LPCSTR)value.Obfuscated(), WSTR_BYTE_SIZE(value.Obfuscated()));
		}

		// Get exit code map (i.e. map exit code 1 to success)
		hr = szSubQuery.Format(L"SELECT `Expression`, `Flags`, `ErrorHandling`, `PromptText` FROM `PSW_ExecOn_ConsoleOutput` WHERE `ExecOnId_`='%ls'", (LPCWSTR)szId);
		ExitOnFailure(hr, "Failed to format string");

		hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
		ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

		// Iterate records
		while ((hr = WcaFetchRecord(hSubView, &hSubRecord)) != E_NOMOREITEMS)
		{
			ExitOnFailure(hr, "Failed to fetch record.");
			CWixString szExpressionFormat, szExpression;
			CWixString szPrompt;
			int onMatch = 1;
			ErrorHandling stdoutHandling = ErrorHandling::fail;
			ConsoleOuputRemap console;

			hr = WcaGetRecordString(hSubRecord, 1, (LPWSTR*)szExpressionFormat);
			ExitOnFailure(hr, "Failed to get Expression.");
			hr = WcaGetRecordInteger(hSubRecord, 2, &onMatch);
			ExitOnFailure(hr, "Failed to get Flags.");
			hr = WcaGetRecordInteger(hSubRecord, 3, (int*)&stdoutHandling);
			ExitOnFailure(hr, "Failed to get ErrorHandling.");
			hr = WcaGetRecordFormattedString(hSubRecord, 4, (LPWSTR*)szPrompt);
			ExitOnFailure(hr, "Failed to get ErrorHandling.");

			hr = szExpression.MsiFormat((LPCWSTR)szExpressionFormat);
			ExitOnFailure(hr, "Failed to format Expression.");

			console.mutable_regex()->set_plain((LPCWSTR)szExpression, WSTR_BYTE_SIZE((LPCWSTR)szExpression));
			console.mutable_regex()->set_obfuscated(szExpression.Obfuscated(), WSTR_BYTE_SIZE(szExpression.Obfuscated()));
			console.set_onmatch(onMatch);
			console.set_errorhandling(stdoutHandling);
			console.set_prompttext((LPCWSTR)szPrompt, WSTR_BYTE_SIZE((LPCWSTR)szPrompt));

			consoleOutput.push_back(console);
		}

		hr = S_OK;

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
			if (nFlags & Flags::OnInstall)
			{
				hr = ScheduleExecution(szId, szCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnInstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_REINSTALL:
			if (nFlags & Flags::OnReinstall)
			{
				hr = ScheduleExecution(szId, szCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnReinstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			if (nFlags & Flags::OnRemove)
			{
				hr = ScheduleExecution(szId, szCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnRemoveRollback)
			{
				hr = ScheduleExecution(szId, szCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGMSG_STANDARD, "Component '%ls' action is unknown. Skipping execution of '%ls'.", (LPCWSTR)szComponent, (LPCWSTR)szId);
			break;
		}
	}

	// Rollback actions
	hr = oRollbackBeforeStop.SetCustomActionData(L"ExecOnComponent_BeforeStop_rollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	hr = oRollbackAfterStop.SetCustomActionData(L"ExecOnComponent_AfterStop_rollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	hr = oRollbackBeforeStart.SetCustomActionData(L"ExecOnComponent_BeforeStart_rollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	hr = oRollbackAfterStart.SetCustomActionData(L"ExecOnComponent_AfterStart_rollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	// Deferred actions
	hr = oDeferredBeforeStop.SetCustomActionData(L"ExecOnComponent_BeforeStop_deferred");
	ExitOnFailure(hr, "Failed setting deferred action data.");

	hr = oDeferredAfterStop.SetCustomActionData(L"ExecOnComponent_AfterStop_deferred");
	ExitOnFailure(hr, "Failed setting deferred action data.");

	hr = oDeferredBeforeStart.SetCustomActionData(L"ExecOnComponent_BeforeStart_deferred");
	ExitOnFailure(hr, "Failed setting deferred action data.");

	hr = oDeferredAfterStart.SetCustomActionData(L"ExecOnComponent_AfterStart_deferred");
	ExitOnFailure(hr, "Failed setting deferred action data.");

	// Rollback actions, impersonated
	hr = oRollbackBeforeStopImp.SetCustomActionData(L"ExecOnComponent_Imp_BeforeStop_rollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	hr = oRollbackAfterStopImp.SetCustomActionData(L"ExecOnComponent_Imp_AfterStop_rollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	hr = oRollbackBeforeStartImp.SetCustomActionData(L"ExecOnComponent_Imp_BeforeStart_rollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	hr = oRollbackAfterStartImp.SetCustomActionData(L"ExecOnComponent_Imp_AfterStart_rollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	// Deferred actions, impersonated
	hr = oDeferredBeforeStopImp.SetCustomActionData(L"ExecOnComponent_Imp_BeforeStop_deferred");
	ExitOnFailure(hr, "Failed setting deferred action data.");

	hr = oDeferredAfterStopImp.SetCustomActionData(L"ExecOnComponent_Imp_AfterStop_deferred");
	ExitOnFailure(hr, "Failed setting deferred action data.");

	hr = oDeferredBeforeStartImp.SetCustomActionData(L"ExecOnComponent_Imp_BeforeStart_deferred");
	ExitOnFailure(hr, "Failed setting deferred action data.");

	hr = oDeferredAfterStartImp.SetCustomActionData(L"ExecOnComponent_Imp_AfterStart_deferred");
	ExitOnFailure(hr, "Failed setting deferred action data.");

	hr = rollbackCAD.SetCustomActionData(L"ExecOnComponentRollback");
	ExitOnFailure(hr, "Failed setting rollback action data.");

	hr = commitCAD.SetCustomActionData(L"ExecOnComponentCommit");
	ExitOnFailure(hr, "Failed setting commit action data.");

LExit:
	ReleaseMem(pbData);
	if ((hFile != INVALID_HANDLE_VALUE) && (hFile != NULL))
	{
		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT ScheduleExecution(LPCWSTR szId, const CWixString &szCommand, LPCWSTR szWorkingDirectory, LPCWSTR szDomain, LPCWSTR szUser, LPCWSTR szPassword, CExecOnComponent::ExitCodeMap* pExitCodeMap, std::vector<ConsoleOuputRemap>* pConsoleOuput, CExecOnComponent::EnvironmentMap* pEnv, int nFlags, int errorHandling, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp)
{
	HRESULT hr = S_OK;

	if (nFlags & Flags::BeforeStopServices)
	{
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Will execute command '%ls' before StopServices", szCommand.Obfuscated());
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStopImp->AddExec(szCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		else
		{
			hr = pBeforeStop->AddExec(szCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStopServices)
	{
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Will execute command '%ls' after StopServices", szCommand.Obfuscated());
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStopImp->AddExec(szCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		else
		{
			hr = pAfterStop->AddExec(szCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::BeforeStartServices)
	{
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Will execute command '%ls' before StartServices", szCommand.Obfuscated());
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStartImp->AddExec(szCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		else
		{
			hr = pBeforeStart->AddExec(szCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStartServices)
	{
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Will execute command '%ls' after StartServices", szCommand.Obfuscated());
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStartImp->AddExec(szCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		else
		{
			hr = pAfterStart->AddExec(szCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}

LExit:
	return hr;
}

CExecOnComponent::CExecOnComponent() 
	: CDeferredActionBase("ExecOn") 
	, _errorPrompter((DWORD)PSW_MSI_MESSAGES::PSW_MSI_MESSAGES_EXEC_ON_ERROR)
	, _alwaysPrompter((DWORD)PSW_MSI_MESSAGES::PSW_MSI_MESSAGES_EXEC_ON_PROMPT_ALWAYS, (INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_USER | MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONINFORMATION), S_OK)
	, _stdoutPrompter((DWORD)PSW_MSI_MESSAGES::PSW_MSI_MESSAGES_EXEC_ON_CONSOLE_ERROR)
{ }

HRESULT CExecOnComponent::AddExec(const CWixString &szCommand, LPCWSTR szWorkingDirectory, LPCWSTR szDomain, LPCWSTR szUser, LPCWSTR szPassword, ExitCodeMap* pExitCodeMap, vector<ConsoleOuputRemap>* pConsoleOuput, EnvironmentMap* pEnv, int nFlags, ErrorHandling errorHandling)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	ExecOnDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CExecOnComponent", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new ExecOnDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->mutable_command()->set_plain((LPCWSTR)szCommand, WSTR_BYTE_SIZE((LPCWSTR)szCommand));
	pDetails->mutable_command()->set_obfuscated(szCommand.Obfuscated(), WSTR_BYTE_SIZE(szCommand.Obfuscated()));
	if (szWorkingDirectory && *szWorkingDirectory)
	{
		pDetails->set_workingdirectory(szWorkingDirectory, WSTR_BYTE_SIZE(szWorkingDirectory));
	}
	pDetails->set_async(nFlags & Flags::ASync);
	pDetails->set_errorhandling(errorHandling);
	pDetails->mutable_exitcoderemap()->insert(pExitCodeMap->begin(), pExitCodeMap->end());
	pDetails->mutable_environment()->insert(pEnv->begin(), pEnv->end());
	if (szUser && *szUser)
	{
		pDetails->set_user(szUser, WSTR_BYTE_SIZE(szUser));
		if (szDomain && *szDomain)
		{
			pDetails->set_domain(szDomain, WSTR_BYTE_SIZE(szDomain));
		}
		if (szPassword && *szPassword)
		{
			pDetails->set_password(szPassword, WSTR_BYTE_SIZE(szPassword));
		}
	}

	for (size_t i = 0; i < pConsoleOuput->size(); ++i)
	{
		ConsoleOuputRemap* pConsole = pDetails->add_consoleouputremap();
		pConsole->mutable_regex()->set_plain(pConsoleOuput->at(i).regex().plain());
		pConsole->mutable_regex()->set_obfuscated(pConsoleOuput->at(i).regex().obfuscated());
		pConsole->set_prompttext(pConsoleOuput->at(i).prompttext());
		pConsole->set_onmatch(pConsoleOuput->at(i).onmatch());
		pConsole->set_errorhandling(pConsoleOuput->at(i).errorhandling());
	}

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CExecOnComponent::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	ExecOnDetails details;

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking ExecOnDetails");

	do
	{
		hr = ExecuteOne(details);
	} while (hr == E_RETRY);
	ExitOnFailure(hr, "Failed to execute command");

LExit:
	return hr;
}

HRESULT CExecOnComponent::ExecuteOne(const com::panelsw::ca::ExecOnDetails& details)
{
	HRESULT hr = S_OK;
	DWORD exitCode = 0;
	BOOL bRes = TRUE;
	LPCWSTR szCommand = nullptr;
	LPCWSTR szObfuscatedCommand = nullptr;
	LPCWSTR szWorkingDirectory = nullptr;
	LPCWSTR szDomain = nullptr;
	LPCWSTR szUser = nullptr;
	LPCWSTR szPassword = nullptr;
	HANDLE hImpersonation = NULL;
	bool bImpersonated = false;
	CWixString commandLineCopy;
	CWixString szLog;
	HANDLE hStdOut = NULL;
	HANDLE hProc = NULL;
	PMSIHANDLE hActionData;

	szCommand = (LPCWSTR)(LPVOID)details.command().plain().data();
	szObfuscatedCommand = (LPCWSTR)(LPVOID)details.command().obfuscated().data();
	szWorkingDirectory = (LPCWSTR)(LPVOID)details.workingdirectory().data();
	szDomain = (LPCWSTR)(LPVOID)details.domain().data();
	szUser = (LPCWSTR)(LPVOID)details.user().data();
	szPassword = (LPCWSTR)(LPVOID)details.password().data();
	_errorPrompter.SetErrorHandling(details.errorhandling());
	_alwaysPrompter.SetErrorHandling(details.errorhandling());

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

	LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Executing '%ls'", szObfuscatedCommand);

	// ActionData: "Executing [1]"
	hActionData = ::MsiCreateRecord(1);
	if (hActionData && SUCCEEDED(WcaSetRecordString(hActionData, 1, szObfuscatedCommand)))
	{
		WcaProcessMessage(INSTALLMESSAGE::INSTALLMESSAGE_ACTIONDATA, hActionData);
	}

	if (details.async())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Not logging output on async command");

		if (szUser && *szUser)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Impersonating '%ls'", szUser);

			bRes = ::LogonUser(szUser, szDomain, szPassword, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hImpersonation);
			ExitOnNullWithLastError(bRes, hr, "Failed logging-in as user");

			bRes = ::ImpersonateLoggedOnUser(hImpersonation);
			ExitOnNullWithLastError(bRes, hr, "Failed impersonating user");
			bImpersonated = true;
		}

		hr = LaunchProcess(const_cast<LPWSTR>(szCommand), &hProc, nullptr);
		ExitOnFailure(hr, "Failed to launch command '%ls'", szCommand);
		hr = S_OK;
		ExitFunction();
	}

	szLog.Release();
	if (hStdOut && (hStdOut != INVALID_HANDLE_VALUE))
	{
		ReleaseHandle(hStdOut);
		hStdOut = INVALID_HANDLE_VALUE;
	}
	if (hProc && (hProc != INVALID_HANDLE_VALUE))
	{
		ReleaseHandle(hProc);
	}

	hr = commandLineCopy.Copy(szCommand);
	ExitOnFailure(hr, "Failed to copy string");

	// We only impersonate for the duration of the process execution because I've encountered crashes when logging impersonated
	if (szUser && *szUser)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Impersonating '%ls'", szUser);

		bRes = ::LogonUser(szUser, szDomain, szPassword, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hImpersonation);
		ExitOnNullWithLastError(bRes, hr, "Failed logging-in as user");

		bRes = ::ImpersonateLoggedOnUser(hImpersonation);
		ExitOnNullWithLastError(bRes, hr, "Failed impersonating user");
		bImpersonated = true;
	}

	// By default, exitCode is what the process returned. If couldn't execute the process, use failure code is result.
	hr = LaunchProcess(commandLineCopy, &hProc, &hStdOut);

	if (bImpersonated)
	{
		bRes = RevertToSelf();
		ExitOnNullWithLastError(bRes, hr, "Failed reverting impersonation");
		bImpersonated = false;

		CloseHandle(hImpersonation);
		hImpersonation = NULL;
	}

	if (SUCCEEDED(hr))
	{
		LogProcessOutput(hProc, hStdOut, ((details.consoleouputremap_size() > 0) || (details.errorhandling() == ErrorHandling::promptAlways)) ? (LPWSTR*)szLog : nullptr);

		hr = ProcWaitForCompletion(hProc, INFINITE, &exitCode);
		if (SUCCEEDED(hr))
		{
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Process exited with code %u", exitCode);
		}
	}
	if (FAILED(hr))
	{
		exitCode = HRESULT_CODE(hr);
	}

	if (details.exitcoderemap().find(exitCode) != details.exitcoderemap().end())
	{
		exitCode = details.exitcoderemap().at(exitCode);
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Remapped exit code to %u", exitCode);
	}

	hr = SearchStdOut((LPCWSTR)szLog, details);
	ExitOnFailure(hr, "Command output hints at an error");

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
		hr = HRESULT_FROM_WIN32(exitCode);
		break;
	}

	if (FAILED(hr))
	{
		hr = _errorPrompter.Prompt(szObfuscatedCommand);
	}
	else if (details.errorhandling() == ErrorHandling::promptAlways)
	{
		hr = _alwaysPrompter.Prompt(szObfuscatedCommand, szLog.IsNullOrEmpty() ? L"" : (LPCWSTR)szLog);
	}
	ExitOnFailure(hr, "Failed to execute command '%ls'", szObfuscatedCommand);

LExit:
	if (hImpersonation)
	{
		if (bImpersonated)
		{
			RevertToSelf();
		}
		CloseHandle(hImpersonation);
	}
	if (hStdOut && (hStdOut != INVALID_HANDLE_VALUE))
	{
		ReleaseHandle(hStdOut);
	}
	if (hProc && (hProc != INVALID_HANDLE_VALUE))
	{
		ReleaseHandle(hProc);
	}

	return hr;
}

HRESULT CExecOnComponent::LogProcessOutput(HANDLE hProcess, HANDLE hStdErrOut, LPWSTR* pszText /* Need to detect whether this is unicode or multibyte */)
{
	DWORD dwBufferSize = 0;
	DWORD dwBytes = 0;
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	BYTE* pBuffer = nullptr;
	FileRegexDetails::FileEncoding encoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None;
	LPWSTR szLog = nullptr;
	DWORD dwLogStart = 0;
	LPCWSTR szLogEnd = 0;
	OVERLAPPED overlapped;
	HANDLE rghHandles[2];
	DWORD dwRes = ERROR_SUCCESS;

	bRes = ::GetNamedPipeInfo(hStdErrOut, nullptr, &dwBufferSize, nullptr, nullptr);
	ExitOnNullWithLastError(bRes, hr, "Failed to get stdout buffer size");

	pBuffer = reinterpret_cast<BYTE*>(MemAlloc(dwBufferSize, FALSE));
	ExitOnNull(pBuffer, hr, E_OUTOFMEMORY, "Failed to allocate buffer for output.");

	overlapped.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	ExitOnNullWithLastError((overlapped.hEvent && (overlapped.hEvent != INVALID_HANDLE_VALUE)), hr, "Failed to create event");

	rghHandles[0] = hProcess;
	rghHandles[1] = overlapped.hEvent;

	bRes = ::ConnectNamedPipe(hStdErrOut, &overlapped);
	if (!bRes)
	{
		dwRes = ::GetLastError();
		if (dwRes == ERROR_IO_PENDING)
		{
			dwRes = ::WaitForSingleObject(overlapped.hEvent, INFINITE);
			ExitOnNull((dwRes == WAIT_OBJECT_0), hr, HRESULT_FROM_WIN32(dwRes), "Failed to wait for process to connect to stdout");
			bRes = TRUE;
		}
		else if (dwRes == ERROR_PIPE_CONNECTED)
		{
			bRes = TRUE;
		}
		ExitOnNullWithLastError(bRes, hr, "Failed to connect to stdout");
	}

	while (true)
	{
		bRes = ::ResetEvent(overlapped.hEvent);
		ExitOnNullWithLastError(bRes, hr, "Failed to reset event");

		bRes = ::ReadFile(hStdErrOut, pBuffer, dwBufferSize, nullptr, &overlapped);
		if (!bRes)
		{
			dwRes = ::GetLastError();
			if (dwRes == ERROR_BROKEN_PIPE)
			{
				break;
			}
			ExitOnNullWithLastError((dwRes == ERROR_IO_PENDING), hr, "Failed to wait for stdout data");
		}

		dwRes = ::WaitForMultipleObjects(ARRAYSIZE(rghHandles), rghHandles, FALSE, INFINITE);
		// Process terminated, or pipe abandoned
		if ((dwRes == WAIT_OBJECT_0) || (dwRes == WAIT_ABANDONED_0) || (dwRes == (WAIT_ABANDONED_0 + 1)))
		{
			break;
		}
		ExitOnNullWithLastError((dwRes != WAIT_FAILED), hr, "Failed to wait for process to terminate or write to stdout");
		if (dwRes != (WAIT_OBJECT_0 + 1))
		{
			ExitOnWin32Error(dwRes, hr, "Failed to wait for process to terminate or write to stdout.");
		}

		bRes = ::GetOverlappedResult(hStdErrOut, &overlapped, &dwBytes, FALSE);
		if (!bRes && (::GetLastError() == ERROR_BROKEN_PIPE))
		{
			break;
		}
		ExitOnNullWithLastError(bRes, hr, "Failed to read stdout");

		// On first read, test multibyte or unicode
		if (encoding == FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None)
		{
			encoding = CFileOperations::DetectEncoding(pBuffer, dwBytes);
		}

		switch (encoding)
		{
		case FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte:
			hr = StrAllocConcatFormatted(&szLog, L"%.*hs", dwBytes, pBuffer);
			ExitOnFailure(hr, "Failed to concatenate output strings");
			break;

		case FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_ReverseUnicode:
		case FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode:
			hr = StrAllocConcatFormatted(&szLog, L"%.*ls", dwBytes / sizeof(WCHAR), (LPCWSTR)(LPVOID)pBuffer, 0);
			ExitOnFailure(hr, "Failed to concatenate output strings");
			break;
		}

		// Log each line of the output
		szLogEnd = ::wcschr(szLog + dwLogStart, L'\r');
		if (szLogEnd == nullptr)
		{
			szLogEnd = ::wcschr(szLog + dwLogStart, L'\n');
		}
		while (szLogEnd && *szLogEnd)
		{
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"%.*ls", (szLogEnd - (szLog + dwLogStart)), szLog + dwLogStart);

			// Go past \n or \r\n
			if ((szLogEnd[0] == L'\r') && (szLogEnd[1] == L'\n'))
			{
				szLogEnd += 2;
			}
			else
			{
				++szLogEnd;
			}
			dwLogStart = szLogEnd - szLog;

			// Next line
			szLogEnd = ::wcschr(szLog + dwLogStart, L'\r');
			if (szLogEnd == nullptr)
			{
				szLogEnd = ::wcschr(szLog + dwLogStart, L'\n');
			}
		}

		// If we don't have to accumulate all log text, we can release whatever text we already logged.
		if (!pszText && !dwLogStart)
		{
			LPWSTR szTemp = nullptr;

			hr = StrAllocString(&szTemp, szLog + dwLogStart, 0);
			if (SUCCEEDED(hr))
			{
				ReleaseStr(szLog);
				szLog = szTemp;
				dwLogStart = 0;
			}
			hr = S_OK;
		}
	}

	// Print any text that didn't end with a new line
	if (szLog && (szLog[dwLogStart] != NULL))
	{
		LogUnformatted(LOGMSG_STANDARD, true, L"%ls", szLog + dwLogStart);
	}

	// Return full log to the caller
	if (pszText)
	{
		*pszText = szLog;
		szLog = nullptr;
	}

LExit:
	ReleaseMem(pBuffer);
	ReleaseStr(szLog);
	ReleaseHandle(overlapped.hEvent);

	return hr;
}

HRESULT CExecOnComponent::SearchStdOut(LPCWSTR szStdOut, const ExecOnDetails& details)
{
	HRESULT hr = S_FALSE;

	for (const ConsoleOuputRemap& console : details.consoleouputremap())
	{
		try
		{
			bool bRes = true;
			std::wregex rx((LPCWSTR)(LPVOID)console.regex().plain().data());
			match_results<LPCWSTR> results;

			bRes = regex_search(szStdOut, results, rx);
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Regex '%ls' search yielded %u matches", (LPCWSTR)(LPVOID)console.regex().obfuscated().data(), results.size());
			if ((bRes && results.size()) != console.onmatch())
			{
				continue;
			}
		}
		catch (std::regex_error ex)
		{
			hr = HRESULT_FROM_WIN32(ex.code());
			if (SUCCEEDED(hr))
			{
				hr = E_FAIL;
			}
			ExitOnFailure(hr, "Failed evaluating regular expression. %hs", ex.what());
		}

		_stdoutPrompter.SetErrorHandling(console.errorhandling());
		hr = _stdoutPrompter.Prompt((LPCWSTR)(LPVOID)details.command().obfuscated().data(), (LPCWSTR)(LPVOID)console.prompttext().data());
		ExitOnFailure(hr, "Console output hints at an error");
	}

LExit:
	return hr;
}

HRESULT CExecOnComponent::SetEnvironment(const ::google::protobuf::Map<std::string, com::panelsw::ca::ObfuscatedString>& customEnv)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	CRegistryKey envKey;
	CWixString szValueName;
	CRegistryKey::RegValueType valueType;

	hr = envKey.Open(CRegistryKey::RegRoot::LocalMachine, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", CRegistryKey::RegArea::X64, CRegistryKey::RegAccess::ReadOnly);
	ExitOnFailure(hr, "Failed to open environment registry key");

	for (DWORD dwIndex = 0; S_OK == envKey.EnumValues(dwIndex, (LPWSTR*)szValueName, &valueType); ++dwIndex)
	{
		if ((valueType == CRegistryKey::RegValueType::String) || (valueType == CRegistryKey::RegValueType::Expandable))
		{
			CWixString szValueData;

			hr = envKey.GetStringValue(szValueName, (LPWSTR*)szValueData);
			ExitOnFailure(hr, "Failed to get environment variable '%ls' from registry key", (LPCWSTR)szValueName);

			bRes = ::SetEnvironmentVariable(szValueName, szValueData);
			ExitOnNullWithLastError(bRes, hr, "Failed setting environment variable '%ls'", (LPCWSTR)szValueName);
		}
		szValueName.Release();
	}
	ExitOnFailure(hr, "Failed enumerating environment registry key");

	for (::google::protobuf::Map<std::string, com::panelsw::ca::ObfuscatedString>::const_iterator itCurr = customEnv.begin(), itEnd = customEnv.end(); itCurr != itEnd; ++itCurr)
	{
		LPCWSTR szName = (LPCWSTR)(LPVOID)itCurr->first.data();
		LPCWSTR szValue = (LPCWSTR)(LPVOID)itCurr->second.plain().data();
		LPCWSTR szObfuscatedValue = (LPCWSTR)(LPVOID)itCurr->second.obfuscated().data();

		if (szName && *szName && szValue && *szValue)
		{
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Setting custom environment variable '%ls' to '%ls'", szName, szObfuscatedValue);

			bRes = ::SetEnvironmentVariable(szName, szValue);
			ExitOnNullWithLastError(bRes, hr, "Failed setting environment variable '%ls'", (LPCWSTR)szName);
		}
	}

LExit:
	return hr;
}

HRESULT CExecOnComponent::LaunchProcess(LPCWSTR szCommand, HANDLE* phProcess, HANDLE* phStdOut)
{
	const UINT OUTPUT_BUFFER_SIZE = 1024;
	HRESULT hr = S_OK;
	SECURITY_ATTRIBUTES sa;
	RPC_STATUS rs = RPC_S_OK;
	UUID guid = {};
	WCHAR wzGuid[39];
	CWixString szStdInPipeName;
	CWixString szStdOutPipeName;
	BOOL bRes = TRUE;
	HANDLE hOutTemp = INVALID_HANDLE_VALUE;
	HANDLE hInTemp = INVALID_HANDLE_VALUE;
	HANDLE hOutRead = INVALID_HANDLE_VALUE;
	HANDLE hOutWrite = INVALID_HANDLE_VALUE;
	HANDLE hErrWrite = INVALID_HANDLE_VALUE;
	HANDLE hInRead = INVALID_HANDLE_VALUE;
	HANDLE hInWrite = INVALID_HANDLE_VALUE;
	PROCESS_INFORMATION pi = { };
	STARTUPINFOW si = { };
	CWixString szCmdLIne;

	ZeroMemory(&sa, sizeof(sa));
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));

	// Generate unique pipe names
	rs = ::UuidCreate(&guid);
	hr = HRESULT_FROM_RPC(rs);
	ExitOnFailure(hr, "Failed to create working folder guid.");

	bRes = ::StringFromGUID2(guid, wzGuid, countof(wzGuid));
	ExitOnNull(bRes, hr, E_OUTOFMEMORY, "Failed to convert UUID to string");

	hr = szStdInPipeName.Format(L"\\\\.\\pipe\\%ls-stdin", wzGuid);
	ExitOnFailure(hr, "Failed to create stdin pipe name.");

	hr = szStdOutPipeName.Format(L"\\\\.\\pipe\\%ls-stdout", wzGuid);
	ExitOnFailure(hr, "Failed to create stdout pipe name.");

	// Fill out security structure so we can inherit handles
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	// Create pipes
	hOutTemp = ::CreateNamedPipe(szStdOutPipeName, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, OUTPUT_BUFFER_SIZE, OUTPUT_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, &sa);
	ExitOnNullWithLastError((hOutTemp && (hOutTemp != INVALID_HANDLE_VALUE)), hr, "Failed to create named pipe for stdout reader");

	hOutWrite = ::CreateFile(szStdOutPipeName, FILE_WRITE_DATA | SYNCHRONIZE | FILE_FLAG_OVERLAPPED, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	ExitOnNullWithLastError((hOutWrite && (hOutWrite != INVALID_HANDLE_VALUE)), hr, "Failed to open named pipe for stdout writer");

	bRes = ::DuplicateHandle(::GetCurrentProcess(), hOutWrite, ::GetCurrentProcess(), &hErrWrite, 0, TRUE, DUPLICATE_SAME_ACCESS);
	ExitOnNullWithLastError((bRes && hErrWrite && (hErrWrite != INVALID_HANDLE_VALUE)), hr, "Failed to duplicate named pipe from stdout to stderr");

	bRes = ::DuplicateHandle(::GetCurrentProcess(), hOutTemp, ::GetCurrentProcess(), &hOutRead, 0, FALSE, DUPLICATE_SAME_ACCESS);
	ExitOnNullWithLastError((bRes && hOutRead && (hOutRead != INVALID_HANDLE_VALUE)), hr, "Failed to duplicate named pipe for stdout reader");
	::CloseHandle(hOutTemp);
	hOutTemp = INVALID_HANDLE_VALUE;

	hInTemp = ::CreateNamedPipe(szStdInPipeName, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, 1, OUTPUT_BUFFER_SIZE, OUTPUT_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, &sa);
	ExitOnNullWithLastError((hInTemp && (hInTemp != INVALID_HANDLE_VALUE)), hr, "Failed to create named pipe for stdin writer");

	hInRead = ::CreateFile(szStdInPipeName, FILE_READ_DATA, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	ExitOnNullWithLastError((hInRead && (hInRead != INVALID_HANDLE_VALUE)), hr, "Failed to open named pipe for stdin reader");

	bRes = ::DuplicateHandle(::GetCurrentProcess(), hInTemp, ::GetCurrentProcess(), &hInWrite, 0, FALSE, DUPLICATE_SAME_ACCESS);
	ExitOnNullWithLastError((hInWrite && (hInWrite != INVALID_HANDLE_VALUE)), hr, "Failed to duplicate named pipe for stdin writer");
	::CloseHandle(hInTemp);
	hInTemp = INVALID_HANDLE_VALUE;

	si.cb = sizeof(STARTUPINFOW);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = hInRead;
	si.hStdOutput = hOutWrite;
	si.hStdError = hErrWrite;

	hr = szCmdLIne.Copy(szCommand);
	ExitOnFailure(hr, "Failed to copy string");

	bRes = ::CreateProcessW(nullptr, (LPWSTR)szCmdLIne, nullptr, nullptr, TRUE, ::GetPriorityClass(::GetCurrentProcess()) | CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
	ExitOnNullWithLastError(bRes, hr, "Failed to create process");

	if (phStdOut)
	{
		*phStdOut = hOutRead;
		hOutRead = INVALID_HANDLE_VALUE;
	}
	if (phProcess)
	{
		*phProcess = pi.hProcess;
		pi.hProcess = INVALID_HANDLE_VALUE;
	}

LExit:
	ReleaseFileHandle(hOutRead);
	ReleaseFileHandle(hOutWrite);
	ReleaseFileHandle(hErrWrite);
	ReleaseFileHandle(hInRead);
	ReleaseFileHandle(hInWrite);
	ReleaseFileHandle(hOutTemp);
	ReleaseFileHandle(hInTemp);
	ReleaseFileHandle(pi.hProcess);
	ReleaseFileHandle(pi.hThread);

	return hr;
}

#include "ExecOnComponent.h"
#include "RegistryKey.h"
#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include <regex>
#include <wcautil.h>
#include <memutil.h>
#include <pathutil.h>
#include <shlwapi.h>
#include <procutil.h>
#include "google\protobuf\any.h"
using namespace std;
using namespace com::panelsw::ca;
using namespace google::protobuf;
#pragma comment (lib, "shlwapi.lib")

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

static HRESULT ScheduleExecution(LPCWSTR szId, LPCWSTR szCommand, LPCWSTR szObfuscatedCommand, LPCWSTR szWorkingDirectory, LPCWSTR szDomain, LPCWSTR szUser, LPCWSTR szPassword, CExecOnComponent::ExitCodeMap *pExitCodeMap, std::vector<ConsoleOuputRemap> *pConsoleOuput, CExecOnComponent::EnvironmentMap *pEnv, int nFlags, int errorHandling, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp);

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
		ReleaseNullStr(szObfuscatedCommand);

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
		std::map<std::string, std::string> environment;

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
		ExitOnFailure(hr, "Failed to get ErrorHandling.");

		// Execute from binary
		if (!szBinary.IsNullOrEmpty())
		{
			CWixString szReplaceMe;
			LPCWSTR szExtension = nullptr;

			hr = szSubQuery.Format(L"SELECT `Data` FROM `Binary` WHERE `Name`='%s'", (LPCWSTR)szBinary);
			ExitOnFailure(hr, "Failed to format string");

			hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
			ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

			hr = WcaFetchRecord(hSubView, &hSubRecord);
			ExitOnFailure(hr, "Failed to fetch Binary record.");

			hr = WcaGetRecordStream(hSubRecord, 1, &pbData, &cbData);
			ExitOnFailure(hr, "Failed to read Binary.Data for certificate.");

			hSubRecord = NULL;
			hSubView = NULL;

			hr = PathCreateTempFile(nullptr, L"EXE%05i.tmp", INFINITE, FILE_ATTRIBUTE_NORMAL, (LPWSTR*)szTempFile, nullptr);
			ExitOnFailure(hr, "Failed getting temporary file name");

			szExtension = ::PathFindExtension((LPCWSTR)szBinary);
			if (szExtension && *szExtension)
			{
				dwRes = ::PathRenameExtension(szTempFile, szExtension);
				ExitOnNullWithLastError(dwRes, hr, "Failed renaming file extension '%ls' to '%ls'", szTempFile, szExtension);
			}

			hr = szReplaceMe.Format(L"{*%s}", (LPCWSTR)szBinary);
			ExitOnFailure(hr, "Failed to format string");

			hr = szCommandFormat.ReplaceAll((LPCWSTR)szReplaceMe, szTempFile);
			ExitOnFailure(hr, "Failed to replace in string");

			hFile = ::CreateFile(szTempFile, GENERIC_ALL, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed opening file");

			dwRes = ::WriteFile(hFile, pbData, cbData, &cbData, nullptr);
			ExitOnNullWithLastError(dwRes, hr, "Failed writing to file");

			// Don't care if this is going to fail- just deleting a temporary file
			rollbackCAD.AddDeleteFile(szTempFile, CFileOperations::FileOperationsAttributes::IgnoreErrors | CFileOperations::FileOperationsAttributes::IgnoreMissingPath);
			commitCAD.AddDeleteFile(szTempFile, CFileOperations::FileOperationsAttributes::IgnoreErrors | CFileOperations::FileOperationsAttributes::IgnoreMissingPath);

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
			hr = szSubQuery.Format(L"SELECT `Domain`, `Name`, `Password` FROM `User` WHERE `User`='%s'", (LPCWSTR)userId);
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

		hr = szCommand.MsiFormat((LPCWSTR)szCommandFormat, &szObfuscatedCommand);
		ExitOnFailure(hr, "Failed expanding command");

		// Get exit code map (i.e. map exit code 1 to success)
		hr = szSubQuery.Format(L"SELECT `From`, `To` FROM `PSW_ExecOnComponent_ExitCode` WHERE `ExecOnId_`='%s'", (LPCWSTR)szId);
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
		hr = szSubQuery.Format(L"SELECT `Name`, `Value` FROM `PSW_ExecOnComponent_Environment` WHERE `ExecOnId_`='%s'", (LPCWSTR)szId);
		ExitOnFailure(hr, "Failed to format string");

		hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
		ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

		// Iterate records
		while ((hr = WcaFetchRecord(hSubView, &hSubRecord)) != E_NOMOREITEMS)
		{
			ExitOnFailure(hr, "Failed to fetch record.");
			CWixString name, value;
			std::string nameA, valueA;

			hr = WcaGetRecordFormattedString(hSubRecord, 1, (LPWSTR*)name);
			ExitOnFailure(hr, "Failed to get From.");
			hr = WcaGetRecordFormattedString(hSubRecord, 2, (LPWSTR*)value);
			ExitOnFailure(hr, "Failed to get To.");

			nameA.assign((LPCSTR)(LPCWSTR)name, WSTR_BYTE_SIZE((LPCWSTR)name));
			valueA.assign((LPCSTR)(LPCWSTR)value, WSTR_BYTE_SIZE((LPCWSTR)value));
			environment[nameA] = valueA;
		}

		// Get exit code map (i.e. map exit code 1 to success)
		hr = szSubQuery.Format(L"SELECT `Expression`, `Flags`, `ErrorHandling`, `PromptText` FROM `PSW_ExecOn_ConsoleOutput` WHERE `ExecOnId_`='%s'", (LPCWSTR)szId);
		ExitOnFailure(hr, "Failed to format string");

		hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
		ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

		// Iterate records
		while ((hr = WcaFetchRecord(hSubView, &hSubRecord)) != E_NOMOREITEMS)
		{
			ExitOnFailure(hr, "Failed to fetch record.");
			CWixString szExpressionFormat, szExpression, szObfuscatedExpression;
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

			hr = szExpression.MsiFormat((LPCWSTR)szExpressionFormat, (LPWSTR*)szObfuscatedExpression);
			ExitOnFailure(hr, "Failed to format Expression.");

			console.set_regex((LPCWSTR)szExpression, WSTR_BYTE_SIZE((LPCWSTR)szExpression));
			console.set_obfuscatedregex((LPCWSTR)szObfuscatedExpression, WSTR_BYTE_SIZE((LPCWSTR)szObfuscatedExpression));
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
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnInstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_REINSTALL:
			if (nFlags & Flags::OnReinstall)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnReinstallRollback)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			if (nFlags & Flags::OnRemove)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oDeferredBeforeStop, &oDeferredAfterStop, &oDeferredBeforeStart, &oDeferredAfterStart, &oDeferredBeforeStopImp, &oDeferredAfterStopImp, &oDeferredBeforeStartImp, &oDeferredAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			if (nFlags & Flags::OnRemoveRollback)
			{
				hr = ScheduleExecution(szId, szCommand, szObfuscatedCommand, workDir, domain, user, password, &exitCodeMap, &consoleOutput, &environment, nFlags, errorHandling, &oRollbackBeforeStop, &oRollbackAfterStop, &oRollbackBeforeStart, &oRollbackAfterStart, &oRollbackBeforeStopImp, &oRollbackAfterStopImp, &oRollbackBeforeStartImp, &oRollbackAfterStartImp);
				ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
			}
			break;

		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGMSG_STANDARD, "Component '%ls' action is unknown. Skipping execution of '%ls'.", (LPCWSTR)szComponent, (LPCWSTR)szId);
			break;
		}
	}

	// Rollback actions
	hr = oRollbackBeforeStop.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStop_rollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackAfterStop.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStop_rollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackBeforeStart.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStart_rollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackAfterStart.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStart_rollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	// Deferred actions
	ReleaseNullStr(szCustomActionData);
	hr = oDeferredBeforeStop.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStop_deferred", szCustomActionData);
	ExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredAfterStop.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStop_deferred", szCustomActionData);
	ExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredBeforeStart.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_BeforeStart_deferred", szCustomActionData);
	ExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredAfterStart.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_AfterStart_deferred", szCustomActionData);
	ExitOnFailure(hr, "Failed setting deferred action data.");

	// Rollback actions, impersonated
	ReleaseNullStr(szCustomActionData);
	hr = oRollbackBeforeStopImp.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStop_rollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackAfterStopImp.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStop_rollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackBeforeStartImp.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStart_rollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oRollbackAfterStartImp.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStart_rollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	// Deferred actions, impersonated
	ReleaseNullStr(szCustomActionData);
	hr = oDeferredBeforeStopImp.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStop_deferred", szCustomActionData);
	ExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredAfterStopImp.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStop_deferred", szCustomActionData);
	ExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredBeforeStartImp.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_BeforeStart_deferred", szCustomActionData);
	ExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredAfterStartImp.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred.");
	hr = WcaSetProperty(L"ExecOnComponent_Imp_AfterStart_deferred", szCustomActionData);
	ExitOnFailure(hr, "Failed setting deferred action data.");

	ReleaseNullStr(szCustomActionData);
	hr = rollbackCAD.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback.");
	hr = WcaSetProperty(L"ExecOnComponentRollback", szCustomActionData);
	ExitOnFailure(hr, "Failed setting rollback action data.");

	ReleaseNullStr(szCustomActionData);
	hr = commitCAD.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for commit.");
	hr = WcaSetProperty(L"ExecOnComponentCommit", szCustomActionData);
	ExitOnFailure(hr, "Failed setting commit action data.");

LExit:
	ReleaseMem(pbData);
	ReleaseStr(szCustomActionData);
	ReleaseStr(szObfuscatedCommand);
	if ((hFile != INVALID_HANDLE_VALUE) && (hFile != NULL))
	{
		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT ScheduleExecution(LPCWSTR szId, LPCWSTR szCommand, LPCWSTR szObfuscatedCommand, LPCWSTR szWorkingDirectory, LPCWSTR szDomain, LPCWSTR szUser, LPCWSTR szPassword, CExecOnComponent::ExitCodeMap* pExitCodeMap, std::vector<ConsoleOuputRemap>* pConsoleOuput, CExecOnComponent::EnvironmentMap* pEnv, int nFlags, int errorHandling, CExecOnComponent* pBeforeStop, CExecOnComponent* pAfterStop, CExecOnComponent* pBeforeStart, CExecOnComponent* pAfterStart, CExecOnComponent* pBeforeStopImp, CExecOnComponent* pAfterStopImp, CExecOnComponent* pBeforeStartImp, CExecOnComponent* pAfterStartImp)
{
	HRESULT hr = S_OK;

	if (nFlags & Flags::BeforeStopServices)
	{
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' before StopServices", szObfuscatedCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStopImp->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		else
		{
			hr = pBeforeStop->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStopServices)
	{
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' after StopServices", szObfuscatedCommand);
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStopImp->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		else
		{
			hr = pAfterStop->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::BeforeStartServices)
	{
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' before StartServices", szObfuscatedCommand, (ErrorHandling)errorHandling);
		if (nFlags & Flags::Impersonate)
		{
			hr = pBeforeStartImp->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		else
		{
			hr = pBeforeStart->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}
	if (nFlags & Flags::AfterStartServices)
	{
		CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, "Will execute command '%ls' after StartServices", szObfuscatedCommand, (ErrorHandling)errorHandling);
		if (nFlags & Flags::Impersonate)
		{
			hr = pAfterStartImp->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		else
		{
			hr = pAfterStart->AddExec(szCommand, szObfuscatedCommand, szWorkingDirectory, szDomain, szUser, szPassword, pExitCodeMap, pConsoleOuput, pEnv, nFlags, (ErrorHandling)errorHandling);
		}
		ExitOnFailure(hr, "Failed scheduling '%ls'", (LPCWSTR)szId);
	}

LExit:
	return hr;
}

HRESULT CExecOnComponent::AddExec(LPCWSTR szCommand, LPCWSTR szObfuscatedCommand, LPCWSTR szWorkingDirectory, LPCWSTR szDomain, LPCWSTR szUser, LPCWSTR szPassword, ExitCodeMap* pExitCodeMap, vector<ConsoleOuputRemap>* pConsoleOuput, EnvironmentMap* pEnv, int nFlags, ErrorHandling errorHandling)
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

	pDetails->set_command(szCommand, WSTR_BYTE_SIZE(szCommand));
	pDetails->set_obfuscatedcommand(szObfuscatedCommand, WSTR_BYTE_SIZE(szObfuscatedCommand));
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
		pConsole->set_regex(pConsoleOuput->at(i).regex());
		pConsole->set_obfuscatedregex(pConsoleOuput->at(i).obfuscatedregex());
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
	DWORD exitCode = 0;
	BOOL bRes = TRUE;
	ExecOnDetails details;
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

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking ExecOnDetails");

	szCommand = (LPCWSTR)details.command().c_str();
	szObfuscatedCommand = (LPCWSTR)details.obfuscatedcommand().c_str();
	szWorkingDirectory = (LPCWSTR)details.workingdirectory().c_str();
	szDomain = (LPCWSTR)details.domain().c_str();
	szUser = (LPCWSTR)details.user().c_str();
	szPassword = (LPCWSTR)details.password().c_str();

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

	LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, "Executing '%ls'", szObfuscatedCommand);
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

		hr = ProcExecute(const_cast<LPWSTR>(szCommand), &hProc, nullptr, nullptr);
		ExitOnFailure(hr, "Failed to launch command '%ls'", szCommand);
		hr = S_OK;
		ExitFunction();
	}

	// Sync
LRetry:
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
	hr = ProcExecute(commandLineCopy, &hProc, nullptr, &hStdOut);

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
		LogProcessOutput(hStdOut, ((details.consoleouputremap_size() > 0) ? (LPWSTR*)szLog : nullptr));

		hr = ProcWaitForCompletion(hProc, INFINITE, &exitCode);
		if (SUCCEEDED(hr))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Process exited with code %u", exitCode);
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
	switch (hr)
	{
	case E_RETRY:
		hr = S_OK;
		goto LRetry;

	case S_OK:
	case E_FAIL:
		ExitFunction();

	case S_FALSE:
	default:
		break;
	}

	if (FAILED(hr) && (exitCode == ERROR_SUCCESS))
	{
		// Shouldn't happen since SearchStdOut doesn't return any value not handled in switch above. However, keeping for sake of future changes.
		exitCode = HRESULT_CODE(hr);
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Remapped exit code to %u after searching console output", exitCode);
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
		hr = HRESULT_FROM_WIN32(exitCode);
		break;
	}

	if (FAILED(hr))
	{
		switch (details.errorhandling())
		{
		case ErrorHandling::fail:
		default:
			// Will fail downstairs.
			break;

		case ErrorHandling::ignore:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Ignoring command failure 0x%08X", hr);
			hr = S_OK;
			break;

		case ErrorHandling::prompt:
		{
			HRESULT hrOp = hr;
			PMSIHANDLE hRec;
			UINT promptResult = IDABORT;

			hRec = ::MsiCreateRecord(2);
			ExitOnNull(hRec, hr, E_FAIL, "Failed creating record");

			hr = WcaSetRecordInteger(hRec, 1, 27001);
			ExitOnFailure(hr, "Failed setting record integer");

			hr = WcaSetRecordString(hRec, 2, szObfuscatedCommand);
			ExitOnFailure(hr, "Failed setting record string");

			promptResult = WcaProcessMessage((INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_ERROR | MB_ABORTRETRYIGNORE | MB_DEFBUTTON1 | MB_ICONERROR), hRec);
			switch (promptResult)
			{
			case IDABORT:
			case IDCANCEL:
			default: // Probably silent (result 0)
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User aborted on command failure (error code 0x%08X)", hrOp);
				hr = hrOp;
				break;

			case IDRETRY:
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User chose to retry on command failure (error code 0x%08X)", hrOp);
				hr = S_OK;
				goto LRetry;

			case IDIGNORE:
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User ignored command failure (error code 0x%08X)", hrOp);
				hr = S_OK;
				break;
			}
			break;
		}
		}
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

HRESULT CExecOnComponent::LogProcessOutput(HANDLE hStdErrOut, LPWSTR* pszText /* Need to detect whether this is unicode or multibyte */)
{
	const int OUTPUT_BUFFER_SIZE = 1024;
	DWORD dwBytes = OUTPUT_BUFFER_SIZE;
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	BYTE* pBuffer = nullptr;
	FileRegexDetails::FileEncoding encoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None;
	LPWSTR szLog = nullptr;
	DWORD dwLogStart = 0;
	LPCWSTR szLogEnd = 0;

	pBuffer = reinterpret_cast<BYTE*>(MemAlloc(OUTPUT_BUFFER_SIZE, FALSE));
	ExitOnNull(pBuffer, hr, E_OUTOFMEMORY, "Failed to allocate buffer for output.");

	while (dwBytes != 0)
	{
		dwBytes = OUTPUT_BUFFER_SIZE;
		::ZeroMemory(pBuffer, OUTPUT_BUFFER_SIZE);

		bRes = ::ReadFile(hStdErrOut, pBuffer, OUTPUT_BUFFER_SIZE - sizeof(WCHAR), &dwBytes, nullptr);
		ExitOnNullWithLastError((bRes || (::GetLastError() == ERROR_BROKEN_PIPE /* Happens if the process terminated. Still may have data to read */)), hr, "Failed to read from handle.");

		// On first read, test multibyte or unicode
		if (encoding == FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None)
		{
			encoding = CFileOperations::DetectEncoding(pBuffer, dwBytes);
		}

		switch (encoding)
		{
		case FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte:
			hr = StrAllocConcatFormatted(&szLog, L"%hs", pBuffer);
			ExitOnFailure(hr, "Failed to concatenate output strings");
			break;

		case FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_ReverseUnicode:
		case FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_Unicode:
			hr = StrAllocConcat(&szLog, (LPCWSTR)pBuffer, 0);
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
			char szFormat[20];

			::sprintf_s<sizeof(szFormat)>(szFormat, "%%.%dls", (szLogEnd - (szLog + dwLogStart)));
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, szFormat, szLog + dwLogStart);

			// Go past \n or \r\n
			if ((*szLogEnd == L'\r') && (*(szLogEnd + 1) == L'\n'))
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
		LogUnformatted(LOGMSG_STANDARD, "%ls", szLog + dwLogStart);
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

	return hr;
}

// S_FALSE: Had no matches, go on with error handling.
// S_OK: Ignore errors and continue
// E_RETRY: Retry
// E_FAIL: Abort
HRESULT CExecOnComponent::SearchStdOut(LPCWSTR szStdOut, const ExecOnDetails& details)
{
	HRESULT hr = S_FALSE;
	HRESULT localHr = S_OK;

	for (const ConsoleOuputRemap& console : details.consoleouputremap())
	{
		try
		{
			bool bRes = true;
			std::wregex rx((LPCWSTR)console.regex().data());
			match_results<LPCWSTR> results;

			bRes = regex_search(szStdOut, results, rx);
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, "Regex '%ls' search yielded %i matches", (LPCWSTR)console.obfuscatedregex().data(), results.size());
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
			ExitOnFailure(hr, "Failed evaluating regular expression. %s", ex.what());
		}

		switch (console.errorhandling())
		{
		case ErrorHandling::fail:
			hr = E_FAIL;
			ExitFunction();

		case ErrorHandling::ignore:
			if (hr == S_FALSE)
			{
				hr = S_OK;
			}
			continue;

		case ErrorHandling::prompt:
			PMSIHANDLE hRec;
			UINT promptResult = IDABORT;

			hRec = ::MsiCreateRecord(3);
			ExitOnNull(hRec, localHr, E_FAIL, "Failed creating record");

			localHr = WcaSetRecordInteger(hRec, 1, 27006);
			ExitOnFailure(localHr, "Failed setting record integer");

			localHr = WcaSetRecordString(hRec, 2, (LPCWSTR)details.obfuscatedcommand().data());
			ExitOnFailure(localHr, "Failed setting record string");

			localHr = WcaSetRecordString(hRec, 3, (LPCWSTR)console.prompttext().data());
			ExitOnFailure(localHr, "Failed setting record string");

			promptResult = WcaProcessMessage((INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_ERROR | MB_ABORTRETRYIGNORE | MB_DEFBUTTON1 | MB_ICONERROR), hRec);
			switch (promptResult)
			{
			case IDABORT:
			case IDCANCEL:
			default: // Probably silent (result 0)
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User aborted on command console output parsing");
				hr = E_FAIL;
				ExitFunction();

			case IDRETRY:
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User chose to retry on command console output parsing");
				hr = E_RETRY;
				ExitFunction();

			case IDIGNORE:
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User ignored command console output parsing");
				if (hr == S_FALSE)
				{
					hr = S_OK;
				}
				break;
			}
			break;
		}
	}

LExit:
	return (SUCCEEDED(localHr) ? hr : localHr);
}

HRESULT CExecOnComponent::SetEnvironment(const ::google::protobuf::Map<std::string, std::string>& customEnv)
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

	for (::google::protobuf::Map<std::string, std::string>::const_iterator itCurr = customEnv.begin(), itEnd = customEnv.end(); itCurr != itEnd; ++itCurr)
	{
		LPCWSTR szName = (LPCWSTR)itCurr->first.c_str();
		LPCWSTR szValue = (LPCWSTR)itCurr->second.c_str();

		if (szName && *szName && szValue && *szValue)
		{
			LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, "Setting custom environment variable '%ls' to '%ls'", szName, szValue);

			bRes = ::SetEnvironmentVariable(szName, szValue);
			ExitOnNullWithLastError(bRes, hr, "Failed setting environment variable '%ls'", (LPCWSTR)szName);
		}
	}

LExit:
	return hr;
}
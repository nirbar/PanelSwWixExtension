#include "TopShelfService.h"
#include <Shellapi.h>
#include <Shlwapi.h>
#include <pathutil.h>
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

#define TopShelfService_QUERY L"SELECT `File.Component_`, `PSW_TopShelfService`.`File_` `PSW_TopShelfService`.`ServiceName`, `PSW_TopShelfService`.`DisplayName`, `PSW_TopShelfService`.`Description`, `PSW_TopShelfService.Instance`," \
							  L"`PSW_TopShelfService`.`Account`, `PSW_TopShelfService`.`UserName`, `PSW_TopShelfService`.`Password`, `PSW_TopShelfService`.`HowToStart` " \
							  L"FROM `PSW_TopShelfService`, `File`" \
							  L"WHERE `PSW_TopShelfService`.`File_` = `File`.`File`"
enum TopShelfServiceQuery { Component_ = 1, File_, ServiceName, DisplayName, Description, Instance, Account, UserName, Password, HowToStart };

extern "C" UINT __stdcall TopShelfService(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CTopShelfService installCAD, installRollbackCAD, uninstallCAD, uninstallRollbackCAD;
	DWORD dwRes = 0;
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_TopShelfService");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_DeletePath'. Have you authored 'PanelSw:DeletePath' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(TopShelfService_QUERY, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", TopShelfService_QUERY);
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString fileId, fileFormat, file, component, serviceName, displayName, description, instance, userName, password;
		int howToStart = 0;
		int account = 0;
		WCA_TODO compToDo = WCA_TODO::WCA_TODO_UNKNOWN;
		bool install = true;

		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::File_, (LPWSTR*)fileId);
		BreakExitOnFailure(hr, "Failed to get File ID.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::Component_, (LPWSTR*)component);
		BreakExitOnFailure(hr, "Failed to get Component ID.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::ServiceName, (LPWSTR*)serviceName);
		BreakExitOnFailure(hr, "Failed to get ServiceName.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::DisplayName, (LPWSTR*)displayName);
		BreakExitOnFailure(hr, "Failed to get DisplayName.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::Description, (LPWSTR*)description);
		BreakExitOnFailure(hr, "Failed to get Description.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::Instance, (LPWSTR*)instance);
		BreakExitOnFailure(hr, "Failed to get Instance.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::UserName, (LPWSTR*)userName);
		BreakExitOnFailure(hr, "Failed to get UserName.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::Password, (LPWSTR*)password);
		BreakExitOnFailure(hr, "Failed to get Password.");
		hr = WcaGetRecordInteger(hRecord, TopShelfServiceQuery::HowToStart, &howToStart);
		BreakExitOnFailure(hr, "Failed to get HowToStart.");
		hr = WcaGetRecordInteger(hRecord, TopShelfServiceQuery::Account, &account);
		BreakExitOnFailure(hr, "Failed to get Account.");

		// Test condition
		compToDo = WcaGetComponentToDo(component);
		switch (compToDo)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
		case WCA_TODO::WCA_TODO_REINSTALL:
			WcaLog(LOGMSG_STANDARD, "Will install service '%ls'.", (LPCWSTR)displayName);
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			WcaLog(LOGMSG_STANDARD, "Will uninstall service '%ls'.", (LPCWSTR)displayName);
			install = false;
			break;

		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGMSG_STANDARD, "Will skip service '%ls'.", (LPCWSTR)displayName);
			continue;
		}

		hr = fileFormat.Format(L"[#%s]", (LPCWSTR)fileId);
		BreakExitOnFailure(hr, "Failed formatting string");

		hr = file.MsiFormat(fileFormat);
		BreakExitOnFailure(hr, "Failed MSI-formatting string");

		// Component condition is false
		if (file.IsNullOrEmpty())
		{
			WcaLog(LOGMSG_STANDARD, "Will skip service '%ls'.", (LPCWSTR)displayName);
			continue;
		}

		// Validate accout
		BreakExitOnNull(((account != TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_custom) || !userName.IsNullOrEmpty()), hr, E_INVALIDARG, "TopShelf Service '%ls' has custom account but account user name is not specified", (LPCWSTR)displayName);

		if (install)
		{
			hr = installRollbackCAD.AddUninstall(file, instance);
			BreakExitOnFailure(hr, "Failed scheduling service install-rollback");

			hr = installCAD.AddInstall(file, serviceName, displayName, description, instance, userName, password, (TopShelfServiceDetails_HowToStart)howToStart, (TopShelfServiceDetails_ServiceAccount)account);
			BreakExitOnFailure(hr, "Failed scheduling service install");
		}
		else
		{
			hr = uninstallRollbackCAD.AddInstall(file, serviceName, displayName, description, instance, userName, password, (TopShelfServiceDetails_HowToStart)howToStart, (TopShelfServiceDetails_ServiceAccount)account);
			BreakExitOnFailure(hr, "Failed scheduling service uninstall-rollback");

			hr = uninstallCAD.AddUninstall(file, instance);
			BreakExitOnFailure(hr, "Failed scheduling service uninstall");
		}
	}

	hr = installRollbackCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaSetProperty(L"TopShelfService_InstallRollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting property");

	ReleaseNullStr(szCustomActionData);
	hr = installCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaSetProperty(L"TopShelfService_Install", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting property");

	ReleaseNullStr(szCustomActionData);
	hr = uninstallRollbackCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaSetProperty(L"TopShelfService_UninstallRollback", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting property");

	ReleaseNullStr(szCustomActionData);
	hr = uninstallCAD.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaSetProperty(L"TopShelfService_Uninstall", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting property");

LExit:
	ReleaseStr(szCustomActionData);
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CTopShelfService::AddInstall(LPCWSTR file, LPCWSTR serviceName, LPCWSTR displayName, LPCWSTR description, LPCWSTR instance, LPCWSTR userName, LPCWSTR passowrd, TopShelfServiceDetails_HowToStart howToStart, TopShelfServiceDetails_ServiceAccount account)
{
	HRESULT hr = S_OK;
	Command *pCmd = nullptr;
	TopShelfServiceDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CTopShelfService", &pCmd);
	BreakExitOnFailure(hr, "Failed to add command");

	pDetails = new TopShelfServiceDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_install(true);
	pDetails->set_file(file, WSTR_BYTE_SIZE(file));
	pDetails->set_servicename(serviceName, WSTR_BYTE_SIZE(serviceName));
	pDetails->set_displayname(displayName, WSTR_BYTE_SIZE(displayName));
	pDetails->set_description(description, WSTR_BYTE_SIZE(description));
	pDetails->set_instance(instance, WSTR_BYTE_SIZE(instance));
	pDetails->set_username(userName, WSTR_BYTE_SIZE(userName));
	pDetails->set_password(passowrd, WSTR_BYTE_SIZE(passowrd));
	pDetails->set_howtostart(howToStart);
	pDetails->set_account(account);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CTopShelfService::AddUninstall(LPCWSTR file, LPCWSTR instance)
{
	HRESULT hr = S_OK;
	Command *pCmd = nullptr;
	TopShelfServiceDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CTopShelfService", &pCmd);
	BreakExitOnFailure(hr, "Failed to add command");

	pDetails = new TopShelfServiceDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_install(false);
	pDetails->set_file(file, WSTR_BYTE_SIZE(file));
	pDetails->set_instance(instance, WSTR_BYTE_SIZE(instance));

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CTopShelfService::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	TopShelfServiceDetails details;
	CWixString commandLine;

	bRes = details.ParseFromString(command);
	BreakExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking TopShelfServiceDetails");

	hr = BuildCommandLine(&details, &commandLine);
	BreakExitOnFailure(hr, "Failed to create TopShelf command line");

	hr = QuietExecEx(commandLine, INFINITE, FALSE, TRUE);
	BreakExitOnFailure(hr, "TopShelf command failed");

LExit:
	return hr;
}

HRESULT CTopShelfService::BuildCommandLine(const ::com::panelsw::ca::TopShelfServiceDetails *pDetails, CWixString *pCommandLine)
{
	HRESULT hr = S_OK;
	CWixString file;
	CWixString logCommand;
	LPCWSTR instance = nullptr;
	LPCWSTR userName = nullptr;
	LPCWSTR password = nullptr;
	LPCWSTR serviceName = nullptr;
	LPCWSTR displayName = nullptr;
	LPCWSTR description = nullptr;

	hr = file.Copy((LPCWSTR)pDetails->file().c_str());
	BreakExitOnFailure(hr, "Failed allocating string");

	hr = PathEnsureQuoted((LPWSTR*)file, FALSE);
	BreakExitOnFailure(hr, "Failed ensuring file path is quoted");

	hr = pCommandLine->Format(L"%s %s"
		, (LPCWSTR)file
		, pDetails->install() ? L"install" : L"uninstall"
	);
	BreakExitOnFailure(hr, "Failed formatting string");

	instance = (LPCWSTR)pDetails->instance().data();
	if (instance && *instance)
	{
		hr = pCommandLine->AppnedFormat(L" -instance \"%s\"", instance);
		BreakExitOnFailure(hr, "Failed formatting string");
	}

	// Uninstall - command is ready
	if (!pDetails->install())
	{
		goto LCommandForLog;
	}

	serviceName = (LPCWSTR)pDetails->servicename().data();
	if (serviceName && *serviceName)
	{
		hr = pCommandLine->AppnedFormat(L" -servicename \"%s\"", serviceName);
		BreakExitOnFailure(hr, "Failed formatting string");
	}
	displayName = (LPCWSTR)pDetails->displayname().data();
	if (displayName && *displayName)
	{
		hr = pCommandLine->AppnedFormat(L" -displayname \"%s\"", displayName);
		BreakExitOnFailure(hr, "Failed formatting string");
	}
	description = (LPCWSTR)pDetails->description().data();
	if (description && *description)
	{
		hr = pCommandLine->AppnedFormat(L" -description \"%s\"", description);
		BreakExitOnFailure(hr, "Failed formatting string");
	}

	switch (pDetails->account())
	{
	case TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_custom:
		userName = (LPCWSTR)pDetails->username().data();

		hr = pCommandLine->AppnedFormat(L" -username \"%s\"", userName);
		BreakExitOnFailure(hr, "Failed formatting string");

		// Password will be added later, when formatting command line for log so it will be hidden.
		break;

	case TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_localservice:
		hr = pCommandLine->AppnedFormat(L" --localservice");
		BreakExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_localsystem:
		hr = pCommandLine->AppnedFormat(L" --localsystem");
		BreakExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_networkservice:
		hr = pCommandLine->AppnedFormat(L" --networkservice");
		BreakExitOnFailure(hr, "Failed formatting string");
		break;

	default:
		BreakExitOnFailure((hr = E_INVALIDARG), "Illegal service account");
		break;
	}

	switch (pDetails->howtostart())
	{
	case TopShelfServiceDetails_HowToStart::TopShelfServiceDetails_HowToStart_auto_:
		hr = pCommandLine->AppnedFormat(L" --autostart");
		BreakExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_HowToStart::TopShelfServiceDetails_HowToStart_delayedAuto:
		hr = pCommandLine->AppnedFormat(L" --delayed");
		BreakExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_HowToStart::TopShelfServiceDetails_HowToStart_disabled:
		hr = pCommandLine->AppnedFormat(L" --disabled");
		BreakExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_HowToStart::TopShelfServiceDetails_HowToStart_manual:
		hr = pCommandLine->AppnedFormat(L" --manual");
		BreakExitOnFailure(hr, "Failed formatting string");
		break;

	default:
		BreakExitOnFailure((hr = E_INVALIDARG), "Illegal service start");
		break;
	}
	
LCommandForLog: // Dump command line to log without password.

	hr = logCommand.Copy(*pCommandLine);
	BreakExitOnFailure(hr, "Failed copying string");

	if (!pDetails->install() && (pDetails->account() == TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_custom))
	{
		password = (LPCWSTR)pDetails->password().data();
		if (password && password)
		{
			hr = logCommand.AppnedFormat(L" -password \"*********\"");
			BreakExitOnFailure(hr, "Failed formatting string");

			hr = pCommandLine->AppnedFormat(L" -password \"%s\"", password);
			BreakExitOnFailure(hr, "Failed formatting string");
		}
	}

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing TopShelf command '%ls'", (LPCWSTR)logCommand);

LExit:

	return hr;
}

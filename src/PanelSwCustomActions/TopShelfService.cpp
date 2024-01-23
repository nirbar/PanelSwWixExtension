#include "pch.h"
#include "TopShelfService.h"
#include <Shellapi.h>
#include <Shlwapi.h>
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

#define TopShelfService_QUERY L"SELECT `File`.`Component_`, `PSW_TopShelf`.`File_`, `PSW_TopShelf`.`ServiceName`, `PSW_TopShelf`.`DisplayName`, `PSW_TopShelf`.`Description`, `PSW_TopShelf`.`Instance`, " \
							  L"`PSW_TopShelf`.`Account`, `PSW_TopShelf`.`UserName`, `PSW_TopShelf`.`Password`, `PSW_TopShelf`.`HowToStart`, `PSW_TopShelf`.`ErrorHandling` " \
							  L"FROM `PSW_TopShelf`, `File` " \
							  L"WHERE `PSW_TopShelf`.`File_` = `File`.`File`"
enum TopShelfServiceQuery { Component_ = 1, File_, ServiceName, DisplayName, Description, Instance, Account, UserName, Password, HowToStart, ErrorHandling };

extern "C" UINT __stdcall TopShelf(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CTopShelfService installCAD, installRollbackCAD, uninstallCAD, uninstallRollbackCAD;
	DWORD dwRes = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_TopShelf");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_TopShelf'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_TopShelf'. Have you authored 'PanelSw:TopShelf' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(TopShelfService_QUERY, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", TopShelfService_QUERY);

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString fileId, fileFormat, file, component, serviceName, displayName, description, instance, userName, password;
		int howToStart = 0;
		int account = 0;
		int promptOnError = 0;
		WCA_TODO compToDo = WCA_TODO::WCA_TODO_UNKNOWN;
		bool install = true;

		hr = WcaGetRecordString(hRecord, TopShelfServiceQuery::File_, (LPWSTR*)fileId);
		ExitOnFailure(hr, "Failed to get File ID.");
		hr = WcaGetRecordString(hRecord, TopShelfServiceQuery::Component_, (LPWSTR*)component);
		ExitOnFailure(hr, "Failed to get Component ID.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::ServiceName, (LPWSTR*)serviceName);
		ExitOnFailure(hr, "Failed to get ServiceName.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::DisplayName, (LPWSTR*)displayName);
		ExitOnFailure(hr, "Failed to get DisplayName.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::Description, (LPWSTR*)description);
		ExitOnFailure(hr, "Failed to get Description.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::Instance, (LPWSTR*)instance);
		ExitOnFailure(hr, "Failed to get Instance.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::UserName, (LPWSTR*)userName);
		ExitOnFailure(hr, "Failed to get UserName.");
		hr = WcaGetRecordFormattedString(hRecord, TopShelfServiceQuery::Password, (LPWSTR*)password);
		ExitOnFailure(hr, "Failed to get Password.");
		hr = WcaGetRecordInteger(hRecord, TopShelfServiceQuery::HowToStart, &howToStart);
		ExitOnFailure(hr, "Failed to get HowToStart.");
		hr = WcaGetRecordInteger(hRecord, TopShelfServiceQuery::Account, &account);
		ExitOnFailure(hr, "Failed to get Account.");
		hr = WcaGetRecordInteger(hRecord, TopShelfServiceQuery::ErrorHandling, &promptOnError);
		ExitOnFailure(hr, "Failed to get PromptOnError.");

		// Test condition
		compToDo = WcaGetComponentToDo(component);
		switch (compToDo)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
		case WCA_TODO::WCA_TODO_REINSTALL:
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			install = false;
			break;

		case WCA_TODO::WCA_TODO_UNKNOWN:
		default:
			WcaLog(LOGMSG_STANDARD, "Will skip TopShelf service for file '%ls'.", (LPCWSTR)fileId);
			continue;
		}

		hr = fileFormat.Format(L"[#%ls]", (LPCWSTR)fileId);
		ExitOnFailure(hr, "Failed formatting string");

		hr = file.MsiFormat(fileFormat);
		ExitOnFailure(hr, "Failed MSI-formatting string");

		// Component condition is false
		if (file.IsNullOrEmpty())
		{
			WcaLog(LOGMSG_STANDARD, "Will skip service '%ls'.", (LPCWSTR)displayName);
			continue;
		}

		// Validate accout
		ExitOnNull(((account != TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_custom) || !userName.IsNullOrEmpty()), hr, E_INVALIDARG, "TopShelf Service '%ls' has custom account but account user name is not specified", (LPCWSTR)displayName);

		if (install)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will install TopShelf service for file '%ls'", (LPCWSTR)file);

			hr = installRollbackCAD.AddUninstall((LPCWSTR)file, (LPCWSTR)instance);
			ExitOnFailure(hr, "Failed scheduling service install-rollback");

			hr = installCAD.AddInstall((LPCWSTR)file, (LPCWSTR)serviceName, (LPCWSTR)displayName, (LPCWSTR)description, (LPCWSTR)instance, (LPCWSTR)userName, (LPCWSTR)password, (TopShelfServiceDetails_HowToStart)howToStart, (TopShelfServiceDetails_ServiceAccount)account, (com::panelsw::ca::ErrorHandling)promptOnError);
			ExitOnFailure(hr, "Failed scheduling service install");
		}
		else
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will uninstall TopShelf service '%ls'", (LPCWSTR)file);

			hr = uninstallRollbackCAD.AddInstall((LPCWSTR)file, (LPCWSTR)serviceName, (LPCWSTR)displayName, (LPCWSTR)description, (LPCWSTR)instance, (LPCWSTR)userName, (LPCWSTR)password, (TopShelfServiceDetails_HowToStart)howToStart, (TopShelfServiceDetails_ServiceAccount)account, (com::panelsw::ca::ErrorHandling)promptOnError);
			ExitOnFailure(hr, "Failed scheduling service uninstall-rollback");

			hr = uninstallCAD.AddUninstall((LPCWSTR)file, (LPCWSTR)instance);
			ExitOnFailure(hr, "Failed scheduling service uninstall");
		}
	}

	hr = installRollbackCAD.SetCustomActionData(L"TopShelfService_InstallRollback");
	ExitOnFailure(hr, "Failed setting property");

	hr = installCAD.SetCustomActionData(L"TopShelfService_Install");
	ExitOnFailure(hr, "Failed setting property");

	hr = uninstallRollbackCAD.SetCustomActionData(L"TopShelfService_UninstallRollback");
	ExitOnFailure(hr, "Failed setting property");

	hr = uninstallCAD.SetCustomActionData(L"TopShelfService_Uninstall");
	ExitOnFailure(hr, "Failed setting property");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

CTopShelfService::CTopShelfService() 
	: CDeferredActionBase("TopShelf")
	, _errorPrompter(PSW_MSI_MESSAGES::PSW_MSI_MESSAGES_TOPSHELF_ERROR) 
{ }

HRESULT CTopShelfService::AddInstall(LPCWSTR file, LPCWSTR serviceName, LPCWSTR displayName, LPCWSTR description, LPCWSTR instance, LPCWSTR userName, LPCWSTR passowrd, TopShelfServiceDetails_HowToStart howToStart, TopShelfServiceDetails_ServiceAccount account, com::panelsw::ca::ErrorHandling promptOnError)
{
	HRESULT hr = S_OK;
	Command* pCmd = nullptr;
	TopShelfServiceDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CTopShelfService", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new TopShelfServiceDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

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
	pDetails->set_errorhandling(promptOnError);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CTopShelfService::AddUninstall(LPCWSTR file, LPCWSTR instance)
{
	HRESULT hr = S_OK;
	Command* pCmd = nullptr;
	TopShelfServiceDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CTopShelfService", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new TopShelfServiceDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_install(false);
	pDetails->set_file(file, WSTR_BYTE_SIZE(file));
	pDetails->set_instance(instance, WSTR_BYTE_SIZE(instance));

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CTopShelfService::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	TopShelfServiceDetails details;
	CWixString commandLine;
	CWixString commandLineCopy;
	LPCWSTR fileName = nullptr;
	MSIHANDLE hMsi = NULL;
	bool isDeferred = false;

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking TopShelfServiceDetails");

	_errorPrompter.SetErrorHandling((PSW_ERROR_HANDLING)details.errorhandling());

	hr = BuildCommandLine(&details, &commandLine);
	ExitOnFailure(hr, "Failed to create TopShelf command line");

	hMsi = WcaGetInstallHandle();
	ExitOnNull(hMsi, hr, E_FAIL, "Failed getting MSI handle");

	isDeferred = ::MsiGetMode(hMsi, MSIRUNMODE::MSIRUNMODE_SCHEDULED);

LRetry:

	hr = commandLineCopy.Copy(commandLine);
	ExitOnFailure(hr, "Failed to copy string");

	hr = QuietExec(commandLineCopy, INFINITE, FALSE, TRUE);
	if (FAILED(hr) && isDeferred)
	{
		hr = _errorPrompter.Prompt(fileName);
		if (hr == E_RETRY)
		{
			hr = S_OK;
			goto LRetry;
		}
	}
	ExitOnFailure(hr, "TopShelf command failed");

LExit:
	return hr;
}

HRESULT CTopShelfService::BuildCommandLine(const ::com::panelsw::ca::TopShelfServiceDetails* pDetails, CWixString* pCommandLine)
{
	HRESULT hr = S_OK;
	LPWSTR file = nullptr;
	LPCWSTR instance = nullptr;
	LPCWSTR userName = nullptr;
	LPCWSTR password = nullptr;
	LPCWSTR serviceName = nullptr;
	LPCWSTR displayName = nullptr;
	LPCWSTR description = nullptr;
	CWixString logCommand;

	hr = StrAllocString(&file, (LPCWSTR)(LPVOID)pDetails->file().data(), 0);
	ExitOnFailure(hr, "Failed allocating string");

	if (::StrChrW(file, '"') == nullptr)
	{
		hr = StrAllocPrefix(&file, L"\"", 0);
		ExitOnFailure(hr, "Failed allocating string");
		hr = StrAllocConcat(&file, L"\"", 0);
		ExitOnFailure(hr, "Failed allocating string");
	}

	hr = pCommandLine->Format(L"%ls %ls", file, pDetails->install() ? L"install" : L"uninstall");
	ExitOnFailure(hr, "Failed formatting string");

	instance = (LPCWSTR)(LPVOID)pDetails->instance().data();
	if (instance && *instance)
	{
		hr = pCommandLine->AppnedFormat(L" -instance \"%ls\"", instance);
		ExitOnFailure(hr, "Failed formatting string");
	}

	// Uninstall - command is ready
	if (!pDetails->install())
	{
		goto LCommandForLog;
	}

	serviceName = (LPCWSTR)(LPVOID)pDetails->servicename().data();
	if (serviceName && *serviceName)
	{
		hr = pCommandLine->AppnedFormat(L" -servicename \"%ls\"", serviceName);
		ExitOnFailure(hr, "Failed formatting string");
	}
	displayName = (LPCWSTR)(LPVOID)pDetails->displayname().data();
	if (displayName && *displayName)
	{
		hr = pCommandLine->AppnedFormat(L" -displayname \"%ls\"", displayName);
		ExitOnFailure(hr, "Failed formatting string");
	}
	description = (LPCWSTR)(LPVOID)pDetails->description().data();
	if (description && *description)
	{
		hr = pCommandLine->AppnedFormat(L" -description \"%ls\"", description);
		ExitOnFailure(hr, "Failed formatting string");
	}

	switch (pDetails->account())
	{
	case TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_custom:
		userName = (LPCWSTR)(LPVOID)pDetails->username().data();

		hr = pCommandLine->AppnedFormat(L" -username \"%ls\"", userName);
		ExitOnFailure(hr, "Failed formatting string");

		// Password will be added later, when formatting command line for log so it will be hidden.
		break;

	case TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_localservice:
		hr = pCommandLine->AppnedFormat(L" --localservice");
		ExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_localsystem:
		hr = pCommandLine->AppnedFormat(L" --localsystem");
		ExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_networkservice:
		hr = pCommandLine->AppnedFormat(L" --networkservice");
		ExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_noAccount:
		break;

	default:
		ExitOnFailure((hr = E_INVALIDARG), "Illegal service account");
		break;
	}

	switch (pDetails->howtostart())
	{
	case TopShelfServiceDetails_HowToStart::TopShelfServiceDetails_HowToStart_auto_:
		hr = pCommandLine->AppnedFormat(L" --autostart");
		ExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_HowToStart::TopShelfServiceDetails_HowToStart_delayedAuto:
		hr = pCommandLine->AppnedFormat(L" --delayed");
		ExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_HowToStart::TopShelfServiceDetails_HowToStart_disabled:
		hr = pCommandLine->AppnedFormat(L" --disabled");
		ExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_HowToStart::TopShelfServiceDetails_HowToStart_manual:
		hr = pCommandLine->AppnedFormat(L" --manual");
		ExitOnFailure(hr, "Failed formatting string");
		break;

	case TopShelfServiceDetails_HowToStart::TopShelfServiceDetails_HowToStart_noStart:
		break;

	default:
		ExitOnFailure((hr = E_INVALIDARG), "Illegal service start");
		break;
	}

LCommandForLog: // Dump command line to log without password.

	hr = logCommand.Copy((LPCWSTR)*pCommandLine);
	ExitOnFailure(hr, "Failed copying string");

	if (!pDetails->install() && (pDetails->account() == TopShelfServiceDetails_ServiceAccount::TopShelfServiceDetails_ServiceAccount_custom))
	{
		password = (LPCWSTR)(LPVOID)pDetails->password().data();
		if (password && *password)
		{
			hr = logCommand.AppnedFormat(L" -password \"*********\"");
			ExitOnFailure(hr, "Failed formatting string");

			hr = pCommandLine->AppnedFormat(L" -password \"%ls\"", password);
			ExitOnFailure(hr, "Failed formatting string");
		}
	}

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing TopShelf command '%ls'", (LPCWSTR)logCommand);

LExit:
	ReleaseStr(file);

	return hr;
}

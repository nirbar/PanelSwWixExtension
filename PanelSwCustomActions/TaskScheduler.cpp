#include "TaskScheduler.h"
#include "../CaCommon/WixString.h"
#include "taskSchedulerDetails.pb.h"
using namespace ::com::panelsw::ca;
using namespace google::protobuf;
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

#define TaskScheduler_QUERY L"SELECT `TaskName`, `Component_`, `TaskXml` FROM `PSW_TaskScheduler`"
enum TaskSchedulerQuery { TaskName = 1, Component = 2, TaskXml = 3 };

static LPCWSTR ACTION_CREATE = L"Create";
static LPCWSTR ACTION_DELETE = L"Remove";
static LPCWSTR ACTION_BACKUP = L"Backup";
static LPCWSTR ACTION_RESTORE = L"Restore";

extern "C" UINT __stdcall TaskScheduler(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPWSTR szCustomActionData = nullptr;
	DWORD dwRes = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	// Create these after logging has been initialized
	CTaskScheduler oDeferred;
	CTaskScheduler oRollback;
	CFileOperations oCommit;

	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_FileRegex exists.
	hr = WcaTableExists(L"PSW_TaskScheduler");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_TaskScheduler'. Have you authored 'PanelSw:TaskScheduler' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(TaskScheduler_QUERY, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", TaskScheduler_QUERY);
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szTaskName, szComponent, szTaskXml;
		CWixString tempFile;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		int nIgnoreCase = 0;

		hr = WcaGetRecordString(hRecord, TaskSchedulerQuery::TaskName, (LPWSTR*)szTaskName);
		BreakExitOnFailure(hr, "Failed to get TaskName.");
		hr = WcaGetRecordString(hRecord, TaskSchedulerQuery::Component, (LPWSTR*)szComponent);
		BreakExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, TaskSchedulerQuery::TaskXml, (LPWSTR*)szTaskXml);
		BreakExitOnFailure(hr, "Failed to get TaskXml.");

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
		case WCA_TODO::WCA_TODO_REINSTALL:
			WcaLog(LOGMSG_STANDARD, "Will create task '%ls'.", (LPCWSTR)szTaskName);
			hr = oDeferred.AddRollbackTask(szTaskName, &oRollback, &oCommit);
			BreakExitOnFailure(hr, "Failed scheduling rollback of task '%ls'", (LPCWSTR)szTaskName);
			hr = oDeferred.AddCreateTask(szTaskName, szTaskXml);
			BreakExitOnFailure(hr, "Failed scheduling creation of task '%ls'", (LPCWSTR)szTaskName);
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			WcaLog(LOGMSG_STANDARD, "Will delete task '%ls'.", (LPCWSTR)szTaskName);
			hr = oDeferred.AddRollbackTask(szTaskName, &oRollback, &oCommit);
			BreakExitOnFailure(hr, "Failed scheduling rollback of task '%ls'", (LPCWSTR)szTaskName);
			hr = oDeferred.AddRemoveTask(szTaskName);
			BreakExitOnFailure(hr, "Failed scheduling removal of task '%ls'", (LPCWSTR)szTaskName);
			break;

		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGMSG_STANDARD, "Component '%ls' action is unknown. Skipping configuration of task '%ls'.", (LPCWSTR)szComponent, (LPCWSTR)szTaskName);
			break;
		}
	}

	ReleaseNullStr(szCustomActionData);
	hr = oCommit.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for commit action.");
	hr = WcaDoDeferredAction(L"TaskScheduler_commit", szCustomActionData, oCommit.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling commit action.");

	// Rollback deletes same files as commit does (after importing tasks).
	hr = oCommit.Prepend(&oRollback);
	BreakExitOnFailure(hr, "Failed pre-pending custom action data for deferred action.");
	hr = oCommit.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback action.");
	hr = WcaDoDeferredAction(L"TaskScheduler_rollback", szCustomActionData, oCommit.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling rollback action.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferred.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"TaskScheduler_deferred", szCustomActionData, oDeferred.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

LExit:
	ReleaseStr(szCustomActionData);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CTaskScheduler::AddCreateTask(LPCWSTR szTaskName, LPCWSTR szTaskXml)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	TaskSchedulerDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CTaskScheduler", &pCmd);
	BreakExitOnFailure(hr, "Failed to add XML element");

	pDetails = new TaskSchedulerDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_name(szTaskName, WSTR_BYTE_SIZE(szTaskName));
	pDetails->set_taskxml(szTaskXml, WSTR_BYTE_SIZE(szTaskXml));
	pDetails->set_action(TaskSchedulerDetails_Action::TaskSchedulerDetails_Action_Create);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CTaskScheduler::AddRemoveTask(LPCWSTR szTaskName)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	TaskSchedulerDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CTaskScheduler", &pCmd);
	BreakExitOnFailure(hr, "Failed to add XML element");

	pDetails = new TaskSchedulerDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_name(szTaskName, WSTR_BYTE_SIZE(szTaskName));
	pDetails->set_action(TaskSchedulerDetails_Action::TaskSchedulerDetails_Action_Delete);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CTaskScheduler::AddRollbackTask(LPCWSTR szTaskName, CTaskScheduler* pRollback, CFileOperations* pCommit)
{
	HRESULT hr = S_OK;
	CComPtr<IRegisteredTask> pTask;

	BreakExitOnNull(pRootFolder_.p, hr, E_FAIL, "Task root folder not set");

	hr = pRootFolder_->GetTask(BSTR(szTaskName), &pTask);
	// New task ==> rollback will delete it.
	if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
	{
		hr = pRollback->AddRemoveTask(szTaskName);
		BreakExitOnFailure(hr, "Failed scheduling removal of task '%ls' on rollback", szTaskName);
		ExitFunction();
	}

	// Task exists and readable ==> Create from XML.
	if (SUCCEEDED(hr))
	{
		CComBSTR xml;
		BreakExitOnNull(pTask, hr, E_FAIL, "Failed getting task '%ls'", szTaskName);

		hr = pTask->get_Xml(&xml);
		BreakExitOnFailure(hr, "Failed getting existing task '%ls' XML definition", szTaskName);

		hr = pRollback->AddCreateTask(szTaskName, (LPWSTR)xml);
		BreakExitOnFailure(hr, "Failed scheduling re-creation of task '%ls' on rollback", szTaskName);
		ExitFunction();
	}

	// Access denied ==> Backup up when deferred
	if (hr == E_ACCESSDENIED)
	{
		CComPtr<IXMLDOMElement> pElem;
		WCHAR szTempFolder[MAX_PATH];
		WCHAR szBackupFile[MAX_PATH];
		DWORD dwRes = ERROR_SUCCESS;

		// Get temp folder
		dwRes = ::GetTempPath(MAX_PATH, szTempFolder);
		BreakExitOnNullWithLastError(dwRes, hr, "Failed getting temporary folder");

		dwRes = ::GetTempFileName(szTempFolder, L"TSK", 0, szBackupFile);
		BreakExitOnNullWithLastError(dwRes, hr, "Failed getting temporary file name");

		hr = AddBackupTask(szTaskName, szBackupFile);
		BreakExitOnFailure(hr, "Failed setting action data to backup task");

		hr = pRollback->AddRestoreTask(szTaskName, szBackupFile);
		BreakExitOnFailure(hr, "Failed setting rollback action data");

		hr = pCommit->AddDeleteFile(szBackupFile);
		BreakExitOnFailure(hr, "Failed setting commit action data");

		ExitFunction();
	}

	// Other failures will fall through

LExit:
	return hr;
}

// Execute the command object
HRESULT CTaskScheduler::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	TaskSchedulerDetails details;
	TaskSchedulerDetails_Action eAction;

	bRes = details.ParseFromString(command);
	BreakExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking TaskSchedulerDetails");

	eAction = details.action();

	switch (eAction)
	{
	case TaskSchedulerDetails_Action::TaskSchedulerDetails_Action_Create:
	{
		LPCWSTR szName = nullptr;
		LPCWSTR szXml = nullptr;

		szName = (LPCWSTR)details.name().data();
		szXml = (LPCWSTR)details.taskxml().data();

		hr = CreateTask(szName, szXml);
		BreakExitOnFailure(hr, "Failed to create task '%ls'", szName);
	}
	break;

	case TaskSchedulerDetails_Action::TaskSchedulerDetails_Action_Delete:
	{
		LPCWSTR szName = nullptr;

		szName = (LPCWSTR)details.name().data();

		hr = RemoveTask(szName);
		BreakExitOnFailure(hr, "Failed to create task '%ls'", szName);
	}
	break;

	case TaskSchedulerDetails_Action::TaskSchedulerDetails_Action_Backup:
	{
		LPCWSTR szName = nullptr;
		LPCWSTR szBackupFile = nullptr;

		szName = (LPCWSTR)details.name().data();
		szBackupFile = (LPCWSTR)details.backupfile().data();

		hr = BackupTask(szName, szBackupFile);
		BreakExitOnFailure(hr, "Failed to backup task '%ls'", szName);
	}
	break;

	default:
		LPCWSTR szName = nullptr;
		szName = (LPCWSTR)details.name().data();
		hr = E_INVALIDARG;
		BreakExitOnFailure(hr, "Bad action for task '%ls'", szName);
	}

LExit:
	return hr;
}

HRESULT CTaskScheduler::CreateTask(LPCWSTR szTaskName, LPCWSTR szTaskXml)
{
	HRESULT hr = S_OK;
	CComPtr<ITaskDefinition> pTask;
	CComPtr<IRegisteredTask> pRegTask;

	BreakExitOnNull(pRootFolder_.p, hr, E_FAIL, "Task root folder not set");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Creating task '%ls'", szTaskName);

	hr = pService_->NewTask(NULL, &pTask);
	BreakExitOnFailure(hr, "Failed creating a new task for '%ls'", szTaskName);

	hr = pTask->put_XmlText(CComBSTR(szTaskXml));
	BreakExitOnFailure(hr, "Failed setting task XML for '%ls'", szTaskName);

	hr = pRootFolder_->RegisterTaskDefinition(CComBSTR(szTaskName), pTask, TASK_CREATE_OR_UPDATE, CComVariant(), CComVariant(), TASK_LOGON_NONE, CComVariant(), &pRegTask);
	BreakExitOnFailure(hr, "Failed creating task '%ls'", szTaskName);

LExit:
	return hr;
}

HRESULT CTaskScheduler::RemoveTask(LPCWSTR szTaskName)
{
	HRESULT hr = S_OK;
	
	BreakExitOnNull(pRootFolder_.p, hr, E_FAIL, "Task root folder not set");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Removing task '%ls'", szTaskName);

	hr = pRootFolder_->DeleteTask(CComBSTR(szTaskName), NULL);
	if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Task '%ls' does not exist", szTaskName);
		ExitFunction1(hr = S_FALSE);
	}
	BreakExitOnFailure(hr, "Failed deleting task '%ls'", szTaskName);

LExit:
	return hr;
}

HRESULT CTaskScheduler::AddBackupTask(LPCWSTR szTaskName, LPCWSTR szBackupFile)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	TaskSchedulerDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CTaskScheduler", &pCmd);
	BreakExitOnFailure(hr, "Failed to add XML element");

	pDetails = new TaskSchedulerDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_name(szTaskName, WSTR_BYTE_SIZE(szTaskName));
	pDetails->set_backupfile(szBackupFile, WSTR_BYTE_SIZE(szBackupFile));
	pDetails->set_action(TaskSchedulerDetails_Action::TaskSchedulerDetails_Action_Backup);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CTaskScheduler::AddRestoreTask(LPCWSTR szTaskName, LPCWSTR szBackupFile)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	TaskSchedulerDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CTaskScheduler", &pCmd);
	BreakExitOnFailure(hr, "Failed to add XML element");

	pDetails = new TaskSchedulerDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_name(szTaskName, WSTR_BYTE_SIZE(szTaskName));
	pDetails->set_backupfile(szBackupFile, WSTR_BYTE_SIZE(szBackupFile));
	pDetails->set_action(TaskSchedulerDetails_Action::TaskSchedulerDetails_Action_Restore);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CTaskScheduler::BackupTask(LPCWSTR szTaskName, LPCWSTR szBackupFile)
{
	HRESULT hr = S_OK;
	CComBSTR xml;
	CComPtr<IRegisteredTask> pTask;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwJunk = 0;

	BreakExitOnNull(pRootFolder_.p, hr, E_FAIL, "Task root folder not set");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Backing-up task '%ls'", szTaskName);

	hr = pRootFolder_->GetTask(BSTR(szTaskName), &pTask);
	BreakExitOnFailure(hr, "Failed getting existing task '%ls'", szTaskName);
	BreakExitOnNull(pTask, hr, E_FAIL, "Failed getting task '%ls'", szTaskName);

	hr = pTask->get_Xml(&xml);
	BreakExitOnFailure(hr, "Failed getting existing task '%ls' XML definition", szTaskName);

	hFile = ::CreateFile(szBackupFile, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	BreakExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed opening backup file");

	dwJunk = ::WriteFile(hFile, (LPWSTR)xml, xml.ByteLength(), &dwJunk, nullptr);
	BreakExitOnNullWithLastError(dwJunk, hr, "Failed writing to backup file");

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}

	return hr;
}

HRESULT CTaskScheduler::RestoreTask(LPCWSTR szTaskName, LPCWSTR szBackupFile)
{
	HRESULT hr = S_OK;
	LPWSTR szTaskXml = nullptr;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwFileSize;
	DWORD dwStrSize;
	DWORD dwReadSize;
	BOOL bRes = TRUE;

	BreakExitOnNull(pRootFolder_.p, hr, E_FAIL, "Task root folder not set");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Restoring task '%ls'", szTaskName);

	hFile = ::CreateFile(szBackupFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	BreakExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed opening backup file");

	dwFileSize = ::GetFileSize(hFile, nullptr);
	BreakExitOnNullWithLastError((dwFileSize != INVALID_FILE_SIZE), hr, "Failed getting backup file size");
	dwStrSize = (dwFileSize / sizeof(WCHAR));

	hr = StrAlloc(&szTaskXml, dwStrSize + 1);
	BreakExitOnFailure(hr, "Failed allocating memory");
	szTaskXml[dwStrSize] = 0;

	bRes = ::ReadFile(hFile, szTaskXml, dwFileSize, &dwReadSize, nullptr);
	BreakExitOnNullWithLastError(bRes, hr, "Failed reading backup file");

	hr = CreateTask(szTaskName, szTaskXml);
	BreakExitOnFailure(hr, "Failed restoring task '%ls'", szTaskName);

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}
	ReleaseStr(szTaskXml);

	return hr;
}

CTaskScheduler::CTaskScheduler()
	: bComInit_(false)
{
	HRESULT hr = S_OK;

	hr = ::CoInitialize(nullptr);
	BreakExitOnFailure(hr, "Failed CoInitializeEx");
	bComInit_ = true;

	hr = ::CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService_);
	BreakExitOnFailure(hr, "Failed to CoCreate an instance of the TaskService class");

	hr = pService_->Connect(CComVariant(), CComVariant(), CComVariant(), CComVariant());
	BreakExitOnFailure(hr, "Failed to connecting to task service");

	hr = pService_->GetFolder(BSTR(L"\\"), &pRootFolder_);
	BreakExitOnFailure(hr, "Failed getting root task folder");

LExit:
	;
}

CTaskScheduler::~CTaskScheduler()
{
	if (bComInit_)
	{
		::CoUninitialize();
		bComInit_ = false;
	}
}

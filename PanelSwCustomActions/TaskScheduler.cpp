#include "TaskScheduler.h"
#include "../CaCommon/WixString.h"
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

#define TaskScheduler_QUERY L"SELECT `TaskName`, `Component_`, `TaskXml` FROM `PSW_TaskScheduler`"
enum TaskSchedulerQuery { TaskName = 1, Component = 2, TaskXml = 3 };

static LPCWSTR ACTION_CREATE = L"Create";
static LPCWSTR ACTION_DELETE = L"Remove";
static LPCWSTR ACTION_BACKUP = L"Backup";
static LPCWSTR ACTION_RESTORE = L"Restore";

extern "C" __declspec(dllexport) UINT TaskScheduler(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CComBSTR szCustomActionData;
	DWORD dwRes = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	// Create these after logging has been initialized
	CTaskScheduler oDeferred;
	CTaskScheduler oRollback;
	CFileOperations oCommit;

	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_FileRegex exists.
	hr = WcaTableExists(L"PSW_TaskScheduler");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_TaskScheduler'. Have you authored 'PanelSw:TaskScheduler' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(TaskScheduler_QUERY, &hView);
	BreakExitOnFailure1(hr, "Failed to execute SQL query '%ls'.", TaskScheduler_QUERY);
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
			BreakExitOnFailure1(hr, "Failed scheduling rollback of task '%ls'", (LPCWSTR)szTaskName);
			hr = oDeferred.AddCreateTask(szTaskName, szTaskXml);
			BreakExitOnFailure1(hr, "Failed scheduling creation of task '%ls'", (LPCWSTR)szTaskName);
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			WcaLog(LOGMSG_STANDARD, "Will delete task '%ls'.", (LPCWSTR)szTaskName);
			hr = oDeferred.AddRollbackTask(szTaskName, &oRollback, &oCommit);
			BreakExitOnFailure1(hr, "Failed scheduling rollback of task '%ls'", (LPCWSTR)szTaskName);
			hr = oDeferred.AddRemoveTask(szTaskName);
			BreakExitOnFailure1(hr, "Failed scheduling removal of task '%ls'", (LPCWSTR)szTaskName);
			break;

		case WCA_TODO::WCA_TODO_UNKNOWN:
			hr = E_INVALIDSTATE;
			BreakExitOnFailure(hr, "Bad component to-do");
		}
	}

	szCustomActionData.Empty();
	hr = oCommit.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for commit action.");
	hr = WcaDoDeferredAction(L"TaskScheduler_commit", szCustomActionData, oCommit.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling commit action.");

	hr = oCommit.Prepend(&oRollback);
	BreakExitOnFailure(hr, "Failed pre-pending custom action data for deferred action.");
	hr = oCommit.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for rollback action.");
	hr = WcaDoDeferredAction(L"TaskScheduler_rollback", szCustomActionData, oCommit.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

	szCustomActionData.Empty();
	hr = oDeferred.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"TaskScheduler_deferred", szCustomActionData, oCommit.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CTaskScheduler::AddCreateTask(LPCWSTR szTaskName, LPCWSTR szTaskXml)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"TaskScheduler", L"CTaskScheduler", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("TaskName"), CComVariant(szTaskName));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'TaskName'");

	hr = pElem->setAttribute(CComBSTR("TaskXml"), CComVariant(szTaskXml));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'TaskXml'");

	hr = pElem->setAttribute(CComBSTR("Action"), CComVariant(ACTION_CREATE));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Action'");

LExit:
	return hr;
}

HRESULT CTaskScheduler::AddRemoveTask(LPCWSTR szTaskName)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"TaskScheduler", L"CTaskScheduler", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("TaskName"), CComVariant(szTaskName));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'TaskName'");

	hr = pElem->setAttribute(CComBSTR("Action"), CComVariant(ACTION_DELETE));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Action'");

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
		BreakExitOnFailure1(hr, "Failed scheduling removal of task '%ls' on rollback", szTaskName);
		ExitFunction();
	}

	// Task exists and readable ==> Create from XML.
	if (SUCCEEDED(hr))
	{
		CComBSTR xml;
		BreakExitOnNull1(pTask, hr, E_FAIL, "Failed getting task '%ls'", szTaskName);

		hr = pTask->get_Xml(&xml);
		BreakExitOnFailure1(hr, "Failed getting existing task '%ls' XML definition", szTaskName);

		hr = pRollback->AddCreateTask(szTaskName, (LPWSTR)xml);
		BreakExitOnFailure1(hr, "Failed scheduling re-creation of task '%ls' on rollback", szTaskName);
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

		hr = AddElement(L"TaskScheduler", L"CTaskScheduler", 1, &pElem);
		BreakExitOnFailure(hr, "Failed to add XML element");

		hr = pElem->setAttribute(CComBSTR("TaskName"), CComVariant(szTaskName));
		BreakExitOnFailure(hr, "Failed to add XML attribute 'TaskName'");

		hr = pElem->setAttribute(CComBSTR("BackupFile"), CComVariant(szBackupFile));
		BreakExitOnFailure(hr, "Failed to add XML attribute 'TaskXml'");

		hr = pElem->setAttribute(CComBSTR("Action"), CComVariant(ACTION_BACKUP));
		BreakExitOnFailure(hr, "Failed to add XML attribute 'Action'");

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

// Execute the command object (XML element)
HRESULT CTaskScheduler::DeferredExecute(IXMLDOMElement* pElem)
{
	HRESULT hr = S_OK;
	CComVariant vTaskName;
	CComVariant vAction;

	// Get Parameters:
	hr = pElem->getAttribute(CComBSTR(L"TaskName"), &vTaskName);
	BreakExitOnFailure(hr, "Failed to get TaskName");

	// Get Action
	hr = pElem->getAttribute(CComBSTR(L"Action"), &vAction);
	BreakExitOnFailure1(hr, "Failed to get Action for task '%ls'", vTaskName.bstrVal);

	// Create
	if (::wcscmp(vAction.bstrVal, ACTION_CREATE) == 0)
	{
		CComVariant vTaskXml;

		hr = pElem->getAttribute(CComBSTR(L"TaskXml"), &vTaskXml);
		BreakExitOnFailure1(hr, "Failed to get TaskXml for task '%ls'", vTaskName.bstrVal);

		hr = CreateTask(vTaskName.bstrVal, vTaskXml.bstrVal);
		BreakExitOnFailure1(hr, "Failed to create task '%ls'", vTaskName.bstrVal);
	}
	// Delete
	else if (::wcscmp(vAction.bstrVal, ACTION_DELETE) == 0)
	{
		hr = RemoveTask(vTaskName.bstrVal);
		BreakExitOnFailure1(hr, "Failed to delete task '%ls'", vTaskName.bstrVal);
	}
	// Backup
	else if (::wcscmp(vAction.bstrVal, ACTION_BACKUP) == 0)
	{
		CComVariant vBackupFile;

		hr = pElem->getAttribute(CComBSTR(L"BackupFile"), &vBackupFile);
		BreakExitOnFailure1(hr, "Failed to get BackupFile for task '%ls'", vTaskName.bstrVal);

		hr = BackupTask(vTaskName.bstrVal, vBackupFile.bstrVal);
		BreakExitOnFailure1(hr, "Failed to backup task '%ls'", vTaskName.bstrVal);
	}
	// Restore
	else if (::wcscmp(vAction.bstrVal, ACTION_RESTORE) == 0)
	{
		CComVariant vBackupFile;

		hr = pElem->getAttribute(CComBSTR(L"BackupFile"), &vBackupFile);
		BreakExitOnFailure1(hr, "Failed to get BackupFile for task '%ls'", vTaskName.bstrVal);

		hr = RestoreTask(vTaskName.bstrVal, vBackupFile.bstrVal);
		BreakExitOnFailure1(hr, "Failed to restore task '%ls'", vTaskName.bstrVal);
	}
	// Error
	else
	{
		hr = E_INVALIDARG;
		BreakExitOnFailure2(hr, "Bad action '%ls' for task '%ls'", vAction.bstrVal, vTaskName.bstrVal);
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
	BreakExitOnFailure1(hr, "Failed creating a new task for '%ls'", szTaskName);

	hr = pTask->put_XmlText(CComBSTR(szTaskXml));
	BreakExitOnFailure1(hr, "Failed setting task XML for '%ls'", szTaskName);

	hr = pRootFolder_->RegisterTaskDefinition(CComBSTR(szTaskName), pTask, TASK_CREATE_OR_UPDATE, CComVariant(), CComVariant(), TASK_LOGON_NONE, CComVariant(), &pRegTask);
	BreakExitOnFailure1(hr, "Failed creating task '%ls'", szTaskName);

LExit:
	return hr;
}

HRESULT CTaskScheduler::RemoveTask(LPCWSTR szTaskName)
{
	HRESULT hr = S_OK;
	
	BreakExitOnNull(pRootFolder_.p, hr, E_FAIL, "Task root folder not set");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Removing task '%ls'", szTaskName);

	hr = pRootFolder_->DeleteTask(CComBSTR(szTaskName), NULL);
	BreakExitOnFailure1(hr, "Failed deleting task '%ls'", szTaskName);

LExit:
	return hr;
}

HRESULT CTaskScheduler::AddBackupTask(LPCWSTR szTaskName, LPCWSTR szBackupFile)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"TaskScheduler", L"CTaskScheduler", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("TaskName"), CComVariant(szTaskName));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'TaskName'");

	hr = pElem->setAttribute(CComBSTR("BackupFile"), CComVariant(szBackupFile));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'TaskXml'");

	hr = pElem->setAttribute(CComBSTR("Action"), CComVariant(ACTION_BACKUP));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Action'");

LExit:
	return hr;
}

HRESULT CTaskScheduler::AddRestoreTask(LPCWSTR szTaskName, LPCWSTR szBackupFile)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"TaskScheduler", L"CTaskScheduler", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("TaskName"), CComVariant(szTaskName));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'TaskName'");

	hr = pElem->setAttribute(CComBSTR("BackupFile"), CComVariant(szBackupFile));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'TaskXml'");

	hr = pElem->setAttribute(CComBSTR("Action"), CComVariant(ACTION_RESTORE));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Action'");

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
	BreakExitOnFailure1(hr, "Failed getting existing task '%ls'", szTaskName);
	BreakExitOnNull1(pTask, hr, E_FAIL, "Failed getting task '%ls'", szTaskName);

	hr = pTask->get_Xml(&xml);
	BreakExitOnFailure1(hr, "Failed getting existing task '%ls' XML definition", szTaskName);

	hFile = ::CreateFile(szBackupFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	BreakExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed opening backup file");

	dwJunk = ::WriteFile(hFile, (LPWSTR)xml, xml.ByteLength(), &dwJunk, NULL);
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
	LPWSTR szTaskXml = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwFileSize;
	DWORD dwStrSize;
	DWORD dwReadSize;
	BOOL bRes = TRUE;

	BreakExitOnNull(pRootFolder_.p, hr, E_FAIL, "Task root folder not set");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Restoring task '%ls'", szTaskName);

	hFile = ::CreateFile(szBackupFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	BreakExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed opening backup file");

	dwFileSize = ::GetFileSize(hFile, NULL);
	BreakExitOnNullWithLastError((dwFileSize != INVALID_FILE_SIZE), hr, "Failed getting backup file size");
	dwStrSize = (dwFileSize / sizeof(WCHAR));

	hr = StrAlloc(&szTaskXml, dwStrSize + 1);
	BreakExitOnFailure(hr, "Failed allocating memory");
	szTaskXml[dwStrSize] = 0;

	bRes = ::ReadFile(hFile, szTaskXml, dwFileSize, &dwReadSize, NULL);
	BreakExitOnNullWithLastError(bRes, hr, "Failed reading backup file");

	hr = CreateTask(szTaskName, szTaskXml);
	BreakExitOnFailure1(hr, "Failed restoring task '%ls'", szTaskName);

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

	hr = ::CoInitialize(NULL);
	BreakExitOnFailure(hr, "Failed CoInitializeEx");
	bComInit_ = true;

	hr = ::CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService_);
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

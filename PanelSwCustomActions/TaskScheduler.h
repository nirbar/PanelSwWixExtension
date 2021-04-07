#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "FileOperations.h"
#include <taskschd.h>
#include <atlbase.h>

class CTaskScheduler :
	public CDeferredActionBase
{
public:

	CTaskScheduler() noexcept;
	virtual ~CTaskScheduler() noexcept;

	HRESULT AddCreateTask(LPCWSTR szTaskName, LPCWSTR szTaskXml, LPCWSTR szUser, LPCWSTR szPassword) noexcept;
	HRESULT AddRemoveTask(LPCWSTR szTaskName) noexcept;
	HRESULT AddRollbackTask(LPCWSTR szTaskName, CTaskScheduler* pRollback, CFileOperations* pCommit) noexcept;

protected:
	// Execute the command object
	HRESULT DeferredExecute(const ::std::string& command) noexcept override;

private:
	HRESULT AddBackupTask(LPCWSTR szTaskName, LPCWSTR szBackupFile) noexcept;
	HRESULT AddRestoreTask(LPCWSTR szTaskName, LPCWSTR szBackupFile) noexcept;

	HRESULT CreateTask(LPCWSTR szTaskName, LPCWSTR szTaskXml, LPCWSTR szUser, LPCWSTR szPassword) noexcept;
	HRESULT RemoveTask(LPCWSTR szTaskName) noexcept;
	HRESULT BackupTask(LPCWSTR szTaskName, LPCWSTR szBackupFile) noexcept;
	HRESULT RestoreTask(LPCWSTR szTaskName, LPCWSTR szBackupFile) noexcept;

	// Fields
	bool bComInit_;
	CComPtr<ITaskService> pService_;
	CComPtr<ITaskFolder> pRootFolder_;
};


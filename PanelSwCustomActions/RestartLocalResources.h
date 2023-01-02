#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "restartLocalResourcesDetails.pb.h"
#include <pathutil.h>
#include <list>
#include <map>
#include <Psapi.h>

class CRestartLocalResources :
	public CDeferredActionBase
{
public:

	CRestartLocalResources() : CDeferredActionBase("RestartLocalResources") { }
	~CRestartLocalResources();

	HRESULT AddRestartLocalResources(const std::list<LPWSTR>& lstFolders);

	HRESULT EnumerateLocalProcesses(const std::list<LPWSTR>& lstFolders, std::map<DWORD, LPWSTR>& mapProcId);

	HRESULT RegisterWithRm(const std::list<LPWSTR>& lstFolders);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:

	HRESULT Execute(const std::list<LPWSTR>& lstFolders);
	HRESULT KillOneProcess(DWORD dwProcessId, LPCWSTR szProcessName);

	INT PromptFilesInUse(const std::map<DWORD, LPWSTR> &mapProcId);

	static BOOL CALLBACK KillWindowsProc(HWND hwnd, LPARAM lParam);

	HRESULT Initialize();
	void Uninitialize();

	decltype(::GetProcessImageFileNameW)* pGetProcessImageFileNameW_ = nullptr;
	HMODULE hGetProcessImageFileNameDll_ = NULL;

	// Helper struct to find a folder that contains the exe by path
	struct IsExeInFolder
	{
		WCHAR szFullExePath[MAX_PATH + 1];

		bool operator()(const LPCWSTR& szFolder) const;
	} visInFolder_;
};

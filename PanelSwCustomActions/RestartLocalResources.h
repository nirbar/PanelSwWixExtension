#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "restartLocalResourcesDetails.pb.h"

class CRestartLocalResources :
	public CDeferredActionBase
{
public:

	CRestartLocalResources() : CDeferredActionBase("RestartLocalResources") { }

	HRESULT AddRestartLocalResources(LPCWSTR szFilePath, DWORD dwProcId);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:

	HRESULT Execute(LPCWSTR szFilePath, DWORD dwProcId);

	static BOOL CALLBACK KillWindowsProc(HWND hwnd, LPARAM lParam);
};
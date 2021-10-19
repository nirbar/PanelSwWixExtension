#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "restartLocalResourcesDetails.pb.h"

class CRestartLocalResources :
	public CDeferredActionBase
{
public:

	CRestartLocalResources() noexcept : CDeferredActionBase("RestartLocalResources") { }

	HRESULT AddRestartLocalResources(LPCWSTR szFilePath, DWORD dwProcId) noexcept;

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) noexcept override;

private:

	HRESULT Execute(LPCWSTR szFilePath, DWORD dwProcId) noexcept;

	static BOOL CALLBACK KillWindowsProc(HWND hwnd, LPARAM lParam) noexcept;
};
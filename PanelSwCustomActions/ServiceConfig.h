#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include <map>
#include <list>
#include "ErrorHandling.pb.h"
#include "servciceConfigDetails.pb.h"

class CServiceConfig :
	public CDeferredActionBase
{
public:

	CServiceConfig() : CDeferredActionBase("ServiceConfig") { }

	HRESULT AddServiceConfig(LPCWSTR szServiceName, LPCWSTR szCommandLine, LPCWSTR szAccount, LPCWSTR szPassword, int start, LPCWSTR szLoadOrderGroup, const std::list<LPWSTR> &lstDependencies, ::com::panelsw::ca::ErrorHandling errorHandling, ::com::panelsw::ca::ServciceConfigDetails_DelayStart delayStart, DWORD dwServiceType);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT ExecuteOne(LPCWSTR szServiceName, LPCWSTR szCommandLine, LPCWSTR szAccount, LPCWSTR szPassword, DWORD dwStart, LPCWSTR szLoadOrderGroup, LPCWSTR szDependencies, ::com::panelsw::ca::ServciceConfigDetails_DelayStart nDelayStart, DWORD dwServiceType);
	HRESULT PromptError(LPCWSTR szServiceName, ::com::panelsw::ca::ErrorHandling errorHandling);
};

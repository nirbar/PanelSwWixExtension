#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include <map>
#include "ErrorHandling.pb.h"

class CServiceConfig :
	public CDeferredActionBase
{
public:

	HRESULT AddServiceConfig(LPCWSTR szServiceName, LPCWSTR szCommandLine, LPCWSTR szAccount, LPCWSTR szPassword, int start, LPCWSTR szLoadOrderGroup, LPCWSTR szDependencies, ::com::panelsw::ca::ErrorHandling errorHandling);

protected:
	// Execute the command object (XML element)
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT ExecuteOne(LPCWSTR szServiceName, LPCWSTR szCommandLine, LPCWSTR szAccount, LPCWSTR szPassword, DWORD dwStart, LPCWSTR szLoadOrderGroup, LPCWSTR szDependencies);
	HRESULT PromptError(LPCWSTR szServiceName, ::com::panelsw::ca::ErrorHandling errorHandling);
};


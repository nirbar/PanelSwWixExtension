#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include <map>

class CServiceConfig :
	public CDeferredActionBase
{
public:

	HRESULT AddServiceConfig(LPCWSTR szServiceName, LPCWSTR szAccount, LPCWSTR szPassword, int start);

protected:
	// Execute the command object (XML element)
	HRESULT DeferredExecute(const ::std::string& command) override;
};


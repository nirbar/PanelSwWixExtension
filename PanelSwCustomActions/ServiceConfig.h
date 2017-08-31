#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include <taskschd.h>
#include <map>

class CServiceConfig :
	public CDeferredActionBase
{
public:

	HRESULT AddServiceConfig(LPCWSTR szServiceName, LPCWSTR szAccount, LPCWSTR szPassword);

protected:
	// Execute the command object (XML element)
	virtual HRESULT DeferredExecute(IXMLDOMElement* pElem);
};


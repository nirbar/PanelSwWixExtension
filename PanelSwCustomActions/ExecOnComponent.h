#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include <taskschd.h>

class CExecOnComponent :
	public CDeferredActionBase
{
public:

	HRESULT AddExec(LPCWSTR szCommand, int nFlags);

protected:
	// Execute the command object (XML element)
	virtual HRESULT DeferredExecute(IXMLDOMElement* pElem);
};


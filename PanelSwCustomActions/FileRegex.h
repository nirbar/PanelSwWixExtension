#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CFileRegex :
	public CDeferredActionBase
{
public:

	HRESULT AddFileRegex(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase);

protected:
	// Execute the command object (XML element)
	virtual HRESULT DeferredExecute(IXMLDOMElement* pElem);

private:
	HRESULT Execute(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase);
};


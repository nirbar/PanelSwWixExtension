#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CFileOperations :
	public CDeferredActionBase
{
public:

	HRESULT AddCopyFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0);
	HRESULT AddMoveFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0);
	HRESULT AddDeleteFile(LPCWSTR szFile, int flags = 0);

protected:
	// Execute the command object (XML element)
	virtual HRESULT DeferredExecute(IXMLDOMElement* pElem);

private:

	HRESULT CopyFile(IXMLDOMElement* pElem);
	HRESULT MoveFile(IXMLDOMElement* pElem);
	HRESULT DeleteFile(IXMLDOMElement* pElem);
};


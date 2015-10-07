#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CFileOperations :
	public CDeferredActionBase
{
public:

	HRESULT AddCopyFile(LPCWSTR szFrom, LPCWSTR szTo);
	HRESULT AddMoveFile(LPCWSTR szFrom, LPCWSTR szTo);
	HRESULT AddDeleteFile(LPCWSTR szFile);

protected:
	// Execute the command object (XML element)
	virtual HRESULT DeferredExecute(IXMLDOMElement* pElem);

private:

	HRESULT CopyFile(IXMLDOMElement* pElem);
	HRESULT MoveFile(IXMLDOMElement* pElem);
	HRESULT DeleteFile(IXMLDOMElement* pElem);
};


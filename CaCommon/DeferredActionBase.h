#pragma once
#include "stdafx.h"
#include <windows.h>
#include <objbase.h>
#include <msxml.h>
#include <atlbase.h>

class CDeferredActionBase
{
public:
	CDeferredActionBase();
	virtual ~CDeferredActionBase();

	// Function that maps a receiver name to a CDeferredActionBase inheritor.
	typedef HRESULT(*ReceiverToExecutorFunc)(LPCWSTR szReceiver, CDeferredActionBase** ppExecutor);
	static HRESULT DeferredEntryPoint(ReceiverToExecutorFunc mapFunc);

	// Overriden by inheriting classes. Execute the command object (XML element)
	virtual HRESULT DeferredExecute(IXMLDOMElement* pElem) = 0;

	// Overriden by inheriting classes. Return the receiver name (usually the class name)
	virtual HRESULT GetReceiverName(LPWSTR* pszName) = 0;

protected:

	HRESULT AddElement(LPCWSTR szName, LPCWSTR szReceiver, UINT uCosting, IXMLDOMElement** ppElem);

private:
	CComPtr<IXMLDOMDocument> _pXmlDoc;
};


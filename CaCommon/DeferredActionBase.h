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

	UINT GetCost() const { return _uCost; }

	HRESULT GetCustomActionData(BSTR* pszCustomActionData);

	HRESULT Prepend( CDeferredActionBase* pOther);

protected:

	// Overriden by inheriting classes. Execute the command object (XML element)
	virtual HRESULT DeferredExecute(IXMLDOMElement* pElem) = 0;

	HRESULT AddElement(LPCWSTR szTag, LPCWSTR szReceiver, UINT uCosting, IXMLDOMElement** ppElem);

private:
	CComPtr<IXMLDOMDocument> _pXmlDoc;
	UINT _uCost;
};


#include "DeferredActionBase.h"
#include "WixString.h"

HRESULT CDeferredActionBase::DeferredEntryPoint(ReceiverToExecutorFunc mapFunc)
{
	HRESULT hr = S_OK;
	CWixString szCustomActionData;
	CComPtr<IXMLDOMDocument> pXmlDoc;
	CComPtr<IXMLDOMNodeList> pNodes;
	IXMLDOMNodeList* pTmpNodes = NULL;
	IXMLDOMElement* pCurrElem = NULL;
	VARIANT_BOOL isXmlSuccess;
	LONG nodeCount = 0;
	DOMNodeType nodeType;
	CDeferredActionBase* pExecutor = NULL;

	// Get CustomActionData
	hr = WcaGetProperty(L"CustomActionData", (LPWSTR*)szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting CustomActionData");

	// XML docs.
	hr = ::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXmlDoc);
	BreakExitOnFailure(hr, "Failed to CoCreateInstance CLSID_DOMDocument");

	hr = pXmlDoc->loadXML(szCustomActionData, &isXmlSuccess);
	BreakExitOnFailure(hr, "Failed to load XML");
	if (!isXmlSuccess)
	{
		hr = E_FAIL;
		BreakExitOnFailure(hr, "Failed to load XML");
	}

	hr = pXmlDoc->selectNodes(CComBSTR(L"/Root/*"), &pTmpNodes);
	BreakExitOnFailure(hr, "Failed to select XML nodes");
	BreakExitOnNull(pTmpNodes, hr, E_FAIL, "Failed to select XML nodes");
	pNodes.Attach(pTmpNodes);

	hr = pNodes->get_length(&nodeCount);
	BreakExitOnFailure(hr, "Failed to get node count");

	// Iterate elements
	for (LONG i = 0; i < nodeCount; ++i)
	{
		IXMLDOMNode* pTmpNode = NULL;
		CComVariant vReceiver;
		CComVariant vCost;
		UINT uCost = 0;

		// Get element
		hr = pNodes->get_item(i, &pTmpNode);
		BreakExitOnFailure(hr, "Failed to get node");
		BreakExitOnNull(pTmpNode, hr, E_FAIL, "Failed to get node");

		hr = pTmpNode->get_nodeType(&nodeType);
		BreakExitOnFailure(hr, "Failed to get node type");
		if (nodeType != DOMNodeType::NODE_ELEMENT)
		{
			hr = E_FAIL;
			BreakExitOnFailure(hr, "Expected an element");
		}

		hr = pTmpNode->QueryInterface(IID_IXMLDOMElement, (void**)&pCurrElem);
		BreakExitOnFailure(hr, "Failed quering as IID_IXMLDOMElement");
		BreakExitOnNull(pCurrElem, hr, E_FAIL, "Failed to get IID_IXMLDOMElement");

		// Get receiver
		hr = pCurrElem->getAttribute(CComBSTR(L"Receiver"), &vReceiver);
		BreakExitOnFailure(hr, "Failed to get receiver name");

		hr = mapFunc(vReceiver.bstrVal, &pExecutor);
		BreakExitOnFailure1(hr, "Failed to get CDeferredActionBase for '%ls'", (LPCWSTR)vReceiver.bstrVal);
		//BreakExitOnNull1(pExecutor, hr, E_INVALIDARG, "Failed to get CDeferredActionBase for '%ls'", (LPCWSTR)vReceiver.bstrVal);

		// Execute
		hr = pExecutor->DeferredExecute(pCurrElem);
		BreakExitOnFailure(hr, "Failed");

		// Report costing progress.
		hr = pCurrElem->getAttribute(CComBSTR(L"Cost"), &vCost);
		BreakExitOnFailure(hr, "Failed to get cost");

		uCost = ::wcstoul(vCost.bstrVal, NULL, 10);
		hr = WcaProgressMessage(uCost, FALSE);
		BreakExitOnFailure(hr, "Failed to progress by cost");

		// Release CDeferredActionBase.
		delete pExecutor;
		pExecutor = NULL;
	}

LExit:

	if (pExecutor != NULL)
	{
		delete pExecutor;
		pExecutor = NULL;
	}

	return hr;
}

CDeferredActionBase::CDeferredActionBase()
	: _uCost( 0)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pRoot;
	IXMLDOMNode *pTmpNode = NULL;

	hr = ::CoInitialize(NULL);
	BreakExitOnFailure(hr, "Failed to CoInitialize");

	// XML docs.
	hr = ::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&_pXmlDoc);
	BreakExitOnFailure(hr, "Failed to CoCreateInstance CLSID_DOMDocument");

	// XML root.
	hr = _pXmlDoc->createElement(L"Root", &pRoot);
	BreakExitOnFailure(hr, "Failed to create root XML element");

	// Append root to docs
	hr = _pXmlDoc->appendChild(pRoot, &pTmpNode);
	BreakExitOnFailure(hr, "Failed to append root XML element");
	pRoot.Attach((IXMLDOMElement*)pTmpNode);

LExit:
	return;
}

CDeferredActionBase::~CDeferredActionBase()
{
	_pXmlDoc.Release();

	::CoUninitialize();
}

HRESULT CDeferredActionBase::AddElement(LPCWSTR szName, LPCWSTR szReceiver, UINT uCosting, IXMLDOMElement** ppElem)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> xmlRoot;
	CComPtr<IXMLDOMNode> tmpNode;
	CComBSTR attName = L"";
	CComVariant value;

	hr = _pXmlDoc->get_documentElement(&xmlRoot);
	BreakExitOnFailure(hr, "Failed to get XML root");

	hr = _pXmlDoc->createElement(CComBSTR(szName), ppElem);
	BreakExitOnFailure(hr, "Failed to create XML element");

	attName = L"Receiver";
	value = szReceiver;
	hr = (*ppElem)->setAttribute(attName, value);
	BreakExitOnFailure(hr, "Failed to set XML attribute");

	attName = L"Cost";
	value = uCosting;
	hr = (*ppElem)->setAttribute(attName, value);
	BreakExitOnFailure(hr, "Failed to set XML attribute");

	hr = xmlRoot->appendChild((*ppElem), &tmpNode);
	tmpNode.Release();
	BreakExitOnFailure(hr, "Failed to append XML element");

LExit:
	return hr;
}

HRESULT CDeferredActionBase::GetCustomActionData(BSTR* pszCustomActionData)
{
	HRESULT hr = S_OK;

	hr = _pXmlDoc->get_xml(pszCustomActionData);
	BreakExitOnFailure(hr, "Failed to get XML string");

LExit:
	return hr;
}

HRESULT CDeferredActionBase::Prepend( CDeferredActionBase* pOther)
{
	HRESULT hr = S_OK;
	CWixString szCustomActionData;
	CComPtr<IXMLDOMDocument> pXmlDoc;
	CComPtr<IXMLDOMNodeList> pNodes;
	IXMLDOMNodeList* pTmpNodes = NULL;
	IXMLDOMElement* pRootElem = NULL;
	IXMLDOMNode* pRootFirstChild = NULL;
	LONG nodeCount = 0;

	hr = pOther->_pXmlDoc->selectNodes(CComBSTR(L"/Root/*"), &pTmpNodes);
	BreakExitOnFailure(hr, "Failed to select XML nodes");
	BreakExitOnNull(pTmpNodes, hr, E_FAIL, "Failed to select XML nodes");
	pNodes.Attach(pTmpNodes);

	hr = pNodes->get_length(&nodeCount);
	BreakExitOnFailure(hr, "Failed to get node count");

	hr = _pXmlDoc->get_documentElement( &pRootElem);
	BreakExitOnFailure(hr, "Failed to get XML root element");
	BreakExitOnNull(pRootElem, hr, E_FAIL, "Failed to get XML root element");

	hr = pRootElem->get_firstChild( &pRootFirstChild);
	BreakExitOnFailure(hr, "Failed to get XML root element's first child");

	// Iterate elements
	for (LONG i = 0; i < nodeCount; ++i)
	{
		IXMLDOMNode* pTmpNode = NULL;
		IXMLDOMNode* pTmpNode1 = NULL;
		IXMLDOMNode* pTmpNode2 = NULL;

		// Get element
		hr = pNodes->get_item(i, &pTmpNode);
		BreakExitOnFailure(hr, "Failed to get node");
		BreakExitOnNull(pTmpNode, hr, E_FAIL, "Failed to get node");
		
		hr = pTmpNode->cloneNode( VARIANT_BOOL(true), &pTmpNode1);
		BreakExitOnFailure(hr, "Failed to clone node");
		BreakExitOnNull(pTmpNode1, hr, E_FAIL, "Failed to clone node");

		hr = pRootElem->insertBefore(pTmpNode1, CComVariant( pRootFirstChild), &pTmpNode2);
		BreakExitOnFailure(hr, "Failed to insert node");
		BreakExitOnNull(pTmpNode2, hr, E_FAIL, "Failed to insert node");
	}

LExit:
	return hr;
}

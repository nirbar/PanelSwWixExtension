
#include "stdafx.h"
#include "RegistryXmlParser.h"
#include "RegistryKey.h"
#include "RegDataSerializer.h"
#include <strutil.h>
#pragma comment( lib, "msxml2.lib")

CRegistryXmlParser::CRegistryXmlParser()
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pRoot;
	IXMLDOMNode *pTmpNode = nullptr;
		
	hr = ::CoInitialize(nullptr); 
	ExitOnFailure( hr, "Failed to CoInitialize");

	// XML docs.
	hr = ::CoCreateInstance(CLSID_DOMDocument, nullptr, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&_pXmlDoc);
	ExitOnFailure( hr, "Failed to CoCreateInstance CLSID_DOMDocument");
	
	// XML root.
	hr = _pXmlDoc->createElement( L"Root", &pRoot);
	ExitOnFailure( hr, "Failed to create root XML element");

	// Append root to docs
	hr = _pXmlDoc->appendChild( pRoot, &pTmpNode);
	ExitOnFailure( hr, "Failed to append root XML element");
	pRoot.Attach( (IXMLDOMElement*) pTmpNode);

LExit:
	return;
}

CRegistryXmlParser::~CRegistryXmlParser()
{
	_pXmlDoc.Release();

	::CoUninitialize();
}

HRESULT CRegistryXmlParser::GetXmlString( BSTR* ppString)
{
	HRESULT hr = S_OK;

	hr = _pXmlDoc->get_xml( ppString);
	ExitOnFailure( hr, "Failed to get XML string");

LExit:
	return hr;
}

HRESULT CRegistryXmlParser::AddDeleteKey( WCHAR* pId, CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> xmlElem, xmlRoot;
	CComPtr<IXMLDOMNode> tmpNode;
	CComPtr<IXMLDOMDocument> xmlDoc;
	CComBSTR attName = L"";
	CComVariant value;

	hr = _pXmlDoc->get_documentElement( &xmlRoot);
	ExitOnFailure( hr, "Failed to get XML root");

	hr = _pXmlDoc->createElement( L"DeleteKey", &xmlElem);
	ExitOnFailure( hr, "Failed to create XML element");

	attName = "Id";
	value = pId;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Root";
	value = (DWORD)root;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Key";
	value = subkeyName;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	if( area == CRegistryKey::RegArea::Default)
	{
		hr = CRegistryKey::GetDefaultArea( &area);
		ExitOnFailure( hr, "Failed to get default registry area");
	}

	attName = "Area";
	value = area;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");
		
	hr = xmlRoot->appendChild( xmlElem, &tmpNode);
	tmpNode.Release();
	ExitOnFailure( hr, "Failed to append XML element");

LExit:
	return hr;
}

HRESULT CRegistryXmlParser::AddDeleteValue( WCHAR* pId, CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area, WCHAR* valName)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> xmlElem, xmlRoot;
	CComPtr<IXMLDOMNode> tmpNode;
	CComBSTR attName = L"";
	CComVariant value;
	
	hr = _pXmlDoc->get_documentElement( &xmlRoot);
	ExitOnFailure( hr, "Failed to get XML root");

	hr = _pXmlDoc->createElement( L"DeleteValue", &xmlElem);
	ExitOnFailure( hr, "Failed to create XML element");

	attName = "Id";
	value = pId;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Root";
	value = (DWORD)root;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Key";
	value = subkeyName;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	if( area == CRegistryKey::RegArea::Default)
	{
		hr = CRegistryKey::GetDefaultArea( &area);
		ExitOnFailure( hr, "Failed to get default registry area");
	}

	attName = "Area";
	value = area;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Name";
	value = valName;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");
		
	hr = xmlRoot->appendChild( xmlElem, &tmpNode);
	tmpNode.Release();
	ExitOnFailure( hr, "Failed to append XML element");

LExit:
	return hr;
}
	
HRESULT CRegistryXmlParser::AddCreateKey( WCHAR* pId, CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> xmlElem, xmlRoot;
	CComPtr<IXMLDOMNode> tmpNode;
	CComBSTR attName = L"";
	CComVariant value;
	
	hr = _pXmlDoc->get_documentElement( &xmlRoot);
	ExitOnFailure( hr, "Failed to get XML root");

	hr = _pXmlDoc->createElement( L"CreateKey", &xmlElem);
	ExitOnFailure( hr, "Failed to create XML element");
	
	attName = "Id";
	value = pId;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Root";
	value = (DWORD)root;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Key";
	value = subkeyName;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	if( area == CRegistryKey::RegArea::Default)
	{
		hr = CRegistryKey::GetDefaultArea( &area);
		ExitOnFailure( hr, "Failed to get default registry area");
	}

	attName = "Area";
	value = area;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");
		
	hr = xmlRoot->appendChild( xmlElem, &tmpNode);
	tmpNode.Release();
	ExitOnFailure( hr, "Failed to append XML element");

LExit:
	return hr;
}

HRESULT CRegistryXmlParser::AddCreateValue(WCHAR* pId, CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area, WCHAR* valName, CRegistryKey::RegValueType valType, WCHAR* pData)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> xmlElem, xmlRoot;
	CComPtr<IXMLDOMNode> tmpNode;
	CComBSTR attName = L"";
	CComVariant value;
	void *pArrayData = nullptr;
	
	hr = _pXmlDoc->get_documentElement( &xmlRoot);
	ExitOnFailure( hr, "Failed to get XML root");

	hr = _pXmlDoc->createElement( L"CreateValue", &xmlElem);
	ExitOnFailure( hr, "Failed to create XML element");

	attName = "Id";
	value = pId;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Root";
	value = (DWORD)root;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Key";
	value = subkeyName;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");
	
	if( area == CRegistryKey::RegArea::Default)
	{
		hr = CRegistryKey::GetDefaultArea( &area);
		ExitOnFailure( hr, "Failed to get default registry area");
	}

	attName = "Area";
	value = area;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Name";
	value = valName;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Type";
	value = valType;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");

	attName = "Data";
	value = pData;
	hr = xmlElem->setAttribute( attName, value);
	ExitOnFailure( hr, "Failed to set XML attribute");
		
	hr = xmlRoot->appendChild( xmlElem, &tmpNode);
	tmpNode.Release();
	ExitOnFailure( hr, "Failed to append XML element");

LExit:
	return hr;
}

HRESULT CRegistryXmlParser::Execute( WCHAR *pCustomActionData)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMDocument> pXmlDoc;
	CComPtr<IXMLDOMNodeList> pNodes;
	IXMLDOMNodeList* pTmpNodes = nullptr;
	IXMLDOMNode* pTmpNode = nullptr;
	IXMLDOMElement* pCurrElem = nullptr;
	VARIANT_BOOL isXmlSuccess;
	LONG nodeCount = 0;
	DOMNodeType nodeType;

	// XML docs.
	hr = ::CoCreateInstance(CLSID_DOMDocument, nullptr, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXmlDoc);
	ExitOnFailure( hr, "Failed to CoCreateInstance CLSID_DOMDocument");

	hr = pXmlDoc->loadXML( pCustomActionData, &isXmlSuccess);
	ExitOnFailure( hr, "Failed to load XML");
	if( ! isXmlSuccess)
	{
		hr = E_FAIL;
		ExitOnFailure( hr, "Failed to load XML");
	}

	hr = pXmlDoc->selectNodes( CComBSTR( L"/Root/*"), &pTmpNodes);
	ExitOnFailure( hr, "Failed to select XML nodes");
	ExitOnNull( pTmpNodes, hr, E_FAIL, "Failed to select XML nodes");
	pNodes.Attach( pTmpNodes);

	hr = pNodes->get_length( &nodeCount);
	ExitOnFailure( hr, "Failed to get node count");

	for( LONG i = 0; i < nodeCount; ++i)
	{
		pTmpNode = nullptr;

		hr = pNodes->get_item( i, &pTmpNode);
		ExitOnFailure( hr, "Failed to get node");
		ExitOnNull( pTmpNode, hr, E_FAIL, "Failed to get node");

		hr = pTmpNode->get_nodeType( &nodeType);
		ExitOnFailure( hr, "Failed to get node type");
		if( nodeType != DOMNodeType::NODE_ELEMENT)
		{
			hr = E_FAIL;
			ExitOnFailure( hr, "Expected an element");
		}

		hr = pTmpNode->QueryInterface( IID_IXMLDOMElement, (void**)&pCurrElem);
		ExitOnFailure( hr, "Failed quering as IID_IXMLDOMElement");
		ExitOnNull( pCurrElem, hr, E_FAIL, "Failed to get IID_IXMLDOMElement");

		hr = XmlExecute( pCurrElem);
		ExitOnFailure( hr, "Failed to parse-execute XML element");
	}

LExit:
	return hr;
}

HRESULT CRegistryXmlParser::XmlExecute( IXMLDOMElement *xmlElem)
{
	HRESULT hr = S_OK;
	CComBSTR tag;
	CComVariant id, root, key, valName, valType, valData, area;
	DWORD dwArea, dwRoot;
	BYTE *pData = nullptr;
	CRegDataSerializer regDataSer;

	// Get common fields.
	hr = xmlElem->get_tagName( &tag);
	ExitOnFailure( hr, "Failed to get XML tag name");	
	hr = xmlElem->getAttribute( CComBSTR( "Id"), &id);
	ExitOnFailure( hr, "Failed to get XML Id attribute");
	hr = xmlElem->getAttribute( CComBSTR( "Root"), &root);
	ExitOnFailure( hr, "Failed to get XML Root attribute");
	hr = xmlElem->getAttribute( CComBSTR( "Key"), &key);
	ExitOnFailure( hr, "Failed to get XML Key attribute");
	hr = xmlElem->getAttribute( CComBSTR( "Area"), &area);
	ExitOnFailure( hr, "Failed to get XML Area attribute");

	Bstr2Dword( root, (DWORD*)&dwRoot);
	Bstr2Dword( area, (DWORD*)&dwArea);

	if( wcscmp( tag, L"DeleteKey") == 0)
	{
		hr = DeleteKey( (CRegistryKey::RegRoot)dwRoot, key.bstrVal, (CRegistryKey::RegArea)dwArea);
		ExitOnFailure(hr, "Failed to delete key '%ls', root '%i', area '%i'", key.bstrVal, dwRoot, dwArea);
		ExitFunction();
	}
	else if( wcscmp( tag, L"CreateKey") == 0)
	{
		hr = CreateKey( (CRegistryKey::RegRoot)dwRoot, key.bstrVal, (CRegistryKey::RegArea)dwArea);
		ExitOnFailure( hr, "Failed to create key");
		ExitFunction();
	}
	else if( wcscmp( tag, L"DeleteValue") == 0)
	{
		hr = xmlElem->getAttribute( CComBSTR( "Name"), &valName);
		ExitOnFailure( hr, "Failed to get XML Name attribute");

		hr = DeleteValue( (CRegistryKey::RegRoot)dwRoot, key.bstrVal, (CRegistryKey::RegArea)dwArea, valName.bstrVal);
		ExitOnFailure( hr, "Failed to create key");
		ExitFunction();
	}
	else if( wcscmp( tag, L"CreateValue") == 0)
	{
		hr = xmlElem->getAttribute( CComBSTR( "Name"), &valName);
		ExitOnFailure( hr, "Failed to get XML Name attribute");
		hr = xmlElem->getAttribute( CComBSTR( "Type"), &valType);
		ExitOnFailure( hr, "Failed to get XML Type attribute");
		hr = xmlElem->getAttribute( CComBSTR( "Data"), &valData);
		ExitOnFailure( hr, "Failed to get XML Data attribute");

		hr = regDataSer.DeSerialize(valData.bstrVal, valType.bstrVal);
		ExitOnFailure(hr, "Failed to desrialize registry data");

		hr = CreateValue( 
			(CRegistryKey::RegRoot)dwRoot
			, key.bstrVal
			, (CRegistryKey::RegArea)dwArea
			, valName.bstrVal
			, regDataSer.DataType()
			, regDataSer.Data()
			, regDataSer.Size());
		ExitOnFailure( hr, "Failed to create value");

		ExitFunction();
	}

LExit:
	return hr;
}

HRESULT CRegistryXmlParser::Bstr2Dword( VARIANT str, DWORD* dwVal)
{
	(*dwVal) = _wtol( str.bstrVal);
	return S_OK;
}

HRESULT CRegistryXmlParser::DeleteKey( CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area)
{
	CRegistryKey regKey;
	HRESULT hr;

	hr = regKey.Open( root, subkeyName, (CRegistryKey::RegArea)area, CRegistryKey::RegAccess::All);
	if( hr == E_FILENOTFOUND)
	{
		return S_OK;
	}
	ExitOnFailure( hr, "Failed to open registry key");

	hr = regKey.Delete();
	ExitOnFailure( hr, "Failed to delete registry key");

LExit:
	return hr;
}

HRESULT CRegistryXmlParser::CreateKey( CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area)
{
	CRegistryKey regKey;
	HRESULT hr;

	hr = regKey.Create( root, subkeyName, (CRegistryKey::RegArea)area, CRegistryKey::RegAccess::All);
	ExitOnFailure( hr, "Failed to create registry key");

LExit:
	return hr;
}

HRESULT CRegistryXmlParser::DeleteValue( CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area, WCHAR* valName)
{
	CRegistryKey regKey;
	HRESULT hr;

	hr = regKey.Open( root, subkeyName, (CRegistryKey::RegArea)area, CRegistryKey::RegAccess::All);
	if( hr == E_FILENOTFOUND)
	{
		return S_OK;
	}
	ExitOnFailure( hr, "Failed to open registry key");

	hr = regKey.DeleteValue( valName);
	ExitOnFailure( hr, "Failed to open registry value");

LExit:
	return hr;
}

HRESULT CRegistryXmlParser::CreateValue( CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area, WCHAR* valName, DWORD valType, BYTE* valData, DWORD dataSize)
{
	CRegistryKey regKey;
	HRESULT hr;

	hr = regKey.Create( root, subkeyName, (CRegistryKey::RegArea)area, CRegistryKey::RegAccess::All);
	ExitOnFailure( hr, "Failed to create registry key");

	hr = regKey.SetValue( valName, (CRegistryKey::RegValueType)valType, valData, dataSize);
	ExitOnFailure( hr, "Failed to create registry value");

LExit:
	return hr;
}

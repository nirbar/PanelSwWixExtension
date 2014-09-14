
#include "stdafx.h"
#include <errno.h>
#include <objbase.h>
#include <msxml.h>
#include <atlbase.h>
#pragma comment( lib, "msxml2.lib")

#define XmlSearchQuery L"SELECT `Id`, `Property_`, `FilePath`, `XPath`, `Match`, `Attributes`, `Condition` FROM `PSW_XmlSearch`"
enum eXmlSearchQueryQuery { Id = 1, Property_, FilePath, XPath, Match, Attributes, Condition };

enum eXmlMatch
{
	first,
	all,
	enforceSingle
};

HRESULT ParseXmlMatch( LPCWSTR pMatchString, eXmlMatch *peMatch);
HRESULT QueryXml( LPCWSTR pFile, LPCWSTR pXpath, eXmlMatch eMatch, LPCWSTR pProperty);

UINT __stdcall XmlSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");
		
	hr = ::CoInitialize(NULL); 
	ExitOnFailure( hr, "Failed to CoInitialize");

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_XmlSearch");
	ExitOnFailure(hr, "Table does not exist 'PSW_XmlSearch'. Have you authored 'PanelSw:XmlSearch' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(XmlSearchQuery, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_XmlSearch'.");
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		WCHAR *Id = NULL;
		WCHAR *Property = NULL;
		WCHAR *FilePath = NULL;
		WCHAR *XPath = NULL;
		WCHAR *Match = NULL;
		WCHAR *Condition = NULL;
		eXmlMatch eMatch = eXmlMatch::first;

		hr = WcaGetRecordString(hRecord, eXmlSearchQueryQuery::Id, &Id);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, eXmlSearchQueryQuery::Property_, &Property);
		ExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, eXmlSearchQueryQuery::FilePath, &FilePath);
		ExitOnFailure(hr, "Failed to get FilePath.");
		hr = WcaGetRecordFormattedString(hRecord, eXmlSearchQueryQuery::XPath, &XPath);
		ExitOnFailure(hr, "Failed to get XPath.");
		hr = WcaGetRecordString(hRecord, eXmlSearchQueryQuery::Match, &Match);
		ExitOnFailure(hr, "Failed to get Section.");
		hr = WcaGetRecordString(hRecord, 7, &Condition);
		ExitOnFailure(hr, "Failed to get Condition.");

		// Test condition
		MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, Condition);
		switch (condRes)
		{
		case MSICONDITION::MSICONDITION_NONE:
		case MSICONDITION::MSICONDITION_TRUE:
			WcaLog(LOGMSG_STANDARD, "Condition evaluated to true / none.");
			break;

		case MSICONDITION::MSICONDITION_FALSE:
			WcaLog(LOGMSG_STANDARD, "Skipping. Condition evaluated to false");
			continue;

		case MSICONDITION::MSICONDITION_ERROR:
			hr = E_FAIL;
			ExitOnFailure(hr, "Bad Condition field");
		}

		// Parse 'match' column
		if(( Match != NULL) && ( wcslen( Match) != 0))
		{
			hr = ParseXmlMatch( Match, &eMatch);
			ExitOnFailure(hr, "Invalide match field");
		}

		hr = QueryXml( FilePath, XPath, eMatch, Property);
		ExitOnFailure(hr, "Failed to query XML.");
	}

	hr = ERROR_SUCCESS;
	WcaLog(LOGMSG_STANDARD, "Done.");

LExit:
	::CoUninitialize();

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT ParseXmlMatch( LPCWSTR pMatchString, eXmlMatch *peMatch)
{
	HRESULT hr = S_OK;

	ExitOnNull( pMatchString, hr, E_INVALIDARG, "pMatchString is null");
	ExitOnNull( peMatch, hr, E_INVALIDARG, "peMatch is null");

	if( _wcsicmp( pMatchString, L"first") == 0)
	{
		(*peMatch) = eXmlMatch::first;
	}
	else if( _wcsicmp( pMatchString, L"all") == 0)
	{
		(*peMatch) = eXmlMatch::all;
	}
	else if( _wcsicmp( pMatchString, L"enforceSingle") == 0)
	{
		(*peMatch) = eXmlMatch::enforceSingle;
	}
	else
	{
		hr = E_INVALIDARG;
		ExitOnFailure( hr, "Invalide match field");
	}

LExit:
	return hr;
}

HRESULT QueryXml( LPCWSTR pFile, LPCWSTR pXpath, eXmlMatch eMatch, LPCWSTR pProperty)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMDocument> pXmlDoc;
	CComPtr<IXMLDOMNodeList> pNodeList;
	CComVariant filePath;
	VARIANT_BOOL isXmlSuccess;
	LONG nodeCount = 0;
	LONG maxMatches = 0;
	CComBSTR result(L"");
	CComBSTR delimiter(L"[~]");

	ExitOnNull( pFile, hr, E_INVALIDARG, "pFile is null");
	ExitOnNull( pXpath, hr, E_INVALIDARG, "pXpath is null");
	ExitOnNull( pProperty, hr, E_INVALIDARG, "pProperty is null");
	
	// Create XML doc.
	hr = ::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXmlDoc);
	ExitOnFailure( hr, "Failed to CoCreateInstance CLSID_DOMDocument");

	// Load XML document
	filePath = pFile;
	hr = pXmlDoc->load( filePath, &isXmlSuccess);
	ExitOnFailure( hr, "Failed to load XML");
	if( ! isXmlSuccess)
	{
		hr = E_FAIL;
		ExitOnFailure( hr, "Failed to load XML");
	}

	// Execute XPath
	hr = pXmlDoc->selectNodes( CComBSTR( pXpath), &pNodeList);
	ExitOnFailure( hr, "Failed to select XML nodes");
	ExitOnNull( pNodeList, hr, E_FAIL, "Failed to select XML nodes");
	
	// Get match-count
	hr = pNodeList->get_length( &nodeCount);
	ExitOnFailure( hr, "Failed to get node count");

	// Validate with request match parameter
	switch (eMatch)
	{
	case first:
		maxMatches = 1;
		break;
	case all:
		maxMatches = nodeCount;
		break;
	case enforceSingle:
		if( nodeCount > 1)
		{
			hr = E_INVALIDARG;
			ExitOnFailure( hr, "Too many XmlSreach matches.");
		}
		break;
	default:
		hr = E_INVALIDARG;
		ExitOnFailure( hr, "Wrong match parameter for XmlSreach.");
		break;
	}

	for( LONG i = 0; i < maxMatches; ++i)
	{
		CComPtr<IXMLDOMNode> pNode;
		CComVariant nodeValue;

		hr = pNodeList->get_item( i, &pNode);
		ExitOnFailure( hr, "Failed to get node.");
		ExitOnNull( pNode.p, hr, E_FAIL, "Failed to get node.");

		hr = pNode->get_nodeValue( &nodeValue);
		ExitOnFailure( hr, "Failed to get node's value.");
		
		hr = nodeValue.ChangeType( VT_BSTR);
		ExitOnFailure( hr, "Failed to get node's value as string.");

		// Add result
		hr = result.AppendBSTR( nodeValue.bstrVal);
		ExitOnFailure( hr, "Failed to append result.");

		// Add delimiter (unless this is the last)
		if( i < maxMatches - 1)
		{
			hr = result.AppendBSTR( delimiter);
			ExitOnFailure( hr, "Failed to append delimiter.");
		}
	}

	// Put in property
	hr = WcaSetProperty( pProperty, (LPWSTR)result);

LExit:
	return hr;
}

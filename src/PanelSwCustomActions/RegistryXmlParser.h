#pragma once

#include "pch.h"
#include <windows.h>
#include <objbase.h>
#include <msxml.h>
#include <atlbase.h>
#include "../CaCommon/RegistryKey.h"

class CRegistryXmlParser
{
public:

	CRegistryXmlParser();
	~CRegistryXmlParser();

	HRESULT GetXmlString( BSTR* ppString);

	HRESULT Execute( WCHAR *pCustomActionData);

	HRESULT AddDeleteKey( WCHAR* pId, CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area);
	HRESULT AddDeleteValue( WCHAR* pId, CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area, WCHAR* valName);
			
	HRESULT AddCreateKey( WCHAR* pId, CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area);
	HRESULT AddCreateValue(WCHAR* pId, CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area, WCHAR* valName, CRegistryKey::RegValueType valType, WCHAR* pData);

private:
	
	HRESULT XmlExecute( IXMLDOMElement *xmlElem);

	HRESULT Bstr2Dword( VARIANT str, DWORD* dwVal);

	HRESULT DeleteKey( CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area);
	HRESULT CreateKey( CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area);
	
	HRESULT DeleteValue( CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area, WCHAR* valName);
	HRESULT CreateValue( CRegistryKey::RegRoot root, WCHAR* subkeyName, CRegistryKey::RegArea area, WCHAR* valName, DWORD valType, BYTE* valData, DWORD dataSize);

	CComPtr<IXMLDOMDocument> _pXmlDoc;
};


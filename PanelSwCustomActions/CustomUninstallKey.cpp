
#include "stdafx.h"
#include "CustomUninstallKey.h"
#include "RegistryKey.h"
#include "RegDataSerializer.h"
#include <strutil.h>
/*
#include <objbase.h>
#pragma comment( lib, "msxml2.lib");
*/

#define CustomUninstallKey_ExecCA L"CustomUninstallKey_deferred"
#define CustomUninstallKey_RollbackCA L"CustomUninstallKey_rollback"
#define CustomUninstallKeyQuery L"SELECT `Id`, `Name`, `Data`, `DataType`, `Attributes`, `Condition` FROM `CustomUninstallKey`"
enum eCustomUninstallKeyQuery { Id = 1, Name, Data, DataType, Attributes, Condition };

UINT __stdcall CustomUninstallKey_Immediate(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CCustomUninstallKey data(hInstall);

	hr = WcaInitialize(hInstall, "CustomUninstallKey_Immediate");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// TODO: Add your custom action code here.
	hr = data.CreateCustomActionData();
	ExitOnFailure(hr, "Failed");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

UINT __stdcall CustomUninstallKey_deferred(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CCustomUninstallKey data(hInstall);

	::MessageBoxA(NULL, "MsiBreak", "MsiBreak", MB_OK);

	hr = WcaInitialize(hInstall, "CustomUninstallKey_deferred");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	hr = data.Execute();
	ExitOnFailure(hr, "Failed");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

CCustomUninstallKey::CCustomUninstallKey( MSIHANDLE hInstall)
	: _hInstall( hInstall)
{
}

CCustomUninstallKey::~CCustomUninstallKey(void)
{
}

HRESULT CCustomUninstallKey::Execute()
{
	CRegistryXmlParser parser;
	WCHAR* customActionData = NULL;
	HRESULT hr = S_OK;

	hr = WcaGetProperty( L"CustomActionData", &customActionData);
	ExitOnFailure( hr, "Failed to get CustomActionData");
	WcaLog( LOGLEVEL::LOGMSG_STANDARD, "CustomActionData= '%ls'", customActionData);

	hr = parser.Execute( customActionData);
	ExitOnFailure( hr, "Failed to get parse-execute CustomActionData");

LExit:
	return hr;
}

HRESULT CCustomUninstallKey::CreateCustomActionData()
{
	PMSIHANDLE hView;
    PMSIHANDLE hRec;
	CRegistryXmlParser xmlParser, xmlRollbackParser;
	CRegistryKey::RegRoot root;
	WCHAR uninstallKey[ MAX_PATH];
	CComBSTR xmlString = L"";
	HRESULT hr = S_OK;
	bool bRemoving = false;

	hr = GetRoot( &root);
	ExitOnFailure(hr, "Failed to get registry root");

	hr = GetUninstallKey( uninstallKey);
	ExitOnFailure(hr, "Failed to get uninstall registry key");
	
	hr = WcaOpenExecuteView( CustomUninstallKeyQuery, &hView);
	ExitOnFailure(hr, "Failed to execute view");

	if (WcaIsPropertySet("REMOVE"))
	{
		bRemoving = true;
	}

	while ( E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
    {
		ExitOnFailure(hr, "Failed to fetch record");
		
		// Get record.
		WCHAR* pId = NULL;
		WCHAR* pName = NULL;
		WCHAR* pData = NULL;
		WCHAR* pDataType = NULL;
		CRegistryKey::RegValueType dataType;
		WCHAR* pCondition = NULL;
		int nAttrib;
		
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::Id, &pId);
		ExitOnFailure(hr, "Failed to get Id");
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::Name, &pName);
		ExitOnFailure(hr, "Failed to get Name");
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::Data, &pData);
		ExitOnFailure(hr, "Failed to get Data");
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::DataType, &pDataType);
		ExitOnFailure(hr, "Failed to get DataType");
		hr = WcaGetRecordInteger( hRec, eCustomUninstallKeyQuery::Attributes, &nAttrib);
		ExitOnFailure(hr, "Failed to get Attributes");
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::Condition, &pCondition);
		ExitOnFailure(hr, "Failed to get Condition");

		MSICONDITION cond = ::MsiEvaluateCondition( _hInstall, pCondition);
		switch( cond)
		{
		case MSICONDITION::MSICONDITION_NONE:
		case MSICONDITION::MSICONDITION_TRUE:
			WcaLog( LOGLEVEL::LOGMSG_STANDARD, "Condition evaluated true/none for %ls", pId);
			break;

		case MSICONDITION::MSICONDITION_FALSE:
			WcaLog( LOGLEVEL::LOGMSG_STANDARD, "Condition evaluated false for %ls", pId);
			continue;

		case MSICONDITION::MSICONDITION_ERROR:
			hr = E_FAIL;
			ExitOnFailure(hr, "Failed to evaluate condition");
		}

		// Save as XML element
		hr = ParseDataType( pDataType, &dataType);
		ExitOnFailure(hr, "Failed to parse data type");
		
		// Install / UnInstall ?
		if (bRemoving)
		{
			hr = xmlParser.AddDeleteKey(pId, root, uninstallKey, CRegistryKey::RegArea::Default);
			ExitOnFailure(hr, "Failed to create XML element");
		}
		else
		{
			hr = xmlParser.AddCreateValue(pId, root, uninstallKey, CRegistryKey::RegArea::Default, pName, dataType, pData);
			ExitOnFailure(hr, "Failed to create XML element");
		}

		hr = CreateRollbackCustomActionData( &xmlRollbackParser, pId, pName);
		ExitOnFailure(hr, "Failed to create rollback XML element");
	}
	
	hr = xmlRollbackParser.GetXmlString( &xmlString);
	ExitOnFailure(hr, "Failed to read XML as text");
	hr = WcaDoDeferredAction( CustomUninstallKey_RollbackCA, xmlString, 0);
	ExitOnFailure(hr, "Failed to set property");	

	hr = xmlParser.GetXmlString( &xmlString);
	ExitOnFailure(hr, "Failed to read XML as text");
	hr = WcaDoDeferredAction( CustomUninstallKey_ExecCA, xmlString, 0);
	ExitOnFailure(hr, "Failed to set property");
	xmlString = std::nullptr_t(NULL);

LExit:

	return hr;
}

HRESULT CCustomUninstallKey::CreateRollbackCustomActionData( CRegistryXmlParser *pRollbackParser, WCHAR* pId, WCHAR* pName)
{
	HRESULT hr = S_OK;
	CRegistryKey key;
	CRegistryKey::RegValueType valType;
	BYTE* pDataBytes = NULL;
	WCHAR* pDataStr = NULL;
	DWORD dwSize = 0;
	CRegistryKey::RegRoot root;
	WCHAR keyName[ MAX_PATH];
	CRegDataSerializer dataSer;

	hr = GetRoot( &root);
	ExitOnFailure( hr, "Failed to get Uninstall key's root");

	hr = GetUninstallKey( keyName);
	ExitOnFailure( hr, "Failed to get Uninstall key");

	hr = key.Open( root, keyName, CRegistryKey::RegArea::Default, CRegistryKey::RegAccess::ReadOnly);
	if( hr == E_FILENOTFOUND)
	{
		// Delete key on rollback
		hr = pRollbackParser->AddDeleteKey( pId, root, keyName, CRegistryKey::RegArea::Default);
		ExitOnFailure( hr, "Failed to create rollback XML element");
		ExitFunction();
	}

	hr = key.GetValue(pName, &pDataBytes, &valType, &dwSize);
	if( hr == E_FILENOTFOUND)
	{
		// Delete value on rollback
		hr = pRollbackParser->AddDeleteValue( pId, root, keyName, CRegistryKey::RegArea::Default, pName);
		ExitOnFailure( hr, "Failed to create rollback XML element");
		ExitFunction();
	}
	
	hr = dataSer.Set(pDataBytes, valType, dwSize);
	ExitOnFailure(hr, "Failed to analyze registry data");

	hr = dataSer.Serialize(&pDataStr);
	ExitOnFailure(hr, "Failed to serialize registry data");

	hr = pRollbackParser->AddCreateValue(pId, root, keyName, CRegistryKey::RegArea::Default, pName, valType, pDataStr);
	ExitOnFailure( hr, "Failed to create rollback XML element");

LExit:
	if (pDataBytes != NULL)
	{
		delete[] pDataBytes;
	}

	return hr;
}

HRESULT CCustomUninstallKey::GetRoot( CRegistryKey::RegRoot *pRoot)
{
	WCHAR* pAllUsers = NULL;
	HRESULT hr = S_OK;

	hr = WcaGetProperty( L"ALLUSERS", &pAllUsers);
	ExitOnFailure( hr, "Failed to get ALLUSERS property");

	if( wcscmp( pAllUsers, L"1") == 0)
	{
		(*pRoot) = CRegistryKey::RegRoot::LocalMachine;
	}
	else if( wcscmp( pAllUsers, L"") == 0)
	{
		(*pRoot) = CRegistryKey::RegRoot::CurrentUser;
	}
	else
	{
		hr = E_INVALIDDATA;
		WcaLogError( hr, "Invalid value for 'ALLUSERS' property: '%ls'", pAllUsers);
	}

LExit:
	return hr;
}

HRESULT CCustomUninstallKey::GetUninstallKey( WCHAR* keyName)
{
	WCHAR *prodCode = NULL;
	HRESULT hr = S_OK;

	hr = WcaGetProperty( L"ProductCode", &prodCode);
	ExitOnFailure( hr, "Failed to get ProductCode");

	wsprintf( keyName, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%ls", prodCode);

LExit:
	return hr;
}

HRESULT CCustomUninstallKey::ParseDataType( WCHAR* attrString, CRegistryKey::RegValueType *pDataType)
{
	HRESULT hr = S_OK;

	if( _wcsicmp( attrString, L"REG_SZ") == 0)
	{
		(*pDataType) = CRegistryKey::RegValueType::String;
	}
	else if (_wcsicmp(attrString, L"REG_DWORD") == 0)
	{
		(*pDataType) = CRegistryKey::RegValueType::DWord;
	}
	else if (_wcsicmp(attrString, L"REG_BINARY") == 0)
	{
		(*pDataType) = CRegistryKey::RegValueType::Binary;
	}
	else if (_wcsicmp(attrString, L"REG_EXPAND_SZ") == 0)
	{
		(*pDataType) = CRegistryKey::RegValueType::Expandable;
	}
	else if (_wcsicmp(attrString, L"REG_MULTI_SZ") == 0)
	{
		(*pDataType) = CRegistryKey::RegValueType::MultiString;
	}
	else if (_wcsicmp(attrString, L"REG_QWORD") == 0)
	{
		(*pDataType) = CRegistryKey::RegValueType::QWord;
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

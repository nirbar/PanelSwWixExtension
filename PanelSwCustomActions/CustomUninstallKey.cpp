
#include "stdafx.h"
#include "CustomUninstallKey.h"
#include "RegistryKey.h"
#include "RegDataSerializer.h"
#include <strutil.h>

#define CustomUninstallKey_ExecCA L"CustomUninstallKey_deferred"
#define CustomUninstallKey_RollbackCA L"CustomUninstallKey_rollback"
#define CustomUninstallKeyQuery L"SELECT `Id`, `ProductCode`, `Name`, `Data`, `DataType`, `Attributes`, `Condition` FROM `PSW_CustomUninstallKey`"
enum eCustomUninstallKeyQuery { Id = 1, ProductCode, Name, Data, DataType, Attributes, Condition };

extern "C" UINT __stdcall CustomUninstallKey_Immediate(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CCustomUninstallKey data(hInstall);

	hr = WcaInitialize(hInstall, "CustomUninstallKey_Immediate");
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);
	
	// Ensure table PSW_CustomUninstallKey exists.
	hr = WcaTableExists(L"PSW_CustomUninstallKey");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_CustomUninstallKey'. Have you authored 'PanelSw:CustomUninstallKey' entries in WiX code?");

	// TODO: Add your custom action code here.
	hr = data.CreateCustomActionData();
	BreakExitOnFailure(hr, "Failed");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

extern "C" UINT __stdcall CustomUninstallKey_deferred(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CCustomUninstallKey data(hInstall);

	hr = WcaInitialize(hInstall, "CustomUninstallKey_deferred");
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = data.Execute();
	BreakExitOnFailure(hr, "Failed");

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
	LPWSTR customActionData = nullptr;
	HRESULT hr = S_OK;

	hr = WcaGetProperty( L"CustomActionData", &customActionData);
	BreakExitOnFailure( hr, "Failed to get CustomActionData");
	WcaLog( LOGLEVEL::LOGMSG_STANDARD, "CustomActionData= '%ls'", customActionData);

	hr = parser.Execute( customActionData);
	BreakExitOnFailure( hr, "Failed to get parse-execute CustomActionData");

LExit:
	ReleaseStr(customActionData);
	return hr;
}

HRESULT CCustomUninstallKey::CreateCustomActionData()
{
	PMSIHANDLE hView;
    PMSIHANDLE hRec;
	CRegistryXmlParser xmlParser, xmlRollbackParser;
	CComBSTR xmlString = L"";
	HRESULT hr = S_OK;
	bool bRemoving = false;
	LPWSTR pId = nullptr;
	LPWSTR szProductCode = nullptr;
	LPWSTR pName = nullptr;
	LPWSTR pData = nullptr;
	LPWSTR pDataType = nullptr;
	LPWSTR pCondition = nullptr;

	hr = WcaOpenExecuteView( CustomUninstallKeyQuery, &hView);
	BreakExitOnFailure(hr, "Failed to execute view");

	if (WcaIsPropertySet("REMOVE"))
	{
		bRemoving = true;
	}

	while ( E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
    {
		BreakExitOnFailure(hr, "Failed to fetch record");
		
		// Get record.
		WCHAR uninstallKey[MAX_PATH];
		int nAttrib;
		
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::Id, &pId);
		BreakExitOnFailure(hr, "Failed to get Id");
		hr = WcaGetRecordFormattedString(hRec, eCustomUninstallKeyQuery::ProductCode, &szProductCode);
		BreakExitOnFailure(hr, "Failed to get ProductCode");
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::Name, &pName);
		BreakExitOnFailure(hr, "Failed to get Name");
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::Data, &pData);
		BreakExitOnFailure(hr, "Failed to get Data");
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::DataType, &pDataType);
		BreakExitOnFailure(hr, "Failed to get DataType");
		hr = WcaGetRecordInteger( hRec, eCustomUninstallKeyQuery::Attributes, &nAttrib);
		BreakExitOnFailure(hr, "Failed to get Attributes");
		hr = WcaGetRecordString( hRec, eCustomUninstallKeyQuery::Condition, &pCondition);
		BreakExitOnFailure(hr, "Failed to get Condition");

		hr = GetUninstallKey(szProductCode, uninstallKey);
		BreakExitOnFailure(hr, "Failed to get uninstall registry key");

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
			BreakExitOnFailure(hr, "Failed to evaluate condition");
		}

		// Install / UnInstall ?
		if (bRemoving)
		{
			hr = xmlParser.AddDeleteKey(pId, CRegistryKey::RegRoot::LocalMachine, uninstallKey, CRegistryKey::RegArea::Default);
			BreakExitOnFailure(hr, "Failed to create XML element");
		}
		else
		{
			CRegDataSerializer dataSer;
			LPWSTR pDataStr = nullptr;

			hr = dataSer.Set(pData, pDataType);
			BreakExitOnFailure(hr, "Failed to create parse registry data");

			hr = dataSer.Serialize(&pDataStr);
			BreakExitOnFailure(hr, "Failed to serialize registry data");

			hr = xmlParser.AddCreateValue(pId, CRegistryKey::RegRoot::LocalMachine, uninstallKey, CRegistryKey::RegArea::Default, pName, (CRegistryKey::RegValueType)dataSer.DataType(), pDataStr);
			BreakExitOnFailure(hr, "Failed to create XML element");
		}

		hr = CreateRollbackCustomActionData(&xmlRollbackParser, szProductCode, pId, pName);
		BreakExitOnFailure(hr, "Failed to create rollback XML element");
	
		ReleaseNullStr(pId);
		ReleaseNullStr(szProductCode);
		ReleaseNullStr(pName);
		ReleaseNullStr(pData);
		ReleaseNullStr(pDataType);
		ReleaseNullStr(pCondition);
	}
	
	hr = xmlRollbackParser.GetXmlString( &xmlString);
	BreakExitOnFailure(hr, "Failed to read XML as text");
	hr = WcaDoDeferredAction( CustomUninstallKey_RollbackCA, xmlString, 0);
	BreakExitOnFailure(hr, "Failed to set property");	

	hr = xmlParser.GetXmlString( &xmlString);
	BreakExitOnFailure(hr, "Failed to read XML as text");
	hr = WcaDoDeferredAction( CustomUninstallKey_ExecCA, xmlString, 0);
	BreakExitOnFailure(hr, "Failed to set property");
	xmlString.Empty();

LExit:
	ReleaseStr(pId);
	ReleaseStr(szProductCode);
	ReleaseStr(pName);
	ReleaseStr(pData);
	ReleaseStr(pDataType);
	ReleaseStr(pCondition);

	return hr;
}

HRESULT CCustomUninstallKey::CreateRollbackCustomActionData(CRegistryXmlParser *pRollbackParser, LPCWSTR szProductCode, LPWSTR pId, LPWSTR pName)
{
	HRESULT hr = S_OK;
	CRegistryKey key;
	CRegistryKey::RegValueType valType;
	BYTE* pDataBytes = nullptr;
	LPWSTR pDataStr = nullptr;
	DWORD dwSize = 0;
	WCHAR keyName[ MAX_PATH];
	CRegDataSerializer dataSer;

	hr = GetUninstallKey(szProductCode, keyName);
	BreakExitOnFailure( hr, "Failed to get Uninstall key");

	hr = key.Open( CRegistryKey::RegRoot::LocalMachine, keyName, CRegistryKey::RegArea::Default, CRegistryKey::RegAccess::ReadOnly);
	if( hr == E_FILENOTFOUND)
	{
		// Delete key on rollback
		hr = pRollbackParser->AddDeleteKey(pId, CRegistryKey::RegRoot::LocalMachine, keyName, CRegistryKey::RegArea::Default);
		BreakExitOnFailure( hr, "Failed to create rollback XML element");
		ExitFunction();
	}

	hr = key.GetValue(pName, &pDataBytes, &valType, &dwSize);
	if( hr == E_FILENOTFOUND)
	{
		// Delete value on rollback
		hr = pRollbackParser->AddDeleteValue(pId, CRegistryKey::RegRoot::LocalMachine, keyName, CRegistryKey::RegArea::Default, pName);
		BreakExitOnFailure( hr, "Failed to create rollback XML element");
		ExitFunction();
	}
	
	hr = dataSer.Set(pDataBytes, valType, dwSize);
	BreakExitOnFailure(hr, "Failed to analyze registry data");

	hr = dataSer.Serialize(&pDataStr);
	BreakExitOnFailure(hr, "Failed to serialize registry data");

	hr = pRollbackParser->AddCreateValue(pId, CRegistryKey::RegRoot::LocalMachine, keyName, CRegistryKey::RegArea::Default, pName, valType, pDataStr);
	BreakExitOnFailure( hr, "Failed to create rollback XML element");

LExit:
	if (pDataBytes)
	{
		delete[] pDataBytes;
	}

	return hr;
}

HRESULT CCustomUninstallKey::GetUninstallKey(LPCWSTR szProductCode, LPWSTR keyName)
{
	LPWSTR prodCode = nullptr;
	HRESULT hr = S_OK;

	if (szProductCode && *szProductCode)
	{
		prodCode = const_cast<LPWSTR>(szProductCode);
	}
	else
	{
		hr = WcaGetProperty(L"ProductCode", &prodCode);
		BreakExitOnFailure(hr, "Failed to get ProductCode");
	}

	wsprintfW( keyName, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%ls", prodCode);

LExit:
	return hr;
}

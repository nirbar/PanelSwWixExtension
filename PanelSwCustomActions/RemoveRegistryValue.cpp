#include "stdafx.h"
#include "RegistryKey.h"
#include "RegistryXmlParser.h"
#include "RegDataSerializer.h"
#include <strutil.h>

#define RemoveRegistryValueQuery L"SELECT `Id`, `Root`, `Key`, `Name`, `Attributes`, `Condition` FROM `PSW_RemoveRegistryValue`"
enum eRemoveRegistryValueQuery { Id = 1, Root, Key, Name, Attributes, Condition };

UINT __stdcall RemoveRegistryValue_Immediate(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CRegistryXmlParser actionData;
	CRegistryXmlParser rollbackActionData;
	PMSIHANDLE hView;
    PMSIHANDLE hRec;
	CComBSTR xmlString;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	
	hr = WcaOpenExecuteView( RemoveRegistryValueQuery, &hView);
	ExitOnFailure(hr, "Failed to execute view");

	while ( E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
    {
		ExitOnFailure(hr, "Failed to fetch record");
		
		// Get record.
		WCHAR* pId = NULL;
		WCHAR* pName = NULL;
		WCHAR* pRoot = NULL;
		WCHAR* pKey = NULL;
		CRegistryKey::RegRoot eRoot;
		CRegistryKey key;
		WCHAR* pCondition = NULL;

		// existing value details
		BYTE* pData = NULL;
		LPWSTR pDataString = NULL;
		CRegistryKey::RegValueType valueType;
		DWORD dwValueSize = 0;
		CRegDataSerializer dataSerialiser;
		
		hr = WcaGetRecordString( hRec, eRemoveRegistryValueQuery::Id, &pId);
		ExitOnFailure(hr, "Failed to get Id");
		hr = WcaGetRecordString( hRec, eRemoveRegistryValueQuery::Root, &pRoot);
		ExitOnFailure(hr, "Failed to get Root");
		hr = WcaGetRecordString( hRec, eRemoveRegistryValueQuery::Key, &pKey);
		ExitOnFailure(hr, "Failed to get Key");
		hr = WcaGetRecordString( hRec, eRemoveRegistryValueQuery::Name, &pName);
		ExitOnFailure(hr, "Failed to get Name");
		hr = WcaGetRecordString( hRec, eRemoveRegistryValueQuery::Condition, &pCondition);
		ExitOnFailure(hr, "Failed to get Condition");

		MSICONDITION cond = ::MsiEvaluateCondition( hInstall, pCondition);
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

		// Parse root name.
		hr = CRegistryKey::ParseRoot( pRoot, &eRoot);
		ExitOnFailure(hr, "Failed to parse root");

		// CustomActionData
		hr = actionData.AddDeleteValue( pId, eRoot, pKey, CRegistryKey::RegArea::Default /* TODO */, pName);
		ExitOnFailure(hr, "Failed to add 'DeleteValue' to CustomActionData");

		// Rollback CustomActionData
		hr = key.Open( eRoot, pKey, CRegistryKey::RegArea::Default /* TODO */, CRegistryKey::RegAccess::ReadOnly);
		if( hr == E_FILENOTFOUND)
		{
			hr = S_OK;
			continue;
		}
		ExitOnFailure(hr, "Failed to query existing key");

		hr = key.GetValue( pName, &pData, &valueType, &dwValueSize);
		if( hr == E_FILENOTFOUND)
		{
			hr = S_OK;
			continue;
		}
		ExitOnFailure(hr, "Failed to query existing value");

		hr = dataSerialiser.Set( pData, valueType, dwValueSize);
		ExitOnFailure(hr, "Failed to initialize data serializer");

		hr = dataSerialiser.Serialize( &pDataString);
		ExitOnFailure(hr, "Failed to serialize data");

		hr = rollbackActionData.AddCreateValue(pId, eRoot, pKey, CRegistryKey::RegArea::Default, pName, valueType, pDataString);
		ExitOnFailure(hr, "Failed to create XML element");
	}
	
	hr = rollbackActionData.GetXmlString( &xmlString);
	ExitOnFailure(hr, "Failed to read XML as text");
	hr = WcaDoDeferredAction( L"RemoveRegistryValue_rollback", xmlString, 0);
	ExitOnFailure(hr, "Failed to set property");	
	xmlString = std::nullptr_t(NULL);

	hr = actionData.GetXmlString( &xmlString);
	ExitOnFailure(hr, "Failed to read XML as text");
	hr = WcaDoDeferredAction( L"RemoveRegistryValue_deferred", xmlString, 0);
	ExitOnFailure(hr, "Failed to set property");
	xmlString = std::nullptr_t(NULL);

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}


UINT __stdcall RemoveRegistryValue_Deferred(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CRegistryXmlParser action;
	LPWSTR pActionData = NULL;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	hr = WcaGetProperty( L"CustomActionData", &pActionData);
	ExitOnFailure(hr, "Failed to get CustomActionData");

	hr = action.Execute( pActionData);
	ExitOnFailure(hr, "Failed to parse-execute RemoveRegistryValue");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

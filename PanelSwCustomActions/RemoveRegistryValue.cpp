#include "stdafx.h"
#include "RegistryKey.h"
#include "RegistryXmlParser.h"
#include "RegDataSerializer.h"
#include <strutil.h>

#define RemoveRegistryValueQuery L"SELECT `Id`, `Root`, `Key`, `Name`, `Area`, `Attributes`, `Condition` FROM `PSW_RemoveRegistryValue`"
enum eRemoveRegistryValueQuery { Id = 1, Root, Key, Name, Area, Attributes, Condition };

extern "C" __declspec( dllexport ) UINT RemoveRegistryValue_Immediate(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CRegistryXmlParser actionData;
	CRegistryXmlParser rollbackActionData;
	PMSIHANDLE hView;
    PMSIHANDLE hRec;
	CComBSTR xmlString;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");
		
	// Ensure table PSW_RemoveRegistryValue exists.
	hr = WcaTableExists(L"PSW_RemoveRegistryValue");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_RemoveRegistryValue'. Have you authored 'PanelSw:RemoveRegistryValue' entries in WiX code?");

	hr = WcaOpenExecuteView( RemoveRegistryValueQuery, &hView);
	BreakExitOnFailure(hr, "Failed to execute view");

	while ( E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
    {
		BreakExitOnFailure(hr, "Failed to fetch record");
		
		// Get record.
		LPWSTR pId = nullptr;
		LPWSTR pName = nullptr;
		LPWSTR pRoot = nullptr;
		LPWSTR pKey = nullptr;
		LPWSTR pArea = nullptr;
		CRegistryKey::RegRoot eRoot;
		CRegistryKey::RegArea eArea;
		CRegistryKey key;
		LPWSTR pCondition = nullptr;

		// existing value details
		BYTE* pData = nullptr;
		LPWSTR pDataString = nullptr;
		CRegistryKey::RegValueType valueType;
		DWORD dwValueSize = 0;
		CRegDataSerializer dataSerialiser;
		
		hr = WcaGetRecordString( hRec, eRemoveRegistryValueQuery::Id, &pId);
		BreakExitOnFailure(hr, "Failed to get Id");
		hr = WcaGetRecordString( hRec, eRemoveRegistryValueQuery::Root, &pRoot);
		BreakExitOnFailure(hr, "Failed to get Root");
		hr = WcaGetRecordFormattedString( hRec, eRemoveRegistryValueQuery::Key, &pKey);
		BreakExitOnFailure(hr, "Failed to get Key");
		hr = WcaGetRecordFormattedString( hRec, eRemoveRegistryValueQuery::Name, &pName);
		BreakExitOnFailure(hr, "Failed to get Name");
		hr = WcaGetRecordString( hRec, eRemoveRegistryValueQuery::Area, &pArea);
		BreakExitOnFailure(hr, "Failed to get Area");
		hr = WcaGetRecordString( hRec, eRemoveRegistryValueQuery::Condition, &pCondition);
		BreakExitOnFailure(hr, "Failed to get Condition");

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
			BreakExitOnFailure(hr, "Failed to evaluate condition");
		}

		// Parse root name.
		hr = CRegistryKey::ParseRoot( pRoot, &eRoot);
		BreakExitOnFailure(hr, "Failed to parse root");

		// Parse area name.
		hr = CRegistryKey::ParseArea( pArea, &eArea);
		BreakExitOnFailure(hr, "Failed to parse area");

		// CustomActionData
		hr = actionData.AddDeleteValue( pId, eRoot, pKey, eArea, pName);
		BreakExitOnFailure(hr, "Failed to add 'DeleteValue' to CustomActionData");

		// Rollback CustomActionData
		hr = key.Open( eRoot, pKey, eArea, CRegistryKey::RegAccess::ReadOnly);
		if( hr == E_FILENOTFOUND)
		{
			hr = S_OK;
			continue;
		}
		BreakExitOnFailure(hr, "Failed to query existing key");

		hr = key.GetValue( pName, &pData, &valueType, &dwValueSize);
		if( hr == E_FILENOTFOUND)
		{
			hr = S_OK;
			continue;
		}
		BreakExitOnFailure(hr, "Failed to query existing value");

		hr = dataSerialiser.Set( pData, valueType, dwValueSize);
		BreakExitOnFailure(hr, "Failed to initialize data serializer");

		hr = dataSerialiser.Serialize( &pDataString);
		BreakExitOnFailure(hr, "Failed to serialize data");

		hr = rollbackActionData.AddCreateValue(pId, eRoot, pKey, eArea, pName, valueType, pDataString);
		BreakExitOnFailure(hr, "Failed to create XML element");
	}
	
	hr = rollbackActionData.GetXmlString( &xmlString);
	BreakExitOnFailure(hr, "Failed to read XML as text");
	hr = WcaDoDeferredAction( L"RemoveRegistryValue_rollback", xmlString, 0);
	BreakExitOnFailure(hr, "Failed to set property");	
	xmlString.Empty();

	hr = actionData.GetXmlString( &xmlString);
	BreakExitOnFailure(hr, "Failed to read XML as text");
	hr = WcaDoDeferredAction( L"RemoveRegistryValue_deferred", xmlString, 0);
	BreakExitOnFailure(hr, "Failed to set property");
	xmlString.Empty();

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}


extern "C" __declspec( dllexport ) UINT RemoveRegistryValue_Deferred(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CRegistryXmlParser action;
	LPWSTR pActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	hr = WcaGetProperty( L"CustomActionData", &pActionData);
	BreakExitOnFailure(hr, "Failed to get CustomActionData");

	hr = action.Execute( pActionData);
	BreakExitOnFailure(hr, "Failed to parse-execute RemoveRegistryValue");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

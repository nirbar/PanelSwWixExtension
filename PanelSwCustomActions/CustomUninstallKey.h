#pragma once

#include "stdafx.h"
#include <windows.h>
#include "RegistryXmlParser.h"

class CCustomUninstallKey
{
public:
	CCustomUninstallKey( MSIHANDLE hInstall);
	~CCustomUninstallKey(void);

	HRESULT Execute();
	
	HRESULT CreateCustomActionData();

private:
	enum Attributes
    {
        None = 0,
        Write = 1,
        Delete = 2
    };

	HRESULT CreateRollbackCustomActionData( CRegistryXmlParser *pRollbackParser, WCHAR* pId, WCHAR* pName);

	HRESULT GetUninstallKey( WCHAR* keyName);

	MSIHANDLE _hInstall;
};


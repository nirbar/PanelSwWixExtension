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

	HRESULT CreateRollbackCustomActionData( CRegistryXmlParser *pRollbackParser, LPCWSTR szProductCode, LPWSTR pId, LPWSTR pName);

	HRESULT GetUninstallKey(LPCWSTR szProductCode, LPWSTR keyName);

	MSIHANDLE _hInstall;
};


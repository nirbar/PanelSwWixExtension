#pragma once

#include "stdafx.h"
#include <windows.h>
#include "RegistryXmlParser.h"

class CCustomUninstallKey
{
public:
	CCustomUninstallKey( MSIHANDLE hInstall) noexcept;
	virtual ~CCustomUninstallKey(void) noexcept;

	HRESULT Execute() noexcept;
	
	HRESULT CreateCustomActionData() noexcept;

private:
	enum Attributes
    {
        None = 0,
        Write = 1,
        Delete = 2
    };

	HRESULT CreateRollbackCustomActionData( CRegistryXmlParser *pRollbackParser, LPCWSTR szProductCode, LPWSTR pId, LPWSTR pName) noexcept;

	HRESULT GetUninstallKey(LPCWSTR szProductCode, LPWSTR keyName) noexcept;

	MSIHANDLE _hInstall;
};


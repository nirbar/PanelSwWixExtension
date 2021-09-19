#pragma once

#include <Windows.h>
#include "..\CaCommon\AutoRelease.h"

class CRegistryKey
{
public:

	enum RegValueType
	{
		String = REG_SZ,
		Expandable = REG_EXPAND_SZ,
		MultiString = REG_MULTI_SZ,

		DWord = REG_DWORD,
		QWord = REG_QWORD,

		Binary = REG_BINARY
	};

	enum RegArea
	{
		Default = 0,
		X86 = KEY_WOW64_32KEY,
		X64 = KEY_WOW64_64KEY
	};

	enum RegAccess
	{
		ReadOnly = KEY_READ,
		All = KEY_ALL_ACCESS
	};

	enum RegRoot
	{
		LocalMachine,
		CurrentUser,
		ClassesRoot,
		CurrentConfig,
		PerformanceData,
		Users
	};

	CRegistryKey() noexcept;
	virtual ~CRegistryKey() noexcept;

	HRESULT Create(RegRoot root, WCHAR* key, RegArea area, RegAccess acecss) noexcept;
	HRESULT Open(RegRoot root, WCHAR* key, RegArea area, RegAccess acecss) noexcept;
	HRESULT Delete() noexcept;

	HRESULT Close() noexcept;

	HRESULT EnumValues(DWORD dwIndex, LPWSTR* pszName, RegValueType* pdwType) noexcept;
	HRESULT GetValue(WCHAR* name, BYTE** pData, RegValueType* pType, DWORD* pDataSize) noexcept;
	HRESULT GetStringValue(LPWSTR szName, LPWSTR* pszData) noexcept;

	HRESULT SetValueString(WCHAR* name, WCHAR* value) noexcept;
	HRESULT SetValue(WCHAR* name, RegValueType type, BYTE* value, DWORD valueSize) noexcept;

	HRESULT DeleteValue(WCHAR* name) noexcept;

	static HRESULT ParseRoot(LPCWSTR pRootString, RegRoot* peRoot) noexcept;
	static HRESULT ParseArea(LPCWSTR pAreaString, RegArea* peArea) noexcept;
	static HRESULT ParseValueType(LPCWSTR pTypeString, RegValueType* peType) noexcept;
	static HRESULT GetDefaultArea(RegArea* pArea) noexcept;

private:

	HKEY Root2Handle(RegRoot root) noexcept;

	CHKEY _hKey;
	HKEY _hRootKey;
	WCHAR _keyName[MAX_PATH];

	RegArea _area;
	RegAccess _samAccess;
};
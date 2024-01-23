#pragma once

#include <Windows.h>

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

	CRegistryKey();
	virtual ~CRegistryKey();

	HRESULT Create(RegRoot root, LPCWSTR key, RegArea area, RegAccess acecss);
	HRESULT Open(RegRoot root, LPCWSTR key, RegArea area, RegAccess acecss);
	HRESULT Delete();

	void Close();

	HRESULT EnumValues(DWORD dwIndex, LPWSTR* pszName, RegValueType* pdwType);
	HRESULT GetValue(WCHAR* name, BYTE** pData, RegValueType* pType, DWORD* pDataSize);
	HRESULT GetStringValue(LPWSTR szName, LPWSTR* pszData);

	HRESULT SetValueString(WCHAR* name, WCHAR* value);
	HRESULT SetValue(WCHAR* name, RegValueType type, BYTE* value, DWORD valueSize);

	HRESULT DeleteValue(WCHAR* name);

	operator HKEY();

	static HRESULT ParseRoot(LPCWSTR pRootString, RegRoot* peRoot);
	static HRESULT ParseArea(LPCWSTR pAreaString, RegArea* peArea);
	static HRESULT ParseValueType(LPCWSTR pTypeString, RegValueType* peType);
	static HRESULT GetDefaultArea(RegArea* pArea);

private:

	HKEY Root2Handle(RegRoot root);

	HKEY _hKey;
	HKEY _hRootKey;
	WCHAR _keyName[MAX_PATH];

	RegArea _area;
	RegAccess _samAccess;
};
#include "pch.h"
#include "RegistryKey.h"
#include <strutil.h>
#include <regutil.h>
#include "SummaryStream.h"
#pragma comment( lib, "dutil.lib")
#pragma comment( lib, "Advapi32.lib")

CRegistryKey::CRegistryKey(void)
	: _hKey(NULL)
	, _hRootKey(NULL)
	, _area(RegArea::Default)
	, _samAccess(RegAccess::All)
{
	Close();
}

CRegistryKey::~CRegistryKey(void)
{
	Close();
}

void CRegistryKey::Close()
{
	if (_hKey)
	{
		RegCloseKey(_hKey);
		_hKey = nullptr;
	}

	_hRootKey = NULL;
	_keyName[0] = NULL;
}

HRESULT CRegistryKey::Create(RegRoot root, LPCWSTR key, RegArea area, RegAccess access)
{
	HRESULT hr = S_OK;
	HKEY hKey = NULL, hParentKey = NULL;

	Close();
	ExitOnNull((key && *key), hr, E_FILENOTFOUND, "key is NULL");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Attempting to create registry key %ls", key);

	hParentKey = Root2Handle(root);
	ExitOnNull(hParentKey, hr, E_FILENOTFOUND, "key is NULL");

	_samAccess = access;
	_area = area;
	if (_area == RegArea::Default)
	{
		hr = GetDefaultArea(&_area);
		ExitOnFailure(hr, "Failed to get default registry area");
	}

	hr = RegCreate(hParentKey, key, _samAccess | _area, &hKey);
	ExitOnFailure(hr, "Failed to create registry key");

	_hKey = hKey;
	_hRootKey = hParentKey;
	wcscpy_s<MAX_PATH>(_keyName, key);

LExit:
	return hr;
}

HRESULT CRegistryKey::Open(RegRoot root, LPCWSTR key, RegArea area, RegAccess access)
{
	HRESULT hr = S_OK;
	HKEY hParentKey = NULL;
	HKEY hKey = NULL;

	ExitOnNull((key && *key), hr, E_INVALIDARG, "key is NULL");

	Close();
	ExitOnFailure(hr, "Failed to close registry key");

	hParentKey = Root2Handle(root);
	ExitOnNull(hParentKey, hr, E_INVALIDARG, "Parent key is NULL");

	_samAccess = access;
	_area = area;
	if (_area == RegArea::Default)
	{
		hr = GetDefaultArea(&_area);
		ExitOnFailure(hr, "Failed to get default registry area");
	}

	hr = RegOpen(hParentKey, key, _samAccess | _area, &hKey);
	ExitOnFailure(hr, "Failed to open registry key");

	_hKey = hKey;
	_hRootKey = hParentKey;
	wcscpy_s<MAX_PATH>(_keyName, key);

LExit:
	return hr;
}

HRESULT CRegistryKey::Delete()
{
	HRESULT hr = S_OK;
	WCHAR keyName[MAX_PATH];

	ExitOnNull(_hKey, hr, E_FILENOTFOUND, "_hKey is NULL");

	// Copy info
	HKEY rootKey = _hRootKey;
	wcscpy_s<MAX_PATH>(keyName, _keyName);

	// Close held handle.
	Close();

	// Delete key
	hr = RegDelete(rootKey, keyName, (_area == RegArea::X64 ? REG_KEY_BITNESS::REG_KEY_64BIT : (_area == RegArea::X86) ? REG_KEY_BITNESS::REG_KEY_32BIT : REG_KEY_BITNESS::REG_KEY_DEFAULT), TRUE);
	ExitOnFailure(hr, "Failed to delete registry key '%ls'", keyName);

LExit:
	return hr;
}

HRESULT CRegistryKey::EnumValues(DWORD dwIndex, LPWSTR* pszName, RegValueType* pdwType)
{
	HRESULT hr = S_OK;

	hr = RegValueEnum(_hKey, dwIndex, pszName, (DWORD*)pdwType);
	if (hr == E_NOMOREITEMS)
	{
		hr = S_FALSE;
		ExitFunction();
	}
	ExitOnFailure(hr, "Failed to enumerate values for registry key");

LExit:
	return hr;
}

HRESULT CRegistryKey::GetStringValue(LPWSTR szName, LPWSTR* pszData)
{
	HRESULT hr = S_OK;

	hr = RegReadString(_hKey, szName, pszData);
	ExitOnFailure(hr, "Failed to get registry string value '%ls'", szName);

LExit:
	return hr;
}

HRESULT CRegistryKey::GetValue(WCHAR* name, BYTE** pData, RegValueType* pType, DWORD* pDataSize)
{
	LONG lRes = ERROR_SUCCESS;
	HRESULT hr = S_OK;

	ExitOnNull(pData, hr, E_INVALIDARG, "pData is NULL");
	ExitOnNull(pType, hr, E_INVALIDARG, "pType is NULL");
	ExitOnNull(pDataSize, hr, E_INVALIDARG, "pDataSize is NULL");

	(*pDataSize) = 0;
	lRes = ::RegQueryValueEx(_hKey, name, 0, (LPDWORD)pType, nullptr, pDataSize);
	hr = HRESULT_FROM_WIN32(lRes);
	ExitOnFailure(hr, "Failed to query registry value");
	ExitOnNull((*pDataSize), hr, E_FILENOTFOUND, "Registry value's size is 0.");

	(*pData) = new BYTE[(*pDataSize) + 1];
	ExitOnNull((*pData), hr, E_NOT_SUFFICIENT_BUFFER, "Can't allocate buffer");

	lRes = ::RegQueryValueEx(_hKey, name, 0, (LPDWORD)pType, (*pData), pDataSize);
	hr = HRESULT_FROM_WIN32(lRes);
	ExitOnFailure(hr, "Failed to get registry value");
	(*pData)[(*pDataSize)] = NULL; // Just to make sure.

LExit:
	return hr;
}

HRESULT CRegistryKey::SetValue(WCHAR* name, RegValueType type, BYTE* value, DWORD valueSize)
{
	HRESULT hr = S_OK;
	LONG lRes = ERROR_SUCCESS;
	ExitOnNull(_hKey, hr, E_FILENOTFOUND, "_hKey is NULL");

	lRes = ::RegSetValueEx(_hKey, name, 0, type, value, valueSize);
	hr = HRESULT_FROM_WIN32(lRes);
	ExitOnFailure(hr, "Failed to set registry value");

LExit:
	return hr;
}

HRESULT CRegistryKey::SetValueString(WCHAR* name, WCHAR* value)
{
	HRESULT hr = S_OK;

	ExitOnNull(name, hr, E_FILENOTFOUND, "name is NULL");
	ExitOnNull(value, hr, E_FILENOTFOUND, "value is NULL");

	DWORD dwSize = ((wcslen(value) + 1) * sizeof(WCHAR));
	hr = SetValue(
		name
		, RegValueType::String
		, (BYTE*)value
		, dwSize
	);

LExit:
	return hr;
}

HRESULT CRegistryKey::DeleteValue(WCHAR* name)
{
	HRESULT hr = S_OK;
	LONG lRes = ERROR_SUCCESS;
	ExitOnNull(_hKey, hr, E_FILENOTFOUND, "_hKey is NULL");

	RegDeleteValue(_hKey, name);

LExit:
	return hr;
}

HKEY CRegistryKey::Root2Handle(RegRoot root)
{
	switch (root)
	{
	case RegRoot::ClassesRoot:
		return HKEY_CLASSES_ROOT;

	case RegRoot::LocalMachine:
		return HKEY_LOCAL_MACHINE;

	case RegRoot::CurrentUser:
		return HKEY_CURRENT_USER;

	case RegRoot::CurrentConfig:
		return HKEY_CURRENT_CONFIG;

	case RegRoot::PerformanceData:
		return HKEY_PERFORMANCE_DATA;

	case RegRoot::Users:
		return HKEY_USERS;
	}

	return NULL;
}

HRESULT CRegistryKey::ParseRoot(LPCWSTR pRootString, RegRoot* peRoot)
{
	HRESULT hr = S_OK;

	ExitOnNull(peRoot, hr, E_INVALIDARG, "Invalid root pointer");

	if (!(pRootString && *pRootString))
	{
		(*peRoot) = RegRoot::CurrentUser;
		ExitFunction();
	}

	if ((::_wcsicmp(pRootString, L"HKLM") == 0)
		|| (::_wcsicmp(pRootString, L"HKEY_LOCAL_MACHINE") == 0))
	{
		(*peRoot) = RegRoot::LocalMachine;
	}
	else if ((::_wcsicmp(pRootString, L"HKCR") == 0)
		|| (::_wcsicmp(pRootString, L"HKEY_CLASSES_ROOT") == 0))
	{
		(*peRoot) = RegRoot::ClassesRoot;
	}
	else if ((::_wcsicmp(pRootString, L"HKCC") == 0)
		|| (::_wcsicmp(pRootString, L"HKEY_CURRENT_CONFIG") == 0))
	{
		(*peRoot) = RegRoot::CurrentConfig;
	}
	else if ((::_wcsicmp(pRootString, L"HKCU") == 0)
		|| (::_wcsicmp(pRootString, L"HKEY_CURRENT_USER") == 0))
	{
		(*peRoot) = RegRoot::CurrentUser;
	}
	else if ((::_wcsicmp(pRootString, L"HKU") == 0)
		|| (::_wcsicmp(pRootString, L"HKEY_USERS") == 0))
	{
		(*peRoot) = RegRoot::Users;
	}
	else
	{
		hr = E_INVALIDARG;
		ExitOnFailure(hr, "Invalid root name");
	}

LExit:
	return hr;
}

HRESULT CRegistryKey::ParseArea(LPCWSTR pAreaString, RegArea* peArea)
{
	HRESULT hr = S_OK;

	ExitOnNull(peArea, hr, E_INVALIDARG, "Invalid area pointer");

	if (!pAreaString || !*pAreaString || (::_wcsicmp(pAreaString, L"default") == 0))
	{
		hr = GetDefaultArea(peArea);
		ExitOnFailure(hr, "Failed to get default registry area");
		ExitFunction();
	}
	else if (::_wcsicmp(pAreaString, L"x86") == 0)
	{
		(*peArea) = RegArea::X86;
	}
	else if (::_wcsicmp(pAreaString, L"x64") == 0)
	{
		(*peArea) = RegArea::X64;
	}
	else
	{
		hr = E_INVALIDARG;
		ExitOnFailure(hr, "Invalid area name");
	}

LExit:
	return hr;
}

HRESULT CRegistryKey::ParseValueType(LPCWSTR pTypeString, RegValueType* peType)
{
	HRESULT hr = S_OK;

	ExitOnNull(peType, hr, E_INVALIDARG, "Invalid type pointer");

	if (!pTypeString || !*pTypeString)
	{
		(*peType) = RegValueType::String;
		ExitFunction();
	}

	if ((::_wcsicmp(pTypeString, L"REG_SZ") == 0)
		|| (::_wcsicmp(pTypeString, L"String") == 0)
		|| (::_wcsicmp(pTypeString, L"1") == 0))
	{
		(*peType) = RegValueType::String;
	}
	else if ((::_wcsicmp(pTypeString, L"REG_BINARY") == 0)
		|| (::_wcsicmp(pTypeString, L"Binary") == 0)
		|| (::_wcsicmp(pTypeString, L"3") == 0))
	{
		(*peType) = RegValueType::Binary;
	}
	else if ((::_wcsicmp(pTypeString, L"REG_DWORD") == 0)
		|| (::_wcsicmp(pTypeString, L"DWord") == 0)
		|| (::_wcsicmp(pTypeString, L"4") == 0))
	{
		(*peType) = RegValueType::DWord;
	}
	else if ((::_wcsicmp(pTypeString, L"REG_EXPAND_SZ") == 0)
		|| (::_wcsicmp(pTypeString, L"Expandable") == 0)
		|| (::_wcsicmp(pTypeString, L"2") == 0))
	{
		(*peType) = RegValueType::Expandable;
	}
	else if ((::_wcsicmp(pTypeString, L"REG_MULTI_SZ") == 0)
		|| (::_wcsicmp(pTypeString, L"MultiString") == 0)
		|| (::_wcsicmp(pTypeString, L"7") == 0))
	{
		(*peType) = RegValueType::MultiString;
	}
	else if ((::_wcsicmp(pTypeString, L"REG_QWORD") == 0)
		|| (::_wcsicmp(pTypeString, L"QWord") == 0)
		|| (::_wcsicmp(pTypeString, L"11") == 0))
	{
		(*peType) = RegValueType::QWord;
	}
	else
	{
		hr = E_INVALIDARG;
		ExitOnFailure(hr, "Invalid area name");
	}

LExit:
	return hr;
}

HRESULT CRegistryKey::GetDefaultArea(CRegistryKey::RegArea* pArea)
{
	HRESULT hr = S_OK;
	DWORD dwRes = ERROR_SUCCESS;

	ExitOnNull(pArea, hr, E_INVALIDARG, "pArea is NULL");
	(*pArea) = RegArea::Default;

	hr = CSummaryStream::IsPackageX64();
	ExitOnFailure(hr, "Failed determining package bitness");

	(*pArea) = (hr == S_OK) ? RegArea::X64 : RegArea::X86;

LExit:
	return hr;
}

CRegistryKey::operator HKEY()
{
	return _hKey;
}
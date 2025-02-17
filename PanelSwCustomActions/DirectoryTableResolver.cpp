#include "stdafx.h"
#include "../CaCommon/WixString.h"
#include "DirectoryTableResolver.h"
#include <shlwapi.h>
#pragma comment (lib, "shlwapi.lib")

CDirectoryTableResolver::~CDirectoryTableResolver()
{
	if (_hDirectoryTable)
	{
		::MsiCloseHandle(_hDirectoryTable);
		_hDirectoryTable = NULL;
	}
	if (_hDirectoryColumns)
	{
		::MsiCloseHandle(_hDirectoryColumns);
		_hDirectoryColumns = NULL;
	}
	if (_hCreateFolderTable)
	{
		::MsiCloseHandle(_hCreateFolderTable);
		_hCreateFolderTable = NULL;
	}
	if (_hCreateFolderColumns)
	{
		::MsiCloseHandle(_hCreateFolderColumns);
		_hCreateFolderColumns = NULL;
	}
}

HRESULT CDirectoryTableResolver::Initialize()
{
	HRESULT hr = S_OK;

	hr = WcaOpenView(L"SELECT `Directory_Parent`, `DefaultDir` FROM `Directory` WHERE `Directory` = ?", &_hQueryDirView);
	ExitOnFailure(hr, "Failed to open query view");

	hr = WcaOpenView(L"SELECT `Directory` FROM `Directory` WHERE `Directory_Parent` = ? AND `DefaultDir` = ?", &_hInsertDirQueryView);
	ExitOnFailure(hr, "Failed to open insert view");

	hr = WcaOpenView(L"SELECT `Directory_`, `Component_` FROM `CreateFolder` WHERE `Directory_` = ? AND `Component_` = ?", &_hQueryCreateFolderView);
	ExitOnFailure(hr, "Failed to open insert view");

LExit:
	return hr;
}

HRESULT CDirectoryTableResolver::InsertHierarchy(LPCWSTR szParentId, LPCWSTR szRelativeHierarchy, LPWSTR* pszDirectoryId)
{
	HRESULT hr = S_OK;
	PMSIHANDLE hQueryRecord, hDataRecord;
	CWixString szClosestParent;
	LPCWSTR szMissingPathParts = 0;
	static DWORD dwUniquifyValue = ::GetTickCount();

	ExitOnNull(_hInsertDirQueryView, hr, E_NOT_VALID_STATE, "Query view not initialized");
	
	hr = InsertDirectoryIfMissing(szParentId, L"TARGETDIR", L"."); // If the root copy-to is a property, we need to have it in Directory table to be able to insert its descendants
	ExitOnFailure(hr, "Failed to insert directory '%ls' to Directory table", szParentId);

	while (szRelativeHierarchy && (*szRelativeHierarchy == L'\\'))
	{
		++szRelativeHierarchy;
	}
	if (!szRelativeHierarchy || !*szRelativeHierarchy)
	{
		hr = StrAllocString(pszDirectoryId, szParentId, 0);
		ExitOnFailure(hr, "Failed to copy string");
		ExitFunction();
	}

	hr = szClosestParent.Copy(szParentId);
	ExitOnFailure(hr, "Failed to copy string");

	hQueryRecord = ::MsiCreateRecord(2);
	ExitOnNullWithLastError(hQueryRecord, hr, "Failed to create record");

	szMissingPathParts = szRelativeHierarchy;
	for (LPCWSTR szNextPart = nullptr; szMissingPathParts && *szMissingPathParts; szMissingPathParts = szNextPart)
	{
		DWORD cPart = INFINITE;
		CWixString szPart;

		// Skip slashes
		while (*szMissingPathParts == L'\\')
		{
			++szMissingPathParts;
		}
		if (!*szMissingPathParts)
		{
			break;
		}
		szNextPart = wcschr(szMissingPathParts, L'\\');
		if (szNextPart)
		{
			cPart = szNextPart - szMissingPathParts;
		}

		hr = szPart.Copy(szMissingPathParts, cPart);
		ExitOnFailure(hr, "Failed to copy string");

		hr = WcaSetRecordString(hQueryRecord, 1, (LPCWSTR)szClosestParent);
		ExitOnFailure(hr, "Failed to set record");

		hr = WcaSetRecordString(hQueryRecord, 2, (LPCWSTR)szPart);
		ExitOnFailure(hr, "Failed to set record");

		hr = WcaExecuteView(_hInsertDirQueryView, hQueryRecord);
		ExitOnFailure(hr, "Failed to execute view");

		hr = WcaFetchSingleRecord(_hInsertDirQueryView, &hDataRecord);
		ExitOnFailure(hr, "Failed to fetch record");
		if (hr == S_FALSE)
		{
			break;
		}

		hr = WcaGetRecordString(hDataRecord, 1, (LPWSTR*)szClosestParent);
		ExitOnFailure(hr, "Failed to get 'Directory'");
	}

	for (LPCWSTR szNextPart = nullptr; szMissingPathParts && *szMissingPathParts; szMissingPathParts = szNextPart)
	{
		DWORD cPart = INFINITE;
		CWixString szPart;
		CWixString szPartId;
		UINT i = 0;

		// Skip slashes
		while (*szMissingPathParts == L'\\')
		{
			++szMissingPathParts;
		}
		if (!*szMissingPathParts)
		{
			break;
		}

		szNextPart = wcschr(szMissingPathParts, L'\\');
		if (szNextPart)
		{
			cPart = szNextPart - szMissingPathParts;
		}

		hr = szPart.Copy(szMissingPathParts, cPart);
		ExitOnFailure(hr, "Failed to copy string");

		hr = szPartId.Format(L"_DUP_%ls%u", szParentId, ++dwUniquifyValue);
		ExitOnFailure(hr, "Failed to format string");

		hr = WcaAddTempRecord(&_hDirectoryTable, &_hDirectoryColumns, L"Directory", nullptr, 0, 3, (LPCWSTR)szPartId, (LPCWSTR)szClosestParent, (LPCWSTR)szPart);
		ExitOnFailure(hr, "Failed to add temporary row table");

		szClosestParent.Attach(szPartId.Detach());
	}

	*pszDirectoryId = szClosestParent.Detach();

LExit:
	return hr;
}

HRESULT CDirectoryTableResolver::ResolvePath(LPCWSTR szDirecotoryId, LPWSTR* pszPath)
{
	HRESULT hr = S_OK;
	LPWSTR *pszPathParts = nullptr;
	UINT cPathParts = 0;
	UINT nPathLen = 1;
	CWixString szNextDirId;
	CWixString szBasePath;
	LPWSTR szFullPath = nullptr;
	PMSIHANDLE hQueryRecord, hDataRecord;

	ExitOnNull(_hQueryDirView, hr, E_NOT_VALID_STATE, "Query view not initialized");

	hr = WcaGetProperty(szDirecotoryId, (LPWSTR*)szBasePath);
	ExitOnFailure(hr, "Failed to get property '%ls'", (LPCWSTR)szDirecotoryId);
	if (!szBasePath.IsNullOrEmpty())
	{
		*pszPath = szBasePath.Detach();
		ExitFunction();
	}

	hQueryRecord = ::MsiCreateRecord(2);
	ExitOnNullWithLastError(hQueryRecord, hr, "Failed to create record");

	hr = szNextDirId.Copy(szDirecotoryId);
	ExitOnFailure(hr, "Failed to copy string");

	do
	{
		CWixString szDirName;

		hr = WcaSetRecordString(hQueryRecord, 1, (LPCWSTR)szNextDirId);
		ExitOnFailure(hr, "Failed to set record");

		hr = WcaExecuteView(_hQueryDirView, hQueryRecord);
		ExitOnFailure(hr, "Failed to execute view");

		hr = WcaFetchSingleRecord(_hQueryDirView, &hDataRecord);
		ExitOnFailure(hr, "Failed to fetch record");
		if (hr == S_FALSE)
		{
			ExitFunction();
		}

		hr = WcaGetRecordString(hDataRecord, 1, (LPWSTR*)szNextDirId);
		ExitOnFailure(hr, "Failed to get 'Directory_Parent'");
		hr = WcaGetRecordString(hDataRecord, 2, (LPWSTR*)szDirName);
		ExitOnFailure(hr, "Failed to get 'DefaultDir'");
		
		if (!szDirName.IsNullOrEmpty() && !szDirName.EqualsIgnoreCase(L"."))
		{
			UINT iLongName = szDirName.Find(L'|');
			if (iLongName < szDirName.StrLen())
			{
				hr = szDirName.Substring(iLongName + 1);
				ExitOnFailure(hr, "Failed to remove short name from dir name '%ls'", (LPCWSTR)szDirName);
			}

			nPathLen += 1 + szDirName.StrLen(); // Add place for backslash

			hr = StrArrayAllocString(&pszPathParts, &cPathParts, szDirName, 0);
			ExitOnFailure(hr, "Failed to insert string to array");
		}
		if (szNextDirId.IsNullOrEmpty())
		{
			hr = WcaGetProperty(L"ROOTDRIVE", (LPWSTR*)szBasePath);
			ExitOnFailure(hr, "Failed to get property 'ROOTDRIVE'");

			if (szNextDirId.IsNullOrEmpty())
			{
				hr = S_FALSE;
				ExitFunction();
			}
			break;
		}

		hr = WcaGetProperty(szNextDirId, (LPWSTR*)szBasePath);
		ExitOnFailure(hr, "Failed to get property '%ls'", (LPCWSTR)szNextDirId);

	} while (szBasePath.IsNullOrEmpty());

	nPathLen += szBasePath.StrLen();
	hr = StrAllocString(&szFullPath, szBasePath, nPathLen);
	ExitOnFailure(hr, "Failed to allocate string");

	for (UINT i = 0; pszPathParts && (i < cPathParts); ++i)
	{
		UINT ri = cPathParts - i - 1;

		::PathAddBackslash(szFullPath);
		
		hr = ::StringCchCat(szFullPath, nPathLen, pszPathParts[ri]);
		ExitOnFailure(hr, "Failed to combine path parts");
	}
	*pszPath = szFullPath;
	szFullPath = nullptr;

LExit:
	ReleaseStrArray(pszPathParts, cPathParts);
	ReleaseStr(szFullPath);

	return hr;
}

HRESULT CDirectoryTableResolver::InsertDirectoryIfMissing(LPCWSTR szDirectory, LPCWSTR szParent, LPCWSTR szName)
{
	HRESULT hr = S_OK;
	PMSIHANDLE hQueryRecord, hDataRecord;

	ExitOnNull(_hQueryDirView, hr, E_NOT_VALID_STATE, "Query view not initialized");

	hQueryRecord = ::MsiCreateRecord(2);
	ExitOnNullWithLastError(hQueryRecord, hr, "Failed to create record");

	hr = WcaSetRecordString(hQueryRecord, 1, szDirectory);
	ExitOnFailure(hr, "Failed to set record");

	hr = WcaExecuteView(_hQueryDirView, hQueryRecord);
	ExitOnFailure(hr, "Failed to execute view");

	hr = WcaFetchSingleRecord(_hQueryDirView, &hDataRecord);
	ExitOnFailure(hr, "Failed to fetch record");
	if (hr == S_OK)
	{
		ExitFunction();
	}

	hr = WcaAddTempRecord(&_hDirectoryTable, &_hDirectoryColumns, L"Directory", nullptr, 0, 3, szDirectory, szParent, szName);
	ExitOnFailure(hr, "Failed to add temporary row table");

LExit:

	return hr;
}

HRESULT CDirectoryTableResolver::InsertCreateFolderIfMissing(LPCWSTR szDirectory, LPCWSTR szComponent)
{
	HRESULT hr = S_OK;
	PMSIHANDLE hQueryRecord, hDataRecord;

	ExitOnNull(_hQueryCreateFolderView, hr, E_NOT_VALID_STATE, "Query view not initialized");

	hQueryRecord = ::MsiCreateRecord(3);
	ExitOnNullWithLastError(hQueryRecord, hr, "Failed to create record");

	hr = WcaSetRecordString(hQueryRecord, 1, szDirectory);
	ExitOnFailure(hr, "Failed to set record");

	hr = WcaSetRecordString(hQueryRecord, 2, szComponent);
	ExitOnFailure(hr, "Failed to set record");

	hr = WcaExecuteView(_hQueryCreateFolderView, hQueryRecord);
	ExitOnFailure(hr, "Failed to execute view");

	hr = WcaFetchSingleRecord(_hQueryCreateFolderView, &hDataRecord);
	ExitOnFailure(hr, "Failed to fetch record");
	if (hr == S_OK)
	{
		ExitFunction();
	}

	hr = WcaAddTempRecord(&_hCreateFolderTable, &_hCreateFolderColumns, L"CreateFolder", nullptr, 0, 2, szDirectory, szComponent);
	ExitOnFailure(hr, "Failed to add temporary row table");

LExit:

	return hr;
}

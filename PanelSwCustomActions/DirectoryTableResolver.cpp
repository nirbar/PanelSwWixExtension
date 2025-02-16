#include "stdafx.h"
#include "../CaCommon/WixString.h"
#include "DirectoryTableResolver.h"
#include <shlwapi.h>
#pragma comment (lib, "shlwapi.lib")

HRESULT CDirectoryTableResolver::Initialize()
{
	HRESULT hr = S_OK;

	hr = WcaOpenView(L"SELECT `Directory_Parent`, `DefaultDir` FROM `Directory` WHERE `Directory` = ?", &_hQueryDirView);
	ExitOnFailure(hr, "Failed to open query view");

	hr = WcaOpenView(L"SELECT `Directory` FROM `Directory` WHERE `Directory_Parent` = ? AND `DefaultDir` = ?", &_hInsertDirQueryView);
	ExitOnFailure(hr, "Failed to open insert view");

LExit:
	return hr;
}

HRESULT CDirectoryTableResolver::InsertHierarchy(LPCWSTR szParentId, LPCWSTR szRelativeHierarchy, LPWSTR* pszDirectoryId)
{
	HRESULT hr = S_OK;
	PMSIHANDLE hQueryRecord, hDataRecord;
	MSIHANDLE hDirectoryTable = NULL;
	MSIHANDLE hDirectoryColumns = NULL;
	CWixString szClosestParent;
	LPCWSTR szMissingPathParts = 0;
	static DWORD dwUniquifyValue = ::GetTickCount();

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

		hr = WcaAddTempRecord(&hDirectoryTable, &hDirectoryColumns, L"Directory", nullptr, 0, 3, (LPCWSTR)szPartId, (LPCWSTR)szClosestParent, (LPCWSTR)szPart);
		ExitOnFailure(hr, "Failed to add temporary row table");

		szClosestParent.Attach(szPartId.Detach());
	}

	*pszDirectoryId = szClosestParent.Detach();

LExit:
	if (hDirectoryTable)
	{
		::MsiCloseHandle(hDirectoryTable);
	}
	if (hDirectoryColumns)
	{
		::MsiCloseHandle(hDirectoryColumns);
	}
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
	ExitOnFailure(hr, "Failed to get property");
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
			nPathLen += 1 + szDirName.StrLen(); // Add place for backslash

			hr = StrArrayAllocString(&pszPathParts, &cPathParts, szDirName, 0);
			ExitOnFailure(hr, "Failed to insert string to array");
		}

		hr = WcaGetProperty(szNextDirId, (LPWSTR*)szBasePath);
		ExitOnFailure(hr, "Failed to get property");

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

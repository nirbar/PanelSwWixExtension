#include "stdafx.h"
#include "FileSpecFilter.h"
#include <strutil.h>
#include <shlwapi.h>
#pragma comment (lib, "shlwapi.lib")

void CFileSpecFilter::Release()
{
	ReleaseNullStr(_szBaseFolder);
	ReleaseNullStr(_szFilter);
	_bRecursive = false;
}

HRESULT CFileSpecFilter::Initialize(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive)
{
	HRESULT hr = S_OK;

	Release();

	hr = StrAllocString(&_szBaseFolder, szBaseFolder, 0);
	ExitOnFailure(hr, "Failed to allocate string");
	PathRemoveBackslash(_szBaseFolder);

	hr = StrAllocString(&_szFilter, szFilter, 0);
	ExitOnFailure(hr, "Failed to allocate string");

	_bRecursive = bRecursive;

LExit:
	return hr;
}

HRESULT CFileSpecFilter::IsMatch(LPCWSTR szFilePath) const
{
	HRESULT hr = S_OK;
	LPCWSTR szTest = nullptr;
	BOOL bRes = TRUE;

	ExitOnNull((_szFilter && _szBaseFolder && *_szFilter && *_szBaseFolder), hr, E_INVALIDSTATE, "File filter not initialized");

	szTest = wcsistr(szFilePath, _szBaseFolder);
	ExitOnNull((szTest == szFilePath), hr, E_INVALIDARG, "File path '%ls' doesn't match expected base folder '%ls'", szFilePath, _szBaseFolder);

	szTest += wcslen(_szBaseFolder);
	while (szTest[0] == L'\\')
	{
		++szTest;
	}
	ExitOnNull(*szTest, hr, E_INVALIDARG, "File path '%ls' has only the base folder", _szBaseFolder);

	bRes = ::PathMatchSpec(szTest, _szFilter);
	hr = bRes ? S_OK : S_FALSE;

LExit:
	return hr;
}

#include "stdafx.h"
#include "FileGlobFilter.h"
#include "../CaCommon/DeferredActionBase.h"
#include <strutil.h>
#include <shlwapi.h>
using namespace std;
#pragma comment (lib, "shlwapi.lib")

void CFileGlobFilter::Release()
{
	_rxPattern.assign(L".*");
	ReleaseNullStr(_szBaseFolder);
}

HRESULT CFileGlobFilter::Initialize(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive)
{
	HRESULT hr = S_OK;
	LPWSTR szRxPattern = nullptr;
	size_t nRxIndex = 0;
	size_t nStrLen = 0;
	size_t nRxStrLen = 0;
	LPCWSTR szAnyFileName = L"[^\\\\/]*";
	bool bOptionalFolder = false;

	Release();
	nStrLen = wcslen(szFilter);
	if (!nStrLen)
	{
		ExitFunction();
	}
	nRxStrLen = 3 + (nStrLen * wcslen(szAnyFileName)); // Max length comes from pattern '*'. Add NULL, ^, $

	hr = StrAlloc(&szRxPattern, nRxStrLen);
	ExitOnFailure(hr, "Failed to allocate memory");
	ZeroMemory(szRxPattern, nRxStrLen * sizeof(WCHAR));

	szRxPattern[nRxIndex++] = L'^';
	for (size_t i = 0; i < nStrLen; ++i)
	{
		WCHAR c = szFilter[i];
		if ((c == L'\\') || (c == L'/'))
		{
			// Skip excessive slashes
			while ((szFilter[i + 1] == L'/') || (szFilter[i + 1] == L'\\'))
			{
				++i;
			}

			if (nRxIndex > 1) // Skip leading slashes
			{
				szRxPattern[nRxIndex++] = L'\\';
				szRxPattern[nRxIndex++] = L'\\';
				if (bOptionalFolder)
				{
					szRxPattern[nRxIndex++] = L'?';
				}
			}
			continue;
		}
		bOptionalFolder = false;

		// Regex special charactres
		if ((c == L'.') || (c == L'(') || (c == L')') || (c == L'[') || (c == L']') || (c == L'{') || (c == L'}') || (c == L'^') || (c == L'$') || (c == L'?'))
		{
			szRxPattern[nRxIndex++] = L'\\';
			szRxPattern[nRxIndex++] = c;
			continue;
		}
		// Plain character
		if (c != L'*')
		{
			szRxPattern[nRxIndex++] = c;
			continue;
		}

		// **
		if (szFilter[i + 1] == L'*')
		{
			++i;
			bOptionalFolder = true;
			szRxPattern[nRxIndex++] = L'.';
			szRxPattern[nRxIndex++] = L'*';
			continue;
		}
		else // *
		{
			hr = StrAllocConcat(&szRxPattern, szAnyFileName, 0);
			ExitOnFailure(hr, "Failed to allocate memory");

			nRxIndex = wcslen(szRxPattern);
			continue;
		}
	}

	// Remove trailing slashes (i.e. '**/')
	while (szRxPattern[nRxIndex - 1] == L'\\')
	{
		szRxPattern[--nRxIndex] = NULL;
	}
	szRxPattern[nRxIndex++] = L'$';
	CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Translated file pattern '%ls' to regex '%ls'", szFilter, szRxPattern);

	try
	{
		_rxPattern.assign(szRxPattern, regex_constants::syntax_option_type::ECMAScript | regex_constants::syntax_option_type::icase | regex_constants::syntax_option_type::optimize);
	}
	catch (std::regex_error ex)
	{
		hr = HRESULT_FROM_WIN32(ex.code());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
		ExitOnFailure(hr, "Failed assigning regular expression '%ls'. %hs", szRxPattern, ex.what());
	}

	hr = StrAllocString(&_szBaseFolder, szBaseFolder, 0);
	ExitOnFailure(hr, "Failed to allocate string");
	PathRemoveBackslash(_szBaseFolder);

LExit:
	ReleaseStr(szRxPattern);

	return hr;
}

HRESULT CFileGlobFilter::IsMatch(LPCWSTR szFilePath) const
{
	HRESULT hr = S_OK;
	LPCWSTR szTest = nullptr;

	ExitOnNull(_szBaseFolder, hr, E_INVALIDSTATE, "File glob filter not initialized");

	szTest = wcsistr(szFilePath, _szBaseFolder);
	ExitOnNull((szTest == szFilePath), hr, E_INVALIDARG, "File path '%ls' doesn't match expected base folder '%ls'", szFilePath, _szBaseFolder);

	szTest += wcslen(_szBaseFolder);
	while (szTest[0] == L'\\')
	{
		++szTest;
	}
	ExitOnNull(*szTest, hr, E_INVALIDARG, "File path '%ls' has only the base folder", _szBaseFolder);

	try
	{
		if (regex_match(szTest, _rxPattern))
		{
			hr = S_OK;
		}
		else
		{
			hr = S_FALSE;
		}
	}
	catch (std::regex_error ex)
	{
		hr = HRESULT_FROM_WIN32(ex.code());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
		ExitOnFailure(hr, "Failed evaluating regular expression on path '%ls'. %hs", szTest, ex.what());
	}

LExit:
	return hr;
}

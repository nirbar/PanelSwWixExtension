#include "pch.h"
#include "FileGlobFilter.h"
#include "../CaCommon/DeferredActionBase.h"
using namespace std;

void CFileGlobFilter::Release()
{
	_rxPattern.assign(L".*");
	ReleaseNullStr(_szBaseFolder);
}

/* Supported glob format as in https://code.visualstudio.com/docs/editor/glob-patterns
	- / to separate path segments
	- * to match zero or more characters in a path segment
	- ? to match on one character in a path segment
	- ** to match any number of path segments, including none
	- {} to group conditions (for example {**\*.html, **\*.txt} matches all HTML and text files)
	- [] to declare a range of characters to match (example.[0-9] to match on example.0, example.1, …)
	- [!...] to negate a range of characters to match (example.[!0-9] to match on example.a, example.b, but not example.0)
*/
HRESULT CFileGlobFilter::Initialize(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive)
{
	HRESULT hr = S_OK;
	LPWSTR szRxPattern = nullptr;
	size_t nStrLen = 0;
	size_t nRxStrLen = 0;
	size_t nMaxPatternSize = 0;
	LPCWSTR szAsterisk = L"[^\\\\]*";		// * = Any character sequence within segment
	LPCWSTR szSeparator = L"\\\\";			// Segment separator
	LPCWSTR szQuestionMark = L"[^\\\\]?";	// ? = Any character within segment
	LPCWSTR szGlobstar = L".*(^|$|\\\\)";	// ** = Multi segment
	LPCWSTR szGlobSpecialChar = L"?*[]{},";
	bool bInGroup = false;
	bool bInRange = false;

	Release();
	nStrLen = wcslen(szFilter);
	if (!nStrLen)
	{
		ExitFunction();
	}

	nMaxPatternSize = max(wcslen(szAsterisk), wcslen(szSeparator), wcslen(szQuestionMark), wcslen(szGlobstar));
	nRxStrLen = 3 + (nStrLen * nMaxPatternSize); // Max pattern length + NULL, ^, $

	hr = StrAlloc(&szRxPattern, nRxStrLen);
	ExitOnFailure(hr, "Failed to allocate memory");
	ZeroMemory(szRxPattern, nRxStrLen * sizeof(WCHAR));

	szRxPattern[0] = L'^';
	for (size_t i = 0; i < nStrLen; ++i)
	{
		size_t nRxIndex = wcslen(szRxPattern);
		WCHAR c = szFilter[i];
		
		// Escaping special glob character
		if (c == L'\\')
		{
			ExitOnNull(wcschr(szGlobSpecialChar, szFilter[i + 1]), hr, E_INVALIDARG, "Invalid glob expression: Escape char before a non-special character '%lc'. Pattern '%ls', index %i", szFilter[i + 1], szFilter, i);

			szRxPattern[nRxIndex++] = L'\\';
			szRxPattern[nRxIndex++] = szFilter[++i];
			continue;
		}
		if (c == L'/')
		{
			// Skip excessive slashes
			while (szFilter[i + 1] == L'/')
			{
				++i;
			}

			if (nRxIndex > 1) // Skip leading slashes
			{
				hr = StringCchCat(szRxPattern, nRxStrLen, szSeparator);
				ExitOnFailure(hr, "Failed to concatentate strings");
			}
			continue;
		}

		// Group start / middle / end
		if (c == L'{')
		{
			ExitOnNull((!bInRange && !bInGroup), hr, E_INVALIDARG, "Invalid glob expression: Group start before previous range/group ended. Pattern '%ls', index %i", szFilter, i);
			bInGroup = true;
			szRxPattern[nRxIndex++] = L'(';
			continue;
		}
		if ((c == L',') && bInGroup)
		{
			szRxPattern[nRxIndex++] = L'|';
			continue;
		}
		if (c == L'}')
		{
			if (bInGroup)
			{
				bInGroup = false;
				szRxPattern[nRxIndex++] = L')';
			}
			else
			{
				szRxPattern[nRxIndex++] = L'\\';
				szRxPattern[nRxIndex++] = L'}';
			}
			continue;
		}

		// Range start / end
		if (c == L'[')
		{
			ExitOnNull(!bInRange, hr, E_INVALIDARG, "Invalid glob expression: Range start before previous range ended. Pattern '%ls', index %i", szFilter, i);
			bInRange = true;
			szRxPattern[nRxIndex++] = L'[';
			if (szFilter[i + 1] == L'!')
			{
				++i;
				szRxPattern[nRxIndex++] = L'^';
			}
			continue;
		}
		if (c == L']')
		{
			if (bInRange)
			{
				bInRange = false;
				szRxPattern[nRxIndex++] = L']';
			}
			else
			{
				szRxPattern[nRxIndex++] = L'\\';
				szRxPattern[nRxIndex++] = L']';
			}
			continue;
		}

		if (c == L'?')
		{
			hr = StringCchCat(szRxPattern, nRxStrLen, szQuestionMark);
			ExitOnFailure(hr, "Failed to concatentate strings");
			continue;
		}

		// Regex special charactres
		if ((c == L'.') || (c == L'(') || (c == L')') || (c == L'^') || (c == L'$'))
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
			// '**/MyFile.txt' should match both in the root folder and in subfolders, so the slash is optional.
			//TODO Problem: It would match for 'NotMyFile.txt'
			hr = StringCchCat(szRxPattern, nRxStrLen, szGlobstar);
			ExitOnFailure(hr, "Failed to concatentate strings");

			// Skip excessive slashes
			while (szFilter[i + 1] == L'/')
			{
				++i;
			}
			continue;
		}
		else // *
		{
			hr = StringCchCat(szRxPattern, nRxStrLen, szAsterisk);
			ExitOnFailure(hr, "Failed to concatentate strings");
			continue;
		}
	}

	// Remove trailing slashes (i.e. '**/')
	StrTrim(szRxPattern, L"\\/");

	hr = StringCchCat(szRxPattern, nRxStrLen, L"$");
	ExitOnFailure(hr, "Failed to concatentate strings");
	

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

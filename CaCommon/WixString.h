#pragma once

#include "stdafx.h"
#include <strutil.h>

class CWixString
{
public:
	CWixString()
		: _pS(NULL)
		, _dwCapacity(0)
		, _pTokenContext( NULL)
	{

	}

	CWixString(const WCHAR *pS)
		: _pS(NULL)
		, _dwCapacity(0)
		, _pTokenContext( NULL)
	{
		Copy(pS);
	}

	CWixString(DWORD dwSize)
		: _pS(NULL)
		, _dwCapacity(0)
		, _pTokenContext( NULL)
	{
		Allocate(dwSize);
	}

	~CWixString()
	{
		Release();
	}

#pragma region Copy C-tor, Assignment operators

	CWixString(const CWixString& other)
	{
		Copy((LPCWSTR)other);
	}

	CWixString& operator=(const CWixString& other)
	{
		Copy((LPCWSTR)other);
		return *this;
	}

	CWixString& operator=(LPCWSTR szOther)
	{
		Copy(szOther);
		return *this;
	}

#pragma endregion

	HRESULT Allocate(DWORD dwSize)
	{
		HRESULT hr = S_OK;

		if (dwSize > Capacity())
		{
			hr = Release();
			BreakExitOnFailure(hr, "Failed to free memory");

			hr = StrAlloc(&_pS, dwSize);
			BreakExitOnFailure(hr, "Failed to allocate memory");

			_dwCapacity = dwSize;
		}

	LExit:
		return hr;
	}

	HRESULT SecureRelease()
	{
		HRESULT hr = S_OK;

		if (_pS != NULL)
		{
			SecureZeroMemory(_pS, _dwCapacity);
			hr = Release();
		}

	LExit:
		return hr;
	}

	HRESULT Release()
	{
		HRESULT hr = S_OK;

		if (_pS != NULL)
		{
			hr = StrFree(_pS);
			BreakExitOnFailure(hr, "Failed to free memory");

			_pS = NULL;
			_dwCapacity = 0;
		}

	LExit:
		return hr;
	}

	HRESULT Copy(const WCHAR* pS, DWORD dwMax = INFINITE - 1)
	{
		DWORD dwSize = 0;
		HRESULT hr = S_OK;
		errno_t err = ERROR_SUCCESS;

		if ((pS == NULL) || (*pS == NULL))
		{
			hr = Release();
			BreakExitOnFailure(hr, "Failed to release string");
			ExitFunction();
		}

		dwSize = ::wcslen(pS);
		if (dwMax <= dwSize)
		{
			dwSize = dwMax;
		}

		hr = Allocate(dwSize + 1);
		BreakExitOnFailure(hr, "Failed to allocate memory");

		// Copying 
		err = ::wcsncpy_s(_pS, _dwCapacity, pS, dwSize);
		BreakExitOnWin32Error(hr, err, "Failed to copy string");

	LExit:
		return hr;
	}

	HRESULT ReplaceAll(LPCWSTR from, LPCWSTR to)
	{
		HRESULT hr = S_OK;

		hr = StrReplaceStringAll(&_pS, from, to);
		BreakExitOnFailure(hr, "Failed to replace in string");

	LExit:
		return hr;
	}

	HRESULT Format(LPCWSTR stFormat, ...)
	{
		HRESULT hr = S_OK;
		va_list va;
		va_start(va, stFormat);

		size_t sSize = ::_vscwprintf(stFormat, va);
		if (sSize == 0)
		{
			hr = Release();
			ExitFunction();
		}

		++sSize;
		hr = Allocate(sSize);
		BreakExitOnFailure(hr, "Failed allocating memory");

		vswprintf_s(_pS, Capacity(), stFormat, va);

	LExit:

		va_end(va);
		return hr;
	}

	// Expand MSI-formatted string.
	// stFormat: MSI format string
	// szObfuscated: If not NULL, will receive the expanded string with hidden properties obfuscated.
	HRESULT MsiFormat(LPCWSTR stFormat, LPWSTR *pszObfuscated = nullptr)
	{
		HRESULT hr = S_OK;
		LPWSTR szNew = nullptr;
		LPWSTR szStripped = nullptr;
		LPWSTR szObfuscated = nullptr;
		LPWSTR szMsiHiddenProperties = nullptr;
		LPWSTR szHideMe = nullptr;

		if (pszObfuscated)
		{
			hr = StrAllocString(&szStripped, stFormat, 0);
			if (SUCCEEDED(hr))
			{
				hr = WcaGetProperty(L"MsiHiddenProperties", &szMsiHiddenProperties);
				if (SUCCEEDED(hr) && szMsiHiddenProperties && *szMsiHiddenProperties)
				{
					for (LPWSTR szContext = nullptr, szProp = ::wcstok_s(szMsiHiddenProperties, L";", &szContext); SUCCEEDED(hr) && szProp; szProp = ::wcstok_s(nullptr, L";", &szContext))
					{
						if (szProp && *szProp)
						{
							hr = StrAllocFormatted(&szHideMe, L"[%s]", szProp);
							if (SUCCEEDED(hr))
							{
								hr = StrReplaceStringAll(&szStripped, szHideMe, L"******");

								StrFree(szHideMe);
								szHideMe = nullptr;
							}
						}
					}
				}

				if (SUCCEEDED(hr))
				{
					hr = WcaGetFormattedString(szStripped, &szObfuscated);
					if (SUCCEEDED(hr))
					{
						hr = StrReplaceStringAll(&szObfuscated, L"[", L"[\\[]"); // Since obfuscated is re-formatted on logging, we want to re-escape '['.
						if (SUCCEEDED(hr))
						{
							*pszObfuscated = szObfuscated;
							szObfuscated = nullptr;
						}
					}
				}
			}
		}

		hr = WcaGetFormattedString(stFormat, &szNew);
		if (SUCCEEDED(hr))
		{
			Release();
			_pS = szNew;
			_dwCapacity = 1 + ::wcslen(szNew);

			szNew = nullptr;
		}

		// Nothing needed obfuscation (MsiHiddenProperties was empty)?
		if (pszObfuscated && !*pszObfuscated)
		{
			StrAllocString(pszObfuscated, _pS, 0);
		}

		if (szNew)
		{
			StrFree(szNew);
		}
		if (szStripped)
		{
			StrFree(szStripped);
		}
		if (szObfuscated)
		{
			StrFree(szObfuscated);
		}
		if (szMsiHiddenProperties)
		{
			StrFree(szMsiHiddenProperties);
		}
		if (szHideMe)
		{
			StrFree(szHideMe);
		}

		return hr;
	}

	HRESULT AppnedFormat(LPCWSTR szFormat, ...)
	{
		HRESULT hr = S_OK;
		va_list va;
		LPWSTR szAppend = nullptr;
		DWORD dwCapacity = 0;
		va_start(va, szFormat);

		hr = StrAllocFormattedArgs(&szAppend, szFormat, va);
		ExitOnFailure(hr, "Failed formatting string");

		hr = StrAllocConcat(&_pS, szAppend, 0);
		ExitOnFailure(hr, "Failed appenfing string");

		dwCapacity = 1 + ::wcslen(_pS);
		if (dwCapacity > _dwCapacity)
		{
			_dwCapacity = dwCapacity;
		}

	LExit:
		if (FAILED(hr))
		{
			Release();
		}

		va_end(va);
		ReleaseStr(szAppend);
		return hr;
	}

	HRESULT ToAnsiString(LPSTR *pszStr)
	{
		HRESULT hr = StrAnsiAllocFormatted(pszStr, "%ls", _pS);
		return hr;
	}

	#pragma region Tokenize
	
	HRESULT Tokenize(LPCWSTR delimiters, LPCWSTR* firstToken)
	{
		_pTokenContext = NULL;
		(*firstToken) = ::wcstok_s(_pS, delimiters, &_pTokenContext);
	
		return ((*firstToken) == NULL) ? E_NOMOREITEMS : S_OK;
	}
	
	HRESULT NextToken(LPCWSTR delimiters, LPCWSTR* nextToken)
	{
		(*nextToken) = ::wcstok_s(NULL, delimiters, &_pTokenContext);
	
		return ((*nextToken) == NULL) ? E_NOMOREITEMS : S_OK;
	}
	
	#pragma endregion	

	DWORD Capacity() const
	{
		if (_dwCapacity > 0)
		{
			return _dwCapacity;
		}

		if (_pS != NULL)
		{
			return (1 + ::wcslen(_pS));
		}

		return 0;
	}

	DWORD StrLen() const
	{
		if (_pS != NULL)
		{
			return ::wcslen(_pS);
		}

		return 0;
	}

	operator const WCHAR*() const
	{
		return _pS;
	}

	operator WCHAR*()
	{
		return _pS;
	}

	operator WCHAR**()
	{
		Release();

		return &_pS;
	}

	BOOL IsNullOrEmpty() const
	{
		return ((_pS == NULL) || (*_pS == NULL));
	}

	BOOL operator==(const DWORD_PTR dw) const
	{
		return (_pS == (WCHAR*)dw);
	}

	BOOL Equals(LPCWSTR szOther)
	{
		if (IsNullOrEmpty())
		{
			return FALSE;
		}

		return (::wcscmp(_pS, szOther) == 0);
	}

	BOOL EqualsIgnoreCase(LPCWSTR szOther)
	{
		if (IsNullOrEmpty())
		{
			return FALSE;
		}

		return (::_wcsicmp(_pS, szOther) == 0);
	}

	DWORD Count(WCHAR c) const
	{
		DWORD cnt = 0;
		LPCWSTR pos = _pS;

		if (IsNullOrEmpty())
		{
			return cnt;
		}

		while (pos = ::wcschr(pos, c))
		{
			++cnt;
		}

		return cnt;
	}

	DWORD Find(LPCWSTR szOther) const
	{
		LPCWSTR pOther = NULL;

		if (IsNullOrEmpty())
		{
			return INFINITE;
		}

		pOther = ::wcsstr(_pS, szOther);
		if (pOther == NULL)
		{
			return INFINITE;
		}

		return (pOther - _pS);
	}

	DWORD Find(WCHAR cOther) const
	{
		LPCWSTR pOther = NULL;

		if (IsNullOrEmpty())
		{
			return INFINITE;
		}

		pOther = ::wcschr(_pS, cOther);
		if (pOther == NULL)
		{
			return INFINITE;
		}

		return (pOther - _pS);
	}

	DWORD RFind(WCHAR cOther) const
	{
		LPCWSTR pOther = NULL;

		if (IsNullOrEmpty())
		{
			return INFINITE;
		}

		pOther = ::wcsrchr(_pS, cOther);
		if (pOther == NULL)
		{
			return INFINITE;
		}

		return (pOther - _pS);
	}

	DWORD FindIgnoreCase(LPCWSTR szOther) const
	{
		LPCWSTR pOther = NULL;

		if (IsNullOrEmpty())
		{
			return INFINITE;
		}

		LPCWSTR wzSource = _pS;
		LPCWSTR wzSearch = NULL;
		DWORD_PTR cchSourceIndex = 0;

		// Walk through wzString (the source string) one character at a time
		while (*wzSource)
		{
			cchSourceIndex = 0;
			wzSearch = szOther;

			// Look ahead in the source string until we get a full match or we hit the end of the source
			while ((NULL != wzSource[cchSourceIndex]) && (NULL != *wzSearch) && (::towlower(wzSource[cchSourceIndex]) == towlower(*wzSearch)))
			{
				++cchSourceIndex;
				++wzSearch;
			}

			// If we found it, return the point that we found it at
			if (NULL == *wzSearch)
			{
				return (wzSource - _pS);
			}

			// Walk ahead one character
			++wzSource;
		}

		return (pOther - _pS);
	}

private:

	WCHAR *_pS;
	WCHAR *_pTokenContext;
	DWORD _dwCapacity;
};

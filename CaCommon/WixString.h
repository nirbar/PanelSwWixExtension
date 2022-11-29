#pragma once

#include "stdafx.h"
#include <strutil.h>
#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")

class CWixString
{
public:
	CWixString()
	{

	}

	CWixString(LPCWSTR pS)
	{
		Copy(pS);
	}

	CWixString(DWORD dwSize)
	{
		Allocate(dwSize);
	}

	~CWixString()
	{
		Release();
	}

#pragma region Copy, move c-tors, assignment operators

	CWixString(const CWixString& other)
	{
		Copy((LPCWSTR)other);
	}

	CWixString(CWixString&& other)
	{
		_dwCapacity = other._dwCapacity;
		_pTokenContext = other._pTokenContext;
		_szObfuscated = other._szObfuscated;
		_pS = other.Detach();
	}

	CWixString& operator=(CWixString&& other)
	{
		_dwCapacity = other._dwCapacity;
		_pTokenContext = other._pTokenContext;
		_szObfuscated = other._szObfuscated;
		_pS = other.Detach();
		return *this;
	}

	CWixString& operator=(const CWixString& other)
	{
		Copy(other);
		return *this;
	}

	CWixString& operator=(LPCWSTR szOther)
	{
		Copy(szOther);
		return *this;
	}

	WCHAR& operator[](size_t i)
	{
		static WCHAR errChr = NULL;
		if ((i < 0) || (i >= Capacity()))
		{
			return errChr;
		}

		return _pS[i];
	}

#pragma endregion

	HRESULT Allocate(DWORD dwSize)
	{
		HRESULT hr = S_OK;

		Release();

		hr = StrAlloc(&_pS, dwSize);
		ExitOnFailure(hr, "Failed to allocate memory");

		_dwCapacity = dwSize;

	LExit:
		return hr;
	}

	HRESULT SecureRelease()
	{
		if (_pS != nullptr)
		{
			SecureZeroMemory(_pS, Capacity());
		}

		return Release();
	}

	HRESULT Release()
	{
		HRESULT hr = S_OK;

		if (_szObfuscated)
		{
			StrFree(_szObfuscated);
			_szObfuscated = nullptr;
		}

		if (_pS != nullptr)
		{
			hr = StrFree(_pS);
			_pS = nullptr;
			_dwCapacity = 0;
			_pTokenContext = nullptr;

			ExitOnFailure(hr, "Failed to free memory");
		}

	LExit:
		return hr;
	}

	LPWSTR Detach()
	{
		LPWSTR pS = _pS;
		_pS = nullptr;
		_dwCapacity = 0;
		_pTokenContext = nullptr;
		_szObfuscated = nullptr;
		return pS;
	}

	HRESULT Copy(const CWixString &other)
	{
		HRESULT hr = S_OK;

		Release();

		hr = Copy(other._pS);
		ExitOnFailure(hr, "Failed to copy string");

		if (other._szObfuscated)
		{
			hr = StrAllocString(&_szObfuscated, other._szObfuscated, NULL);
			ExitOnFailure(hr, "Failed to copy string");
		}

	LExit:
		return hr;
	}

	HRESULT Copy(LPCWSTR pS, DWORD dwMax = INFINITE - 1)
	{
		DWORD dwSize = 0;
		HRESULT hr = S_OK;
		errno_t err = ERROR_SUCCESS;

		Release();

		if (pS == nullptr)
		{
			ExitFunction();
		}

		dwSize = ::wcslen(pS);
		if (dwMax <= dwSize)
		{
			dwSize = dwMax;
		}

		hr = Allocate(dwSize + 1);
		ExitOnFailure(hr, "Failed to allocate memory");

		// Copying 
		err = ::wcsncpy_s(_pS, _dwCapacity, pS, dwSize);
		ExitOnWin32Error(hr, err, "Failed to copy string");

	LExit:
		return hr;
	}

	HRESULT ReplaceAll(LPCWSTR from, LPCWSTR to)
	{
		HRESULT hr = S_OK;

		ExitOnNull(_pS, hr, E_INVALIDSTATE, "Can't replace in null string");

		hr = StrReplaceStringAll(&_pS, from, to);
		if (FAILED(hr))
		{
			Release();
		}
		ExitOnFailure(hr, "Failed to replace in string");

	LExit:
		return hr;
	}

	HRESULT Format(LPCWSTR stFormat, ...)
	{
		HRESULT hr = S_OK;
		va_list va;
		va_start(va, stFormat);

		Release();

		size_t sSize = ::_vscwprintf(stFormat, va);
		if (sSize == 0)
		{
			ExitFunction();
		}

		++sSize;
		hr = Allocate(sSize);
		ExitOnFailure(hr, "Failed allocating memory");

		vswprintf_s(_pS, Capacity(), stFormat, va);

	LExit:

		va_end(va);
		return hr;
	}

	LPCWSTR Obfuscated() const
	{
		return _szObfuscated ? _szObfuscated : _pS;
	}

	// Expand MSI-formatted string.
	// szFormat: MSI format string
	// pszObfuscated: Optional pointer to a string that will contain the obfuscated formatted string
	HRESULT MsiFormat(LPCWSTR szFormat, LPWSTR* pszObfuscated = nullptr)
	{
		HRESULT hr = S_OK;
		LPWSTR szNew = nullptr;
		LPWSTR szStripped = nullptr;
		LPWSTR szObfuscated = nullptr;
		LPWSTR szMsiHiddenProperties = nullptr;
		LPWSTR szHideMe = nullptr;
		
		Release();

		hr = StrAllocString(&szStripped, szFormat, 0);
		if (SUCCEEDED(hr))
		{
			hr = WcaGetProperty(L"MsiHiddenProperties", &szMsiHiddenProperties);
			if (SUCCEEDED(hr) && szMsiHiddenProperties && *szMsiHiddenProperties)
			{
				for (LPWSTR szContext = nullptr, szProp = ::wcstok_s(szMsiHiddenProperties, L";", &szContext); SUCCEEDED(hr) && szProp; szProp = ::wcstok_s(nullptr, L";", &szContext))
				{
					if (szProp && *szProp)
					{
						hr = StrAllocFormatted(&szHideMe, L"[%ls]", szProp);
						if (SUCCEEDED(hr))
						{
							hr = StrReplaceStringAll(&szStripped, szHideMe, L"******");
							if (FAILED(hr))
							{
								break;
							}

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
						_szObfuscated = szObfuscated;
						szObfuscated = nullptr;

						if (pszObfuscated)
						{
							StrAllocString(pszObfuscated, _szObfuscated, NULL);
						}
					}
				}
			}
		}

		hr = WcaGetFormattedString(szFormat, &szNew);
		if (SUCCEEDED(hr))
		{
			if (_pS)
			{
				StrFree(_pS);
			}
			_pS = szNew;
			_dwCapacity = 1 + ::wcslen(szNew);
			_pTokenContext = nullptr;
			szNew = nullptr;
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
		ExitOnFailure(hr, "Failed appending string");

		dwCapacity = 1 + ::wcslen(_pS);
		if (dwCapacity > _dwCapacity)
		{
			_dwCapacity = dwCapacity;
		}

		if (_szObfuscated)
		{
			hr = StrAllocConcat(&_szObfuscated, szAppend, 0);
			ExitOnFailure(hr, "Failed appending string");
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

	HRESULT ToAnsiString(LPSTR* pszStr)
	{
		HRESULT hr = StrAnsiAllocFormatted(pszStr, "%ls", _pS);
		return hr;
	}

#pragma region Tokenize

	HRESULT Tokenize(LPCWSTR delimiters, LPCWSTR* firstToken)
	{
		_pTokenContext = nullptr;
		(*firstToken) = ::wcstok_s(_pS, delimiters, &_pTokenContext);

		return ((*firstToken) == nullptr) ? E_NOMOREITEMS : S_OK;
	}

	HRESULT NextToken(LPCWSTR delimiters, LPCWSTR* nextToken)
	{
		(*nextToken) = ::wcstok_s(nullptr, delimiters, &_pTokenContext);

		return ((*nextToken) == nullptr) ? E_NOMOREITEMS : S_OK;
	}

#pragma endregion	

	DWORD Capacity() const
	{
		if (_dwCapacity > 0)
		{
			return _dwCapacity;
		}

		if (_pS != nullptr)
		{
			return (1 + ::wcslen(_pS));
		}

		return 0;
	}

	size_t StrLen() const
	{
		if (_pS != nullptr)
		{
			return ::wcslen(_pS);
		}

		return 0;
	}

	operator const WCHAR* () const
	{
		return _pS;
	}

	operator WCHAR* ()
	{
		return _pS;
	}

	operator WCHAR** ()
	{
		Release();

		return &_pS;
	}

	BOOL IsNullOrEmpty() const
	{
		return ((_pS == nullptr) || (*_pS == NULL));
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
		LPCWSTR pOther = nullptr;

		if (IsNullOrEmpty())
		{
			return INFINITE;
		}

		pOther = ::wcsstr(_pS, szOther);
		if (pOther == nullptr)
		{
			return INFINITE;
		}

		return (pOther - _pS);
	}

	DWORD Find(WCHAR cOther) const
	{
		LPCWSTR pOther = nullptr;

		if (IsNullOrEmpty())
		{
			return INFINITE;
		}

		pOther = ::wcschr(_pS, cOther);
		if (pOther == nullptr)
		{
			return INFINITE;
		}

		return (pOther - _pS);
	}

	DWORD RFind(WCHAR cOther) const
	{
		LPCWSTR pOther = nullptr;

		if (IsNullOrEmpty())
		{
			return INFINITE;
		}

		pOther = ::wcsrchr(_pS, cOther);
		if (pOther == nullptr)
		{
			return INFINITE;
		}

		return (pOther - _pS);
	}

	DWORD FindIgnoreCase(LPCWSTR szOther) const
	{
		if (IsNullOrEmpty() || !szOther || !*szOther)
		{
			return INFINITE;
		}

		LPCWSTR szOtherFound = ::StrStrIW(_pS, szOther);
		return szOtherFound ? (szOtherFound - szOther) : INFINITE;
	}

private:

	LPWSTR _pS = nullptr;
	LPWSTR _szObfuscated = nullptr;
	LPWSTR _pTokenContext = nullptr;
	DWORD _dwCapacity = 0;
};

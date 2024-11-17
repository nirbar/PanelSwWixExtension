#pragma once

#include "pch.h"
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

	CWixString(CWixString&& other) noexcept
	{
		_dwCapacity = other._dwCapacity;
		_pTokenContext = other._pTokenContext;
		_szObfuscated = other._szObfuscated;
		_pS = other.Detach();
	}

	CWixString& operator=(CWixString&& other) noexcept
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

		if ((_dwCapacity >= dwSize) || (StrLen() >= dwSize))
		{
			ExitFunction();
		}

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
		}

		if (_pS)
		{
			hr = StrFree(_pS);
			ExitOnFailure(hr, "Failed to free memory");
		}

	LExit:
		_pS = nullptr;
		_szObfuscated = nullptr;
		_dwCapacity = 0;
		_pTokenContext = nullptr;
		return hr;
	}

	void Attach(LPWSTR szPath)
	{
		Release();

		_pS = szPath;
		if (_pS && *_pS)
		{
			_dwCapacity = wcslen(_pS) + 1;
		}
	}

	LPWSTR Detach()
	{
		LPWSTR pS = _pS;
		_pS = nullptr;
		Release();

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

	HRESULT Copy(LPCWSTR pS, DWORD dwMax = INFINITE)
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

	HRESULT Substring(DWORD dwStart, DWORD dwLength = INFINITE)
	{
		HRESULT hr = S_OK;
		LPWSTR szOld = nullptr;

		ExitOnNull((dwStart < StrLen()), hr, E_INVALIDSTATE, "Substring start index is out of range");

		szOld = Detach();
		
		hr = Copy(szOld + dwStart, dwLength);
		ExitOnFailure(hr, "Failed to copy string");

	LExit:
		ReleaseStr(szOld);
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
					_szObfuscated = szObfuscated;
					szObfuscated = nullptr;

					if (pszObfuscated)
					{
						StrAllocString(pszObfuscated, _szObfuscated, NULL);
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

	// Remove characters from the left
	void RemoveLeft(DWORD dwCount)
	{
		DWORD dwNewLen = 0;
		DWORD i = 0;

		if (dwCount >= StrLen())
		{
			Release();
			return;
		}

		dwNewLen = StrLen() - dwCount;
		for (i = 0; i < dwNewLen; ++i)
		{
			_pS[i] = _pS[i + dwCount];
		}
		_pS[i] = NULL;
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

	HRESULT MultiStringInsertString(LPCWSTR sz, DWORD dwIndex = INFINITE)
	{
		HRESULT hr = S_OK;
		DWORD i = 0;
		DWORD dwPos = 0;
		DWORD dwSzLen = 0;
		DWORD dwOldTotalCapacity = 0;
		DWORD dwNeededCapacity = 0;
		LPWSTR szNew = nullptr;

		if (!_pS)
		{
			hr = Format(L"%ls%lc", sz, L'\0');
			ExitOnFailure(hr, "Failed to allocate string");
			ExitFunction();
		}

		dwOldTotalCapacity = MultiStringTotalLen();
		while ((i < dwIndex) && (dwPos < dwOldTotalCapacity) && _pS[dwPos])
		{
			LPCWSTR szCurr = _pS + dwPos;
			DWORD dwLen = wcslen(szCurr);
			dwPos += dwLen + 1;
			++i;
		}
		if (dwPos > dwOldTotalCapacity)
		{
			dwPos = dwOldTotalCapacity;
		}

		dwSzLen = wcslen(sz) + 1;
		dwNeededCapacity = dwOldTotalCapacity + dwSzLen;

		if (dwNeededCapacity > _dwCapacity)
		{
			LPWSTR szTemp = _pS;
			hr = StrAlloc(&szNew, dwNeededCapacity);
			ExitOnFailure(hr, "Failed to allocate memory");

			memcpy(szNew, _pS, dwOldTotalCapacity * sizeof(WCHAR));
			_pS = szNew;
			_dwCapacity = dwNeededCapacity;
			szNew = szTemp;
		}

		// If we're inserting in the middle, make room
		if (dwPos < dwOldTotalCapacity)
		{
			for (DWORD j = 0, dwMoveLen = dwOldTotalCapacity - dwPos; j < dwMoveLen; ++j)
			{
				_pS[dwNeededCapacity - j - 1] = _pS[dwOldTotalCapacity - j - 1];
			}
		}

		memcpy(_pS + dwPos, sz, dwSzLen * sizeof(WCHAR));

	LExit:
		ReleaseStr(szNew);

		return hr;
	}

	HRESULT MultiStringReplaceString(DWORD i, LPCWSTR sz)
	{
		HRESULT hr = S_OK;
		LPCWSTR szOld = nullptr;
		DWORD dwOldSzStart = 0;
		DWORD dwOldSzEnd = 0;
		DWORD dwOldSzLen = 0;
		DWORD dwAfterOldSzLen = 0;
		DWORD dwNewSzLen = 0;
		DWORD dwTotalLen = 0;
		DWORD dwNeededCapacity = 0;
		LPWSTR szNew = nullptr;

		hr = MultiStringGet(i, &szOld);
		ExitOnNull(szOld, hr, E_INVALIDARG, "Multi string index %u is out of range", i);

		dwOldSzStart = szOld - _pS;
		dwOldSzLen = wcslen(szOld) + 1;
		dwOldSzEnd = dwOldSzStart + dwOldSzLen;
		dwNewSzLen = wcslen(sz) + 1;

		dwTotalLen = MultiStringTotalLen();
		dwNeededCapacity = dwTotalLen + dwNewSzLen - dwOldSzLen;
		dwAfterOldSzLen = dwTotalLen - dwOldSzLen - dwOldSzStart;

		if (dwNeededCapacity > _dwCapacity)
		{
			LPWSTR szTemp = _pS;
			hr = StrAlloc(&szNew, dwNeededCapacity);
			ExitOnFailure(hr, "Failed to allocate memory");

			memcpy(szNew, _pS, dwTotalLen * sizeof(WCHAR));
			_pS = szNew;
			_dwCapacity = dwNeededCapacity;
			szNew = szTemp;
		}

		// If we have sufficient space, just copy
		if (dwOldSzLen >= dwNewSzLen)
		{
			memcpy(_pS + dwOldSzStart, sz, dwNewSzLen * sizeof(WCHAR));

			for (DWORD j = 0; j < dwAfterOldSzLen; ++j)
			{
				DWORD dwNewPos = dwOldSzStart + dwNewSzLen + j;
				DWORD dwOldPos = dwOldSzEnd + j;

				_pS[dwNewPos] = _pS[dwOldPos];
			}
		}
		// Insufficient space, need to use a double buffer
		else
		{
			hr = StrAlloc(&szNew, dwAfterOldSzLen);
			ExitOnFailure(hr, "Failed to allocate memory");

			memcpy(szNew, _pS + dwOldSzEnd, dwAfterOldSzLen * sizeof(WCHAR));
			memcpy(_pS + dwOldSzStart, sz, dwNewSzLen * sizeof(WCHAR));
			memcpy(_pS + dwOldSzStart + dwNewSzLen, szNew, dwAfterOldSzLen * sizeof(WCHAR));
		}

	LExit:
		ReleaseStr(szNew);

		return hr;
	}

	// Get string number i, or the last one
	HRESULT MultiStringGet(DWORD i, LPCWSTR* pSz)
	{
		HRESULT hr = S_OK;

		LPCWSTR sz = _pS;
		DWORD j = 0;
		for (; (j < i) && sz && *sz; sz += 1 + wcslen(sz))
		{
			++j;
		}
		if ((j != i) || !sz || !*sz)
		{
			*pSz = nullptr;
			hr = E_NOMOREITEMS;
			ExitFunction();
		}
		*pSz = sz;

	LExit:
		return hr;
	}

	// Length, including the terminating double-null
	DWORD MultiStringTotalLen()
	{
		LPCWSTR sz = nullptr;

		for (sz = _pS; sz && *sz; sz += wcslen(sz) + 1)
		{
		}
		return sz ? (sz - _pS) + 1 : 0;
	}

private:

	LPWSTR _pS = nullptr;
	LPWSTR _szObfuscated = nullptr;
	LPWSTR _pTokenContext = nullptr;
	DWORD _dwCapacity = 0;
};

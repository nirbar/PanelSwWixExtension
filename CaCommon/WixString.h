#pragma once

#include "stdafx.h"
#include <strutil.h>
#include <varargs.h>

class CWixString
{
public:
	CWixString()
		: _pS(NULL)
		, _sSize(0)
	{

	}

	CWixString(const WCHAR *pS)
		: _pS(NULL)
		, _sSize(0)
	{
		Copy(pS);
	}

	CWixString(size_t sSize)
		: _pS(NULL)
		, _sSize(0)
	{
		Allocate(sSize);
	}

	~CWixString()
	{
		Release();
	}

	HRESULT Allocate(size_t sSize)
	{
		HRESULT hr = S_OK;

		if (sSize > Capacity())
		{
			hr = Release();
			BreakExitOnFailure(hr, "Failed to free memory");

			hr = StrAlloc(&_pS, sSize);
			BreakExitOnFailure(hr, "Failed to allocate memory");

			_sSize = sSize;
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
			_sSize = 0;
		}

	LExit:
		return hr;
	}

	HRESULT Copy(const WCHAR* pS)
	{
		size_t sSize = 0;
		HRESULT hr = S_OK;
		errno_t err = ERROR_SUCCESS;

		if ((pS == NULL) || (*pS == NULL))
		{
			hr = Release();
			BreakExitOnFailure(hr, "Failed to release string");
			ExitFunction();
		}

		sSize = wcslen(pS) + 1;
		hr = Allocate(sSize);
		BreakExitOnFailure(hr, "Failed to allocate memory");

		err = ::wcscpy_s(_pS, _sSize, pS);
		BreakExitOnWin32Error(hr, err, "Failed to copy string");

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

		_vswprintf(_pS, stFormat, va);

	LExit:

		va_end(va);
		return hr;
	}

	size_t StrLength() const
	{
		if (IsNullOrEmpty())
		{
			return 0;
		}

		return wcslen(_pS);
	}

	size_t Capacity() const
	{
		if (_sSize > 0)
		{
			return _sSize;
		}

		if (_pS != NULL)
		{
			return ::wcslen(_pS);
		}

		return 0;
	}

	operator const WCHAR*()
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

	WCHAR* operator=(const WCHAR* pS)
	{
		Copy(pS);

		return _pS;
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

private:

	WCHAR *_pS;
	size_t _sSize;
};

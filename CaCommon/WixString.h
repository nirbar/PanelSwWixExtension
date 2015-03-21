#pragma once

#include "stdafx.h"
#include <strutil.h>

class CWixString
{
public:
	CWixString()
		: _pS(NULL)
		, _dwCapacity(0)
	{

	}

	CWixString(const WCHAR *pS)
		: _pS(NULL)
		, _dwCapacity(0)
	{
		Copy(pS);
	}

	CWixString(DWORD dwSize)
		: _pS(NULL)
		, _dwCapacity(0)
	{
		Allocate(dwSize);
	}

	~CWixString()
	{
		Release();
	}

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

	HRESULT Copy(const WCHAR* pS)
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

		dwSize = wcslen(pS) + 1;
		hr = Allocate(dwSize);
		BreakExitOnFailure(hr, "Failed to allocate memory");

		err = ::wcscpy_s(_pS, _dwCapacity, pS);
		BreakExitOnWin32Error(hr, err, "Failed to copy string");

	LExit:
		return hr;
	}

	DWORD Capacity() const
	{
		if (_dwCapacity > 0)
		{
			return _dwCapacity;
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
	DWORD _dwCapacity;
};

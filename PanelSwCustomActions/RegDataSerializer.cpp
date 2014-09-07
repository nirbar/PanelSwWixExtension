#include "stdafx.h"
#include "RegDataSerializer.h"
#include <memory>
#include <strutil.h>

#define WI_MULTISTRING_NULL L"[~]"
#define WI_MULTISTRING_NULL_SIZE 3

CRegDataSerializer::CRegDataSerializer()
	:_bytes( NULL)
	, _size( 0)
	, _bufSize( 0)
	, _dataType( 0)
{
}

CRegDataSerializer::~CRegDataSerializer()
{
	Release();
}

HRESULT CRegDataSerializer::Release()
{
	if (_bytes != NULL)
	{
		free(_bytes);

		_bytes = NULL;
	}

	_size = 0;
	_bufSize = 0;
	_dataType = 0;

	return S_OK;
}

HRESULT CRegDataSerializer::Allocate(DWORD size)
{
	HRESULT hr = S_OK;

	_size = 0;
	if (_bufSize >= size)
	{
		_size = size;
		ExitFunction();
	}

	_bufSize = 0;
	_bytes = (BYTE*)realloc(_bytes, size);
	if (_bytes == NULL)
	{
		hr = E_INSUFFICIENT_BUFFER;
		ExitOnFailure(hr, "Failed to allocate buffer");
	}

	_size = size;
	_bufSize = size;

LExit:
	return hr;
}

HRESULT CRegDataSerializer::Set(const BYTE* pData, DWORD dwDataType, DWORD dwSize)
{
	HRESULT hr = S_OK;
	errno_t err;

	hr = Allocate(dwSize);
	ExitOnFailure(hr, "Failed to allocate buffer");

	err = memcpy_s(_bytes, _bufSize, pData, dwSize);
	hr = HRESULT_FROM_WIN32(err);
	ExitOnFailure(hr, "Failed to copy memory");

	_size = dwSize;
	_dataType = dwDataType;

LExit:
	return hr;
}

HRESULT CRegDataSerializer::Serialize(LPWSTR* ppDst) const
{
	HRESULT hr = S_OK;
	errno_t err;
	DWORD dwStrSize;
	int n;

	switch (_dataType)
	{
	case REG_MULTI_SZ:		
		hr = SerializeMultiString(ppDst);
		break;

	case REG_DWORD:
		dwStrSize = 10;
		hr = StrAlloc(ppDst, dwStrSize + 1);
		ExitOnFailure(hr, "Failed to allocate string");
		
		n = wsprintfW((*ppDst), L"0x%08X", (DWORD)(*_bytes));
		if (n != dwStrSize)
		{
			hr = E_INVALIDDATA;
			ExitOnFailure(hr, "Failed to serialize DWORD");
		}
		break;

	case REG_QWORD:
		dwStrSize = 18;
		hr = StrAlloc(ppDst, dwStrSize + 1);
		ExitOnFailure(hr, "Failed to allocate string");
		
		n = wsprintfW((*ppDst), L"0x%016X", (ULONGLONG)(*_bytes));
		if (n != dwStrSize)
		{
			hr = E_INVALIDDATA;
			ExitOnFailure(hr, "Failed to serialize QWORD");
		}
		break;

	case REG_SZ:
	case REG_EXPAND_SZ:
		dwStrSize = wcslen((LPCWSTR)_bytes);
		hr = StrAlloc(ppDst, dwStrSize + 1);
		ExitOnFailure(hr, "Failed to allocate string");

		err = wcscpy_s((*ppDst), dwStrSize + 1, (LPCWSTR)_bytes);
		hr = HRESULT_FROM_WIN32(err);
		ExitOnFailure(hr, "Failed to copy string");

		break;

	case REG_BINARY:
		dwStrSize = _size * 2;
		hr = StrAlloc(ppDst, dwStrSize + 1);
		ExitOnFailure(hr, "Failed to allocate buffer");

		for (DWORD i = 0; i < _size; ++i)
		{
			n = wsprintf(((*ppDst) + (i * 2)), L"%02X", _bytes[i]);
			if ( n != 2)
			{
				hr = E_INVALIDDATA;
				ExitOnFailure(hr, "Failed to serialize BINARY");
			}
		}
		
		(*ppDst)[dwStrSize] = NULL;
		break;

	default:
		hr = E_INVALIDSTATE;
		ExitOnFailure(hr, "Unsupported registry data type");
	}

LExit:
	return hr;
}

HRESULT CRegDataSerializer::SerializeMultiString(LPWSTR* pDst) const
{
	HRESULT hr = S_OK;
	errno_t err;
	LPWSTR pCurrDst = NULL;
	LPCWSTR pCurrSrc = (LPWSTR)_bytes;
	DWORD dwWordSize = 0;
	DWORD dwStrCount = 0;
	DWORD dwSize;

	while ((LPWSTR)_bytes[dwWordSize] != NULL)
	{
		dwWordSize += wcslen(pCurrSrc) + 1;
		++dwStrCount;
	}
	dwSize = (
		dwWordSize // Number of characters
		+ ((dwStrCount + 1) * WI_MULTISTRING_NULL_SIZE) + 1) // WI separators, and final NULL
		* sizeof(DWORD);

	hr = StrAlloc(pDst, dwSize);
	ExitOnFailure(hr, "Failed to allocate buffer");

	pCurrDst = (*pDst);
	for (DWORD i = 0; i < dwStrCount; ++i)
	{
		dwWordSize = wcslen(pCurrSrc);

		err = wcscpy_s((LPWSTR)pCurrDst, dwWordSize + 1, pCurrSrc);
		hr = HRESULT_FROM_WIN32(err);
		ExitOnFailure(hr, "Failed to copy memory");

		pCurrSrc += dwWordSize + 1;
		pCurrDst += dwWordSize;

		err = wcscpy_s((LPWSTR)pCurrDst, WI_MULTISTRING_NULL_SIZE + 1, WI_MULTISTRING_NULL);
		hr = HRESULT_FROM_WIN32(err);
		ExitOnFailure(hr, "Failed to copy memory");

		pCurrDst += WI_MULTISTRING_NULL_SIZE + 1;
	}

	// Terminating null.
	err = wcscpy_s((LPWSTR)pCurrDst, WI_MULTISTRING_NULL_SIZE + 1, WI_MULTISTRING_NULL);
	hr = HRESULT_FROM_WIN32(err);
	ExitOnFailure(hr, "Failed to copy memory");

LExit:
	return hr;
}

HRESULT CRegDataSerializer::DeSerializeMultiString(LPCWSTR pSrc)
{
	HRESULT hr = S_OK;
	DWORD dwStrCount = 0, dwNullCount = 0;
	LPCWSTR pCurrSrc, pNextSrc;
	DWORD dwSize, dwStrSize;
	LPWSTR pCurrDest;

	pCurrSrc = pSrc;
	pNextSrc = pCurrSrc;
	while (pNextSrc != NULL)
	{
		pNextSrc = wcsstr(pCurrSrc, WI_MULTISTRING_NULL);
		if (pNextSrc != NULL)
		{
			++dwNullCount;
			pCurrSrc = pNextSrc + WI_MULTISTRING_NULL_SIZE;
		}
	}

	dwSize = (wcslen(pSrc) - (dwNullCount * 2) + 1) * sizeof(WCHAR);
	dwStrCount = dwNullCount - 1;
	hr = Allocate(dwSize);
	ExitOnFailure(hr, "Failed to allocate memory");

	pCurrSrc = pSrc;
	pNextSrc = pCurrSrc;
	pCurrDest = (LPWSTR)_bytes;
	for (DWORD i = 0; i < dwStrCount; ++i)
	{
		pNextSrc = wcsstr(pCurrSrc, WI_MULTISTRING_NULL);
		dwStrSize = pNextSrc - pCurrSrc;

		memcpy(pCurrDest, pCurrSrc, dwStrSize*sizeof(WCHAR));
		pCurrDest[dwStrSize] = NULL;
		pCurrDest += (dwStrSize + 1);
	}

	pCurrDest[0] = NULL;

LExit:
	return hr;
}

HRESULT CRegDataSerializer::DeSerialize(LPCWSTR pSrc, LPCWSTR sDataType)
{
	HRESULT hr = S_OK;
	DWORD dwSize;
	ULONGLONG *ullTmp;
	DWORD *dwTmp;

	_dataType = wcstol(sDataType, NULL, 10);
	switch (_dataType)
	{
	case REG_DWORD:
		hr = Allocate(sizeof(DWORD));
		ExitOnFailure(hr, "Failed allocating memory");


		dwTmp = reinterpret_cast<DWORD*>(_bytes);
		(*dwTmp) = wcstoul(pSrc, NULL, 16);
		break;

	case REG_QWORD:
		hr = Allocate(sizeof(ULONGLONG));
		ExitOnFailure(hr, "Failed allocating memory");

		ullTmp = reinterpret_cast<ULONGLONG*>(_bytes);
		(*ullTmp) = wcstoull(pSrc, NULL, 16);
		break;

	case REG_SZ:
	case REG_EXPAND_SZ:
		dwSize = wcslen(pSrc) + 1;
		hr = Allocate(dwSize * 2);
		ExitOnFailure(hr, "Failed allocating memory");

		wcscpy_s((LPWSTR)_bytes, dwSize, pSrc);
		break;

	case REG_MULTI_SZ:
		hr = DeSerializeMultiString(pSrc);
		break;

	case REG_BINARY:
		dwSize = wcslen(pSrc) / 2;
		hr = Allocate(dwSize);
		ExitOnFailure(hr, "Failed allocating memory");

		WCHAR sTmp[3];
		for (DWORD i = 0; i < dwSize; ++i)
		{
			sTmp[0] = pSrc[i * 2];
			sTmp[1] = pSrc[i * 2 + 1];
			sTmp[2] = NULL;
			
			_bytes[i] = (BYTE)wcstoul(pSrc, NULL, 16);
		}

		break;

	default:
		hr = E_INVALIDSTATE;
		ExitOnFailure(hr, "Unsupported registry data type");
	}
	
LExit:
	return hr;
}

#include "stdafx.h"
#include "RegDataSerializer.h"
#include "RegistryKey.h"
#include <memory>
#include <strutil.h>
#include <memutil.h>
#include <atlenc.h>
#include <stdlib.h>

#define WI_MULTISTRING_NULL L"[~]"
#define WI_MULTISTRING_NULL_SIZE 3

CRegDataSerializer::CRegDataSerializer()
	:_bytes(nullptr)
	, _size(0)
	, _bufSize(0)
	, _dataType(0)
{
}

CRegDataSerializer::~CRegDataSerializer()
{
	Release();
}

HRESULT CRegDataSerializer::Release()
{
	if (_bytes)
	{
		::free(_bytes);
		_bytes = nullptr;
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
	BreakExitOnNull(_bytes, hr, E_INSUFFICIENT_BUFFER, "Failed to allocate buffer");

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
	BreakExitOnFailure(hr, "Failed to allocate buffer");

	err = memcpy_s(_bytes, _bufSize, pData, dwSize);
	hr = HRESULT_FROM_WIN32(err);
	BreakExitOnFailure(hr, "Failed to copy memory");

	_size = dwSize;
	_dataType = dwDataType;

LExit:
	return hr;
}

HRESULT CRegDataSerializer::Set(LPCWSTR pDataString, LPCWSTR pDataTypeString)
{
	HRESULT hr = S_OK;
	DWORD dwSize = 0;
	DWORD dwTmpSize = 0;
	LONG lValue = 0;
	LONG64 l64Value = 0;
	BYTE *pData = nullptr;
	LPCWSTR pTmp = nullptr;
	CRegistryKey::RegValueType eType;

	hr = CRegistryKey::ParseValueType(pDataTypeString, &eType);
	BreakExitOnFailure(hr, "Failed to parse data type");
	_dataType = eType;

	switch (eType)
	{
	case CRegistryKey::String:
	case CRegistryKey::Expandable:
		dwSize = ((1 + wcslen(pDataString)) * sizeof(WCHAR));
		pData = (BYTE*)pDataString;
		break;
	
	case CRegistryKey::MultiString:
		pTmp = pDataString;
		pData = (BYTE*)pDataString;
		while ((dwTmpSize = wcslen(pTmp)) > 0)
		{
			pTmp += dwTmpSize + 1;
			dwTmpSize = ((1 + dwTmpSize) * sizeof(WCHAR));
			dwSize += dwTmpSize;
		}
		dwSize += sizeof(WCHAR);
		break;
	
	case CRegistryKey::DWord:
		lValue = ::wcstol(pDataString, nullptr, 0);
		pData = (BYTE*)&lValue;
		dwSize = sizeof(lValue);
		break;
	
	case CRegistryKey::QWord:
		l64Value = ::_wcstoi64(pDataString, nullptr, 0);
		pData = (BYTE*)&l64Value;
		dwSize = sizeof(l64Value);
		break;

	case CRegistryKey::Binary:
		hr = DeSerialize(pDataString, pDataTypeString);
		ExitFunction();
		break;
	
	default:
		hr = E_INVALIDARG;
		BreakExitOnFailure(hr, "Invalid registry data type");
		break;
	}

	hr = Set(pData, _dataType, dwSize);
	BreakExitOnFailure(hr, "Invalid registry data");

LExit:
	return hr;
}

HRESULT CRegDataSerializer::Serialize(LPWSTR* ppDst) const
{
	HRESULT hr = S_OK;
	errno_t err = ERROR_SUCCESS;
	size_t stStrSize = 0;
	int iStrSize = 0;
	LPSTR pAnsiStr = nullptr;

	stStrSize = 1 + Base64EncodeGetRequiredLength( _size, ATL_BASE64_FLAG_NOCRLF);
	pAnsiStr = (LPSTR)MemAlloc( stStrSize, TRUE);
	BreakExitOnNull( pAnsiStr, hr, NTE_NO_MEMORY,  "Failed to allocate memory");

	iStrSize = stStrSize;
	if( ! Base64Encode( _bytes, _size, pAnsiStr, &iStrSize, ATL_BASE64_FLAG_NOCRLF))
	{
		hr = E_FAIL;
		BreakExitOnFailure( hr, "Failed to encode to base64 string");
	}

	hr = StrAlloc( ppDst, iStrSize);
	BreakExitOnFailure(hr, "Failed to allocate string");

	err = mbstowcs_s( &stStrSize, (*ppDst), iStrSize, pAnsiStr, iStrSize - 1);
	hr = HRESULT_FROM_WIN32( err);
	BreakExitOnFailure(hr, "Failed to convert ansi-string to wide string.");

LExit:
	return hr;
}

HRESULT CRegDataSerializer::DeSerialize(LPCWSTR pSrc, LPCWSTR sDataType)
{
	HRESULT hr = S_OK;
	errno_t err = ERROR_SUCCESS;
	size_t iStrSize = 0;
	int iMemSize = 0;
	LPSTR pAnsiStr = nullptr;
	CRegistryKey::RegValueType eType;

	_dataType = ::wcstol(sDataType, nullptr, 10);
	hr = CRegistryKey::ParseValueType(sDataType, &eType);
	BreakExitOnFailure(hr, "Failed to parse data type");
	_dataType = eType;

	// Allocate c-string
	iStrSize = 1 + wcslen( pSrc);
	pAnsiStr = (LPSTR)MemAlloc( iStrSize, TRUE);
	BreakExitOnNull( pAnsiStr, hr, NTE_NO_MEMORY,  "Failed to allocate memory");

	// w-string to c-string
	err = ::wcstombs_s( &iStrSize, pAnsiStr, iStrSize, pSrc, (iStrSize - 1) * sizeof( WCHAR));
	hr = HRESULT_FROM_WIN32( err);
	BreakExitOnFailure(hr, "Failed to convert wide-string to ansi-string.");

	iMemSize = 1 + Base64DecodeGetRequiredLength( iStrSize);	
	hr = Allocate( iMemSize);
	BreakExitOnFailure(hr, "Failed to allocate memory.");

	iMemSize = _bufSize;
	if( ! Base64Decode( pAnsiStr, iStrSize, _bytes, &iMemSize))
	{
		hr = E_FAIL;
		BreakExitOnFailure( hr, "Failed to decode base64 string");
	}
	_size = iMemSize;

LExit:
	return hr;	
}

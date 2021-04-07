#pragma once
#include "stdafx.h"

class CRegDataSerializer
{
public:
	CRegDataSerializer() noexcept;
	virtual ~CRegDataSerializer() noexcept;

	HRESULT Set(const BYTE* pData, DWORD dwDataType, DWORD dwSize) noexcept;
	HRESULT Set(LPCWSTR pDataString, LPCWSTR pDataTypeString) noexcept;

	BYTE* Data() const noexcept { return _bytes; }
	DWORD Size() const noexcept { return _size; }
	DWORD DataType() const noexcept { return _dataType; }

	HRESULT Serialize(LPWSTR* ppDst) const noexcept;

	HRESULT DeSerialize(LPCWSTR pSrc, LPCWSTR sDataType) noexcept;

	HRESULT Release() noexcept;

private:

	HRESULT Allocate(DWORD size) noexcept;

	//TODO
	HRESULT SerializeMultiString(LPWSTR* pDst) const noexcept;
	HRESULT DeSerializeMultiString(LPCWSTR pSrc) noexcept;


	BYTE* _bytes;
	DWORD _size;
	DWORD _bufSize;
	DWORD _dataType;
};
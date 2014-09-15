#pragma once
#include "stdafx.h"

class CRegDataSerializer
{
public:
	CRegDataSerializer();
	~CRegDataSerializer();

	HRESULT Set(const BYTE* pData, DWORD dwDataType, DWORD dwSize);
	HRESULT Set(LPCWSTR pDataString, LPCWSTR pDataTypeString);

	BYTE* Data() const { return _bytes; }
	DWORD Size() const { return _size; }
	DWORD DataType() const { return _dataType; }

	HRESULT Serialize(LPWSTR* ppDst) const;

	HRESULT DeSerialize(LPCWSTR pSrc, LPCWSTR sDataType);

	HRESULT Release();

private:

	HRESULT Allocate(DWORD size);

	HRESULT SerializeMultiString(LPWSTR* pDst) const;
	HRESULT DeSerializeMultiString(LPCWSTR pSrc);


	BYTE* _bytes;
	DWORD _size;
	DWORD _bufSize;
	DWORD _dataType;
};


#pragma once
#include "pch.h"
#include "lzma-sdk/CPP/Common/Common.h"
#include "lzma-sdk/CPP/7zip/IDecl.h"
#include "lzma-sdk/CPP/7zip/IStream.h"
#include "lzma-sdk/CPP/Common/MyCom.h"
#include "lzma-sdk/CPP/Windows/FileIO.h"

Z7_CLASS_IMP_COM_1(
	CPanelSwLzmaOutStream
	, IOutStream
)
Z7_IFACE_COM7_IMP(ISequentialOutStream)
public:

	~CPanelSwLzmaOutStream();

	HRESULT Create(LPCWSTR szPath, const FILETIME ftCreationTime, const FILETIME ftLastAccessTime, const FILETIME ftLastWriteTime);

	HRESULT Close();

	private:
		HANDLE _hFile = INVALID_HANDLE_VALUE;
		LPWSTR _szPath = nullptr;
		FILETIME _ftCreationTime = { 0,0 };
		FILETIME _ftLastAccessTime = { 0,0 };
		FILETIME _ftLastWriteTime = { 0,0 };
		
		static unsigned const MAX_RETRIES = 10;
};

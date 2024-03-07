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

	virtual ~CPanelSwLzmaOutStream();

	HRESULT Create(LPCWSTR szPath, UInt64 ullSize, const FILETIME ftCreationTime, const FILETIME ftLastAccessTime, const FILETIME ftLastWriteTime);

	HRESULT Close();

  private:

	  HRESULT Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition, bool updateNextWritePos);

	  HRESULT CompleteWrite();

	  HANDLE _hFile = INVALID_HANDLE_VALUE;
	  LPWSTR _szPath;
	  FILETIME _ftCreationTime = { 0,0 };
	  FILETIME _ftLastAccessTime = { 0,0 };
	  FILETIME _ftLastWriteTime = { 0,0 };

	  static unsigned const MAX_RETRIES = 10;

	  static DWORD WINAPI ExtractThreadProc(LPVOID lpParameter);

	  HANDLE _hExtractThread = NULL;
	  HANDLE _hExtractStarted = NULL;
	  UInt64 _ullWriteSize = 0;
	  UInt64 _ullBufferSize = 0;
	  unsigned char* _pWriteData = nullptr;
	  ULARGE_INTEGER _ullNextWritePos = { 0,0 };
};

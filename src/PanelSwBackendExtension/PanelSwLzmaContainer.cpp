#include "pch.h"
#include "PanelSwLzmaContainer.h"
#pragma comment(lib, "7zC.lib")

CPanelSwLzmaContainer::~CPanelSwLzmaContainer()
{
	Reset();
}

HRESULT CPanelSwLzmaContainer::Reset()
{
	if (_db.get())
	{
		SzArEx_Free(_db.get(), &_alloc);
		_db.reset();
	}
	if (_lookStream.get())
	{
		if (_lookStream->buf)
		{
			ISzAlloc_Free(&_alloc, _lookStream->buf);
		}
		_lookStream.reset();
	}
	if (_archiveStream.get())
	{
		File_Close(&_archiveStream->file);
		_archiveStream.reset();
	}
	if (_outBuffer)
	{
		ISzAlloc_Free(&_alloc, _outBuffer);
		_outBuffer = nullptr;
	}

	_fileIndex = -1;
	_blockIndex = ~0;
	_outBufferSize = 0;

	return S_OK;
}

HRESULT CPanelSwLzmaContainer::ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath)
{
	HRESULT hr = S_OK;
	WRes wRes = ERROR_SUCCESS;
	SRes sRes = 0;

	_archiveStream.reset(new CFileInStream());
	BextExitOnNull(_archiveStream.get(), hr, E_FAIL, "Failed to create 7zip file stream");

	_lookStream.reset(new CLookToRead2());
	BextExitOnNull(_archiveStream.get(), hr, E_FAIL, "Failed to create 7zip look stream");

	_db.reset(new CSzArEx());
	BextExitOnNull(_archiveStream.get(), hr, E_FAIL, "Failed to create 7zip database");

	wRes = InFile_OpenW(&_archiveStream->file, wzFilePath);
	BextExitOnWin32Error(wRes, hr, "Failed to open 7z container '%ls'", wzFilePath);

	FileInStream_CreateVTable(_archiveStream.get());
	_archiveStream->wres = ERROR_SUCCESS;

	LookToRead2_CreateVTable(_lookStream.get(), FALSE);
	_lookStream->buf = nullptr;
	
	_lookStream->buf = (LPBYTE)ISzAlloc_Alloc(&_alloc, _kInputBufSize);
	BextExitOnNull(_lookStream->buf, hr, E_FAIL, "Failed to allocate memory");

	_lookStream->bufSize = _kInputBufSize;
	_lookStream->realStream = &_archiveStream->vt;
	LookToRead2_INIT(_lookStream.get());

	CrcGenerateTable();

	SzArEx_Init(_db.get());
	
	sRes = SzArEx_Open(_db.get(), &_lookStream->vt, &_alloc, &_allocTemp);
	BextExitOnNull((sRes == SZ_OK), hr, sRes, "Failed to open 7zip database");

	BextLog(BUNDLE_EXTENSION_LOG_LEVEL_STANDARD, "Openned 7Z container '%ls'", wzFilePath);

LExit:
	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerNextStream(BSTR* psczStreamName)
{
	HRESULT hr = S_OK;
	LPWSTR szCurrFile = nullptr;
	size_t cchName = 0;

	if (psczStreamName && *psczStreamName)
	{
		::SysFreeString(*psczStreamName);
		*psczStreamName = nullptr;
	}

	// Skip folder entries
	do
	{
		++_fileIndex;
	} while ((_fileIndex < _db->NumFiles) && SzArEx_IsDir(_db.get(), _fileIndex));
	if (_fileIndex >= _db->NumFiles)
	{
		hr = E_NOMOREITEMS;
		ExitFunction();
	}

	cchName = SzArEx_GetFileNameUtf16(_db.get(), _fileIndex, nullptr);
	BextExitOnNull(cchName, hr, E_FAIL, "File name length is 0");

	hr = StrAlloc(&szCurrFile, ++cchName);
	BextExitOnFailure(hr, "Failed to allocate memory");

	SzArEx_GetFileNameUtf16(_db.get(), _fileIndex, (UInt16*)szCurrFile);
	BextExitOnNull((szCurrFile && *szCurrFile), hr, E_FAIL, "Failed to get file name");

	if (psczStreamName)
	{
		*psczStreamName = ::SysAllocString(szCurrFile);
		BextExitOnNull(*psczStreamName, hr, E_FAIL, "Failed to allocate sys string");
	}

LExit:
	ReleaseStr(szCurrFile);
	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerStreamToFile(LPCWSTR wzFileName)
{
	HRESULT hr = S_OK;
	SRes sRes = 0;
	WRes wRes = ERROR_SUCCESS;
	size_t offset = 0;
	size_t outSizeProcessed = 0;

	BextExitOnNull((_fileIndex < _db->NumFiles), hr, E_INVALIDSTATE, "7z container is exhausted, can't extract '%ls'", wzFileName);

	sRes = SzArEx_Extract(_db.get(), &_lookStream->vt, _fileIndex, &_blockIndex, &_outBuffer, &_outBufferSize, &offset, &outSizeProcessed, &_alloc, &_allocTemp);
	BextExitOnNull((sRes == SZ_OK), hr, sRes, "Failed to open 7zip database");

	hr = FileWrite(wzFileName, FILE_ATTRIBUTE_NORMAL, _outBuffer + offset, outSizeProcessed, nullptr);
	BextExitOnFailure(hr, "Failed to write to file '%ls'", wzFileName);

LExit:
	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerStreamToBuffer(BYTE** ppbBuffer, SIZE_T* pcbBuffer)
{
	return E_NOTIMPL;
}

HRESULT CPanelSwLzmaContainer::ContainerSkipStream()
{
	return S_OK;
}

HRESULT CPanelSwLzmaContainer::ContainerClose()
{
	return Reset();
}

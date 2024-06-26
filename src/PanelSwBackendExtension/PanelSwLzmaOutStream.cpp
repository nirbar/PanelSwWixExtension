#include "pch.h"
#include "PanelSwLzmaOutStream.h"

CPanelSwLzmaOutStream::~CPanelSwLzmaOutStream()
{
	Close();
}

HRESULT CPanelSwLzmaOutStream::Close()
{
	HRESULT hr = S_OK;

	hr = CompleteWrite();
	BextExitOnFailure(hr, "Failed to complete previous write");

	if (_hFile != INVALID_HANDLE_VALUE)
	{
		const FILETIME* pftCreate = (_ftCreationTime.dwLowDateTime || _ftCreationTime.dwHighDateTime) ? &_ftCreationTime : nullptr;
		const FILETIME* pftAccess = (_ftLastAccessTime.dwLowDateTime || _ftLastAccessTime.dwHighDateTime) ? &_ftLastAccessTime : nullptr;
		const FILETIME* pftWrite = (_ftLastWriteTime.dwLowDateTime || _ftLastWriteTime.dwHighDateTime) ? &_ftLastWriteTime : nullptr;

		if (pftCreate || pftAccess || pftWrite)
		{
			BOOL bRes = ::SetFileTime(_hFile, pftCreate, pftAccess, pftWrite);
			if (!bRes)
			{
				BextLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to set file times for '%ls'", _szPath);
			}
		}

		BextLog(BOOTSTRAPPER_EXTENSION_LOG_LEVEL_DEBUG, "Extracted '%ls'", _szPath);
	}

LExit:
	ReleaseHandle(_hExtractStarted);
	ReleaseNullStr(_szPath);
	ReleaseFileHandle(_hFile);
	ReleaseNullMem(_pWriteData);
	_dwBufferSize = 0;
	_dwWriteSize = 0;
	_ullNextWritePos.QuadPart = 0;
	_ftCreationTime = { 0,0 };
	_ftLastAccessTime = { 0,0 };
	_ftLastWriteTime = { 0,0 };

	return hr;
}

HRESULT CPanelSwLzmaOutStream::Create(LPCWSTR szPath, UInt64 ullSize, const FILETIME ftCreationTime, const FILETIME ftLastAccessTime, const FILETIME ftLastWriteTime)
{
	BOOL bRes = TRUE;
	HRESULT hr = S_OK;

	Close();

	_ftCreationTime = ftCreationTime;
	_ftLastAccessTime = ftLastAccessTime;
	_ftLastWriteTime = ftLastWriteTime;

	hr = StrAllocString(&_szPath, szPath, 0);
	BextExitOnFailure(hr, "Failed to copy string");

	_hExtractStarted = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	BextExitOnNullWithLastError(_hExtractStarted, hr, "Failed to create event");

	::SetFileAttributesW(_szPath, FILE_ATTRIBUTE_NORMAL);
	::DeleteFileW(_szPath);

	_hFile = ::CreateFileW(_szPath, GENERIC_ALL, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	BextExitOnNullWithLastError((_hFile != INVALID_HANDLE_VALUE), hr, "Failed to create file '%ls'", _szPath);

	// Best effort to set file size
	if (ullSize)
	{
		SetSize(ullSize);
	}

LExit:
	if (FAILED(hr))
	{
		Close();
	}
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition))
{
	return Seek(offset, seekOrigin, newPosition, true);
}

HRESULT CPanelSwLzmaOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition, bool updateNextWritePos)
{
	BOOL bRes = FALSE;
	HRESULT hr = S_OK;
	LARGE_INTEGER liPos = { 0 };
	LARGE_INTEGER liNewPos = { 0 };

	switch (seekOrigin)
	{
	default:
	case ESzSeek::SZ_SEEK_SET:
		seekOrigin = FILE_BEGIN;
		break;
	case ESzSeek::SZ_SEEK_CUR:
		seekOrigin = FILE_CURRENT;
		break;
	case ESzSeek::SZ_SEEK_END:
		seekOrigin = FILE_END;
		break;
	}

	liPos.QuadPart = offset;
	bRes = ::SetFilePointerEx(_hFile, liPos, &liNewPos, seekOrigin);
	BextExitOnNullWithLastError(bRes, hr, "Failed to seek in file '%ls'", (LPCWSTR)_szPath);

	if (newPosition)
	{
		*newPosition = liNewPos.QuadPart;
	}
	if (updateNextWritePos)
	{
		_ullNextWritePos.QuadPart = liNewPos.QuadPart;
	}

LExit:
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaOutStream::SetSize(UInt64 newSize))
{
	BOOL bRes = FALSE;
	HRESULT hr = S_OK;
	ULARGE_INTEGER liCurrPos = { 0 };

	hr = Seek(0, ESzSeek::SZ_SEEK_CUR, &liCurrPos.QuadPart, false);
	BextExitOnFailure(hr, "Failed to get current seek position in file '%ls'", _szPath);

	hr = Seek(newSize, ESzSeek::SZ_SEEK_SET, nullptr, false);
	BextExitOnFailure(hr, "Failed to set seek position in file '%ls' to %I64u", _szPath, newSize);

	bRes = ::SetEndOfFile(_hFile);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set EOF in file '%ls'", (LPCWSTR)_szPath);

	hr = Seek(liCurrPos.QuadPart, ESzSeek::SZ_SEEK_SET, nullptr, false);
	BextExitOnFailure(hr, "Failed to set current seek position in file '%ls'", _szPath);

LExit:
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaOutStream::Write(const void* data, UInt32 size, UInt32* processedSize))
{
	BOOL bRes = TRUE;
	HRESULT hr = S_OK;
	ULARGE_INTEGER ullPos = { 0,0 };
	DWORD dwRes = ERROR_SUCCESS;

	hr = CompleteWrite();
	BextExitOnFailure(hr, "Failed to complete previous write");

	// Allocate buffer if the existing one is insufficient
	if (_dwBufferSize < size)
	{
		ReleaseNullMem(_pWriteData);
		_dwBufferSize = 0;

		_pWriteData = (unsigned char*)MemAlloc(size, FALSE);
		BextExitOnNull(_pWriteData, hr, E_OUTOFMEMORY, "Failed to allocate write buffer");

		_dwBufferSize = size;
	}

	_dwWriteSize = size;
	memcpy(_pWriteData, data, size);

	bRes = ::ResetEvent(_hExtractStarted);
	BextExitOnNullWithLastError(bRes, hr, "Failed to reset event");

	_hExtractThread = ::CreateThread(nullptr, 0, WriteThreadProc, this, 0, nullptr);
	BextExitOnNullWithLastError(_hExtractThread, hr, "Failed to create thread");

	dwRes = ::WaitForSingleObject(_hExtractStarted, INFINITE);
	BextExitOnNullWithLastError((dwRes != WAIT_FAILED), hr, "Failed to wait for extraction threadt to start");

	_ullNextWritePos.QuadPart += size;

LExit:
	if (processedSize)
	{
		*processedSize = FAILED(hr) ? 0 : size;
	}

	return hr;
}

HRESULT CPanelSwLzmaOutStream::CompleteWrite()
{
	HRESULT hr = S_OK;

	if (_hExtractThread)
	{
		DWORD dwRes = ERROR_SUCCESS;
		BOOL bRes = TRUE;

		dwRes = ::WaitForSingleObject(_hExtractThread, INFINITE);
		BextExitOnNullWithLastError((dwRes != WAIT_FAILED), hr, "Failed to wait for previous write to complete");

		bRes = ::GetExitCodeThread(_hExtractThread, &dwRes);
		BextExitOnNullWithLastError(bRes, hr, "Failed to get previous write end result");
		BextExitOnWin32Error(dwRes, hr, "Previous write ended with an error");
	}

LExit:
	ReleaseHandle(_hExtractThread);

	return hr;
}

/*static*/ DWORD WINAPI CPanelSwLzmaOutStream::WriteThreadProc(LPVOID lpParameter)
{
	HRESULT hr = S_OK;
	DWORD dwWritten = 0;
	BOOL bRes = TRUE;

	CPanelSwLzmaOutStream* pThis = (CPanelSwLzmaOutStream*)lpParameter;
	ULARGE_INTEGER ullStartPos = pThis->_ullNextWritePos;
	DWORD dwWriteSize = pThis->_dwWriteSize;

	bRes = ::SetEvent(pThis->_hExtractStarted);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set event");

	hr = pThis->Seek(ullStartPos.QuadPart, ESzSeek::SZ_SEEK_SET, nullptr, false);
	BextExitOnFailure(hr, "Failed to seek to write position");

	bRes = ::WriteFile(pThis->_hFile, pThis->_pWriteData, dwWriteSize, &dwWritten, nullptr);
	BextExitOnNullWithLastError(bRes, hr, "Failed write to file '%ls'", pThis->_szPath);
	BextExitOnNull((dwWritten == dwWriteSize), hr, E_FAIL, "Failed write sufficient data to file '%ls'. Wrote %u/%u bytes", pThis->_szPath, dwWritten, dwWriteSize);

LExit:
	return HRESULT_CODE(hr);
}

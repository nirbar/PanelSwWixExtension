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
			BOOL bRes = FALSE;
			for (unsigned i = 0; !bRes && (i < MAX_RETRIES); ++i)
			{
				hr = S_OK;

				bRes = ::SetFileTime(_hFile, pftCreate, pftAccess, pftWrite);
				if (!bRes)
				{
					hr = HRESULT_FROM_WIN32(::GetLastError());
					BextLogError(hr, "Failed to set file times for '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
					continue;
				}
				break;
			}
			hr = S_OK; // Ignoring failure to set file time
		}

		BextLog(BUNDLE_EXTENSION_LOG_LEVEL_DEBUG, "Extracted '%ls'", _szPath);
	}

LExit:
	ReleaseHandle(_hExtractStarted);
	ReleaseNullStr(_szPath);
	ReleaseFileHandle(_hFile);
	ReleaseNullMem(_pWriteData);
	_ullBufferSize = 0;
	_ullWriteSize = 0;
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

	for (unsigned i = 0; (_hFile == INVALID_HANDLE_VALUE) && (i < MAX_RETRIES); ++i)
	{
		hr = S_OK;

		::SetFileAttributesW(_szPath, FILE_ATTRIBUTE_NORMAL);
		::DeleteFileW(_szPath);

		_hFile = ::CreateFileW(_szPath, GENERIC_ALL, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (_hFile == INVALID_HANDLE_VALUE)
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			BextLogError(hr, "Failed to create file '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
			continue;
		}
		break;
	}
	BextExitOnFailure(hr, "Failed to create file");

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

	bRes = FALSE;
	liPos.QuadPart = offset;
	for (unsigned i = 0; !bRes && (i < MAX_RETRIES); ++i)
	{
		hr = S_OK;

		bRes = ::SetFilePointerEx(_hFile, liPos, &liNewPos, seekOrigin);
		if (!bRes)
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			BextLogError(hr, "Failed to seek in file '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
			continue;
		}
		break;
	}
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

	bRes = FALSE;
	for (unsigned i = 0; !bRes && (i < MAX_RETRIES); ++i)
	{
		hr = S_OK;

		bRes = ::SetEndOfFile(_hFile);
		if (!bRes)
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			BextLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to set EOF in file '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
			continue;
		}
		break;
	}
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
	if (_ullBufferSize < size)
	{
		ReleaseNullMem(_pWriteData);
		_ullBufferSize = 0;

		_pWriteData = (unsigned char*)MemAlloc(size, FALSE);
		BextExitOnNull(_pWriteData, hr, E_OUTOFMEMORY, "Failed to allocate write buffer");

		_ullBufferSize = size;
	}

	_ullWriteSize = size;
	memcpy(_pWriteData, data, size);

	bRes = ::ResetEvent(_hExtractStarted);
	BextExitOnNullWithLastError(bRes, hr, "Failed to reset event");

	_hExtractThread = ::CreateThread(nullptr, 0, ExtractThreadProc, this, 0, nullptr);
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

/*static*/ DWORD WINAPI CPanelSwLzmaOutStream::ExtractThreadProc(LPVOID lpParameter)
{
	HRESULT hr = S_OK;
	CPanelSwLzmaOutStream* pThis = (CPanelSwLzmaOutStream*)lpParameter;
	ULARGE_INTEGER ullStartPos = pThis->_ullNextWritePos;
	UInt64 ullWriteSize = pThis->_ullWriteSize;
	UInt64 ullWritten = 0;
	BOOL bRes = TRUE;

	bRes = ::SetEvent(pThis->_hExtractStarted);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set event");

	for (unsigned i = 0; (i < MAX_RETRIES) && (ullWritten < ullWriteSize); ++i)
	{
		DWORD dwRes = ERROR_SUCCESS;
		DWORD dwWritten = 0;
		ULARGE_INTEGER ullPos = { 0,0 };
		hr = S_OK;

		ullPos.QuadPart = ullStartPos.QuadPart + ullWritten;

		hr = pThis->Seek(ullPos.QuadPart, ESzSeek::SZ_SEEK_SET, nullptr, false);
		BextExitOnFailure(hr, "Failed to seek to write position");

		bRes = ::WriteFile(pThis->_hFile, pThis->_pWriteData + ullWritten, ullWriteSize - ullWritten, &dwWritten, nullptr);
		if (!bRes)
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			BextLogError(hr, "Failed write to file '%ls' on attempt %u/%u", pThis->_szPath, i, MAX_RETRIES);
			continue;
		}

		ullWritten += dwWritten;
		if (ullWritten < ullWriteSize)
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			BextLogError(hr, "Failed to write all data to '%ls' on attempt %u/%u. %I64u/%I64u written", pThis->_szPath, i, MAX_RETRIES, ullWritten, ullWriteSize);
			continue;
		}
		break;
	}

LExit:
	return HRESULT_CODE(hr);
}

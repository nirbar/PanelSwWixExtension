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
	BextExitOnFailure(hr, "Failed to complete write");

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
				bRes = ::SetFileTime(_hFile, pftCreate, pftAccess, pftWrite);
				if (!bRes)
				{
					hr = HRESULT_FROM_WIN32(::GetLastError());
					BextLogError(hr, "Failed to set file times for '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
					continue;
				}
				hr = S_OK;
				break;
			}
			hr = S_OK; // Ignoring failure to set file time
		}
	}

LExit:
	ReleaseNullStr(_szPath);
	ReleaseFileHandle(_hFile);
	ReleaseNullMem(_pWriteData);
	ZeroMemory(&_overlapped, sizeof(_overlapped));
	_dwWriteAttempts = 0;
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

	for (unsigned i = 0; (_hFile == INVALID_HANDLE_VALUE) && (i < MAX_RETRIES); ++i)
	{
		_hFile = ::CreateFileW(_szPath, GENERIC_ALL, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
		if (_hFile == INVALID_HANDLE_VALUE)
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			BextLogError(hr, "Failed to create file '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
			continue;
		}
		hr = S_OK;
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
		bRes = ::SetFilePointerEx(_hFile, liPos, &liNewPos, seekOrigin);
		if (!bRes)
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			BextLogError(hr, "Failed to seek in file '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
			continue;
		}
		hr = S_OK;
		break;
	}
	BextExitOnNullWithLastError(bRes, hr, "Failed to seek in file '%ls'", (LPCWSTR)_szPath);
	
	if (newPosition)
	{
		*newPosition = liNewPos.QuadPart;
	}
	_ullNextWritePos.QuadPart = liNewPos.QuadPart;

LExit:
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaOutStream::SetSize(UInt64 newSize))
{
	BOOL bRes = FALSE;
	HRESULT hr = S_OK;
	ULARGE_INTEGER liCurrPos = { 0 };

	hr = Seek(0, ESzSeek::SZ_SEEK_CUR, &liCurrPos.QuadPart);
	BextExitOnFailure(hr, "Failed to get current seek position in file '%ls'", _szPath);

	hr = Seek(newSize, ESzSeek::SZ_SEEK_SET, nullptr);
	BextExitOnFailure(hr, "Failed to set seek position in file '%ls' to %I64u", _szPath, newSize);

	bRes = FALSE;
	for (unsigned i = 0; !bRes && (i < MAX_RETRIES); ++i)
	{
		bRes = ::SetEndOfFile(_hFile);
		if (!bRes)
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			BextLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to set EOF in file '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
			continue;
		}
		hr = S_OK;
		break;
	}
	BextExitOnNullWithLastError(bRes, hr, "Failed to set EOF in file '%ls'", (LPCWSTR)_szPath);

	hr = Seek(liCurrPos.QuadPart, ESzSeek::SZ_SEEK_SET, nullptr);
	BextExitOnFailure(hr, "Failed to set current seek position in file '%ls'", _szPath);

LExit:
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaOutStream::Write(const void* data, UInt32 size, UInt32* processedSize))
{
	BOOL bRes = TRUE;
	HRESULT hr = S_OK;
	ULARGE_INTEGER ullPos = { 0,0 };
	DWORD i = 0;

	hr = CompleteWrite();
	BextExitOnFailure(hr, "Failed to complete previous write");

	// Allocate buffer if the existing one is insufficient
	if (_ullBufferSize < size)
	{
		ReleaseNullMem(_pWriteData);

		_pWriteData = (unsigned char*)MemAlloc(size, FALSE);
		BextExitOnNull(_pWriteData, hr, E_OUTOFMEMORY, "Failed to allocate write buffer");

		_ullBufferSize = size;
	}

	_ullWriteSize = size;
	memcpy(_pWriteData, data, size);

	_overlapped.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	BextExitOnNullWithLastError(_overlapped.hEvent, hr, "Failed to create event");

	_overlapped.Offset = _ullNextWritePos.LowPart;
	_overlapped.OffsetHigh = _ullNextWritePos.HighPart;
	_ullNextWritePos.QuadPart += size;
	_dwWriteAttempts = 0;

	hr = WriteCore();
	BextExitOnFailure(hr, "Failed write to file");

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

	if (_overlapped.hEvent)
	{
		for (; _dwWriteAttempts < MAX_RETRIES; ++_dwWriteAttempts)
		{
			DWORD dwWritten = 0;
			DWORD dwRes = ERROR_SUCCESS;

			dwRes = ::WaitForSingleObject(_overlapped.hEvent, INFINITE);
			if (dwRes == WAIT_FAILED) // Any other value is good in this case
			{
				hr = HRESULT_FROM_WIN32(::GetLastError());
				BextLogError(hr, "Failed to wait for write to complete on attempt %u/%u", _dwWriteAttempts, MAX_RETRIES);

				if (_dwWriteAttempts < (MAX_RETRIES - 1))
				{
					hr = WriteCore();
				}
				continue;
			}

			dwRes = ::GetOverlappedResult(_hFile, &_overlapped, &dwWritten, TRUE);
			if (!dwRes)
			{
				hr = HRESULT_FROM_WIN32(::GetLastError());
				BextLogError(hr, "Failed to get write status on attempt %u/%u", _dwWriteAttempts, MAX_RETRIES);

				if (_dwWriteAttempts < (MAX_RETRIES - 1))
				{
					hr = WriteCore();
				}
				continue;
			}

			if (dwWritten < _ullWriteSize)
			{
				hr = HRESULT_FROM_WIN32(::GetLastError());
				BextLogError(hr, "Failed to write all data to '%ls' on attempt %u/%u. %u/%I64u written", _szPath, _dwWriteAttempts, MAX_RETRIES, dwWritten, _ullWriteSize);

				if (_dwWriteAttempts < (MAX_RETRIES - 1))
				{
					hr = WriteCore();
				}
				continue;
			}

			hr = S_OK;
			break;
		}
	}

LExit:
	ReleaseHandle(_overlapped.hEvent);
	ZeroMemory(&_overlapped, sizeof(_overlapped));

	return hr;
}

HRESULT CPanelSwLzmaOutStream::WriteCore()
{
	HRESULT hr = S_OK;

	for (; _dwWriteAttempts < MAX_RETRIES; ++_dwWriteAttempts)
	{
		_overlapped.Internal = 0;
		_overlapped.InternalHigh = 0;
		_overlapped.Pointer = 0;
		::ResetEvent(_overlapped.hEvent);

		BOOL bRes = ::WriteFile(_hFile, _pWriteData, _ullWriteSize, nullptr, &_overlapped);
		if (!bRes && (::GetLastError() != ERROR_IO_PENDING))
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			BextLogError(hr, "Failed write to file '%ls' on attempt %u/%u", _szPath, _dwWriteAttempts, MAX_RETRIES);
			continue;
		}

		hr = S_OK;
		break;
	}

LExit:

	return hr;
}

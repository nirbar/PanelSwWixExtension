#include "pch.h"
#include "PanelSwLzmaOutStream.h"

CPanelSwLzmaOutStream::~CPanelSwLzmaOutStream()
{
	Close();
}

HRESULT CPanelSwLzmaOutStream::Close()
{
	HRESULT hr = S_OK;

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
					BextLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to set file times for '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
				}
			}
			BextExitOnNullWithLastError(bRes, hr, "Failed to set file times for '%ls'", _szPath);
		}
	}

LExit:
	ReleaseFileHandle(_hFile);
	ReleaseNullStr(_szPath);
	_ftCreationTime = { 0,0 };
	_ftLastAccessTime = { 0,0 };
	_ftLastWriteTime = { 0,0 };

	return hr;
}

HRESULT CPanelSwLzmaOutStream::Create(LPCWSTR szPath, const FILETIME ftCreationTime, const FILETIME ftLastAccessTime, const FILETIME ftLastWriteTime)
{
	BOOL bRes = TRUE;
	HRESULT hr = S_OK;
	DWORD dwAttempts = 0;

	Close();

	_ftCreationTime = ftCreationTime;
	_ftLastAccessTime = ftLastAccessTime;
	_ftLastWriteTime = ftLastWriteTime;

	hr = StrAllocString(&_szPath, szPath, 0);
	BextExitOnFailure(hr, "Failed to copy string");

	for (unsigned i = 0; (_hFile == INVALID_HANDLE_VALUE) && (i < MAX_RETRIES); ++i)
	{
		_hFile = ::CreateFile(szPath, GENERIC_ALL, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (_hFile == INVALID_HANDLE_VALUE)
		{
			BextLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to create file '%ls' on attempt %u/%u", szPath, i, MAX_RETRIES);
		}
	}
	BextExitOnNullWithLastError((_hFile != INVALID_HANDLE_VALUE), hr, "Failed to create file '%ls'", (LPCWSTR)szPath);

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
	for (unsigned i = 0; !bRes && (i < MAX_RETRIES); ++i)
	{
		bRes = ::SetFilePointerEx(_hFile, liPos, &liNewPos, seekOrigin);
		if (!bRes)
		{
			BextLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to seek in file '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
		}
	}
	BextExitOnNullWithLastError(bRes, hr, "Failed to seek in file '%ls'", (LPCWSTR)_szPath);
	
	if (newPosition)
	{
		*newPosition = liNewPos.QuadPart;
	}

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
			BextLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to set EOF in file '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
		}
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
	DWORD dwTotalWritten = 0;

	for (unsigned i = 0; (dwTotalWritten < size) && (i < MAX_RETRIES); ++i)
	{
		DWORD dwWritten = 0;

		bRes = ::WriteFile(_hFile, ((LPBYTE)data) + dwTotalWritten, size - dwTotalWritten, &dwWritten, nullptr);
		if (!bRes)
		{
			BextLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to set EOF in file '%ls' on attempt %u/%u", _szPath, i, MAX_RETRIES);
		}
		dwTotalWritten += dwWritten;
		if (dwTotalWritten < size)
		{
			BextLogError(HRESULT_FROM_WIN32(ERROR_WRITE_FAULT), "Succedded to write %u/%u bytes to '%ls' on attempt %u/%u", dwTotalWritten, size, _szPath, i, MAX_RETRIES);
		}
	}
	BextExitOnNullWithLastError(bRes, hr, "Failed to set EOF in file '%ls'", (LPCWSTR)_szPath);
	if (dwTotalWritten < size)
	{
		hr = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
		BextExitOnFailure(hr, "Failed to write to '%ls'", _szPath);
	}

LExit:
	if (processedSize)
	{
		*processedSize = dwTotalWritten;
	}

	return hr;
}

#include "pch.h"
#include "PanelSwLzmaOutStream.h"

CPanelSwLzmaOutStream::~CPanelSwLzmaOutStream()
{
	Close();
}

HRESULT CPanelSwLzmaOutStream::Create(LPCWSTR szPath)
{
	bool bRes = true;
	HRESULT hr = S_OK;

	Close();

	bRes = _file.Create(szPath, true);
	BextExitOnNullWithLastError(bRes, hr, "Failed to create file '%ls'", (LPCWSTR)szPath);

	_filePath = szPath;

LExit:
	return hr;
}

HRESULT CPanelSwLzmaOutStream::Close()
{
	bool bRes = true;
	HRESULT hr = S_OK;

	if (_file.GetHandle() != INVALID_HANDLE_VALUE)
	{
		bRes = _file.Close();
		BextExitOnNullWithLastError(bRes, hr, "Failed to close file '%ls'", (LPCWSTR)_filePath);
	}

LExit:
	_filePath.Empty();

	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition))
{
	bool bRes = true;
	HRESULT hr = S_OK;
	UInt64 ps = 0;

	bRes = _file.Seek(offset, seekOrigin, ps);
	BextExitOnNullWithLastError(bRes, hr, "Failed to seek in file '%ls'", (LPCWSTR)_filePath);
	
	if (newPosition)
	{
		*newPosition = ps;
	}

LExit:
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaOutStream::SetSize(UInt64 newSize))
{
	bool bRes = true;
	HRESULT hr = S_OK;
	UInt64 ps = 0;

	bRes = _file.SetLength(newSize);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set size %I64u for file '%ls'", newSize, (LPCWSTR)_filePath);

LExit:
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaOutStream::Write(const void* data, UInt32 size, UInt32* processedSize))
{
	bool bRes = true;
	HRESULT hr = S_OK;
	UInt32 ps = 0;

	bRes = _file.Write(data, size, ps);
	BextExitOnNullWithLastError(bRes, hr, "Failed to write %u bytes to '%ls'", size, (LPCWSTR)_filePath);

	if (processedSize)
	{
		*processedSize = ps;
	}

LExit:
	return hr;
}

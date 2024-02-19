#include "pch.h"
#include "PanelSwLzmaInStream.h"

CPanelSwLzmaInStream::~CPanelSwLzmaInStream()
{
	ReleaseContainer();
}

HRESULT CPanelSwLzmaInStream::InitContainer(HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize)
{
	HRESULT hr = S_OK;
	LARGE_INTEGER llPos;
	LARGE_INTEGER llPosNew;
	BOOL bRes = TRUE;

	bRes = ::DuplicateHandle(::GetCurrentProcess(), hBundle, ::GetCurrentProcess(), &_hBundle, GENERIC_READ, FALSE, 0);
	BextExitOnNullWithLastError(bRes, hr, "Failed to duplicte handle");

	llPos.QuadPart = 0;
	llPosNew.QuadPart = 0;
	bRes = ::GetFileSizeEx(_hBundle, &llPosNew);
	BextExitOnNullWithLastError(bRes, hr, "Failed to get container size");

	_qwContainerStartPos = qwContainerStartPos;
	_qwContainerSize = (qwContainerSize != INFINITE_CONTAINER_SIZE) ? qwContainerSize : llPosNew.QuadPart;
	_qwBundleSize = llPosNew.QuadPart;
	BextExitOnNull((_qwBundleSize >= (_qwContainerStartPos + _qwContainerSize)), hr, E_INVALIDARG, "Attached container size exceeds bundle size");

	llPos.QuadPart = _qwContainerStartPos;
	bRes = ::SetFilePointerEx(_hBundle, llPos, nullptr, FILE_BEGIN);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set container seek position to %I64i", llPos.QuadPart);

LExit:
	if (FAILED(hr))
	{
		ReleaseContainer();
	}

	return hr;
}

void CPanelSwLzmaInStream::ReleaseContainer()
{
	ReleaseFileHandle(_hBundle);
	_hBundle = INVALID_HANDLE_VALUE;
	_qwContainerStartPos = 0;
	_qwContainerSize = 0;
	_qwBundleSize = 0;
}

Z7_COM7F_IMF(CPanelSwLzmaInStream::Read(void* data, UInt32 size, UInt32* processedSize))
{
	HRESULT hr = S_OK;
	LARGE_INTEGER llPos;
	LARGE_INTEGER llCurrPos;
	LARGE_INTEGER llMaxRead;
	DWORD dwActualRead = 0;
	BOOL bRes = TRUE;

	llPos.QuadPart = 0;
	bRes = ::SetFilePointerEx(_hBundle, llPos, &llCurrPos, FILE_CURRENT);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set container seek position");

	llMaxRead.QuadPart = _qwContainerStartPos + _qwContainerSize - llCurrPos.QuadPart;
	if (llMaxRead.QuadPart <= 0)
	{
		if (processedSize)
		{
			*processedSize = 0;
		}
		goto LExit;
	}

	if (llMaxRead.QuadPart > size)
	{
		llMaxRead.QuadPart = size;
	}

	bRes = ::ReadFile(_hBundle, data, llMaxRead.LowPart, &dwActualRead, nullptr);
	BextExitOnNullWithLastError(bRes, hr, "Failed to read from container");

	if (processedSize)
	{
		*processedSize = dwActualRead;
	}

LExit:
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition))
{
	HRESULT hr = S_OK;
	LARGE_INTEGER llPos;
	LARGE_INTEGER llCurrPos;
	LARGE_INTEGER llNewSeek;
	BOOL bRes = TRUE;

	llPos.QuadPart = 0;
	llCurrPos.QuadPart = 0;
	bRes = ::SetFilePointerEx(_hBundle, llPos, &llCurrPos, FILE_CURRENT);
	BextExitOnNullWithLastError(bRes, hr, "Failed to get container seek position");

	switch (seekOrigin)
	{
	case ESzSeek::SZ_SEEK_CUR:
		llNewSeek.QuadPart = llCurrPos.QuadPart + offset;
		break;
	case ESzSeek::SZ_SEEK_END:
		llNewSeek.QuadPart = _qwContainerStartPos + _qwContainerSize + offset;
		break;
	case ESzSeek::SZ_SEEK_SET:
		llNewSeek.QuadPart = _qwContainerStartPos + offset;
		break;
	default:
		hr = E_INVALIDARG;
		goto LExit;
	}

	bRes = ::SetFilePointerEx(_hBundle, llNewSeek, &llCurrPos, FILE_BEGIN);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set container seek position");

	if (newPosition)
	{
		*newPosition = llCurrPos.QuadPart - _qwContainerStartPos;
	}

LExit:
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaInStream::GetSize(UInt64* size))
{
	if (size)
	{
		*size = _qwContainerSize;
	}
	return S_OK;
}

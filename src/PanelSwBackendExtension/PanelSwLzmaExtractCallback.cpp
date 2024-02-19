#include "pch.h"
#include "PanelSwLzmaExtractCallback.h"
#include "lzma-sdk/CPP/7zip/Common/FileStreams.h"
#include "lzma-sdk/CPP/7zip/Archive/IArchive.h"

HRESULT CPanelSwLzmaExtractCallback::Init(IInArchive* archive, UInt32 extractCount, UInt32* extractIndices, FString* extractPaths)
{
	HRESULT hr = S_OK;

	_archive = archive;

	_extractIndices.reset(new UInt32[extractCount]);
	BextExitOnNull(_extractIndices.get(), hr, E_OUTOFMEMORY, "Failed to allocate memory");

	_extractPaths.reset(new FString[extractCount]);
	BextExitOnNull(_extractPaths.get(), hr, E_OUTOFMEMORY, "Failed to allocate memory");

	_dwErrCount = 0;
	_extractCount = extractCount;
	for (UInt32 i = 0; i < _extractCount; ++i)
	{
		_extractIndices[i] = extractIndices[i];
		_extractPaths[i] = extractPaths[i];
	}

LExit:
	return S_OK;
}


Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode))
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	COutFileStream* os = nullptr;
	CMyComPtr<ISequentialOutStream> outOs;

	if (outStream != nullptr)
	{
		*outStream = nullptr;

		for (UInt32 i = 0; i < _extractCount; ++i)
		{
			if (_extractIndices[i] == index)
			{
				os = new COutFileStream();
				BextExitOnNull(os, hr, E_OUTOFMEMORY, "Failed to allocate file stream object");

				bRes = os->File.CreateAlways(_extractPaths[i], FILE_ATTRIBUTE_NORMAL);
				BextExitOnNullWithLastError(os, hr, "Failed to create file '%ls'", (const wchar_t*)_extractPaths[i]);

				outOs = os;
				os = nullptr;
				*outStream = outOs.Detach();

				_extractIndices[i] = INFINITE; // Allow the same index to be extracted to different targets
				break;
			}
		}
	}

LExit:
	if (os)
	{
		delete os;
	}

	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::PrepareOperation(Int32 askExtractMode))
{
	HRESULT hr = S_OK;
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::SetOperationResult(Int32 opRes))
{
	if (opRes != NArchive::NExtract::NOperationResult::kOK)
	{
		++_dwErrCount;
	}
	return _dwErrCount ? E_FAIL : S_OK;
}

Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::ReportExtractResult(UInt32 indexType, UInt32 index, Int32 opRes))
{
	if (opRes != NArchive::NExtract::NOperationResult::kOK)
	{
		++_dwErrCount;
	}
	return _dwErrCount ? E_FAIL : S_OK;
}

Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::SetCompleted(const UInt64* total))
{
	HRESULT hr = S_OK;
	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::SetTotal(UInt64 total))
{
	HRESULT hr = S_OK;
	return hr;
}

BOOL CPanelSwLzmaExtractCallback::HasErrors() const
{
	return _dwErrCount ? TRUE : FALSE;
}

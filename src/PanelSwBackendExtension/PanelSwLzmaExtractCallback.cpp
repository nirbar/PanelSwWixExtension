#include "pch.h"
#include "PanelSwLzmaExtractCallback.h"
#include "PanelSwLzmaOutStream.h"
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
	CPanelSwLzmaOutStream* os = nullptr;
	CMyComPtr<ISequentialOutStream> outOs;

	if (outStream != nullptr)
	{
		*outStream = nullptr;

		if (askExtractMode == NArchive::NExtract::NAskMode::kExtract)
		{
			for (UInt32 i = 0; i < _extractCount; ++i)
			{
				if (_extractIndices[i] == index)
				{
					PROPVARIANT pv = {};
					FILETIME ftCreate = { 0,0 };
					FILETIME ftAccess = { 0,0 };
					FILETIME ftWrite = { 0,0 };
					UInt64 ullSize = 0;

					if (SUCCEEDED(_archive->GetProperty(index, kpidCTime, &pv)) && (pv.vt == VT_FILETIME))
					{
						ftCreate.dwLowDateTime = pv.filetime.dwLowDateTime;
						ftCreate.dwHighDateTime = pv.filetime.dwHighDateTime;
					}
					if (SUCCEEDED(_archive->GetProperty(index, kpidATime, &pv)) && (pv.vt == VT_FILETIME))
					{
						ftAccess.dwLowDateTime = pv.filetime.dwLowDateTime;
						ftAccess.dwHighDateTime = pv.filetime.dwHighDateTime;
					}
					if (SUCCEEDED(_archive->GetProperty(index, kpidMTime, &pv)) && (pv.vt == VT_FILETIME))
					{
						ftWrite.dwLowDateTime = pv.filetime.dwLowDateTime;
						ftWrite.dwHighDateTime = pv.filetime.dwHighDateTime;
					}
					if (SUCCEEDED(_archive->GetProperty(index, kpidSize, &pv)))
					{
						ullSize = (pv.vt == VT_UI8) ? pv.uhVal.QuadPart : (pv.vt == VT_I8) ? pv.hVal.QuadPart : (pv.vt == VT_UI4) ? pv.uintVal : (pv.vt == VT_I4) ? pv.intVal : 0;
					}

					os = new CPanelSwLzmaOutStream();
					BextExitOnNull(os, hr, E_OUTOFMEMORY, "Failed to allocate file stream object");

					hr = os->Create(_extractPaths[i], ullSize, ftCreate, ftAccess, ftWrite);
					BextExitOnFailure(hr, "Failed to create file '%ls'", (const wchar_t*)_extractPaths[i]);

					outOs = os;
					os = nullptr;
					*outStream = outOs.Detach();

					_extractIndices[i] = INFINITE; // Allow the same index to be extracted to different targets
					break;
				}
			}
		}
	}

LExit:
	if (os)
	{
		delete os;
	}
	if (SUCCEEDED(hr) && _dwErrCount)
	{
		hr = E_FAIL;
	}

	return hr;
}

Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::PrepareOperation(Int32 askExtractMode))
{
	return _dwErrCount ? E_FAIL : S_OK;
}

Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::SetOperationResult(Int32 opRes))
{
	HRESULT hr = S_OK;
	LPWSTR szErr = nullptr;

	if (opRes != NArchive::NExtract::NOperationResult::kOK)
	{
		++_dwErrCount;
		hr = HRESULT_FROM_WIN32(opRes);

		OperationResultToString(opRes, &szErr);
		if (szErr && *szErr)
		{
			BextExitOnFailure(hr, "Failed to extract. %ls", szErr);
		}
		BextExitOnFailure(hr, "Failed to extract. Error code %i", opRes);
	}

LExit:
	ReleaseStr(szErr);

	return FAILED(hr) ? hr : _dwErrCount ? E_FAIL : S_OK;
}

Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::ReportExtractResult(UInt32 indexType, UInt32 index, Int32 opRes))
{
	HRESULT hr = S_OK;
	LPWSTR szErr = nullptr;

	if (opRes != NArchive::NExtract::NOperationResult::kOK)
	{
		++_dwErrCount;
		hr = HRESULT_FROM_WIN32(opRes);

		OperationResultToString(opRes, &szErr);

		if ((indexType == NArchive::NEventIndexType::kInArcIndex) && (index != (UInt32)(Int32)-1))
		{
			PROPVARIANT pv = {};
			if (SUCCEEDED(_archive->GetProperty(index, kpidPath, &pv)) &&  (pv.vt == VT_BSTR) && pv.bstrVal && *pv.bstrVal)
			{
				if (szErr && *szErr)
				{
					BextExitOnFailure(hr, "Failed to extract '%ls'. %ls", pv.bstrVal, szErr);
				}
				BextExitOnFailure(hr, "Failed to extract '%ls'. Error code %i", pv.bstrVal, opRes);
			}
		}
		if (szErr && *szErr)
		{
			BextExitOnFailure(hr, "Failed to extract #%u-#%u. %ls", indexType, index, szErr);
		}
		BextExitOnFailure(hr, "Failed to extract #%u-#%u. Error code %i", indexType, index, opRes);
	}

LExit:
	ReleaseStr(szErr);
	
	return FAILED(hr) ? hr : _dwErrCount ? E_FAIL : S_OK;
}

HRESULT CPanelSwLzmaExtractCallback::OperationResultToString(Int32 opRes, LPWSTR* psz) const
{
	HRESULT hr = S_OK;

	switch (opRes)
	{
	case NArchive::NExtract::NOperationResult::kOK:
		hr = StrAllocString(psz, L"Succedded", 0);
		break;
	case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
		hr = StrAllocString(psz, L"Unsupported method", 0);
		break;
	case NArchive::NExtract::NOperationResult::kDataError:
		hr = StrAllocString(psz, L"Data error", 0);
		break;
	case NArchive::NExtract::NOperationResult::kCRCError:
		hr = StrAllocString(psz, L"CRC error", 0);
		break;
	case NArchive::NExtract::NOperationResult::kUnavailable:
		hr = StrAllocString(psz, L"Unavailable", 0);
		break;
	case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
		hr = StrAllocString(psz, L"Unexpected end", 0);
		break;
	case NArchive::NExtract::NOperationResult::kDataAfterEnd:
		hr = StrAllocString(psz, L"Data after end", 0);
		break;
	case NArchive::NExtract::NOperationResult::kIsNotArc:
		hr = StrAllocString(psz, L"Not an archive", 0);
		break;
	case NArchive::NExtract::NOperationResult::kHeadersError:
		hr = StrAllocString(psz, L"Headers error", 0);
		break;
	case NArchive::NExtract::NOperationResult::kWrongPassword:
		hr = StrAllocString(psz, L"Wrong password", 0);
		break;
	default:
		hr = StrAllocFormatted(psz, L"Unknown error code (%i)", opRes);
		break;
	}

LExit:
	return hr;
}


Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::SetCompleted(const UInt64* total))
{
	return _dwErrCount ? E_FAIL : S_OK;
}

Z7_COM7F_IMF(CPanelSwLzmaExtractCallback::SetTotal(UInt64 total))
{
	return _dwErrCount ? E_FAIL : S_OK;
}

BOOL CPanelSwLzmaExtractCallback::HasErrors() const
{
	return _dwErrCount ? TRUE : FALSE;
}

#include "pch.h"
#include "lzma-sdk/CPP/Common/MyInitGuid.h"
#include "lzma-sdk/CPP/7zip/UI/Console/ExtractCallbackConsole.h"
#include "PanelSwLzmaContainer.h"
#include "PanelSwLzmaInStream.h"
#include "PanelSwLzmaExtractCallback.h"

extern bool g_IsNT;
bool g_IsNT = true;

extern class CStdOutStream* g_ErrStream;
class CStdOutStream* g_ErrStream = nullptr;

CPanelSwLzmaContainer::~CPanelSwLzmaContainer()
{
	Reset();
}

HRESULT CPanelSwLzmaContainer::Reset()
{
	_archive.reset();
	_codecs.reset();
	_inStream.Release();
	_extractIndices.reset();
	_extractPaths.reset();
	_entryCount = 0;
	_fileCount = 0;
	_extractCount = 0;

	_pxMappingsDoc.Release();

	ReleaseHandle(_hExtract);
	ReleaseHandle(_hEndExtract);
	ReleaseHandle(_hExtractThread);

	return S_OK;
}

HRESULT CPanelSwLzmaContainer::Init(LPCWSTR wzContainerId, HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize)
{
	HRESULT hr = S_OK;
	COpenOptions openOpts;
	DWORD dwMappingCount = 0;

	_archive.reset(new CArc());
	BextExitOnNull(_archive.get(), hr, E_OUTOFMEMORY, "Failed to allocate archive");

	_codecs.reset(new CCodecs());
	BextExitOnNull(_codecs.get(), hr, E_OUTOFMEMORY, "Failed to allocate codecs");

	hr = _codecs->Load();
	BextExitOnFailure(hr, "Failed to load codecs");

	CPanelSwLzmaInStream* inStream = new CPanelSwLzmaInStream();
	_inStream = inStream;
	BextExitOnNull(!!_inStream, hr, E_OUTOFMEMORY, "Failed to allocate stream");

	openOpts.codecs = _codecs.get();
	openOpts.stream = _inStream;
	openOpts.excludedFormats = new CIntVector();
	openOpts.props = new CObjectVector<CProperty>();

	hr = inStream->InitContainer(hBundle, qwContainerStartPos, qwContainerSize);
	BextExitOnFailure(hr, "Failed to open container");

	hr = _archive->OpenStream(openOpts);
	BextExitOnFailure(hr, "Failed to open container stream");
	BextExitOnNull(_archive->Archive, hr, E_FAIL, "Failed to initialize container");

	hr = _archive->Archive->GetNumberOfItems(&_entryCount);
	BextExitOnFailure(hr, "Failed to get container file count");

	hr = LoadMappings(&dwMappingCount);
	BextExitOnFailure(hr, "Failed to read mappings");

	_fileCount = _entryCount + dwMappingCount;
	_extractIndices.reset(new UInt32[_fileCount]);
	BextExitOnNull(_extractIndices.get(), hr, E_OUTOFMEMORY, "Failed to create file index array");

	_extractPaths.reset(new FString[_fileCount]);
	BextExitOnNull(_extractPaths.get(), hr, E_OUTOFMEMORY, "Failed to create file paths array");

	_extractCount = 0;

	_hExtract = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	BextExitOnNullWithLastError(_hExtract, hr, "Failed to create semaphore");

	_hEndExtract = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	BextExitOnNullWithLastError(_hEndExtract, hr, "Failed to create event");

LExit:
	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath)
{
	HRESULT hr = S_OK;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	hFile = ::CreateFile(wzFilePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	BextExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed to open container");

	hr = Init(wzContainerId, hFile, 0, CPanelSwLzmaInStream::INFINITE_CONTAINER_SIZE);
	BextExitOnFailure(hr, "Failed to open container '%ls'", wzFilePath);

	BextLog(BOOTSTRAPPER_EXTENSION_LOG_LEVEL_STANDARD, "Openned 7Z container '%ls'", wzFilePath);

LExit:
	ReleaseFileHandle(hFile);

	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerOpenAttached(LPCWSTR wzContainerId, HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize)
{
	HRESULT hr = S_OK;

	hr = Init(wzContainerId, hBundle, qwContainerStartPos, qwContainerSize);
	BextExitOnFailure(hr, "Failed to open container '%ls'", wzContainerId);

	BextLog(BOOTSTRAPPER_EXTENSION_LOG_LEVEL_STANDARD, "Openned 7Z attached container '%ls'", wzContainerId);

LExit:
	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerExtractFiles(DWORD cFiles, LPCWSTR* psczEmbeddedIds, LPCWSTR* psczTargetPaths)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	LPWSTR szMappingXPath = nullptr;
	DWORD dwWait = ERROR_SUCCESS;

	BextExitOnNull((cFiles <= _fileCount), hr, E_INVALIDARG, "Too many files requested to be extracted. Requested %u, have only %u", cFiles, _fileCount);

	_extractCount = 0;

	bRes = ::ResetEvent(_hExtract);
	BextExitOnNullWithLastError(bRes, hr, "Failed to reset event");

	bRes = ::ResetEvent(_hEndExtract);
	BextExitOnNullWithLastError(bRes, hr, "Failed to reset event");

	_hExtractThread = ::CreateThread(nullptr, 0, ExtractThreadProc, this, 0, nullptr);
	BextExitOnNullWithLastError(_hExtractThread, hr, "Failed to create thread");

	for (UInt32 i = 0; i < _entryCount; ++i)
	{
		PROPVARIANT pv = {};

		hr = _archive->Archive->GetProperty(i, kpidIsDir, &pv);
		BextExitOnFailure(hr, "Failed to get IsDir property for file %u", i);

		if ((pv.vt == VT_BOOL) && pv.boolVal)
		{
			continue;
		}

		hr = _archive->Archive->GetProperty(i, kpidPath, &pv);
		BextExitOnFailure(hr, "Failed to get Path property for file %u", i);
		BextExitOnNull(((pv.vt == VT_BSTR) && pv.bstrVal && *pv.bstrVal), hr, E_INVALIDDATA, "Failed to get Path property for file %u. Property type is %u", i, pv.vt);

		for (DWORD j = 0; j < cFiles; ++j)
		{
			if (::wcsicmp(pv.bstrVal, psczEmbeddedIds[j]) == 0)
			{
				_extractIndices[_extractCount] = i;
				_extractPaths[_extractCount] = psczTargetPaths[j];
				InterlockedIncrement(&_extractCount);

				bRes = ::SetEvent(_hExtract);
				BextExitOnNullWithLastError(bRes, hr, "Failed to set extract event");

				continue;
			}

			if (!!_pxMappingsDoc)
			{
				CComPtr<IXMLDOMNodeList> pxCurrFileMappings;
				long lXpathMatches = 0;

				hr = StrAllocFormatted(&szMappingXPath, L"/Root/Mapping[./@Source='%ls' and ./@Target='%ls']", pv.bstrVal, psczEmbeddedIds[j]);
				BextExitOnFailure(hr, "Failed to allocate xpath string");

				hr = _pxMappingsDoc->selectNodes(CComBSTR(szMappingXPath), &pxCurrFileMappings);
				BextExitOnFailure(hr, "Failed to read mappings for file '%ls'", pv.bstrVal);

				hr = pxCurrFileMappings->get_length(&lXpathMatches);
				BextExitOnFailure(hr, "Failed to get mappings count for file '%ls'", pv.bstrVal);

				if (lXpathMatches > 0)
				{
					_extractIndices[_extractCount] = i;
					_extractPaths[_extractCount] = psczTargetPaths[j];
					InterlockedIncrement(&_extractCount);

					bRes = ::SetEvent(_hExtract);
					BextExitOnNullWithLastError(bRes, hr, "Failed to set extract event");
				}
			}
		}
	}

	bRes = ::SetEvent(_hEndExtract);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set event");

	dwWait = ::WaitForSingleObject(_hExtractThread, INFINITE);
	BextExitOnNullWithLastError((dwWait == WAIT_OBJECT_0), hr, "Failed to wait for extract thread to terminate");

	BextExitOnNull((_extractCount == cFiles), hr, E_NOTFOUND, "Failed to find some files in container. Found %u/%u files", _extractCount, cFiles);

LExit:
	::SetEvent(_hEndExtract); // Just to make sure the thread will terminated if we had an errror
	ReleaseStr(szMappingXPath);

	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerStreamToFileNow(UInt32 nFileIndex, LPCWSTR wzFileName)
{
	HRESULT hr = S_OK;
	FString targetFile;
	CMyComPtr<IArchiveExtractCallback> extractClbk;
	CPanelSwLzmaExtractCallback* pExtractCallback = nullptr;

	targetFile = wzFileName;

	pExtractCallback = new CPanelSwLzmaExtractCallback();
	BextExitOnNull(pExtractCallback, hr, E_OUTOFMEMORY, "Failed to allocate extract callback object");
	extractClbk = pExtractCallback;

	hr = pExtractCallback->Init(_archive->Archive, 1, &nFileIndex, &targetFile);
	BextExitOnFailure(hr, "Failed to initialize extract callbck");

	hr = _archive->Archive->Extract(&nFileIndex, 1, 0, extractClbk);
	BextExitOnFailure(hr, "Failed to extract '%ls'", wzFileName);
	BextExitOnNull(!pExtractCallback->HasErrors(), hr, E_FAIL, "Failed to extract '%ls'", wzFileName);

LExit:
	return S_OK;
}

HRESULT CPanelSwLzmaContainer::ContainerClose()
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	DWORD dwWait = ERROR_SUCCESS;

	if (_hEndExtract && _hExtractThread)
	{
		bRes = ::SetEvent(_hEndExtract);
		BextExitOnNullWithLastError(bRes, hr, "Failed to set event");

		dwWait = ::WaitForSingleObject(_hExtractThread, INFINITE);
		BextExitOnNullWithLastError((dwWait == WAIT_OBJECT_0), hr, "Failed to wait for extract thread to terminate");
	}

LExit:
	Reset();
	return hr;
}

HRESULT CPanelSwLzmaContainer::LoadMappings(LPDWORD pdwMappingCount)
{
	HRESULT hr = S_OK;
	LPWSTR szXmlFile = nullptr;
	CComPtr<IXMLDOMNodeList> pxCurrFileMappings;
	UInt32 uiMappingFile = _entryCount + 1;
	long lMappingCount = 0;
	*pdwMappingCount = 0;

	for (UInt32 i = 0; i < _entryCount; ++i)
	{
		PROPVARIANT pv = {};

		hr = _archive->Archive->GetProperty(i, kpidIsDir, &pv);
		BextExitOnFailure(hr, "Failed to get IsDir property for file %u", i);

		if ((pv.vt == VT_BOOL) && pv.boolVal)
		{
			continue;
		}

		hr = _archive->Archive->GetProperty(i, kpidPath, &pv);
		BextExitOnFailure(hr, "Failed to get Path property for file %u", i);
		BextExitOnNull(((pv.vt == VT_BSTR) && pv.bstrVal && *pv.bstrVal), hr, E_INVALIDDATA, "Failed to get Path property for file %u. Property type is %u", i, pv.vt);
		
		if (::wcsicmp(pv.bstrVal, MAPPINGS_FILE_NAME) == 0)
		{
			uiMappingFile = i;
			break;
		}
	}
	if (uiMappingFile >= _entryCount)
	{
		hr = S_FALSE;
		ExitFunction();
	}

	hr = FileCreateTemp(L"CNTNR", L"xml", &szXmlFile, nullptr);
	BextExitOnFailure(hr, "Failed to load mappings");

	hr = ContainerStreamToFileNow(uiMappingFile, szXmlFile);
	BextExitOnFailure(hr, "Failed to extract mappings file '%ls'", szXmlFile);

	hr = XmlLoadDocumentFromFile(szXmlFile, &_pxMappingsDoc);
	BextExitOnFailure(hr, "Failed to load mappings from file '%ls'", szXmlFile);

	hr = _pxMappingsDoc->selectNodes(CComBSTR(L"/Root/Mapping"), &pxCurrFileMappings);
	BextExitOnFailure(hr, "Failed to select mappings");

	hr = pxCurrFileMappings->get_length(&lMappingCount);
	BextExitOnFailure(hr, "Failed to get mapping count");
	*pdwMappingCount = lMappingCount;

LExit:
	if (szXmlFile && *szXmlFile)
	{
		FileEnsureDelete(szXmlFile);
		ReleaseStr(szXmlFile);
	}

	return hr;
}

/*static*/ DWORD WINAPI CPanelSwLzmaContainer::ExtractThreadProc(LPVOID lpParameter)
{
	HRESULT hr = S_OK;
	CPanelSwLzmaContainer* pThis = (CPanelSwLzmaContainer*)lpParameter;
	CMyComPtr<IArchiveExtractCallback> extractClbk;
	CPanelSwLzmaExtractCallback* pExtractCallback = nullptr;
	HANDLE hWaits[2] = { pThis->_hExtract, pThis->_hEndExtract };
	DWORD dwPrevExtractCount = 0;

	pExtractCallback = new CPanelSwLzmaExtractCallback();
	BextExitOnNull(pExtractCallback, hr, E_OUTOFMEMORY, "Failed to allocate extract callback object");
	extractClbk = pExtractCallback;

	while (true)
	{
		DWORD dwExtractCount = 0;
		DWORD dwOverallExtractCount = 0;

		// If both handles are set then the returned value is in index order (that is, 0). That ensures all files will be extracted before terminating.
		DWORD dwWait = ::WaitForMultipleObjects(2, hWaits, FALSE, INFINITE);
		switch (dwWait)
		{
		case WAIT_OBJECT_0: // Files are ready
			// Determine the number of files that can be extracted
			dwOverallExtractCount = pThis->_extractCount; // _extractCount may change so we're getting it once and won't get it again in this iteration
			dwExtractCount = dwOverallExtractCount - dwPrevExtractCount;
			break;
		case WAIT_OBJECT_0 + 1: // Terminate
			ExitFunction();
		default:
			hr = E_INVALIDSTATE;
			BextExitOnFailure(hr, "Unexpected wait result %u", dwWait);
			break;
		}

		if (dwExtractCount)
		{
			const UInt32* pIndices = pThis->_extractIndices.get() + dwPrevExtractCount;
			const FString* pPaths = pThis->_extractPaths.get() + dwPrevExtractCount;

			hr = pExtractCallback->Init(pThis->_archive->Archive, dwExtractCount, pIndices, pPaths);
			BextExitOnFailure(hr, "Failed to initialize extract callbck");

			hr = pThis->_archive->Archive->Extract(pIndices, dwExtractCount, 0, extractClbk);
			BextExitOnFailure(hr, "Failed to extract files");
			BextExitOnNull(!pExtractCallback->HasErrors(), hr, E_FAIL, "Failed to extract files");

			dwPrevExtractCount = dwOverallExtractCount;
		}
	}

LExit:
	return 0;
}

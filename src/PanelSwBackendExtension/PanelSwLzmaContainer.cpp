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
	_codecs.reset();
	_inStream.Release();
	_extractIndices.reset();
	_extractPaths.reset();
	_entryCount = 0;
	_fileCount = 0;
	_extractCount = 0;
	_fileIndex = -1;

	_nCurrMappingIndex = -1;
	_pxCurrFileMappings.Release();
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

	inStream->InitContainer(hBundle, qwContainerStartPos, qwContainerSize);
	BextExitOnFailure(hr, "Failed to open container stream");

	openOpts.codecs = _codecs.get();
	openOpts.stream = _inStream;
	openOpts.excludedFormats = new CIntVector();
	openOpts.props = new CObjectVector<CProperty>();
	
	hr = _archive->OpenStream(openOpts);
	BextExitOnFailure(hr, "Failed to open container");
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

	_hExtractThread = ::CreateThread(nullptr, 0, ExtractThreadProc, this, 0, nullptr);
	BextExitOnNullWithLastError(_hExtractThread, hr, "Failed to create thread");

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
	hFile = INVALID_HANDLE_VALUE;

	BextLog(BUNDLE_EXTENSION_LOG_LEVEL_STANDARD, "Openned 7Z container '%ls'", wzFilePath);

LExit:
	ReleaseFileHandle(hFile);

	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerOpenAttached(LPCWSTR wzContainerId, HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize)
{
	HRESULT hr = S_OK;

	hr = Init(wzContainerId, hBundle, qwContainerStartPos, qwContainerSize);
	BextExitOnFailure(hr, "Failed to open container '%ls'", wzContainerId);

	BextLog(BUNDLE_EXTENSION_LOG_LEVEL_STANDARD, "Openned 7Z attached container '%ls'", wzContainerId);

LExit:
	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerNextStream(BSTR* psczStreamName)
{
	HRESULT hr = S_OK;
	size_t cchName = 0;
	PROPVARIANT pv = {};

	if (psczStreamName && *psczStreamName)
	{
		::SysFreeString(*psczStreamName);
		*psczStreamName = nullptr;
	}

	hr = GetNextMapping(psczStreamName);
	if (SUCCEEDED(hr) && psczStreamName && *psczStreamName)
	{
		ExitFunction();
	}
	if (hr == E_NOMOREITEMS)
	{
		hr = S_OK;
	}
	BextExitOnFailure(hr, "Failed to get next entry mapping");

	do
	{
		++_fileIndex;

		hr = _archive->Archive->GetProperty(_fileIndex, kpidIsDir, &pv);
		BextExitOnFailure(hr, "Failed to get IsDir property for file %u", _fileIndex);

		// File -> stop.
		if ((pv.vt == VT_BOOL) && !pv.boolVal)
		{
			break;
		}
	} while (_fileIndex < _entryCount);
	if (_fileIndex >= _entryCount)
	{
		hr = E_NOMOREITEMS;
		ExitFunction();
	}

	hr = _archive->Archive->GetProperty(_fileIndex, kpidPath, &pv);
	BextExitOnFailure(hr, "Failed to get Path property for file %u", _fileIndex);
	BextExitOnNull(((pv.vt == VT_BSTR) && pv.bstrVal && *pv.bstrVal), hr, E_INVALIDDATA, "Failed to get Path property for file %u. Property type is %u", _fileIndex, pv.vt);

	hr = ReadFileMappings(pv.bstrVal);
	BextExitOnFailure(hr, "Failed to read mappings for entry '%ls'", pv.bstrVal);

	if (psczStreamName)
	{
		*psczStreamName = ::SysAllocString(pv.bstrVal);
		BextExitOnNull(*psczStreamName, hr, E_FAIL, "Failed to allocate sys string");
	}

LExit:
	return hr;
}

HRESULT CPanelSwLzmaContainer::ContainerStreamToFile(LPCWSTR wzFileName)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	BextExitOnNull((_fileIndex < _entryCount), hr, E_INVALIDSTATE, "7z container is exhausted, can't extract '%ls'", wzFileName);

	// Creating a file for placeholder
	hFile = ::CreateFile(wzFileName, FILE_ALL_ACCESS, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	BextExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed to create file '%ls'", wzFileName);

	_extractIndices[_extractCount] = _fileIndex;
	_extractPaths[_extractCount] = wzFileName;
	::InterlockedIncrement(&_extractCount);
	
	bRes = ::SetEvent(_hExtract);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set extract event");

LExit:
	ReleaseFile(hFile);

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
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	DWORD dwWait = ERROR_SUCCESS;

	bRes = ::SetEvent(_hEndExtract);
	BextExitOnNullWithLastError(bRes, hr, "Failed to set event");

	dwWait = ::WaitForSingleObject(_hExtractThread, INFINITE);
	BextExitOnNullWithLastError((dwWait == WAIT_OBJECT_0), hr, "Failed to wait for extract thread to terminate");

LExit:
	Reset();
	return hr;
}

HRESULT CPanelSwLzmaContainer::LoadMappings(LPDWORD pdwMappingCount)
{
	HRESULT hr = S_OK;
	LPWSTR szXmlFile = nullptr;
	CComBSTR szEntryName;
	CComPtr<IXMLDOMNodeList> pxCurrFileMappings;
	long lMappingCount = 0;
	*pdwMappingCount = 0;

	while ((hr = ContainerNextStream(&szEntryName)) != E_NOMOREITEMS)
	{
		BextExitOnFailure(hr, "Failed to get next entry");

		if (::wcscmp((BSTR)szEntryName, MAPPINGS_FILE_NAME) == 0)
		{
			break;
		}
	}
	if (hr == E_NOMOREITEMS)
	{
		hr = S_FALSE;
		ExitFunction();
	}

	hr = FileCreateTemp(L"CNTNR", L"xml", &szXmlFile, nullptr);
	BextExitOnFailure(hr, "Failed to load mappings");

	hr = ContainerStreamToFileNow(_fileIndex, szXmlFile);
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
	_fileIndex = -1;

	return hr;
}

HRESULT CPanelSwLzmaContainer::ReadFileMappings(LPCWSTR szEntryName)
{
	HRESULT hr = S_OK;
	LPWSTR szXpath = nullptr;

	if (!_pxMappingsDoc)
	{
		ExitFunction();
	}

	hr = StrAllocFormatted(&szXpath, L"/Root/Mapping[./@Source='%ls']/@Target", szEntryName);
	BextExitOnFailure(hr, "Failed to allocate string");

	hr = _pxMappingsDoc->selectNodes(CComBSTR(szXpath), &_pxCurrFileMappings);
	BextExitOnFailure(hr, "Failed to read mappings for file '%ls'", szEntryName);

LExit:
	ReleaseStr(szXpath);
	_nCurrMappingIndex = -1;

	return hr;
}

HRESULT CPanelSwLzmaContainer::GetNextMapping(BSTR* psczStreamName)
{
	HRESULT hr = S_OK;
	long lNodeCount = -1;
	CComPtr<IXMLDOMNode> pNode;
	CComVariant nodeValue;
	CComBSTR result(L"");

	if (!_pxMappingsDoc || !_pxCurrFileMappings)
	{
		ExitFunction();
	}

	hr = _pxCurrFileMappings->get_length(&lNodeCount);
	BextExitOnFailure(hr, "Failed to get mappings count");

	++_nCurrMappingIndex;
	if (_nCurrMappingIndex >= lNodeCount)
	{
		hr = E_NOMOREITEMS;
		ExitFunction();
	}

	hr = _pxCurrFileMappings->get_item(_nCurrMappingIndex, &pNode);
	BextExitOnFailure(hr, "Failed to get next mapping");

	hr = pNode->get_nodeValue(&nodeValue);
	ExitOnFailure(hr, "Failed to get mapping value.");

	hr = nodeValue.ChangeType(VT_BSTR);
	ExitOnFailure(hr, "Failed to get mapping value as string.");

	result = nodeValue.bstrVal;
	*psczStreamName = result.Detach();

LExit:
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
			UInt32* pIndices = pThis->_extractIndices.get() + dwPrevExtractCount;
			FString* pPaths = pThis->_extractPaths.get() + dwPrevExtractCount;

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

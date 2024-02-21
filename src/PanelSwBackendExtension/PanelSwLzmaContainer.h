#pragma once
#include "pch.h"
#include "lzma-sdk/CPP/7zip/UI/Common/LoadCodecs.h"
#include "lzma-sdk/CPP/7zip/UI/Common/OpenArchive.h"
#include <memory>
#include <list>

class CPanelSwLzmaContainer : public IPanelSwContainer
{
public:

	~CPanelSwLzmaContainer();

	HRESULT ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath) override;

	HRESULT ContainerOpenAttached(LPCWSTR wzContainerId, HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize) override;

	HRESULT ContainerNextStream(BSTR* psczStreamName) override;

	HRESULT ContainerStreamToFile(LPCWSTR wzFileName) override;

	HRESULT ContainerStreamToBuffer(BYTE** ppbBuffer, SIZE_T* pcbBuffer) override;

	HRESULT ContainerSkipStream() override;

	HRESULT ContainerClose() override;

private:

	HRESULT Init(LPCWSTR wzContainerId, HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize);
	HRESULT Reset();

	HRESULT LoadMappings(LPDWORD pdwMappingCount);
	HRESULT ReadFileMappings(LPCWSTR szEntryName);
	HRESULT GetNextMapping(BSTR* psczStreamName);

	HRESULT ContainerStreamToFileNow(UInt32 nFileIndex, LPCWSTR wzFileName);

	// Mappings (same compressed file has several target names)
	CComPtr<IXMLDOMDocument> _pxMappingsDoc;
	CComPtr<IXMLDOMNodeList> _pxCurrFileMappings;
	size_t _nCurrMappingIndex = -1;
	const LPCWSTR MAPPINGS_FILE_NAME = L"PanelSwWixContainer";

	// LZMA
	std::unique_ptr<CArc> _archive;
	std::unique_ptr<CCodecs> _codecs;
	CMyComPtr<IInStream> _inStream;
	size_t _fileIndex = -1;

	std::unique_ptr<UInt32[]> _extractIndices;
	std::unique_ptr<FString[]> _extractPaths;
	UInt32 _entryCount = 0; // 7z file count
	UInt32 _fileCount = 0; // File count including mappings
	UInt32 _extractCount = 0;

	// Extraction thread
	static DWORD WINAPI ExtractThreadProc(LPVOID lpParameter);

	HANDLE _hExtract = NULL;
	HANDLE _hEndExtract = NULL;
	HANDLE _hExtractThread = NULL;
};

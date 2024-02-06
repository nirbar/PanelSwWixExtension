#pragma once
#include "pch.h"
#include "7zC/CpuArch.h"
#include "7zC/7z.h"
#include "7zC/7zAlloc.h"
#include "7zC/7zBuf.h"
#include "7zC/7zCrc.h"
#include "7zC/7zFile.h"
#include "7zC/7zVersion.h"
#include <memory>

class CPanelSwLzmaContainer : public IPanelSwContainer
{
public:

	~CPanelSwLzmaContainer();

	HRESULT ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath) override;

	HRESULT ContainerNextStream(BSTR* psczStreamName) override;

	HRESULT ContainerStreamToFile(LPCWSTR wzFileName) override;

	HRESULT ContainerStreamToBuffer(BYTE** ppbBuffer, SIZE_T* pcbBuffer) override;

	HRESULT ContainerSkipStream() override;

	HRESULT ContainerClose() override;

private:

	HRESULT Reset();

	HRESULT LoadMappings();
	HRESULT ReadFileMappings(LPCWSTR szEntryName);
	HRESULT GetNextMapping(BSTR* psczStreamName);

	std::auto_ptr<CFileInStream> _archiveStream;
	std::auto_ptr<CLookToRead2> _lookStream;
	std::auto_ptr<CSzArEx> _db;
	size_t _fileIndex = -1;
	UInt32 _blockIndex = ~0;
	LPBYTE _outBuffer = nullptr;
	size_t _outBufferSize = 0;

	const size_t _kInputBufSize = ((size_t)1 << 18);
	const ISzAlloc _alloc = { SzAlloc, SzFree };
	const ISzAlloc _allocTemp = { SzAllocTemp, SzFreeTemp };

	CComPtr<IXMLDOMDocument> _pxMappingsDoc;
	CComPtr<IXMLDOMNodeList> _pxCurrFileMappings;
	size_t _nCurrMappingIndex = -1;
	const LPCWSTR MAPPINGS_FILE_NAME = L"PanelSwWixContainer";
};

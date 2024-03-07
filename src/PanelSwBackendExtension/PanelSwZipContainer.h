#pragma once
#include "pch.h"
#include "..\poco\Zip\include\Poco\Zip\ZipArchive.h"
#include <memory>
#include <string>

class CPanelSwZipContainer : public IPanelSwContainer
{
public:

	virtual ~CPanelSwZipContainer();

	HRESULT ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath) override;

	HRESULT ContainerOpenAttached(LPCWSTR wzContainerId, HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize) override;

	HRESULT ContainerNextStream(BSTR* psczStreamName) override;

	HRESULT ContainerStreamToFile(LPCWSTR wzFileName) override;

	HRESULT ContainerStreamToBuffer(BYTE** ppbBuffer, SIZE_T* pcbBuffer) override;

	HRESULT ContainerSkipStream() override;

	HRESULT ContainerClose() override;

private:

	HRESULT Reset();

	std::auto_ptr<std::istream> _zipStream;
	std::auto_ptr<Poco::Zip::ZipArchive> _pArchive;
	std::string _currFile;
};

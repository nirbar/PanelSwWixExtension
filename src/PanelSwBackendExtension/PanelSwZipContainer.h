#pragma once
#include "pch.h"
#include "..\poco\Zip\include\Poco\Zip\ZipArchive.h"
#include <memory>
#include <string>

class CPanelSwZipContainer : public IPanelSwContainer
{
public:

	~CPanelSwZipContainer();

	HRESULT ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath) override;

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

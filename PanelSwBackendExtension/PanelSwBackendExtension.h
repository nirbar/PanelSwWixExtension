#pragma once

#include <BextBaseBundleExtension.h>
#include "..\poco\Zip\include\Poco\Zip\ZipArchive.h"
#include <memory>
#include <string>

class CPanelSwBundleExtension : public CBextBaseBundleExtension
{
public:
	CPanelSwBundleExtension(IBundleExtensionEngine* pEngine);

	~CPanelSwBundleExtension();

	STDMETHOD(ContainerOpen)(LPCWSTR wzContainerId, LPCWSTR wzFilePath, LPVOID* pContext) override;

	STDMETHOD(ContainerNextStream)(LPVOID pContext, LPWSTR* psczStreamName) override;

	STDMETHOD(ContainerStreamToFile)(LPVOID pContext, LPCWSTR wzFileName) override;

	STDMETHOD(ContainerStreamToBuffer)(LPVOID pContext, BYTE** ppbBuffer, SIZE_T* pcbBuffer) override;

	STDMETHOD(ContainerSkipStream)(LPVOID pContext) override;

	// Don't forget to release everything in the context
	STDMETHOD(ContainerClose)(LPVOID pContext) override;

private:

	HRESULT Reset();

	std::auto_ptr<std::istream> _zipStream;
	std::auto_ptr<Poco::Zip::ZipArchive> _pArchive;
	std::string _currFile;
};

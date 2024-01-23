#pragma once

#include "pch.h"
#include <list>

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

	HRESULT GetContainer(LPVOID pContext, IPanelSwContainer** ppContainer);
	HRESULT ReleaseContainer(IPanelSwContainer* pContainer);
	HRESULT Reset();

	std::list<IPanelSwContainer*> _containers;
	typedef std::list<IPanelSwContainer*>::iterator ContainerIterator;
};

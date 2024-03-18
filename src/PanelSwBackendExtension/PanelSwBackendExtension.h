#pragma once

#include "pch.h"
#include <list>

class CPanelSwBundleExtension : public CBextBaseBootstrapperExtension
{
public:
	CPanelSwBundleExtension(IBootstrapperExtensionEngine* pEngine);

	virtual ~CPanelSwBundleExtension();

	STDMETHOD(Search)(LPCWSTR wzId, LPCWSTR wzVariable) override;

#ifdef EnableZipContainer
	STDMETHOD(ContainerOpen)(LPCWSTR wzContainerId, LPCWSTR wzFilePath, LPVOID* pContext) override;

	STDMETHOD(ContainerOpenAttached)(LPCWSTR wzContainerId, HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize, LPVOID* ppContext) override;

	STDMETHOD(ContainerNextStream)(LPVOID pContext, LPWSTR* psczStreamName) override;

	STDMETHOD(ContainerStreamToFile)(LPVOID pContext, LPCWSTR wzFileName) override;

	STDMETHOD(ContainerStreamToBuffer)(LPVOID pContext, BYTE** ppbBuffer, SIZE_T* pcbBuffer) override;

	STDMETHOD(ContainerSkipStream)(LPVOID pContext) override;

	// Don't forget to release everything in the context
	STDMETHOD(ContainerClose)(LPVOID pContext) override;
#endif

private:

	HRESULT CreateContainer(LPCWSTR wzContainerId, IPanelSwContainer** ppContainer);
	HRESULT GetContainer(LPVOID pContext, IPanelSwContainer** ppContainer);
	HRESULT ReleaseContainer(IPanelSwContainer* pContainer);
	HRESULT Reset();

	std::list<IPanelSwContainer*> _containers;
	typedef std::list<IPanelSwContainer*>::iterator ContainerIterator;
};

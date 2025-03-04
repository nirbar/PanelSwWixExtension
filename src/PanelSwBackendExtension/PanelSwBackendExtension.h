#pragma once

#include "pch.h"
#include "PanelSwBundleVariables.h"
#include <list>

class CPanelSwBundleExtension : public CBextBaseBootstrapperExtension
{
public:
	CPanelSwBundleExtension(IBootstrapperExtensionEngine* pEngine);

	virtual ~CPanelSwBundleExtension();

	STDMETHOD(Initialize)(const BOOTSTRAPPER_EXTENSION_CREATE_ARGS* pCreateArgs) override;

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

	struct BUNDLE_VARIABLE_SEARCH
	{
		LPWSTR _szId = nullptr;
		LPWSTR _szUpgradeCode = nullptr;
		LPWSTR _szSearchVariable = nullptr;
		BOOL _bFormat = TRUE;
	};

	HRESULT ParseSearches(IXMLDOMNode* pixnBundleExtension);
	HRESULT SearchBundleVariable(LPCWSTR szUpgradeCode, LPCWSTR szVariableName, BOOL bFormat, LPCWSTR szResultVariableName);

	HRESULT CreateContainer(LPCWSTR wzContainerId, IPanelSwContainer** ppContainer);
	HRESULT GetContainer(LPVOID pContext, IPanelSwContainer** ppContainer);
	HRESULT ReleaseContainer(IPanelSwContainer* pContainer);
	
	HRESULT Reset();

	std::list<IPanelSwContainer*> _containers;
	typedef std::list<IPanelSwContainer*>::iterator ContainerIterator;
	BUNDLE_VARIABLE_SEARCH* _pSearches = nullptr;
	long _cSearches = 0;
	CPanelSwBundleVariables _bundles;
};

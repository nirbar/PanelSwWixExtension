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

	enum PANELSW_SEARCH_TYPE
	{
		PANELSW_SEARCH_TYPE_UNKNOWN,
		PANELSW_SEARCH_TYPE_BUNDLE_VARIABLE,
		PANELSW_SEARCH_TYPE_ARP_ENTRY,
	};

	typedef struct _PANELSW_SEARCH
	{
		LPWSTR szId;
		PANELSW_SEARCH_TYPE type;

		union
		{
			struct
			{
				LPWSTR szUpgradeCode;
				LPWSTR szSearchVariable;
				BOOL bFormat;
			} BundleVariable;
			
			struct
			{
				LPWSTR szArpSpec;
				REG_KEY_BITNESS bitness;
				BOOL bUserContext;
			} ArpEntry;
		};
	} PANELSW_SEARCH;

	HRESULT ParseSearches(IXMLDOMNode* pixnBundleExtension);
	HRESULT SearchBundleVariable(PANELSW_SEARCH *pSearch, LPWSTR *pszValue);
	HRESULT SearchArpEntry(PANELSW_SEARCH* pSearch, LPWSTR* pszValue);
	HRESULT IsOnCommandLine(LPCWSTR szVariableName);
	HRESULT FormatString(LPCWSTR szFormat, LPWSTR* pszValue);

	HRESULT CreateContainer(LPCWSTR wzContainerId, IPanelSwContainer** ppContainer);
	HRESULT GetContainer(LPVOID pContext, IPanelSwContainer** ppContainer);
	HRESULT ReleaseContainer(IPanelSwContainer* pContainer);
	
	HRESULT Reset();

	std::list<IPanelSwContainer*> _containers;
	typedef std::list<IPanelSwContainer*>::iterator ContainerIterator;
	PANELSW_SEARCH* _pSearches = nullptr;
	long _cSearches = 0;
	CPanelSwBundleVariables _bundles;
	BAL_INFO_BUNDLE _bundleInfo = {};
	LPWSTR* _pszCommandLineArgs = nullptr;
	int _cCommandLineArgs = 0;
};

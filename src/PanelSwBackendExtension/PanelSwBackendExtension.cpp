#include "pch.h"
#include "PanelSwBackendExtension.h"
#include "PanelSwZipContainer.h"
#include "PanelSwLzmaContainer.h"
#include <BextBaseBootstrapperExtensionProc.h>
using namespace std;
extern HINSTANCE g_hInstance;

CPanelSwBundleExtension::CPanelSwBundleExtension(IBootstrapperExtensionEngine* pEngine)
	: CBextBaseBootstrapperExtension(pEngine)
	, _bundles(pEngine)
{
	XmlInitialize();
}

CPanelSwBundleExtension::~CPanelSwBundleExtension()
{
	Reset();
	XmlUninitialize();
}

STDMETHODIMP CPanelSwBundleExtension::Initialize(const BOOTSTRAPPER_EXTENSION_CREATE_ARGS* pCreateArgs)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMDocument> pixdBextManifest;
	CComPtr<IXMLDOMDocument> pixdBadManifest;
	CComPtr<IXMLDOMNode> pixnBundleExtension;

	hr = __super::Initialize(pCreateArgs);
	BextExitOnFailure(hr, "CBextBaseBootstrapperExtension initialization failed.");

	hr = BalManifestLoad((HMODULE)g_hInstance, &pixdBadManifest);
	BextExitOnFailure(hr, "Failed to load bootstrapper application manifest.");

	hr = BalInfoParseFromXml(&_bundleInfo, pixdBadManifest);
	BextExitOnFailure(hr, "Failed to load bundle information.");

	hr = AppParseCommandLine(::GetCommandLineW(), &_cCommandLineArgs, &_pszCommandLineArgs);
	BextExitOnFailure(hr, "Failed to parse command line.");

	hr = XmlLoadDocumentFromFile(m_sczBootstrapperExtensionDataPath, &pixdBextManifest);
	BextExitOnFailure(hr, "Failed to load bundle extension manifest from path: %ls", m_sczBootstrapperExtensionDataPath);

	hr = BextGetBootstrapperExtensionDataNode(pixdBextManifest, PANELSW_BACKEND_EXTENSION_ID, &pixnBundleExtension);
	BextExitOnFailure(hr, "Failed to get BundleExtensionData entry for '%ls'", PANELSW_BACKEND_EXTENSION_ID);

	hr = ParseSearches(pixnBundleExtension);
	BextExitOnFailure(hr, "Failed to parse searches from bundle extension manifest.");

LExit:

	return hr;
}

HRESULT CPanelSwBundleExtension::ParseSearches(IXMLDOMNode* pixnBundleExtension)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMNodeList> pixnNodes;

	// Select BundleVariableSearch nodes.
	hr = XmlSelectNodes(pixnBundleExtension, L"PSW_BundleVariableSearch|PSW_ArpEntrySearch", &pixnNodes);
	BextExitOnFailure(hr, "Failed to select search nodes.");

	hr = pixnNodes->get_length(&_cSearches);
	BextExitOnFailure(hr, "Failed to get search node count.");

	hr = MemAllocArray((void**)&_pSearches, sizeof(PANELSW_SEARCH), _cSearches);
	BextExitOnFailure(hr, "Failed to allocate memory.");

	for (long i = 0; i < _cSearches; ++i)
	{
		CComPtr<IXMLDOMNode> pixnSearch;
		PANELSW_SEARCH* pSearch = _pSearches + i;
		CComBSTR sz;

		hr = pixnNodes->get_item(i, &pixnSearch);
		BextExitOnFailure(hr, "Failed to get next search node.");

		hr = pixnSearch->get_nodeName(&sz);
		BextExitOnFailure(hr, "Failed to get search node name.");

		hr = XmlGetAttributeEx(pixnSearch, L"Id", &pSearch->szId);
		BextExitOnFailure(hr, "Failed to get @Id.");

		if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, sz, -1, L"PSW_BundleVariableSearch", -1))
		{
			pSearch->type = PANELSW_SEARCH_TYPE_BUNDLE_VARIABLE;

			hr = XmlGetAttributeEx(pixnSearch, L"UpgradeCode", &pSearch->BundleVariable.szUpgradeCode);
			BextExitOnFailure(hr, "Failed to get @UpgradeCode.");

			hr = XmlGetAttributeEx(pixnSearch, L"SearchVariable", &pSearch->BundleVariable.szSearchVariable);
			BextExitOnFailure(hr, "Failed to get @VariableName.");

			hr = XmlGetAttributeNumber(pixnSearch, L"Format", (DWORD*)&pSearch->BundleVariable.bFormat);
			BextExitOnFailure(hr, "Failed to get @Format.");
		}
		else if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, sz, -1, L"PSW_ArpEntrySearch", -1))
		{
			pSearch->type = PANELSW_SEARCH_TYPE_ARP_ENTRY;

			hr = XmlGetAttributeEx(pixnSearch, L"ArpSpec", &pSearch->ArpEntry.szArpSpec);
			BextExitOnFailure(hr, "Failed to get @ArpSpec.");

			hr = XmlGetAttributeNumber(pixnSearch, L"Bitness", (DWORD*)&pSearch->ArpEntry.bitness);
			BextExitOnFailure(hr, "Failed to get @Bitness.");

			hr = XmlGetAttributeNumber(pixnSearch, L"IsUserContext", (DWORD*)&pSearch->ArpEntry.bUserContext);
			BextExitOnFailure(hr, "Failed to get @IsUserContext.");
		}
		else
		{
			hr = E_NOT_VALID_STATE;
			BextExitOnFailure(hr, "Illegal search type '%ls' for '%ls'", sz.m_str, _pSearches[i].szId);
		}
	}

LExit:

	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::Search(LPCWSTR wzId, LPCWSTR wzVariable)
{
	HRESULT hr = S_OK;
	LPWSTR szValue = nullptr;
	LPWSTR szTemp = nullptr;
	VERUTIL_VERSION* pVersion = nullptr;
	long lVal = 0;

	hr = IsOnCommandLine(wzVariable);
	BextExitOnFailure(hr, "Failed to check whether or not variable '%ls' is on the command line", wzVariable);

	if (hr == S_OK)
	{
		BextLog(BOOTSTRAPPER_EXTENSION_LOG_LEVEL_STANDARD, "Skipping search for variable '%ls' because it was set on the command line", wzVariable);
		ExitFunction();
	}

	for (long i = 0; i < _cSearches; ++i)
	{
		if (CSTR_EQUAL != ::CompareStringW(LOCALE_INVARIANT, 0, _pSearches[i].szId, -1, wzId, -1))
		{
			continue;
		}

		switch (_pSearches[i].type)
		{
		case PANELSW_SEARCH_TYPE_BUNDLE_VARIABLE:
			hr = SearchBundleVariable(&_pSearches[i], &szValue);
			BextExitOnFailure(hr, "Failed to search for variable '%ls' in bundles with upgrade code '%ls'", _pSearches[i].BundleVariable.szSearchVariable, _pSearches[i].BundleVariable.szUpgradeCode);
			break;
		case PANELSW_SEARCH_TYPE_ARP_ENTRY:
			hr = SearchArpEntry(&_pSearches[i], &szValue);
			BextExitOnFailure(hr, "Failed to search for ARP entry '%ls'", _pSearches[i].ArpEntry.szArpSpec);
			break;
		default:
			hr = E_NOT_VALID_STATE;
			BextExitOnFailure(hr, "Illegal search type %u for '%ls'", _pSearches[i].type, _pSearches[i].szId);
			break;
		}

		if (hr == S_FALSE)
		{
			ExitFunction();
		}
		if (!szValue || !*szValue)
		{
			hr = m_pEngine->SetVariableString(wzVariable, L"", FALSE);
			BextExitOnFailure(hr, "Failed to clear variable");
			ExitFunction();
		}

		lVal = wcstol(szValue, &szTemp, 10);
		if ((errno == 0) && szTemp && !*szTemp)
		{
			hr = m_pEngine->SetVariableNumeric(wzVariable, lVal);
			BextExitOnFailure(hr, "Failed to set variable");
		}
		else if (SUCCEEDED(VerParseVersion(szValue, 0, TRUE, &pVersion)) && pVersion)
		{
			hr = m_pEngine->SetVariableVersion(wzVariable, szValue);
			BextExitOnFailure(hr, "Failed to set variable");
		}
		else
		{
			hr = m_pEngine->SetVariableString(wzVariable, szValue, FALSE);
			BextExitOnFailure(hr, "Failed to set variable");
		}

		ExitFunction();
	}
	hr = E_NOTFOUND;

LExit:
	
	return hr;
}

HRESULT CPanelSwBundleExtension::Reset()
{
	ContainerIterator endIt = _containers.end();
	for (ContainerIterator it = _containers.begin(); it != endIt; ++it)
	{
		if (*it)
		{
			delete* it;
		}
	}
	_containers.clear();

	for (long i = 0; i < _cSearches; ++i)
	{
		PANELSW_SEARCH* pSearch = _pSearches + i;

		ReleaseStr(pSearch->szId);

		switch (pSearch->type)
		{
		case PANELSW_SEARCH_TYPE_BUNDLE_VARIABLE:
			ReleaseStr(pSearch->BundleVariable.szSearchVariable);
			ReleaseStr(pSearch->BundleVariable.szUpgradeCode);
			break;
		case PANELSW_SEARCH_TYPE_ARP_ENTRY:
			ReleaseStr(pSearch->ArpEntry.szArpSpec);
			break;
		default:
			break;
		}
	}
	ReleaseNullMem(_pSearches);
	_cSearches = 0;

	if (_pszCommandLineArgs)
	{
		AppFreeCommandLineArgs(_pszCommandLineArgs);
		_pszCommandLineArgs = nullptr;
	}
	_cCommandLineArgs = 0;

	BalInfoUninitialize(&_bundleInfo);

	return S_OK;
}

HRESULT CPanelSwBundleExtension::CreateContainer(LPCWSTR wzContainerId, IPanelSwContainer** ppContainer)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMDocument> pixdManifest;
	CComPtr<IXMLDOMNode> pixnBundleExtension;
	CComPtr<IXMLDOMNode> pixnCompression;
	CComVariant compression;
	LPWSTR szXPath = nullptr;
	IPanelSwContainer* pContainer = nullptr;

	hr = XmlLoadDocumentFromFile(m_sczBootstrapperExtensionDataPath, &pixdManifest);
	BextExitOnFailure(hr, "Failed to load bundle extension manifest from path: %ls", m_sczBootstrapperExtensionDataPath);

	hr = BextGetBootstrapperExtensionDataNode(pixdManifest, PANELSW_BACKEND_EXTENSION_ID, &pixnBundleExtension);
	BextExitOnFailure(hr, "Failed to get BundleExtension '%ls'", PANELSW_BACKEND_EXTENSION_ID);

	hr = StrAllocFormatted(&szXPath, L"PSW_ContainerExtensionData[@ContainerId='%ls']/@Compression", wzContainerId);
	BextExitOnFailure(hr, "Failed to allocate XPath string");

	hr = XmlSelectSingleNode(pixnBundleExtension, szXPath, &pixnCompression);
	BextExitOnFailure(hr, "Failed to get container extension data");

	hr = pixnCompression->get_nodeValue(&compression);
	BextExitOnFailure(hr, "Failed to get container compression");

	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, compression.bstrVal, -1, L"Zip", -1))
	{
		pContainer = new CPanelSwZipContainer();
		BextExitOnNull(pContainer, hr, E_FAIL, "Failed to allocate zip container");
	}
	else if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, compression.bstrVal, -1, L"SevenZip", -1))
	{
		pContainer = new CPanelSwLzmaContainer();
		BextExitOnNull(pContainer, hr, E_FAIL, "Failed to allocate 7z container");
	}
	else
	{
		hr = E_INVALIDDATA;
		BextExitOnFailure(hr, "Unsupported container compression '%ls'", compression.bstrVal);
	}

	*ppContainer = pContainer;
	pContainer = nullptr;

LExit:
	ReleaseStr(szXPath);
	if (pContainer)
	{
		delete pContainer;
	}

	return hr;
}

#ifdef EnableZipContainer

STDMETHODIMP CPanelSwBundleExtension::ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath, LPVOID* ppContext)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = CreateContainer(wzContainerId, &pContainer);
	BextExitOnFailure(hr, "Failed to create container");

	_containers.push_back(pContainer);
	*ppContext = pContainer;

	hr = pContainer->ContainerOpen(wzContainerId, wzFilePath);
	BextExitOnFailure(hr, "Failed to open container");

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerOpenAttached(LPCWSTR wzContainerId, HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize, LPVOID* ppContext)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = CreateContainer(wzContainerId, &pContainer);
	BextExitOnFailure(hr, "Failed to create container");

	_containers.push_back(pContainer);
	*ppContext = pContainer;

	hr = pContainer->ContainerOpenAttached(wzContainerId, hBundle, qwContainerStartPos, qwContainerSize);
	BextExitOnFailure(hr, "Failed to open attached container");

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerNextStream(LPVOID pContext, BSTR* psczStreamName)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");

	hr = pContainer->ContainerNextStream(psczStreamName);

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerStreamToFile(LPVOID pContext, LPCWSTR wzFileName)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");
	
	hr = pContainer->ContainerStreamToFile(wzFileName);

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerStreamToBuffer(LPVOID pContext, BYTE** ppbBuffer, SIZE_T* pcbBuffer)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");
	
	hr = pContainer->ContainerStreamToBuffer(ppbBuffer, pcbBuffer);

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerSkipStream(LPVOID pContext)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");
	
	hr = pContainer->ContainerSkipStream();

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerClose(LPVOID pContext)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");
	
	hr = pContainer->ContainerClose();
	BextExitOnFailure(hr, "Failed to close container");

	ReleaseContainer(pContainer);

LExit:
	return hr;
}
#endif

HRESULT CPanelSwBundleExtension::GetContainer(LPVOID pContext, IPanelSwContainer** ppContainer)
{
	HRESULT hr = E_NOTFOUND;

	ContainerIterator endIt = _containers.end();
	for (ContainerIterator it = _containers.begin(); it != endIt; ++it)
	{
		if (*it == (IPanelSwContainer*)pContext)
		{
			hr = S_OK;
			*ppContainer = (IPanelSwContainer*)pContext;
			break;
		}
	}

	return hr;
}

HRESULT CPanelSwBundleExtension::ReleaseContainer(IPanelSwContainer* pContainer)
{
	HRESULT hr = E_NOTFOUND;

	ContainerIterator endIt = _containers.end();
	for (ContainerIterator it = _containers.begin(); it != endIt; ++it)
	{
		if (*it == (IPanelSwContainer*)pContainer)
		{
			hr = S_OK;
			_containers.remove(pContainer);
			delete pContainer;
			break;
		}
	}
	return hr;
}

HRESULT CPanelSwBundleExtension::SearchBundleVariable(PANELSW_SEARCH* pSearch, LPWSTR* pszValue)
{
	HRESULT hr = S_OK;
	LPWSTR szValue = nullptr;

	hr = _bundles.SearchBundleVariable(pSearch->BundleVariable.szUpgradeCode, pSearch->BundleVariable.szSearchVariable, pSearch->BundleVariable.bFormat, &szValue);
	BextExitOnFailure(hr, "Failed to search for bundle variable '%ls'", pSearch->BundleVariable.szSearchVariable);

	if (hr == S_FALSE)
	{
		ExitFunction();
	}

	if (pSearch->BundleVariable.bFormat && szValue && *szValue)
	{
		hr = FormatString(szValue, pszValue);
		BextExitOnFailure(hr, "Failed to format string");
	}
	else
	{
		*pszValue = szValue;
		szValue = nullptr;
	}

LExit:
	ReleaseStr(szValue);

	return hr;
}

HRESULT CPanelSwBundleExtension::SearchArpEntry(PANELSW_SEARCH* pSearch, LPWSTR* pszValue)
{
	HRESULT hr = S_OK;
	HKEY hkRoot = pSearch->ArpEntry.bUserContext ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
	HKEY hkUninstall = NULL;
	HKEY hkProduct = NULL;
	DWORD i = 0;
	LPWSTR szSubkey = nullptr;
	LPWSTR szDisplayName = nullptr;
	VERUTIL_VERSION* pMaxVersion = nullptr;
	VERUTIL_VERSION* pCurrVersion = nullptr;

	hr = RegOpenEx(hkRoot, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", KEY_READ, pSearch->ArpEntry.bitness, &hkUninstall);
	BextExitOnFailure(hr, "Failed to open uninstall key");

	while (E_NOMOREITEMS != (hr = RegKeyEnum(hkUninstall, i++, &szSubkey)))
	{
		BextExitOnFailure(hr, "Failed to enumerate uninstall key");
		ReleaseNullStr(szDisplayName);
		ReleaseVerutilVersion(pCurrVersion);
		ReleaseRegKey(hkProduct);
		int nCompare = 0;

		if (!::PathMatchSpec(szSubkey, pSearch->ArpEntry.szArpSpec))
		{
			continue;
		}

		hr = RegOpenEx(hkUninstall, szSubkey, KEY_READ, pSearch->ArpEntry.bitness, &hkProduct);
		BextExitOnFailure(hr, "Failed to open product key");

		hr = RegReadWixVersion(hkProduct, L"DisplayVersion", &pCurrVersion);
		if (FAILED(hr))
		{
			BextLogError(hr, "Failed to parse DisplayVersion of '%ls'", szSubkey);
			hr = S_OK;
			continue;
		}

		RegReadString(hkProduct, L"DisplayName", &szDisplayName);
		BextLog(BOOTSTRAPPER_EXTENSION_LOG_LEVEL_STANDARD, "Detected product '%ls' version '%ls'", szDisplayName && *szDisplayName ? szDisplayName : szSubkey, pCurrVersion->sczVersion);

		if (!pMaxVersion)
		{
			pMaxVersion = pCurrVersion;
			pCurrVersion = nullptr;
			continue;
		}

		hr = VerCompareParsedVersions(pMaxVersion, pCurrVersion, &nCompare);
		BextExitOnFailure(hr, "Failed to compare versions");

		if (nCompare < 0)
		{
			ReleaseVerutilVersion(pMaxVersion);
			pMaxVersion = pCurrVersion;
			pCurrVersion = nullptr;
		}
	}

	if (!pMaxVersion)
	{
		hr = S_FALSE;
		ExitFunction();
	}

	hr = StrAllocString(pszValue, pMaxVersion->sczVersion, 0);
	BextExitOnFailure(hr, "Failed to allocate string");

LExit:
	ReleaseStr(szDisplayName);
	ReleaseStr(szSubkey);
	ReleaseVerutilVersion(pCurrVersion);
	ReleaseVerutilVersion(pMaxVersion);
	ReleaseRegKey(hkUninstall);
	ReleaseRegKey(hkProduct);

	return hr;
}

HRESULT CPanelSwBundleExtension::IsOnCommandLine(LPCWSTR szVariableName)
{
	HRESULT hr = S_OK;
	BAL_INFO_OVERRIDABLE_VARIABLE* pOverridableVar = nullptr;
	DWORD dwCmpFlags = 0;

	hr = DictGetValue(_bundleInfo.overridableVariables.sdVariables, szVariableName, (void**)&pOverridableVar);
	if ((hr == E_NOTFOUND) || (hr == E_INVALIDARG))
	{
		hr = S_FALSE;
		ExitFunction();
	}

	if (_bundleInfo.overridableVariables.commandLineType == BAL_INFO_VARIABLE_COMMAND_LINE_TYPE_CASE_INSENSITIVE)
	{
		dwCmpFlags |= NORM_IGNORECASE;
	}

	for (int i = 0; i < _cCommandLineArgs; ++i)
	{
		LPCWSTR szArg = _pszCommandLineArgs[i];
		LPCWSTR szVarNameEnd = ::wcschr(szArg, L'=');

		if (!szVarNameEnd)
		{
			continue;
		}

		if (CSTR_EQUAL == ::CompareString(LOCALE_NEUTRAL, dwCmpFlags, szArg, szVarNameEnd - szArg, szVariableName, -1))
		{
			hr = S_OK;
			ExitFunction();
		}
	}
	hr = S_FALSE;

LExit:
	return hr;
}

HRESULT CPanelSwBundleExtension::FormatString(LPCWSTR szFormat, LPWSTR* pszValue)
{
	HRESULT hr = S_OK;
	LPWSTR szValue = nullptr;
	SIZE_T cch = 0;

	hr = m_pEngine->FormatString(szFormat, szValue, &cch);
	if (hr == E_MOREDATA)
	{
		hr = StrAlloc(&szValue, ++cch);
		BextExitOnFailure(hr, "Failed to allocate memory");

		hr = m_pEngine->FormatString(szFormat, szValue, &cch);
	}
	BextExitOnFailure(hr, "Failed to format string");

	*pszValue = szValue;
	szValue = nullptr;

LExit:
	ReleaseStr(szValue);

	return hr;
}

extern "C" HRESULT WINAPI BootstrapperExtensionCreate(const BOOTSTRAPPER_EXTENSION_CREATE_ARGS* pArgs, BOOTSTRAPPER_EXTENSION_CREATE_RESULTS* pResults)
{
	HRESULT hr = S_OK;
	IBootstrapperExtensionEngine* pEngine = nullptr;
	CPanelSwBundleExtension* pExtension = nullptr;

	hr = BextInitializeFromCreateArgs(pArgs, &pEngine);
	BextExitOnFailure(hr, "Failed to initialize bext");
	BextLog(BOOTSTRAPPER_EXTENSION_LOG_LEVEL_STANDARD, "Loading Panel::Software bundle extension v" FullVersion);

	pExtension = new CPanelSwBundleExtension(pEngine);
	BextExitOnNull(pExtension, hr, E_OUTOFMEMORY, "Failed to create new CPanelSwBundleExtension.");

	hr = pExtension->Initialize(pArgs);
	BextExitOnFailure(hr, "CPanelSwBundleExtension initialization failed.");

	pResults->pfnBootstrapperExtensionProc = BextBaseBootstrapperExtensionProc;
	pResults->pvBootstrapperExtensionProcContext = pExtension;

LExit:
	ReleaseObject(pEngine);

	return hr;
}

extern "C" void WINAPI BootstrapperExtensionDestroy()
{
	BextUninitialize();
}

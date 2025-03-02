#include "pch.h"
#include "PanelSwBackendExtension.h"
#include "PanelSwZipContainer.h"
#include "PanelSwLzmaContainer.h"
#include <BextBaseBootstrapperExtensionProc.h>
using namespace std;

CPanelSwBundleExtension::CPanelSwBundleExtension(IBootstrapperExtensionEngine* pEngine)
	: CBextBaseBootstrapperExtension(pEngine)
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
	CComPtr<IXMLDOMDocument> pixdManifest;
	CComPtr<IXMLDOMNode> pixnBundleExtension;

	hr = __super::Initialize(pCreateArgs);
	BextExitOnFailure(hr, "CBextBaseBootstrapperExtension initialization failed.");

	hr = XmlLoadDocumentFromFile(m_sczBootstrapperExtensionDataPath, &pixdManifest);
	BextExitOnFailure(hr, "Failed to load bundle extension manifest from path: %ls", m_sczBootstrapperExtensionDataPath);

	hr = BextGetBootstrapperExtensionDataNode(pixdManifest, PANELSW_BACKEND_EXTENSION_ID, &pixnBundleExtension);
	BextExitOnFailure(hr, "Failed to get BundleExtensionData entry for '%ls'", PANELSW_BACKEND_EXTENSION_ID);

	hr = ParseSearches(pixnBundleExtension);
	BextExitOnFailure(hr, "Failed to parse searches from bundle extension manifest.");

LExit:

	return hr;
}

HRESULT CPanelSwBundleExtension::ParseSearches(IXMLDOMNode *pixnBundleExtension)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMNodeList> pixnNodes;

	// Select BundleVariableSearch nodes.
	hr = XmlSelectNodes(pixnBundleExtension, L"PSW_BundleVariableSearch", &pixnNodes);
	BextExitOnFailure(hr, "Failed to select PSW_BundleVariableSearch nodes.");

	hr = pixnNodes->get_length(&_cSearches);
	BextExitOnFailure(hr, "Failed to get PSW_BundleVariableSearch node count.");

	hr = MemAllocArray((void**)&_pSearches, sizeof(BUNDLE_VARIABLE_SEARCH), _cSearches);
	BextExitOnFailure(hr, "Failed to allocate memory.");

	for (long i = 0; i < _cSearches; ++i)
	{
		CComPtr<IXMLDOMNode> pixnSearch;
		BUNDLE_VARIABLE_SEARCH* pSearch = _pSearches + i;
		DWORD dwFormat = 0;

		hr = pixnNodes->get_item(i, &pixnSearch);
		BextExitOnFailure(hr, "Failed to get next PSW_BundleVariableSearch node.");

		hr = XmlGetAttributeEx(pixnSearch, L"Id", &pSearch->_szId);
		BextExitOnFailure(hr, "Failed to get @Id.");

		hr = XmlGetAttributeEx(pixnSearch, L"UpgradeCode", &pSearch->_szUpgradeCode);
		BextExitOnFailure(hr, "Failed to get @UpgradeCode.");

		hr = XmlGetAttributeEx(pixnSearch, L"SearchVariable", &pSearch->_szSearchVariable);
		BextExitOnFailure(hr, "Failed to get @VariableName.");

		hr = XmlGetAttributeNumber(pixnSearch, L"Format", &dwFormat);
		BextExitOnFailure(hr, "Failed to get @Format.");
		pSearch->_bFormat = dwFormat;
	}

LExit:

	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::Search(LPCWSTR wzId, LPCWSTR wzVariable)
{
	HRESULT hr = S_OK;

	for (long i = 0; i < _cSearches; ++i)
	{
		if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, _pSearches[i]._szId, -1, wzId, -1))
		{
			hr = SearchBundleVariable(_pSearches[i]._szUpgradeCode, _pSearches[i]._szSearchVariable, _pSearches[i]._bFormat, wzVariable);
			BextExitOnFailure(hr, "Failed to search for variable '%ls' in bundles with upgrade code '%ls'", _pSearches[i]._szSearchVariable, _pSearches[i]._szUpgradeCode);
			ExitFunction();
		}
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
		ReleaseStr(_pSearches[i]._szId);
		ReleaseStr(_pSearches[i]._szSearchVariable);
		ReleaseStr(_pSearches[i]._szUpgradeCode);
	}
	ReleaseNullMem(_pSearches);
	_cSearches = 0;
	_cSearchRecursion = 0;

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

	if (::wcsicmp(compression.bstrVal, L"Zip") == 0)
	{
		pContainer = new CPanelSwZipContainer();
		BextExitOnNull(pContainer, hr, E_FAIL, "Failed to allocate zip container");
	}
	else if (::wcsicmp(compression.bstrVal, L"SevenZip") == 0)
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
	
LExit:
	return hr;
}

HRESULT CPanelSwBundleExtension::SearchBundleVariable(LPCWSTR szUpgradeCode, LPCWSTR szVariableName, BOOL bFormat, LPCWSTR szResultVariableName)
{
	HRESULT hr = S_OK;

	hr = SearchBundleVariable(BUNDLE_INSTALL_CONTEXT_MACHINE, REG_KEY_64BIT, szUpgradeCode, szVariableName, bFormat, szResultVariableName);
	BextExitOnFailure(hr, "Failed to search for bundle variable '%ls'", szVariableName);
	if (hr == S_OK)
	{
		ExitFunction();
	}

	hr = SearchBundleVariable(BUNDLE_INSTALL_CONTEXT_MACHINE, REG_KEY_32BIT, szUpgradeCode, szVariableName, bFormat, szResultVariableName);
	BextExitOnFailure(hr, "Failed to search for bundle variable '%ls'", szVariableName);
	if (hr == S_OK)
	{
		ExitFunction();
	}

	hr = SearchBundleVariable(BUNDLE_INSTALL_CONTEXT_USER, REG_KEY_64BIT, szUpgradeCode, szVariableName, bFormat, szResultVariableName);
	BextExitOnFailure(hr, "Failed to search for bundle variable '%ls'", szVariableName);
	if (hr == S_OK)
	{
		ExitFunction();
	}

	hr = SearchBundleVariable(BUNDLE_INSTALL_CONTEXT_USER, REG_KEY_32BIT, szUpgradeCode, szVariableName, bFormat, szResultVariableName);
	BextExitOnFailure(hr, "Failed to search for bundle variable '%ls'", szVariableName);
	if (hr == S_OK)
	{
		ExitFunction();
	}

LExit:
	return hr;
}

HRESULT CPanelSwBundleExtension::SearchBundleVariable(BUNDLE_INSTALL_CONTEXT context, REG_KEY_BITNESS kbKeyBitness, LPCWSTR szUpgradeCode, LPCWSTR szVariableName, BOOL bFormat, LPCWSTR szResultVariableName)
{
	HRESULT hr = S_OK;
	HKEY hkRoot = (context == BUNDLE_INSTALL_CONTEXT_USER) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
	HKEY hkUninstall = NULL;
	HKEY hkVariables = NULL;
	VERUTIL_VERSION* pVersion = nullptr;
	LPWSTR szBundleId = nullptr;
	LPWSTR szVariablesKey = nullptr;
	LPWSTR szValue = nullptr;
	LPWSTR szTemp = nullptr;
	long lVal = 0;

	hr = RegOpenEx(hkRoot, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", KEY_READ, kbKeyBitness, &hkUninstall);
	if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
	{
		hr = S_FALSE;
		ExitFunction();
	}
	BextExitOnFailure(hr, "Failed to open registry uninstall key.");

	for (DWORD dwIndex = 0;(hr = BundleEnumRelatedBundle(szUpgradeCode, context, kbKeyBitness, &dwIndex, &szBundleId)) == S_OK; ++dwIndex)
	{
		hr = StrAllocFormatted(&szVariablesKey, L"%ls\\variables", szBundleId);
		BextExitOnFailure(hr, "Failed to format string");

		hr = RegOpenEx(hkUninstall, szVariablesKey, KEY_READ, kbKeyBitness, &hkVariables);
		if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
		{
			continue;
		}
		BextExitOnFailure(hr, "Failed to open bundle '%ls' variables key.", szBundleId);

		_cSearchRecursion = 0;
		hr = ReadBundleVariable(hkVariables, szVariableName, bFormat, &szValue);
		if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
		{
			continue;
		}
		BextExitOnFailure(hr, "Failed to get variable '%ls' for bundle '%ls'", szVariableName, szBundleId);
		BextLog(BOOTSTRAPPER_EXTENSION_LOG_LEVEL_STANDARD, "Detected variable '%ls' belonging to bundle '%ls' in %ls %ls context", szVariableName, szBundleId, (kbKeyBitness == REG_KEY_BITNESS::REG_KEY_32BIT) ? L"x86" : L"x64", (context == BUNDLE_INSTALL_CONTEXT::BUNDLE_INSTALL_CONTEXT_MACHINE) ? L"machine" : L"user");

		if (!szValue || !*szValue)
		{
			hr = m_pEngine->SetVariableString(szResultVariableName, L"", FALSE);
			BextExitOnFailure(hr, "Failed to set variable");
			ExitFunction();
		}

		if (bFormat)
		{
			SIZE_T cch = 0;

			hr = m_pEngine->FormatString(szValue, szTemp, &cch);
			if (hr == E_MOREDATA)
			{
				hr = StrAlloc(&szTemp, ++cch);
				BextExitOnFailure(hr, "Failed to allocate memory");

				hr = m_pEngine->FormatString(szValue, szTemp, &cch);
			}
			BextExitOnFailure(hr, "Failed to format string");
			BextExitOnNull(szTemp, hr, E_INVALIDSTATE, "Failed to format string"); // Just to keep the compiler happy
			ReleaseStr(szValue);
			szValue = szTemp;
			szTemp = nullptr;
		}

		lVal = wcstol(szValue, &szTemp, 10);
		if ((errno == 0) && (szTemp == (szValue + wcslen(szValue))))
		{
			hr = m_pEngine->SetVariableNumeric(szResultVariableName, lVal);
			BextExitOnFailure(hr, "Failed to set variable");
		}
		else if (SUCCEEDED(VerParseVersion(szValue, 0, TRUE, &pVersion)) && pVersion)
		{
			hr = m_pEngine->SetVariableVersion(szResultVariableName, szValue);
			BextExitOnFailure(hr, "Failed to set variable");
		}
		else
		{
			hr = m_pEngine->SetVariableString(szResultVariableName, szValue, FALSE);
			BextExitOnFailure(hr, "Failed to set variable");
		}
		ExitFunction();
	}
	if (hr == E_NOMOREITEMS)
	{
		hr = S_FALSE;
	}
	BextExitOnFailure(hr, "Failed to find bundles with upgrade code '%ls'", szUpgradeCode);

LExit:
	ReleaseStr(szValue);
	ReleaseStr(szVariablesKey);
	ReleaseStr(szBundleId);
	ReleaseRegKey(hkVariables);
	ReleaseRegKey(hkUninstall);
	ReleaseVerutilVersion(pVersion);

	return hr;
}

HRESULT CPanelSwBundleExtension::ReadBundleVariable(HKEY hkVariables, LPCWSTR szVariableName, BOOL bFormat, LPWSTR* pszValue)
{
	HRESULT hr = S_OK;
	LPWSTR szValue = nullptr;
	BYTE* pbData = nullptr;
	SIZE_T cbData = 0;
	DWORD dwType = 0;
	LPWSTR szEmbeddedVar = nullptr;
	LPWSTR szEmbeddedValue = nullptr;
	LPWSTR szTemp = nullptr;

	if (++_cSearchRecursion >= MAX_SEARCH_RECURSION)
	{
		BextLog(BOOTSTRAPPER_EXTENSION_LOG_LEVEL_ERROR, "To deep recursion when formatting bundle search variable");

		hr = StrAllocString(pszValue, L"", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}

	if (!szVariableName || !*szVariableName)
	{
		hr = StrAllocString(pszValue, L"", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}

	hr = RegReadValue(hkVariables, szVariableName, TRUE, &pbData, &cbData, &dwType);
	if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
	{
		ExitFunction();
	}
	BextExitOnFailure(hr, "Failed to get variable '%ls'", szVariableName);

	switch (dwType)
	{
	case REG_NONE:
		hr = StrAllocString(pszValue, L"", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
		break;

	case REG_SZ:
	case REG_EXPAND_SZ:
		hr = StrAllocFormatted(&szValue, L"%.*ls", cbData / 2, (LPCWSTR)pbData);
		BextExitOnFailure(hr, "Failed to allocate string");
		break;

	case REG_DWORD:
		if (pbData && (cbData == sizeof(DWORD)))
		{
			hr = StrAllocFormatted(pszValue, L"%ul", (DWORD)*pbData);
			BextExitOnFailure(hr, "Failed to allocate string");
		}
		else
		{
			hr = StrAllocString(pszValue, L"", 0);
			BextExitOnFailure(hr, "Failed to allocate string");
		}
		ExitFunction();
		break;

	default:
		hr = E_INVALIDDATA;
		BextExitOnFailure(hr, "Unsupported variable data type in registry. Variable '%ls', data type %u", szVariableName, dwType);
		break;
	}

	if (!bFormat || !szValue || !*szValue)
	{
		*pszValue = szValue;
		szValue = nullptr;
		ExitFunction();
	}

	// Now parse embedded formatting.
	for (LPWSTR szNext = StrChr(szValue, L'['); szNext && *szNext; szNext = StrChr(szNext, L'['))
	{
		LPCWSTR szNextEnd = nullptr;

		ReleaseNullStr(szEmbeddedVar);
		ReleaseNullStr(szEmbeddedValue);
		ReleaseNullStr(szTemp);

		szNextEnd = StrChr(szNext, L']');
		if (!szNextEnd || !*szNextEnd)
		{
			break;
		}

		hr = StrAllocFormatted(&szEmbeddedVar, L"%.*ls", szNextEnd - szNext - 1, szNext + 1);
		BextExitOnFailure(hr, "Failed to allocate string");

		hr = ReadBundleVariable(hkVariables, szEmbeddedVar, bFormat, &szEmbeddedValue);
		if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
		{
			hr = S_OK;
			++szNext;
			continue;
		}
		BextExitOnFailure(hr, "Failed to read embedded variable '%ls'", szEmbeddedVar);

		// Replace the embedded variable and restart
		hr = StrAllocFormatted(&szTemp, L"%.*ls%ls%ls", szNext - szValue, szValue, szEmbeddedValue, szNextEnd + 1);
		BextExitOnFailure(hr, "Failed to allocate string");

		ReleaseStr(szValue);
		szValue = szTemp;
		szNext = szValue;
		szTemp = nullptr;
	}

	*pszValue = szValue;
	szValue = nullptr;

LExit:
	ReleaseStr(szValue);
	ReleaseStr(szEmbeddedVar);
	ReleaseStr(szEmbeddedValue);
	ReleaseStr(szTemp);
	--_cSearchRecursion;

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

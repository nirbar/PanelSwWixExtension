#include "pch.h"
#include "PanelSwBundleVariables.h"

CPanelSwBundleVariables::CPanelSwBundleVariables(IBootstrapperExtensionEngine* pEngine)
	: _pEngine(pEngine)
{
}

CPanelSwBundleVariables::~CPanelSwBundleVariables()
{
	Reset();
}

void CPanelSwBundleVariables::Reset()
{
	for (SIZE_T i = 0; i < _cBundles; ++i)
	{
		BUNDLE_INFO* pBundle = _rgBundles + i;

		for (SIZE_T j = 0; j < pBundle->_cVariables; ++j)
		{
			BUNDLE_VARIABLE* pVariable = pBundle->_rgVariables + j;
			ReleaseNullStr(pVariable->_szName);
			ReleaseNullStr(pVariable->_szValue);
		}

		ReleaseNullStr(pBundle->_szUpgradeCode);
		ReleaseNullStr(pBundle->_szId);
		ReleaseNullStr(pBundle->_szDisplayName);
		ReleaseNullStr(pBundle->_szStatePath);
		ReleaseNullStr(pBundle->_szTag);
		ReleaseNullStr(pBundle->_szPublisher);
		ReleaseNullStr(pBundle->_szProviderKey);
		ReleaseNullMem(pBundle->_rgVariables);
		ReleaseVerutilVersion(pBundle->_pEngineVersion);
		ReleaseVerutilVersion(pBundle->_pDisplayVersion);
		pBundle->_bIsX64 = false;
		pBundle->_bIsMachineContext = false;
		pBundle->_cVariables = 0;
	}
	ReleaseNullMem(_rgBundles);
	_cBundles = 0;
	_cSearchRecursion = 0;
}

HRESULT CPanelSwBundleVariables::SearchBundleVariable(LPCWSTR szUpgradeCode, LPCWSTR szVariableName, BOOL bFormat, LPWSTR* pszValue)
{
	HRESULT hr = S_OK;
	BUNDLE_INFO* pBundle = nullptr;

	hr = SearchBundle(szUpgradeCode, &pBundle);
	BextExitOnFailure(hr, "Failed to search for bundle '%ls'", szUpgradeCode);
	if (hr == S_FALSE)
	{
		ExitFunction();
	}

	_cSearchRecursion = 0;
	hr = GetVariable(pBundle, szVariableName, bFormat, pszValue);
	BextExitOnFailure(hr, "Failed to get bundle variable '%ls' for bundle '%ls'", szVariableName, szUpgradeCode);

LExit:
	return hr;
}


HRESULT CPanelSwBundleVariables::SearchBundle(LPCWSTR szUpgradeCode, BUNDLE_INFO** ppBundleInfo)
{
	HRESULT hr = S_OK;

	for (SIZE_T i = 0; i < _cBundles; ++i)
	{
		BUNDLE_INFO* pBundle = _rgBundles + i;
		LPCWSTR szLeft = pBundle->_szUpgradeCode;
		LPCWSTR szRight = szUpgradeCode;
		SIZE_T lLeft = wcslen(szLeft);
		SIZE_T lRight = wcslen(szRight);

		// Normalize compare without brackets
		if (szLeft[0] == L'{')
		{
			++szLeft;
			--lLeft;
		}
		if (szLeft[lLeft - 1] == L'}')
		{
			--lLeft;
		}

		if (szRight[0] == L'{')
		{
			++szRight;
			--lRight;
		}
		if (szRight[lRight - 1] == L'}')
		{
			--lRight;
		}

		if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, szLeft, lLeft, szRight, lRight))
		{
			*ppBundleInfo = pBundle;
			ExitFunction();
		}
	}

	hr = SearchBundle(BUNDLE_INSTALL_CONTEXT_MACHINE, REG_KEY_64BIT, szUpgradeCode, ppBundleInfo);
	BextExitOnFailure(hr, "Failed to search for bundle '%ls'", szUpgradeCode);
	if (hr == S_OK)
	{
		ExitFunction();
	}

	hr = SearchBundle(BUNDLE_INSTALL_CONTEXT_MACHINE, REG_KEY_32BIT, szUpgradeCode, ppBundleInfo);
	BextExitOnFailure(hr, "Failed to search for bundle '%ls'", szUpgradeCode);
	if (hr == S_OK)
	{
		ExitFunction();
	}

	hr = SearchBundle(BUNDLE_INSTALL_CONTEXT_USER, REG_KEY_64BIT, szUpgradeCode, ppBundleInfo);
	BextExitOnFailure(hr, "Failed to search for bundle '%ls'", szUpgradeCode);
	if (hr == S_OK)
	{
		ExitFunction();
	}

	hr = SearchBundle(BUNDLE_INSTALL_CONTEXT_USER, REG_KEY_32BIT, szUpgradeCode, ppBundleInfo);
	BextExitOnFailure(hr, "Failed to search for bundle '%ls'", szUpgradeCode);
	if (hr == S_OK)
	{
		ExitFunction();
	}

LExit:
	return hr;
}

HRESULT CPanelSwBundleVariables::SearchBundle(BUNDLE_INSTALL_CONTEXT context, REG_KEY_BITNESS kbKeyBitness, LPCWSTR szUpgradeCode, BUNDLE_INFO** ppBundleInfo)
{
	HRESULT hr = S_OK;
	HKEY hkRoot = (context == BUNDLE_INSTALL_CONTEXT_USER) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
	HKEY hkUninstall = NULL;
	HKEY hkBundle = NULL;
	LPWSTR szBundleId = nullptr;
	LPWSTR szCachePath = nullptr;
	BUNDLE_INFO* pBundle = nullptr;

	hr = RegOpenEx(hkRoot, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", KEY_READ, kbKeyBitness, &hkUninstall);
	if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
	{
		hr = S_FALSE;
		ExitFunction();
	}
	BextExitOnFailure(hr, "Failed to open registry uninstall key.");

	for (DWORD dwIndex = 0;(hr = BundleEnumRelatedBundle(szUpgradeCode, context, kbKeyBitness, &dwIndex, &szBundleId)) == S_OK; ++dwIndex)
	{
		LPWSTR szCacheDir = nullptr;
		DWORD dwVarLen = 0;

		hr = RegOpenEx(hkUninstall, szBundleId, KEY_READ, kbKeyBitness, &hkBundle);
		if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
		{
			continue;
		}
		BextExitOnFailure(hr, "Failed to open bundle '%ls' key.", szBundleId);

		hr = MemEnsureArraySize((void**)&_rgBundles, 1 + _cBundles, sizeof(BUNDLE_INFO), 1);
		BextExitOnFailure(hr, "Failed to allocate memory");
		
		pBundle = &_rgBundles[_cBundles++];
		ZeroMemory(pBundle, sizeof(BUNDLE_INFO));
		pBundle->_szId = szBundleId;
		pBundle->_bIsX64 = (kbKeyBitness == REG_KEY_BITNESS::REG_KEY_64BIT);
		pBundle->_bIsMachineContext = (context == BUNDLE_INSTALL_CONTEXT::BUNDLE_INSTALL_CONTEXT_MACHINE);
		szBundleId = nullptr;

		hr = StrAllocString(&pBundle->_szUpgradeCode, szUpgradeCode, 0);
		BextExitOnFailure(hr, "Failed to allocate string");

		hr = RegReadWixVersion(hkBundle, L"EngineVersion", &pBundle->_pEngineVersion);
		BextExitOnFailure(hr, "Failed to read EngineVersion");

		hr = RegReadString(hkBundle, L"BundleCachePath", &szCachePath);
		BextExitOnFailure(hr, "Failed to read BundleCachePath");

		szCacheDir = StrRChr(szCachePath, nullptr, L'\\');
		BextExitOnNull(szCacheDir, hr, E_INVALIDDATA, "Failed to parse bundle cache dir");

		dwVarLen = szCacheDir - szCachePath;
		hr = StrAllocFormatted(&pBundle->_szStatePath, L"%.*ls\\state.rsm", dwVarLen, szCachePath);
		BextExitOnFailure(hr, "Failed to format string");

		// Optional values
		RegReadString(hkBundle, L"DisplayName", &pBundle->_szDisplayName);
		RegReadString(hkBundle, L"BundleTag", &pBundle->_szTag);
		RegReadString(hkBundle, L"Publisher", &pBundle->_szPublisher);
		RegReadString(hkBundle, L"BundleProviderKey", &pBundle->_szProviderKey);
		RegReadWixVersion(hkBundle, L"DisplayVersion", &pBundle->_pDisplayVersion);

		hr = LoadBundleVariables(pBundle);
		BextExitOnFailure(hr, "Failed to load bundle variables");

		BextLog(BOOTSTRAPPER_EXTENSION_LOG_LEVEL_STANDARD, "Detected variables of bundle '%ls' v%ls in %ls %ls context", pBundle->_szDisplayName, pBundle->_pDisplayVersion && pBundle->_pDisplayVersion->sczVersion ? pBundle->_pDisplayVersion->sczVersion : L"N/A", pBundle->_bIsX64 ? L"x64" : L"x86", pBundle->_bIsMachineContext ? L"machine" : L"user");
		*ppBundleInfo = pBundle;
		ExitFunction();
	}
	if (hr == E_NOMOREITEMS)
	{
		hr = S_FALSE;
	}
	BextExitOnFailure(hr, "Failed to find bundles with upgrade code '%ls'", szUpgradeCode);

LExit:
	ReleaseStr(szCachePath);
	ReleaseStr(szBundleId);
	ReleaseRegKey(hkBundle);
	ReleaseRegKey(hkUninstall);

	return hr;
}

HRESULT CPanelSwBundleVariables::LoadBundleVariables(BUNDLE_INFO* pBundleInfo)
{
	HRESULT hr = S_OK;
	int nCompare = 0;

	hr = VerCompareStringVersions(pBundleInfo->_pEngineVersion->sczVersion, L"4.0.0", TRUE, &nCompare);
	BextExitOnFailure(hr, "Failed to compare versions");

	// WiX3
	if (nCompare < 0)
	{
		hr = LoadBundleVariablesV3(pBundleInfo);
		BextExitOnFailure(hr, "Failed to load bundle variables");
		ExitFunction();
	}

	if (pBundleInfo->_pEngineVersion->dwRevision) // psw-wix has revision, official WiX doesn't
	{
		hr = VerCompareStringVersions(pBundleInfo->_pEngineVersion->sczVersion, L"5.0.0.51", TRUE, &nCompare);
		BextExitOnFailure(hr, "Failed to compare versions");
	}
	else 
	{
		hr = VerCompareStringVersions(pBundleInfo->_pEngineVersion->sczVersion, L"5.0.0", TRUE, &nCompare);
		BextExitOnFailure(hr, "Failed to compare versions");
	}

	// WiX4
	if (nCompare < 0)
	{
		hr = LoadBundleVariablesV4(pBundleInfo);
		BextExitOnFailure(hr, "Failed to load bundle variables");
		ExitFunction();
	}
	else // WiX5
	{
		hr = LoadBundleVariablesV5(pBundleInfo);
		BextExitOnFailure(hr, "Failed to load bundle variables");
		ExitFunction();
	}

LExit:

	return hr;
}

HRESULT CPanelSwBundleVariables::LoadBundleVariablesV3(BUNDLE_INFO* pBundleInfo)
{
	HRESULT hr = S_OK;
	int nCompare = 0;
	bool bHasLiteral = false;
	HANDLE hVariables = INVALID_HANDLE_VALUE;

	hr = VerCompareStringVersions(pBundleInfo->_pEngineVersion->sczVersion, L"3.10", TRUE, &nCompare);
	BextExitOnFailure(hr, "Failed to compare versions");
	bHasLiteral = (nCompare >= 0);

	hVariables = ::CreateFile(pBundleInfo->_szStatePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	BextExitOnNullWithLastError((hVariables != INVALID_HANDLE_VALUE), hr, "Failed to open state file");

	// Read variable count.
	hr = FileReadHandle(hVariables, (LPBYTE)&pBundleInfo->_cVariables, sizeof(pBundleInfo->_cVariables));
	BextExitOnFailure(hr, "Failed to read variable count.");

	hr = MemAllocArray((void**)&pBundleInfo->_rgVariables, sizeof(BUNDLE_VARIABLE), pBundleInfo->_cVariables);
	BextExitOnFailure(hr, "Failed to allocate memory");
	ZeroMemory(pBundleInfo->_rgVariables, sizeof(BUNDLE_VARIABLE) * pBundleInfo->_cVariables);

	// Read variables.
	for (DWORD i = 0; i < pBundleInfo->_cVariables; ++i)
	{
		BUNDLE_VARIABLE* pVariable = &pBundleInfo->_rgVariables[i];
		DWORD dwIncluded = 0;
		DWORD dwType = 0;
		DWORD64 qwValue = 0;

		hr = FileReadHandle(hVariables, (LPBYTE)&dwIncluded, sizeof(dwIncluded));
		BextExitOnFailure(hr, "Failed to read variable count.");
		if (!dwIncluded)
		{
			continue;
		}

		// Variable name
		hr = ReadString(hVariables, &pVariable->_szName);
		BextExitOnFailure(hr, "Failed to read variable name");

		// Type.
		hr = FileReadHandle(hVariables, (LPBYTE)&dwType, sizeof(dwType));
		BextExitOnFailure(hr, "Failed to read variable value type.");

		// Value.
		switch ((BURN_VARIANT_TYPE_V3)dwType)
		{
		case BURN_VARIANT_TYPE_V3_NONE:
			break;
		case BURN_VARIANT_TYPE_V3_NUMERIC:
			hr = FileReadHandle(hVariables, (LPBYTE)&qwValue, sizeof(qwValue));
			BextExitOnFailure(hr, "Failed to read variable value as number.");

			hr = StrAllocFormatted(&pVariable->_szValue, L"%I64u", qwValue);
			BextExitOnFailure(hr, "Failed to format string.");
			break;

		case BURN_VARIANT_TYPE_V3_VERSION:
			hr = FileReadHandle(hVariables, (LPBYTE)&qwValue, sizeof(qwValue));
			BextExitOnFailure(hr, "Failed to read variable value as number.");

			hr = FileVersionToStringEx(qwValue, &pVariable->_szValue);
			BextExitOnFailure(hr, "Failed to format string.");
			break;

		case BURN_VARIANT_TYPE_V3_STRING:
			hr = ReadString(hVariables, &pVariable->_szValue);
			BextExitOnFailure(hr, "Failed to read variable value.");
			break;
	
		default:
			hr = E_INVALIDARG;
			BextExitOnFailure(hr, "Unsupported variable type.");
		}

		if (bHasLiteral)
		{
			DWORD dwValue = 0;
			hr = FileReadHandle(hVariables, (LPBYTE)&dwValue, sizeof(dwValue));
			ExitOnFailure(hr, "Failed to read variable literal flag.");
		}
	}

LExit:
	ReleaseFileHandle(hVariables);

	return hr;
}

HRESULT CPanelSwBundleVariables::LoadBundleVariablesV4(BUNDLE_INFO* pBundleInfo)
{
	HRESULT hr = S_OK;
	HANDLE hVariables = INVALID_HANDLE_VALUE;

	hVariables = ::CreateFile(pBundleInfo->_szStatePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	BextExitOnNullWithLastError((hVariables != INVALID_HANDLE_VALUE), hr, "Failed to open state file");

	// Read variable count.
	hr = FileReadHandle(hVariables, (LPBYTE)&pBundleInfo->_cVariables, sizeof(pBundleInfo->_cVariables));
	BextExitOnFailure(hr, "Failed to read variable count.");

	hr = MemAllocArray((void**)&pBundleInfo->_rgVariables, sizeof(BUNDLE_VARIABLE), pBundleInfo->_cVariables);
	BextExitOnFailure(hr, "Failed to allocate memory");
	ZeroMemory(pBundleInfo->_rgVariables, sizeof(BUNDLE_VARIABLE) * pBundleInfo->_cVariables);

	// Read variables.
	for (DWORD i = 0; i < pBundleInfo->_cVariables; ++i)
	{
		BUNDLE_VARIABLE* pVariable = &pBundleInfo->_rgVariables[i];
		DWORD dwIncluded = 0;
		DWORD dwType = 0;
		DWORD64 qwValue = 0;

		hr = FileReadHandle(hVariables, (LPBYTE)&dwIncluded, sizeof(dwIncluded));
		BextExitOnFailure(hr, "Failed to read variable count.");
		if (!dwIncluded)
		{
			continue;
		}

		// Variable name
		if (pBundleInfo->_bIsX64)
		{
			hr = ReadString64(hVariables, &pVariable->_szName);
			BextExitOnFailure(hr, "Failed to read variable value");
		}
		else
		{
			hr = ReadString(hVariables, &pVariable->_szName);
			BextExitOnFailure(hr, "Failed to read variable value");
		}

		// Type.
		hr = FileReadHandle(hVariables, (LPBYTE)&dwType, sizeof(dwType));
		BextExitOnFailure(hr, "Failed to read variable value type.");

		// Value.
		switch ((BURN_VARIANT_TYPE_V4)dwType)
		{
		case BURN_VARIANT_TYPE_V4_NONE:
			break;
		case BURN_VARIANT_TYPE_V4_NUMERIC:
			hr = FileReadHandle(hVariables, (LPBYTE)&qwValue, sizeof(qwValue));
			BextExitOnFailure(hr, "Failed to read variable value as number.");

			hr = StrAllocFormatted(&pVariable->_szValue, L"%I64u", qwValue);
			BextExitOnFailure(hr, "Failed to format string.");
			break;

		case BURN_VARIANT_TYPE_V4_VERSION:
		case BURN_VARIANT_TYPE_V4_FORMATTED:
		case BURN_VARIANT_TYPE_V4_STRING:
			if (pBundleInfo->_bIsX64)
			{
				hr = ReadString64(hVariables, &pVariable->_szValue);
				BextExitOnFailure(hr, "Failed to read variable value");
			}
			else
			{
				hr = ReadString(hVariables, &pVariable->_szValue);
				BextExitOnFailure(hr, "Failed to read variable value");
			}
			break;
	
		default:
			hr = E_INVALIDARG;
			BextExitOnFailure(hr, "Unsupported variable type.");
		}
	}

LExit:
	ReleaseFileHandle(hVariables);

	return hr;
}

HRESULT CPanelSwBundleVariables::LoadBundleVariablesV5(BUNDLE_INFO* pBundleInfo)
{
	HRESULT hr = S_OK;
	HANDLE hVariables = INVALID_HANDLE_VALUE;

	hVariables = ::CreateFile(pBundleInfo->_szStatePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	BextExitOnNullWithLastError((hVariables != INVALID_HANDLE_VALUE), hr, "Failed to open state file");

	// Read variable count.
	hr = FileReadHandle(hVariables, (LPBYTE)&pBundleInfo->_cVariables, sizeof(pBundleInfo->_cVariables));
	BextExitOnFailure(hr, "Failed to read variable count.");

	hr = MemAllocArray((void**)&pBundleInfo->_rgVariables, sizeof(BUNDLE_VARIABLE), pBundleInfo->_cVariables);
	BextExitOnFailure(hr, "Failed to allocate memory");
	ZeroMemory(pBundleInfo->_rgVariables, sizeof(BUNDLE_VARIABLE) * pBundleInfo->_cVariables);

	// Read variables.
	for (DWORD i = 0; i < pBundleInfo->_cVariables; ++i)
	{
		BUNDLE_VARIABLE* pVariable = &pBundleInfo->_rgVariables[i];
		DWORD dwIncluded = 0;
		DWORD dwType = 0;
		DWORD64 qwValue = 0;

		hr = FileReadHandle(hVariables, (LPBYTE)&dwIncluded, sizeof(dwIncluded));
		BextExitOnFailure(hr, "Failed to read variable count.");
		if (!dwIncluded)
		{
			continue;
		}

		// Variable name
		hr = ReadString(hVariables, &pVariable->_szName);
		BextExitOnFailure(hr, "Failed to read variable name");

		// Type.
		hr = FileReadHandle(hVariables, (LPBYTE)&dwType, sizeof(dwType));
		BextExitOnFailure(hr, "Failed to read variable value type.");

		// Value.
		switch ((BURN_VARIANT_TYPE_V4)dwType)
		{
		case BURN_VARIANT_TYPE_V4_NONE:
			break;
		case BURN_VARIANT_TYPE_V4_NUMERIC:
			hr = FileReadHandle(hVariables, (LPBYTE)&qwValue, sizeof(qwValue));
			BextExitOnFailure(hr, "Failed to read variable value as number.");

			hr = StrAllocFormatted(&pVariable->_szValue, L"%I64u", qwValue);
			BextExitOnFailure(hr, "Failed to format string.");
			break;

		case BURN_VARIANT_TYPE_V4_VERSION:
		case BURN_VARIANT_TYPE_V4_FORMATTED:
		case BURN_VARIANT_TYPE_V4_STRING:
			hr = ReadString(hVariables, &pVariable->_szValue);
			BextExitOnFailure(hr, "Failed to read variable value");
			break;
	
		default:
			hr = E_INVALIDARG;
			BextExitOnFailure(hr, "Unsupported variable type.");
		}
	}

LExit:
	ReleaseFileHandle(hVariables);

	return hr;
}

HRESULT CPanelSwBundleVariables::ReadString(HANDLE hVariables, LPWSTR* psz)
{
	HRESULT hr = S_OK;
	DWORD cchStr = 0;
	DWORD cbStr = 0;
	LPWSTR sz = nullptr;

	hr = FileReadHandle(hVariables, (LPBYTE)&cchStr, sizeof(cchStr));
	BextExitOnFailure(hr, "Failed to read string size.");

	hr = DWordMult(cchStr, sizeof(WCHAR), &cbStr);
	BextExitOnFailure(hr, "String size overflow");

	hr = StrAlloc(&sz, ++cchStr);
	BextExitOnFailure(hr, "Failed to allocate memory");
	BextExitOnNull(sz, hr, E_OUTOFMEMORY, "Failed to allocate memory");
	sz[cchStr - 1] = NULL;

	hr = FileReadHandle(hVariables, (LPBYTE)sz, cbStr);
	BextExitOnFailure(hr, "Failed to read string");

	*psz = sz;
	sz = nullptr;

LExit:
	ReleaseStr(sz);
	return hr;
}
HRESULT CPanelSwBundleVariables::ReadString64(HANDLE hVariables, LPWSTR* psz)
{
	HRESULT hr = S_OK;
	ULONG64 cchStr = 0;
	ULONG64 cbStr = 0;
	LPWSTR sz = nullptr;

	hr = FileReadHandle(hVariables, (LPBYTE)&cchStr, sizeof(cchStr));
	BextExitOnFailure(hr, "Failed to read string size.");

	hr = ULongLongMult(cchStr, sizeof(WCHAR), &cbStr);
	BextExitOnFailure(hr, "String size overflow");

	hr = StrAlloc(&sz, ++cchStr);
	BextExitOnFailure(hr, "Failed to allocate memory");
	BextExitOnNull(sz, hr, E_OUTOFMEMORY, "Failed to allocate memory");
	sz[cchStr - 1] = NULL;

	hr = FileReadHandle(hVariables, (LPBYTE)sz, cbStr);
	BextExitOnFailure(hr, "Failed to read string");

	*psz = sz;
	sz = nullptr;

LExit:
	ReleaseStr(sz);
	return hr;
}


HRESULT CPanelSwBundleVariables::GetVariable(BUNDLE_INFO* pBundleInfo, LPCWSTR szVariableName, BOOL bFormat, LPWSTR* pszValue)
{
	HRESULT hr = S_OK;
	LPWSTR szValue = nullptr;
	BYTE* pbData = nullptr;
	SIZE_T cbData = 0;
	DWORD dwType = 0;
	LPWSTR szEmbeddedVar = nullptr;
	LPWSTR szEmbeddedValue = nullptr;
	LPWSTR szTemp = nullptr;
	BUNDLE_VARIABLE* pVariable = nullptr;

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

	hr = GetBuiltInVariable(pBundleInfo, szVariableName, pszValue);
	BextExitOnFailure(hr, "Failed to replace builtin variable '%ls'", szVariableName);
	if (hr == S_OK)
	{
		ExitFunction();
	}
	hr = S_OK;

	for (SIZE_T i = 0; i < pBundleInfo->_cVariables; ++i)
	{
		if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, pBundleInfo->_rgVariables[i]._szName, -1, szVariableName, -1))
		{
			pVariable = &pBundleInfo->_rgVariables[i];
			hr = StrAllocString(&szValue, pVariable->_szValue, 0);
			BextExitOnFailure(hr, "Failed to allocate string");
			break;
		}
	}
	if (!pVariable)
	{
		hr = S_FALSE;
		ExitFunction();
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
		DWORD dwVarLen = 0;

		ReleaseNullStr(szEmbeddedVar);
		ReleaseNullStr(szEmbeddedValue);
		ReleaseNullStr(szTemp);

		szNextEnd = StrChr(szNext, L']');
		if (!szNextEnd || !*szNextEnd)
		{
			break;
		}

		dwVarLen = szNextEnd - szNext - 1;
		hr = StrAllocFormatted(&szEmbeddedVar, L"%.*ls", dwVarLen, szNext + 1);
		BextExitOnFailure(hr, "Failed to allocate string");

		hr = GetVariable(pBundleInfo, szEmbeddedVar, bFormat, &szEmbeddedValue);
		BextExitOnFailure(hr, "Failed to read embedded variable '%ls'", szEmbeddedVar);
		if (hr == S_FALSE)
		{
			hr = S_OK;
			++szNext;
			continue;
		}

		// Replace the embedded variable and restart
		dwVarLen = szNext - szValue;
		hr = StrAllocFormatted(&szTemp, L"%.*ls%ls%ls", dwVarLen, szValue, szEmbeddedValue, szNextEnd + 1);
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

HRESULT CPanelSwBundleVariables::GetBuiltInVariable(BUNDLE_INFO* pBundleInfo, LPCWSTR szVariableName, LPWSTR* pszValue)
{
	HRESULT hr = S_OK;

	// Bundle property?
	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"WixBundleName", -1, szVariableName, -1))
	{
		hr = StrAllocString(pszValue, pBundleInfo->_szDisplayName ? pBundleInfo->_szDisplayName : L"", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}
	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"WixBundleId", -1, szVariableName, -1))
	{
		hr = StrAllocString(pszValue, pBundleInfo->_szId, 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}
	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"WixBundlePlatform", -1, szVariableName, -1))
	{
		hr = StrAllocString(pszValue, pBundleInfo->_bIsX64 ? L"x64" : L"x86", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}
	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"WixBundleContext", -1, szVariableName, -1))
	{
		hr = StrAllocString(pszValue, pBundleInfo->_bIsMachineContext ? L"machine" : L"user", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}
	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"WixBundleTag", -1, szVariableName, -1))
	{
		hr = StrAllocString(pszValue, pBundleInfo->_szTag ? pBundleInfo->_szTag : L"", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}
	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"WixBundleManufacturer", -1, szVariableName, -1))
	{
		hr = StrAllocString(pszValue, pBundleInfo->_szPublisher ? pBundleInfo->_szPublisher : L"", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}
	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"WixBundleProviderKey", -1, szVariableName, -1))
	{
		hr = StrAllocString(pszValue, pBundleInfo->_szProviderKey ? pBundleInfo->_szProviderKey : L"", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}
	if ((CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"WixBundleVersion", -1, szVariableName, -1)) || (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"WixBundleFileVersion", -1, szVariableName, -1)))
	{
		hr = StrAllocString(pszValue, (pBundleInfo->_pDisplayVersion && pBundleInfo->_pDisplayVersion->sczVersion) ? pBundleInfo->_pDisplayVersion->sczVersion : L"", 0);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}

	// Bitness variables
	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"ProgramFiles6432Folder", -1, szVariableName, -1))
	{
		hr = FormatString(pBundleInfo->_bIsX64 ? L"[ProgramFiles64Folder]" : L"[ProgramFilesFolder]", pszValue);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}
	if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, 0, L"CommonFiles6432Folder", -1, szVariableName, -1))
	{
		hr = FormatString(pBundleInfo->_bIsX64 ? L"[CommonFiles64Folder]" : L"[CommonFilesFolder]", pszValue);
		BextExitOnFailure(hr, "Failed to allocate string");
		ExitFunction();
	}

	hr = S_FALSE;

LExit:
	return hr;
}

HRESULT CPanelSwBundleVariables::FormatString(LPCWSTR szFormat, LPWSTR* pszValue)
{
	HRESULT hr = S_OK;
	LPWSTR szValue = nullptr;
	SIZE_T cch = 0;

	hr = _pEngine->FormatString(szFormat, szValue, &cch);
	if (hr == E_MOREDATA)
	{
		hr = StrAlloc(&szValue, ++cch);
		BextExitOnFailure(hr, "Failed to allocate memory");

		hr = _pEngine->FormatString(szFormat, szValue, &cch);
	}
	BextExitOnFailure(hr, "Failed to format string");

	*pszValue = szValue;
	szValue = nullptr;

LExit:
	ReleaseStr(szValue);

	return hr;
}

#pragma once
#include "pch.h"

class CPanelSwBundleVariables
{
public:
	virtual ~CPanelSwBundleVariables();

	HRESULT SearchBundleVariable(LPCWSTR szUpgradeCode, LPCWSTR szVariableName, BOOL bFormat, LPWSTR *pszValue);

private:

	struct BUNDLE_VARIABLE
	{
		LPWSTR _szName = nullptr;
		LPWSTR _szValue = nullptr;
	};

	struct BUNDLE_INFO
	{
		LPWSTR _szUpgradeCode = nullptr;
		LPWSTR _szId = nullptr;
		LPWSTR _szDisplayName = nullptr;
		LPWSTR _szStatePath = nullptr;
		bool _bIsX64 = false;
		VERUTIL_VERSION* _pEngineVersion = nullptr;
		VERUTIL_VERSION* _pDisplayVersion = nullptr;
		
		BUNDLE_VARIABLE *_rgVariables = nullptr;
		DWORD _cVariables = 0;
	};

	enum BURN_VARIANT_TYPE_V3
	{
		BURN_VARIANT_TYPE_V3_NONE,
		BURN_VARIANT_TYPE_V3_NUMERIC,
		BURN_VARIANT_TYPE_V3_STRING,
		BURN_VARIANT_TYPE_V3_VERSION,
	};

	enum BURN_VARIANT_TYPE_V4
	{
		BURN_VARIANT_TYPE_V4_NONE,
		BURN_VARIANT_TYPE_V4_FORMATTED,
		BURN_VARIANT_TYPE_V4_NUMERIC,
		BURN_VARIANT_TYPE_V4_STRING,
		BURN_VARIANT_TYPE_V4_VERSION,
	};

	HRESULT SearchBundle(LPCWSTR szUpgradeCode, BUNDLE_INFO** ppBundleInfo);
	HRESULT SearchBundle(BUNDLE_INSTALL_CONTEXT context, REG_KEY_BITNESS kbKeyBitness, LPCWSTR szUpgradeCode, BUNDLE_INFO** ppBundleInfo);
	
	HRESULT LoadBundleVariables(BUNDLE_INFO* pBundleInfo);
	HRESULT LoadBundleVariablesV3(BUNDLE_INFO* pBundleInfo);
	HRESULT LoadBundleVariablesV4(BUNDLE_INFO* pBundleInfo);
	HRESULT LoadBundleVariablesV5(BUNDLE_INFO* pBundleInfo);
	HRESULT ReadString(HANDLE hVariables, LPWSTR* psz);
	HRESULT ReadString64(HANDLE hVariables, LPWSTR* psz);
	
	HRESULT GetVariable(BUNDLE_INFO* pBundleInfo, LPCWSTR szVariableName, BOOL bFormat, LPWSTR *pszValue);

	void Reset();

	int _cSearchRecursion = 0;
	int MAX_SEARCH_RECURSION = 100;
	
	BUNDLE_INFO *_rgBundles = nullptr;
	SIZE_T _cBundles = 0;
};

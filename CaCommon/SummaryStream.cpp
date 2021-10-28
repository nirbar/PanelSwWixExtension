#include "stdafx.h"
#include "SummaryStream.h"
#include "WixString.h"
#include <wtypes.h>

HRESULT CSummaryStream::IsUserContext()
{
	HRESULT hr = S_OK;
	INT iWordCount = 0;

	hr = GetSummaryStreamProperty(SummaryStreamProperties::PID_WORDCOUNT, &iWordCount);
	ExitOnFailure(hr, "Failed getting word-count property from summary stream");

	if ((iWordCount & SummaryStreamWordCount::WORD_COUNT_UAC_COMPLIANT) == SummaryStreamWordCount::WORD_COUNT_UAC_COMPLIANT)
	{
		hr = S_OK;
	}
	else
	{
		hr = S_FALSE;
	}

LExit:
	return hr;
}

HRESULT CSummaryStream::IsPackageX64()
{
	HRESULT hr = S_OK;
	CWixString szTemplate;
	DWORD dwSemiColonIndex = INFINITE;
	DWORD dw64Index = INFINITE;

	hr = GetSummaryStreamProperty(SummaryStreamProperties::PID_TEMPLATE, (LPWSTR*)szTemplate);
	ExitOnFailure(hr, "Failed getting template property from summary stream");

	if (szTemplate.IsNullOrEmpty())
	{
		hr = S_FALSE;
		ExitFunction();
	}

	// See https://docs.microsoft.com/en-us/windows/win32/msi/template-summary
	// Template syntax: '[Platform];[lang1[,lang2[,... ]]]'
	dw64Index = szTemplate.Find(L"64");
	if (dw64Index != INFINITE)
	{
		dwSemiColonIndex = szTemplate.Find(L';');
		if ((dwSemiColonIndex == INFINITE) || (dw64Index < dwSemiColonIndex))
		{
			hr = S_OK;
			ExitFunction();
		}
	}
	hr = S_FALSE;

LExit:
	return hr;
}

HRESULT CSummaryStream::GetSummaryStreamProperty(SummaryStreamProperties iProperty, LPWSTR* pszValue)
{
	HRESULT hr = S_OK;
	DWORD dwRes = ERROR_SUCCESS;
	MSIHANDLE hDatabase = NULL;
	PMSIHANDLE hSummaryInfo;
	UINT uiDataType = 0;
	DWORD dwDataSize = 0;
	FILETIME ftJunk;
	INT iJunk;
	CWixString szValue;

	ExitOnNull(pszValue, hr, E_INVALIDARG, "pszValue is NULL");

	hDatabase = WcaGetDatabaseHandle();
	ExitOnNull(hDatabase, hr, E_FAIL, "Failed to get MSI database");

	dwRes = ::MsiGetSummaryInformation(hDatabase, nullptr, 0, &hSummaryInfo);
	ExitOnWin32Error(dwRes, hr, "Failed to open summary stream");

	dwRes = ::MsiSummaryInfoGetProperty(hSummaryInfo, iProperty, &uiDataType, &iJunk, &ftJunk, L"", &dwDataSize);
	if (dwRes != ERROR_MORE_DATA)
	{
		ExitOnWin32Error(dwRes, hr, "Failed to get summary stream property");
		ExitFunction(); // The property is empty
	}
	ExitOnNull((uiDataType == VT_LPSTR), hr, E_INVALIDARG, "Property data type is %u. Expected VT_LPSTR", uiDataType);

	++dwDataSize;
	hr = szValue.Allocate(dwDataSize);
	ExitOnFailure(hr, "Failed to allocate memory");

	dwRes = ::MsiSummaryInfoGetProperty(hSummaryInfo, iProperty, &uiDataType, &iJunk, &ftJunk, (LPWSTR)szValue, &dwDataSize);
	ExitOnWin32Error(dwRes, hr, "Failed to get summary stream property");

	*pszValue = szValue;
	szValue = nullptr;

LExit:
	return hr;
}

HRESULT CSummaryStream::GetSummaryStreamProperty(SummaryStreamProperties iProperty, INT* piValue)
{
	HRESULT hr = S_OK;
	DWORD dwRes = ERROR_SUCCESS;
	MSIHANDLE hDatabase = NULL;
	PMSIHANDLE hSummaryInfo;
	UINT uiDataType = 0;
	DWORD dwDataSize = 0;
	FILETIME ftJunk;

	ExitOnNull(piValue, hr, E_INVALIDARG, "pdwValue is NULL");

	hDatabase = WcaGetDatabaseHandle();
	ExitOnNull(hDatabase, hr, E_FAIL, "Failed to get MSI database");

	dwRes = ::MsiGetSummaryInformation(hDatabase, nullptr, 0, &hSummaryInfo);
	ExitOnWin32Error(dwRes, hr, "Failed to open summary stream");

	dwRes = ::MsiSummaryInfoGetProperty(hSummaryInfo, iProperty, &uiDataType, piValue, &ftJunk, L"", &dwDataSize);
	ExitOnWin32Error(dwRes, hr, "Failed to get summary stream property");
	ExitOnNull(((uiDataType == VT_I4) || (uiDataType == VT_I2)), hr, E_INVALIDARG, "Property data type is %u. Expected VT_I4", uiDataType);

LExit:
	return hr;
}
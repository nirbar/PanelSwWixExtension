#include "stdafx.h"
#include "SummaryStream.h"
#include <strutil.h>

CSummaryStream CSummaryStream::_sInst;


CSummaryStream* CSummaryStream::GetInstance()
{
	return &_sInst;
}

CSummaryStream::CSummaryStream(void)
	: _pTemplate( nullptr)
{
}

CSummaryStream::~CSummaryStream(void)
{
}

HRESULT CSummaryStream::IsPackageX64( bool *pIsX64)
{
	HRESULT hr = S_OK;

	BreakExitOnNull( pIsX64, hr, E_INVALIDARG, "pIsX64 is NULL");

	if (!_pTemplate)
	{
		hr = GetProperty(SummaryStreamProperties::PID_TEMPLATE, &_pTemplate);
		BreakExitOnFailure(hr, "Failed getting template property from summary stream");
		BreakExitOnNull(_pTemplate, hr, E_FAIL, "Failed getting template property from summary stream");
	}
	
	MsiDebugBreak();

	if (::wcsstr(_pTemplate, L"Intel64") || ::wcsstr(_pTemplate, L"x64"))
	{
		(*pIsX64) = false;
	}
	else
	{
		(*pIsX64) = true;
	}

LExit:

	return hr; 
}

HRESULT CSummaryStream::GetProperty( SummaryStreamProperties eProp, LPWSTR *ppProp)
{
	HRESULT hr = S_OK;
	DWORD dwRes = ERROR_SUCCESS;
	MSIHANDLE hDatabase = NULL;
	PMSIHANDLE hSummaryInfo;
	UINT uiDataType = 0;
	DWORD dwDataSize = 0;
	FILETIME ftJunk;
	INT iJunk;

	BreakExitOnNull( ppProp, hr, E_INVALIDARG, "ppProp is NULL");

	hDatabase = WcaGetDatabaseHandle();
	BreakExitOnNull( hDatabase, hr, E_FAIL, "Failed to get MSI database");

	dwRes = ::MsiGetSummaryInformation( hDatabase, nullptr, 0, &hSummaryInfo);
	hr = HRESULT_FROM_WIN32( dwRes);
	BreakExitOnFailure( hr, "Failed to get summary stream");

	dwRes = ::MsiSummaryInfoGetProperty( hSummaryInfo, eProp , &uiDataType, &iJunk, &ftJunk, L"", &dwDataSize);
	if( dwRes != ERROR_MORE_DATA)
	{
		hr = E_INVALIDSTATE;
		BreakExitOnFailure( hr, "Failed getting MSI Template property size from summary stream");
	}

	++dwDataSize;
	hr = StrAlloc( ppProp, dwDataSize);
	BreakExitOnFailure( hr, "Failed to allocate memory");
	
	dwRes = ::MsiSummaryInfoGetProperty( hSummaryInfo, eProp , &uiDataType, &iJunk, &ftJunk, (*ppProp), &dwDataSize);
	hr = HRESULT_FROM_WIN32( dwRes);
	BreakExitOnFailure( hr, "Failed to get summary stream property with PID=%u", eProp);
	
LExit:
	return hr;
}

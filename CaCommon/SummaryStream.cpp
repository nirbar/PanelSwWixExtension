#include "stdafx.h"
#include "SummaryStream.h"
#include <strutil.h>

CSummaryStream CSummaryStream::_sInst;


CSummaryStream* CSummaryStream::GetInstance()
{
	return &_sInst;
}

CSummaryStream::CSummaryStream(void)
	: _pTemplate( NULL)
{
}

CSummaryStream::~CSummaryStream(void)
{
}

HRESULT CSummaryStream::IsPackageX64( bool *pIsX64)
{
	HRESULT hr = S_OK;

	if( _pTemplate == NULL)
	{
		hr = GetProperty( SummaryStreamProperties::PID_TEMPLATE, &_pTemplate);
		ExitOnFailure( hr, "Failed getting template property from summary stream");
		ExitOnNull( _pTemplate, hr, E_FAIL, "Failed getting template property from summary stream");
	}
	
	if(( ::wcsstr( _pTemplate, L"Intel64") != NULL) || ( ::wcsstr( _pTemplate, L"x64") != NULL))
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

	ExitOnNull( ppProp, hr, E_INVALIDARG, "ppProp is NULL");

	hDatabase = WcaGetDatabaseHandle();
	ExitOnNull( hDatabase, hr, E_FAIL, "Failed to get MSI database");

	dwRes = ::MsiGetSummaryInformation( hDatabase, NULL, 0, &hSummaryInfo);
	hr = HRESULT_FROM_WIN32( dwRes);
	ExitOnFailure( hr, "Failed to get summary stream");

	dwRes = ::MsiSummaryInfoGetProperty( hSummaryInfo, eProp , &uiDataType, &iJunk, &ftJunk, L"", &dwDataSize);
	if( dwRes != ERROR_MORE_DATA)
	{
		hr = E_INVALIDSTATE;
		ExitOnFailure( hr, "Failed getting MSI Template property size from summary stream");
	}

	++dwDataSize;
	hr = StrAlloc( ppProp, dwDataSize);
	ExitOnFailure( hr, "Failed to allocate memory");
	
	dwRes = ::MsiSummaryInfoGetProperty( hSummaryInfo, eProp , &uiDataType, &iJunk, &ftJunk, (*ppProp), &dwDataSize);
	hr = HRESULT_FROM_WIN32( dwRes);
	ExitOnFailure( hr, "Failed to get summary stream template property");
	
LExit:
	return hr;
}

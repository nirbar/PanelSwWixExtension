#include "stdafx.h"
#include "IFileFilter.h"
#include "FileGlobFilter.h"
#include "FileSpecFilter.h"

/*static*/ HRESULT IFileFilter::InferFilter(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive, IFileFilter** ppFilter)
{
	HRESULT hr = S_OK;
	bool bGlob = false;
	IFileFilter* pFilter = nullptr;
	
	if (!(szFilter && *szFilter))
	{
		ExitFunction();
	}

	bGlob = wcschr(szFilter, L'/') || wcschr(szFilter, L'\\') || wcschr(szFilter, L'{') || wcschr(szFilter, L'[') || wcsstr(szFilter, L"**");

	if (bGlob)
	{
		pFilter = new CFileGlobFilter();
		ExitOnNull(pFilter, hr, E_OUTOFMEMORY, "Failed to instanciate CFileGlobFilter");
	}
	else
	{
		pFilter = new CFileSpecFilter();
		ExitOnNull(pFilter, hr, E_OUTOFMEMORY, "Failed to instanciate CFileSpecFilter");
	}
		
	hr = pFilter->Initialize(szBaseFolder, szFilter, bRecursive);
	ExitOnFailure(hr, "Failed to initialize filter for pattern '%ls'", szFilter);
	
	*ppFilter = pFilter;
	pFilter = nullptr;
	

LExit:
	if (pFilter)
	{
		delete pFilter;
	}

	return hr;
}

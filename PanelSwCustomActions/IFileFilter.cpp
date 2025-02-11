#include "stdafx.h"
#include "IFileFilter.h"
#include "FileGlobFilter.h"
#include "FileSpecFilter.h"

/*static*/ HRESULT IFileFilter::InferFilter(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive, IFileFilter** ppFilter)
{
	HRESULT hr = S_OK;
	bool bMustBeGlob = false;
	bool bMustBeFileSpec = false;
	IFileFilter* pFilter = nullptr;
	
	if (!(szFilter && *szFilter))
	{
		ExitFunction();
	}

	bMustBeGlob = wcschr(szFilter, L'/') || wcschr(szFilter, L'\\');
	bMustBeFileSpec = wcschr(szFilter, L'?');
	ExitOnNull((!(bMustBeFileSpec && bMustBeGlob)), hr, E_INVALIDARG, "File pattern '%ls' is invalid as either glob or filespec", szFilter);

	if (bMustBeGlob)
	{
		pFilter = new CFileGlobFilter();
		ExitOnNull(pFilter, hr, E_OUTOFMEMORY, "Failed to instanciate CFileGlobFilter");
	}
	else if (bMustBeFileSpec)
	{
		pFilter = new CFileSpecFilter();
		ExitOnNull(pFilter, hr, E_OUTOFMEMORY, "Failed to instanciate CFileSpecFilter");
	}
	// ** is probably glob
	else if (wcsstr(szFilter, L"**"))
	{
		pFilter = new CFileGlobFilter();
		ExitOnNull(pFilter, hr, E_OUTOFMEMORY, "Failed to instanciate CFileGlobFilter");
	}
	// Default to filespec because it is more widespread in Windows
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

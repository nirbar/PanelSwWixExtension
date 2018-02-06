#include "../CaCommon/WixString.h"
#include <stdlib.h>
#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")

#pragma region SplitPath

#define PathToSplitProp				L"FULL_PATH_TO_SPLIT"
#define SplitDriveProp				L"SPLIT_DRIVE"
#define SplitDirectoryProp			L"SPLIT_FOLDER"
#define SplitFileNameProp			L"SPLIT_FILE_NAME"
#define SplitFileExtProp			L"SPLIT_FILE_EXTENSION"

extern "C" __declspec(dllexport) UINT SplitPath(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	CWixString szFullPath;
	WCHAR szDrive[_MAX_DRIVE + 1];
	WCHAR szFolder[_MAX_DIR + 1];
	WCHAR szName[_MAX_FNAME + 1];
	WCHAR szExt[_MAX_EXT + 1];

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");

	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Get property-to-encrypt name
	hr = WcaGetProperty(PathToSplitProp, (LPWSTR*)szFullPath);
	ExitOnFailure1(hr, "Failed getting %ls", PathToSplitProp);
	if (szFullPath.IsNullOrEmpty())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No path to split...");
		ExitFunction();
	}
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will split property '%ls'", (LPCWSTR)szFullPath);

	er = ::_wsplitpath_s<_MAX_DRIVE + 1, _MAX_DIR + 1, _MAX_FNAME + 1, _MAX_EXT + 1>((LPCWSTR)szFullPath, szDrive, szFolder, szName, szExt);
	ExitOnNull1( ( er==ERROR_SUCCESS), hr, E_FAIL, "Failed splitting '%ls' full-path", (LPCWSTR)szFullPath);
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Split full path '%ls' to '%ls' '%ls' '%ls' '%ls'", szFullPath, szDrive, szFolder, szName, szExt);

	// Store back in properties
	hr = WcaSetProperty(SplitDriveProp, szDrive);
	ExitOnFailure1(hr, "Failed setting the '%ls'", szDrive);

	hr = WcaSetProperty(SplitDirectoryProp, szFolder);
	ExitOnFailure1(hr, "Failed setting the '%ls'", szFolder);

	hr = WcaSetProperty(SplitFileNameProp, szName);
	ExitOnFailure1(hr, "Failed setting the '%ls'", szName);

	hr = WcaSetProperty(SplitFileExtProp, szExt);
	ExitOnFailure1(hr, "Failed setting the '%ls'", SplitFileExtProp);

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

#undef PathToSplitProp		
#undef SplitDriveProp		
#undef SplitDirectoryProp	
#undef SplitFileNameProp	
#undef SplitFileExtProp	

#pragma endregion

#pragma region PathExists

#define PathExistsProp				L"FULL_PATH_TO_TEST"
#define PathExistsResultProp		L"PATH_EXISTS"

extern "C" __declspec(dllexport) UINT PathExists(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	CWixString szFullPath;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Get full path to test
	hr = WcaGetProperty(PathExistsProp, (LPWSTR*)szFullPath);
	ExitOnFailure1(hr, "Failed getting %ls", PathExistsProp);
	if (szFullPath.IsNullOrEmpty())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No path to test...");

		hr = WcaSetProperty(PathExistsResultProp, L"");
		ExitOnFailure1(hr, "Failed setting %ls", PathExistsResultProp);

		ExitFunction();
	}

	// Test if path exists
	if (::PathFileExists((LPCWSTR)szFullPath))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Path '%ls' exists", (LPCWSTR)szFullPath);

		hr = WcaSetIntProperty(PathExistsResultProp, 1);
		ExitOnFailure1(hr, "Failed setting the '%ls'", PathExistsResultProp);
	}
	// Doesn't exist.
	else
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Path '%ls' does not exist", (LPCWSTR)szFullPath);

		hr = WcaSetProperty(PathExistsResultProp, L"");
		ExitOnFailure1(hr, "Failed setting %ls", PathExistsResultProp);
	}

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

#undef PathExistsProp		
#undef PathExistsResultProp

#pragma endregion

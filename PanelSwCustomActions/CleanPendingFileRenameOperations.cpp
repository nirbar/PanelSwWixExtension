
#include "stdafx.h"
#include <strutil.h>
#include <regutil.h>
#include <vector>
#include <Shlwapi.h>
using namespace std;
#pragma comment(lib, "Shlwapi.lib")

// See MoveFileEx() https://msdn.microsoft.com/en-us/library/windows/desktop/aa365240(v=vs.85).aspx
extern "C" __declspec(dllexport) int __cdecl CleanPendingFileRenameOperations(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	HKEY hKey = NULL;
	DWORD dwSize = 0;
	DWORD dwStrSize = 0;
	DWORD dwIndex = 0;
	LPWSTR szPendingFileRenameOperations = NULL;
	LPWSTR szCleanPendingFileRenameOperations = NULL;
	vector<LPCWSTR> vecFiles;
	BOOL bAnyChange = FALSE;
	errno_t ern = 0;

	/*
	::MoveFileEx(L"C:\\Program Files (x86)\\UnitTestSetup\\Product.wxs", L"C:\\Program Files (x86)\\UnitTestSetup\\Product.wxs1", MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
	::MoveFileEx(L"C:\\Program Files (x86)\\UnitTestSetup\\Product.wxs1", NULL, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
	return 0;
	*/

	hr = WcaInitialize(hInstall, "CleanPendingFileRenameOps");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	hr = RegOpen(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager", GENERIC_ALL, &hKey);
	ExitOnFailure(hr, "Failed to open Session Manager registry key");

	er = ::RegQueryValueEx(hKey, L"PendingFileRenameOperations", NULL, NULL, NULL, &dwSize);
	if (er == ERROR_FILE_NOT_FOUND)
	{
		WcaLog(LOGMSG_STANDARD, "No pending file rename operations.");
		ExitFunction1(hr = S_OK);
	}
	ExitOnWin32Error(er, hr, "Failed querying value size of PendingFileRenameOperations");

	dwStrSize = 1 + (dwSize / sizeof(WCHAR)); // Append NULL.
	hr = StrAlloc(&szPendingFileRenameOperations, dwStrSize);
	ExitOnFailure(hr, "Failed allocating memory");

	er = ::RegQueryValueEx(hKey, L"PendingFileRenameOperations", NULL, NULL, (LPBYTE)szPendingFileRenameOperations, &dwSize);
	ExitOnWin32Error(er, hr, "Failed querying value of PendingFileRenameOperations");
	szPendingFileRenameOperations[dwStrSize - 1] = NULL;

	dwIndex = 0;
	while (dwIndex < (dwStrSize - 1))
	{
		dwSize = ::wcslen(szPendingFileRenameOperations + dwIndex);
		vecFiles.push_back(szPendingFileRenameOperations + dwIndex);

		dwIndex += dwSize + 1;
	}

	// Odd indices will be created/replaced
	// Even indices will be deleted/moved
	// If an odd spot and even spot resolve to the same file, we'll keep the creation only.
	for (DWORD i = 1; i < vecFiles.size(); i += 2)
	{
		LPCWSTR szRename = vecFiles[i];
		if (!szRename || !*szRename)
		{
			continue;
		}

		// Undocumented PendingFileRenameOperations operators.
		if (wcsncmp(szRename, L"!", 1) == 0)
		{
			++szRename;
		}
		if (wcsncmp(szRename, L"\\??\\", 4) == 0)
		{
			szRename += 4;
		}

		for (DWORD j = 0; j < vecFiles.size(); j += 2)
		{
			LPCWSTR szDelete = vecFiles[j];
			if (!szDelete || !*szDelete)
			{
				continue;
			}
			// Undocumented PendingFileRenameOperations operators.
			if (wcsncmp(szDelete, L"!", 1) == 0)
			{
				++szDelete;
			}
			if (wcsncmp(szDelete, L"\\??\\", 4) == 0)
			{
				szDelete += 4;
			}

			// Same file is to be both created and deleted?
			if (::wcsicmp(szRename, szDelete) == 0)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File '%ls' is scheduled to be created and deleted after reboot. Keeping the creation only.", szDelete);
				bAnyChange = TRUE;
				vecFiles[j] = NULL; // Keep the creation only.
			}
		}
	}

	if (!bAnyChange)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Creation and deletion of files after reboot are OK.");
		ExitFunction1(hr = S_OK);
	}

	// Allocate max size
	hr = ::StrAlloc(&szCleanPendingFileRenameOperations, dwStrSize);
	ExitOnFailure(hr, "Failed allocating memory");

	// Ensure terminating NULL's
	::ZeroMemory(szCleanPendingFileRenameOperations, dwStrSize * sizeof(WCHAR));
	dwIndex = 0;

	for (DWORD i = 1; i < vecFiles.size(); i += 2)
	{
		LPCWSTR szDelete = vecFiles[i - 1];
		LPCWSTR szCreate = vecFiles[i];

		// Cleaned that operation?
		if (!szDelete || !*szDelete)
		{
			continue;
		}

		dwSize = ::wcslen(szDelete);
		ern = ::wcscpy_s(szCleanPendingFileRenameOperations + dwIndex, dwStrSize - dwIndex, szDelete);
		ExitOnNull((ern == 0), hr, -::abs(ern), "Failed copying string");
		dwIndex += 1 + dwSize;

		// Delete only?
		if (!szCreate || !*szCreate)
		{
			++dwIndex;
			continue;
		}

		dwSize = ::wcslen(szCreate);
		ern = ::wcscpy_s(szCleanPendingFileRenameOperations + dwIndex, dwStrSize - dwIndex, szCreate);
		ExitOnNull((ern == 0), hr, -::abs(ern), "Failed copying string");
		dwIndex += 1 + dwSize;
	}
	++dwIndex; // Terminate with double NULL

	er = ::RegSetValueEx(hKey, L"PendingFileRenameOperations", 0, REG_MULTI_SZ, (LPCBYTE)szCleanPendingFileRenameOperations, dwIndex * sizeof(WCHAR));
	ExitOnWin32Error(er, hr, "Failed writing PendingFileRenameOperations registry key");

LExit:

	if (hKey)
	{
		::RegCloseKey(hKey);
	}
	if (szPendingFileRenameOperations)
	{
		StrFree(szPendingFileRenameOperations);
	}
	if (szCleanPendingFileRenameOperations)
	{
		StrFree(szCleanPendingFileRenameOperations);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}


#include "stdafx.h"
#include <strutil.h>
#include <regutil.h>
#include <vector>
#include <list>
#include <Shlwapi.h>
#include "../CaCommon/WixString.h"
using namespace std;
#pragma comment(lib, "Shlwapi.lib")

#define CleanPendingFileRenameOperationsSched_QUERY L"SELECT `File`, `Component_` FROM `File`"
enum CleanPendingFileRenameOperationsSchedQuery { File = 1, Component = 2 };

extern "C" __declspec(dllexport) int __cdecl CleanPendingFileRenameOperationsSched(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	HKEY hKey = NULL;
	DWORD dwSize = 0;
	DWORD dwStrSize = 0;
	DWORD dwIndex = 0;
	LPWSTR szPendingFileRenameOperations = nullptr;
	CWixString szCleanPendingFileRenameOperations;
	vector<LPCWSTR> vecFiles;
	list<LPCWSTR> lstDeletedFiles;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = WcaInitialize(hInstall, "CleanPendingFileRenameOps");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = RegOpen(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager", GENERIC_READ, &hKey);
	ExitOnFailure(hr, "Failed to open Session Manager registry key");

	er = ::RegQueryValueEx(hKey, L"PendingFileRenameOperations", nullptr, nullptr, nullptr, &dwSize);
	if (er == ERROR_FILE_NOT_FOUND)
	{
		WcaLog(LOGMSG_STANDARD, "No pending file rename operations.");
		ExitFunction1(hr = S_OK);
	}
	ExitOnWin32Error(er, hr, "Failed querying value size of PendingFileRenameOperations");

	dwStrSize = 1 + (dwSize / sizeof(WCHAR)); // Append NULL.
	hr = StrAlloc(&szPendingFileRenameOperations, dwStrSize);
	ExitOnFailure(hr, "Failed allocating memory");

	// Ensure terminating NULL's
	::ZeroMemory(szPendingFileRenameOperations, dwStrSize * sizeof(WCHAR));

	er = ::RegQueryValueEx(hKey, L"PendingFileRenameOperations", nullptr, nullptr, (LPBYTE)szPendingFileRenameOperations, &dwSize);
	ExitOnWin32Error(er, hr, "Failed querying value of PendingFileRenameOperations");

	dwIndex = 0;
	while (dwIndex < (dwStrSize - 1))
	{
		dwSize = ::wcslen(szPendingFileRenameOperations + dwIndex);
		vecFiles.push_back(szPendingFileRenameOperations + dwIndex);

		dwIndex += dwSize + 1;
	}
	
	// Detect files scheduled to delete
	for (DWORD i = 1; i < vecFiles.size(); i += 2)
	{
		LPCWSTR szRename = vecFiles[i];
		LPCWSTR szDelete = vecFiles[i - 1];
		if ((szRename && *szRename) || !szDelete || !*szDelete)
		{
			continue;
		}

		// Undocumented PendingFileRenameOperations operators.
		if (::wcsncmp(szDelete, L"!", 1) == 0)
		{
			++szDelete;
		}
		if (::wcsncmp(szDelete, L"\\??\\", 4) == 0)
		{
			szDelete += 4;
		}

		lstDeletedFiles.push_back(szDelete);
	}

	// Allocate max size
	hr = szCleanPendingFileRenameOperations.Allocate(dwStrSize);
	ExitOnFailure(hr, "Failed allocating memory");

	// Iterate files to be installed.
	hr = WcaOpenExecuteView(CleanPendingFileRenameOperationsSched_QUERY, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", CleanPendingFileRenameOperationsSched_QUERY);

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szComponent, szFilePathFmt, szFilePath;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, CleanPendingFileRenameOperationsSchedQuery::File, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, CleanPendingFileRenameOperationsSchedQuery::Component, (LPWSTR*)szComponent);
		BreakExitOnFailure(hr, "Failed to get Component.");

		compAction = WcaGetComponentToDo((LPCWSTR)szComponent);
		if (compAction != WCA_TODO::WCA_TODO_INSTALL)
		{
			continue;
		}

		hr = szFilePathFmt.Format(L"[#%s]", (LPCWSTR)szId);
		BreakExitOnFailure(hr, "Failed formatting string.");

		hr = szFilePath.MsiFormat((LPCWSTR)szFilePathFmt);
		BreakExitOnFailure(hr, "Failed MSI-formatting string.");

		for (LPCWSTR szDelete : lstDeletedFiles)
		{
			if (::wcsicmp(szDelete, (LPCWSTR)szFilePath) == 0)
			{
				hr = szCleanPendingFileRenameOperations.AppnedFormat(L"%s;", szDelete);
				BreakExitOnFailure(hr, "Failed to append string.");
			}
		}
	}
	hr = S_OK;

	if (!szCleanPendingFileRenameOperations.IsNullOrEmpty())
	{
		hr = WcaSetProperty(L"CleanPendingFileRenameOperations", (LPCWSTR)szCleanPendingFileRenameOperations);
		BreakExitOnFailure(hr, "Failed to set property.");
	}

LExit:

	if (hKey)
	{
		::RegCloseKey(hKey);
	}
	if (szPendingFileRenameOperations)
	{
		StrFree(szPendingFileRenameOperations);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

extern "C" __declspec(dllexport) int __cdecl CleanPendingFileRenameOperations(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	HKEY hKey = NULL;
	DWORD dwSize = 0;
	DWORD dwStrSize = 0;
	DWORD dwIndex = 0;
	LPWSTR szPendingFileRenameOperations = nullptr;
	LPWSTR szCleanPendingFileRenameOperations = nullptr;
	CWixString szPreventDelete;
	LPCWSTR szToken = nullptr;
	vector<LPCWSTR> vecFiles;
	BOOL bAnyChange = FALSE;
	errno_t ern = 0;

	hr = WcaInitialize(hInstall, "CleanPendingFileRenameOps");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaGetProperty(L"CustomActionData", (LPWSTR*)szPreventDelete);
	BreakExitOnFailure(hr, "Failed getting 'CustomActionData'");

	hr = RegOpen(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager", GENERIC_ALL, &hKey);
	ExitOnFailure(hr, "Failed to open Session Manager registry key");

	er = ::RegQueryValueEx(hKey, L"PendingFileRenameOperations", nullptr, nullptr, nullptr, &dwSize);
	if (er == ERROR_FILE_NOT_FOUND)
	{
		WcaLog(LOGMSG_STANDARD, "No pending file rename operations.");
		ExitFunction1(hr = S_OK);
	}
	ExitOnWin32Error(er, hr, "Failed querying value size of PendingFileRenameOperations");

	dwStrSize = 1 + (dwSize / sizeof(WCHAR)); // Append NULL.
	hr = StrAlloc(&szPendingFileRenameOperations, dwStrSize);
	ExitOnFailure(hr, "Failed allocating memory");

	// Ensure terminating NULL's
	::ZeroMemory(szPendingFileRenameOperations, dwStrSize * sizeof(WCHAR));

	er = ::RegQueryValueEx(hKey, L"PendingFileRenameOperations", nullptr, nullptr, (LPBYTE)szPendingFileRenameOperations, &dwSize);
	ExitOnWin32Error(er, hr, "Failed querying value of PendingFileRenameOperations");

	dwIndex = 0;
	while (dwIndex < (dwStrSize - 1))
	{
		dwSize = ::wcslen(szPendingFileRenameOperations + dwIndex);
		vecFiles.push_back(szPendingFileRenameOperations + dwIndex);

		dwIndex += dwSize + 1;
	}

	// Remove files for which we prevent deletion from the list
	for (hr = szPreventDelete.Tokenize(L";", &szToken); SUCCEEDED(hr); hr = szPreventDelete.NextToken(L";", &szToken))
	{
		for (DWORD i = 1; i < vecFiles.size(); i += 2)
		{
			LPCWSTR szRename = vecFiles[i];
			LPCWSTR szDelete = vecFiles[i - 1];
			if ((szRename && *szRename) || !szDelete || !*szDelete)
			{
				continue;
			}
			if (::wcsncmp(szDelete, L"!", 1) == 0)
			{
				++szDelete;
			}
			if (::wcsncmp(szDelete, L"\\??\\", 4) == 0)
			{
				szDelete += 4;
			}

			if (::wcsicmp(szToken, szDelete) == 0)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File '%ls' is scheduled to be deleted after reboot. Unscheduling.", szDelete);
				bAnyChange = TRUE;
				vecFiles[i - 1] = nullptr;
			}
		}
	}
	ExitOnNull((hr == E_NOMOREITEMS), hr, hr, "Failed tokenizing string");
	hr = S_OK;

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
		if (::wcsncmp(szRename, L"!", 1) == 0)
		{
			++szRename;
		}
		if (::wcsncmp(szRename, L"\\??\\", 4) == 0)
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
			if (::wcsncmp(szDelete, L"!", 1) == 0)
			{
				++szDelete;
			}
			if (::wcsncmp(szDelete, L"\\??\\", 4) == 0)
			{
				szDelete += 4;
			}

			// Same file is to be both created and deleted?
			if (::wcsicmp(szRename, szDelete) == 0)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File '%ls' is scheduled to be created and deleted after reboot. Keeping the creation only.", szDelete);
				bAnyChange = TRUE;
				vecFiles[j] = nullptr; // Keep the creation only.
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

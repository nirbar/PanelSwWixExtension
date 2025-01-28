#include "stdafx.h"
#include "FileOperations.h"
#include "ReparsePoint.h"
#include "../CaCommon/ErrorPrompter.h"
#include "../CaCommon/WixString.h"
#include "FileEntry.h"
#include "FileIterator.h"
#include <fileutil.h>
#include <dirutil.h>
#include <memutil.h>
#include <pathutil.h>
#include <Shellapi.h>
#include <Shlwapi.h>

enum RemoveFolderExLongPathHandling
{
	RemoveFolderExLongPathHandling_Default = 0,
	RemoveFolderExLongPathHandling_Ignore = 1,
	RemoveFolderExLongPathHandling_Rename = 2,
	RemoveFolderExLongPathHandling_Prompt = 3,
};

static void HandleLongPaths(LPWSTR* pszLongPaths, UINT cLongPaths, RemoveFolderExLongPathHandling longPathHandling);

extern "C" UINT __stdcall RemoveFolderEx(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	MSIHANDLE hRemoveFileTable = NULL;
	MSIHANDLE hRemoveFileColumns = NULL;
	LPWSTR* pszLongPaths = nullptr;
	UINT cLongPaths = 0;
	DWORD dwRes = 0;
	RemoveFolderExLongPathHandling longPathHandling = RemoveFolderExLongPathHandling::RemoveFolderExLongPathHandling_Default;
	bool bCanRenameLongPath = true;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_RemoveFolderEx exists.
	hr = WcaTableExists(L"PSW_RemoveFolderEx");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_RemoveFolderEx'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_RemoveFolderEx'. Have you authored 'PanelSw:RemoveFolderEx' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Component_`, `Property`, `InstallMode`, `LongPathHandling`, `Condition` FROM `PSW_RemoveFolderEx`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szComponent, szBaseProperty, szBasePath, szCondition;
		int flags = 0, currLongPathHandling = 0;
		CFileIterator fileFinder;
		UINT i = 0;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component_.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szBaseProperty);
		ExitOnFailure(hr, "Failed to get Property.");
		hr = WcaGetRecordInteger(hRecord, 3, &flags);
		ExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordInteger(hRecord, 4, &currLongPathHandling);
		ExitOnFailure(hr, "Failed to get LongPathHandling.");
		hr = WcaGetRecordString(hRecord, 5, (LPWSTR*)szCondition);
		ExitOnFailure(hr, "Failed to get Property.");

		hr = WcaGetProperty(szBaseProperty, (LPWSTR*)szBasePath);
		ExitOnFailure(hr, "Failed to get property");

		// Apply long path handling even if conditions don't apply
		if (currLongPathHandling != RemoveFolderExLongPathHandling::RemoveFolderExLongPathHandling_Default)
		{
			longPathHandling = (RemoveFolderExLongPathHandling)currLongPathHandling;
		}

		// Test condition
		if (!szCondition.IsNullOrEmpty())
		{
			MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, szCondition);
			switch (condRes)
			{
			case MSICONDITION::MSICONDITION_NONE:
			case MSICONDITION::MSICONDITION_TRUE:
				break;
			case MSICONDITION::MSICONDITION_FALSE:
				WcaLog(LOGMSG_STANDARD, "Skipping %ls for '%ls'. Condition evaluated false", __FUNCTIONW__, (LPCWSTR)szBaseProperty);
				continue;
			case MSICONDITION::MSICONDITION_ERROR:
				hr = E_FAIL;
				ExitOnFailure(hr, "Bad Condition field");
			}
		}

		if (szBasePath.IsNullOrEmpty())
		{
			CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Skipping RemoveFolderEx for property '%ls' because it is empty", (LPCWSTR)szBaseProperty);
			continue;
		}

		hr = WcaAddTempRecord(&hRemoveFileTable, &hRemoveFileColumns, L"RemoveFile", nullptr, 1, 5, L"RfxFolder", (LPCWSTR)szComponent, nullptr, (LPCWSTR)szBaseProperty, flags);
		ExitOnFailure(hr, "Failed to add temporary row table");

		// Skip if this isn't a folder, or if it is a reaprse point
		{
			CFileEntry fileEntry(szBasePath);
			if (!fileEntry.IsDirectory() || fileEntry.IsSymlink() || fileEntry.IsMountPoint())
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Path is not a folder, or it is a symlink, or it is a mounted folder: '%ls'", (LPCWSTR)szBasePath);
				continue;
			}
		}

		hr = WcaAddTempRecord(&hRemoveFileTable, &hRemoveFileColumns, L"RemoveFile", nullptr, 1, 5, L"RfxFiles", (LPCWSTR)szComponent, L"*", (LPCWSTR)szBaseProperty, flags);
		ExitOnFailure(hr, "Failed to add temporary row table");

		for (CFileEntry fileEntry = fileFinder.Find(szBasePath, L"*", true); !fileFinder.IsEnd(); fileEntry = fileFinder.Next())
		{
			ExitOnNull(fileEntry.IsValid(), hr, fileFinder.Status(), "Failed to find files in '%ls'", (LPCWSTR)szBasePath);

			if (fileEntry.IsDirectory())
			{
				CWixString szDirProperty;
				++i;

				hr = szDirProperty.Format(L"_DIR_%ls_%u", (LPCWSTR)szBaseProperty, i);
				ExitOnFailure(hr, "Failed to format string");

				hr = WcaSetProperty((LPCWSTR)szDirProperty, (LPCWSTR)fileEntry.Path());
				ExitOnFailure(hr, "Failed to set property");

				hr = WcaAddTempRecord(&hRemoveFileTable, &hRemoveFileColumns, L"RemoveFile", nullptr, 1, 5, L"RfxFolder", (LPCWSTR)szComponent, nullptr, (LPCWSTR)szDirProperty, flags);
				ExitOnFailure(hr, "Failed to add temporary row table");

				if (!fileEntry.IsMountPoint() && !fileEntry.IsSymlink())
				{
					hr = WcaAddTempRecord(&hRemoveFileTable, &hRemoveFileColumns, L"RemoveFile", nullptr, 1, 5, L"RfxFiles", (LPCWSTR)szComponent, L"*", (LPCWSTR)szDirProperty, flags);
					ExitOnFailure(hr, "Failed to add temporary row table");
				}
			}
			else if (bCanRenameLongPath && (fileEntry.Path().StrLen() > MAX_PATH))
			{
				if (fileEntry.ParentPath().StrLen() > (MAX_PATH - 13)) // If the folder name doesn't allow 8.3 files then we can't help
				{
					WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Folder has too long path, so there is insufficient space for 8.3 file names. Disabling long path handling. '%ls'", (LPCWSTR)fileEntry.ParentPath());
					bCanRenameLongPath = false;
				}
				else
				{
					hr = StrArrayAllocString(&pszLongPaths, &cLongPaths, fileEntry.Path(), 0);
					ExitOnFailure(hr, "Failed to allocate string");
				}
			}
		}
	}
	hr = S_OK;
	
	if (bCanRenameLongPath && pszLongPaths && cLongPaths)
	{
		HandleLongPaths(pszLongPaths, cLongPaths, longPathHandling);
	}

LExit:
	if (hRemoveFileTable)
	{
		::MsiCloseHandle(hRemoveFileTable);
	}
	if (hRemoveFileColumns)
	{
		::MsiCloseHandle(hRemoveFileColumns);
	}
	ReleaseStrArray(pszLongPaths, cLongPaths);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

// This code runs before CostInitialize so we don't know component action yet, hence we don't know if the file is scheduled for deletion
// Both rollback and deferred actions should be scheduled to revert the file move
// The user is prompted whether or not to irrevokably attempt to delete the files
static void HandleLongPaths(LPWSTR* pszLongPaths, UINT cLongPaths, RemoveFolderExLongPathHandling longPathHandling)
{
	CFileOperations cad;
	CWixString szAllFiles;
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;

	if (!pszLongPaths || !cLongPaths || (longPathHandling == RemoveFolderExLongPathHandling::RemoveFolderExLongPathHandling_Ignore))
	{
		ExitFunction();
	}

	if ((longPathHandling == RemoveFolderExLongPathHandling::RemoveFolderExLongPathHandling_Prompt) || (longPathHandling == RemoveFolderExLongPathHandling::RemoveFolderExLongPathHandling_Default))
	{
		CErrorPrompter warnPrompt(PSW_MSI_MESSAGES::PSW_MSI_MESSAGES_LONG_PATHS_WARNING, (INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_WARNING | MB_YESNOCANCEL | MB_DEFBUTTON1 | MB_ICONWARNING), S_YES, com::panelsw::ca::ErrorHandling::prompt);

		for (UINT i = 0; i < cLongPaths; ++i)
		{
			hr = szAllFiles.AppnedFormat(L"\n%ls", pszLongPaths[i]);
			ExitOnFailure(hr, "Failed to allocate string");
		}

		hr = warnPrompt.Prompt((LPCWSTR)szAllFiles);
		ExitOnFailure(hr, "User aborted");
		ExitOnNull((hr == S_YES), hr, hr, "User opted not to delete files with long paths");
	}

	for (UINT i = 0; i < cLongPaths; ++i)
	{
		CWixString szSrc, szDst, szParentPath;
		LPCWSTR szLastBackslash = nullptr;

		hr = szSrc.AppnedFormat(L"\\\\?\\%ls", pszLongPaths[i]);
		ExitOnFailure(hr, "Failed to allocate string");

		szLastBackslash = wcsrchr(pszLongPaths[i], L'\\');
		ExitOnNull(szLastBackslash, hr, E_INVALIDDATA, "Failed to find last backslash in '%ls'", pszLongPaths[i]);

		hr = szParentPath.AppnedFormat(L"%.*ls", szLastBackslash - pszLongPaths[i] + 1, pszLongPaths[i]);
		ExitOnFailure(hr, "Failed to allocate string");

		hr = PathCreateTempFile(szParentPath, L"RFE%05i.tmp", INFINITE, FILE_ATTRIBUTE_NORMAL, (LPWSTR*)szDst, nullptr);
		ExitOnFailure(hr, "Failed to create file with short name");

		if (::MoveFileEx(szSrc, szDst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Renamed file with too long path: '%ls' -> '%ls'", pszLongPaths[i], (LPCWSTR)szDst);

			hr = cad.AddMoveFile(szDst, szSrc, CFileOperations::FileOperationsAttributes::IgnoreErrors | CFileOperations::FileOperationsAttributes::IgnoreMissingPath);
			ExitOnFailure(hr, "Failed to add operation to CAD");
		}
		else
		{
			WcaLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to move file with long name '%ls' -> '%ls'", pszLongPaths[i], (LPCWSTR)szDst);
			::DeleteFile((LPCWSTR)szDst);
		}
	}

	hr = cad.SetCustomActionData(L"PSW_REMOVEFOLDEREXROLLBACK");
	ExitOnFailure(hr, "Failed to set CAD");

	hr = cad.SetCustomActionData(L"PSW_REMOVEFOLDEREXEXEC");
	ExitOnFailure(hr, "Failed to set CAD");

LExit:
	return;
}

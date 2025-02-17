#include "pch.h"
#include "../CaCommon/WixString.h"
#include "FileEntry.h"
#include "FileIterator.h"
#include "DirectoryTableResolver.h"

extern "C" UINT __stdcall DuplicateFolder(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	MSIHANDLE hMoveFileTable = NULL;
	MSIHANDLE hMoveFileColumns = NULL;
	MSIHANDLE hCreateFolderTable = NULL;
	MSIHANDLE hCreateFolderColumns = NULL;
	CDirectoryTableResolver dirResolver;
	UINT i = 0;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = dirResolver.Initialize();
	ExitOnFailure(hr, "Failed to initialize");

	// Ensure table PSW_DuplicateFolder exists.
	hr = WcaTableExists(L"PSW_DuplicateFolder");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_DuplicateFolder'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_DuplicateFolder'. Have you authored 'PanelSw:DuplicateFolder' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Component_`, `SourceDir_`, `DestinationDir_` FROM `PSW_DuplicateFolder` WHERE `DuplicateExistingFiles` = 1", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szComponent, szSourceDir_, szDestinationDir_, szSourcePath;
		CFileIterator fileFinder;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component_.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szSourceDir_);
		ExitOnFailure(hr, "Failed to get Property.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)szDestinationDir_);
		ExitOnFailure(hr, "Failed to get Property.");

		hr = dirResolver.ResolvePath(szSourceDir_, (LPWSTR*)szSourcePath);
		ExitOnFailure(hr, "Failed to get source path for '%ls'", (LPCWSTR)szSourceDir_);
		if (hr == S_FALSE)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping DuplicateFolder from directory '%ls' to '%ls' because source path could not be resolved.", (LPCWSTR)szSourceDir_, (LPCWSTR)szDestinationDir_);
			continue;
		}
		if (!::PathIsDirectory(szSourcePath))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping DuplicateFolder from '%ls' to '%ls' because source path '%ls' does not exist.", (LPCWSTR)szSourceDir_, (LPCWSTR)szDestinationDir_, (LPCWSTR)szSourcePath);
			continue;
		}
		::PathRemoveBackslash((LPWSTR)szSourcePath);

		hr = WcaAddTempRecord(&hMoveFileTable, &hMoveFileColumns, L"MoveFile", nullptr, 1, 7, L"DupFolder", (LPCWSTR)szComponent, L"*", nullptr, (LPCWSTR)szSourceDir_, (LPCWSTR)szDestinationDir_, 0);
		ExitOnFailure(hr, "Failed to add temporary MoveFile row table");

		for (CFileEntry fileEntry = fileFinder.Find(szSourcePath, nullptr, nullptr, true); !fileFinder.IsEnd(); fileEntry = fileFinder.Next())
		{
			ExitOnNull(fileEntry.IsValid(), hr, fileFinder.Status(), "Failed to find files in '%ls'", (LPCWSTR)szSourcePath);

			if (fileEntry.IsDirectory() && !fileEntry.IsMountPoint() && !fileEntry.IsSymlink())
			{
				CWixString szSrcDirProperty;
				CWixString szDstDirProperty;
				CWixString szRelativeDestPath;
				MSIDBERROR dbErr = MSIDBERROR::MSIDBERROR_NOERROR;

				hr = szRelativeDestPath.Copy((LPCWSTR)fileEntry.Path() + szSourcePath.StrLen());
				ExitOnFailure(hr, "Failed to copy string");

				hr = dirResolver.InsertHierarchy(szDestinationDir_, szRelativeDestPath, (LPWSTR*)szDstDirProperty);
				ExitOnFailure(hr, "Failed to resolve target directory");

				hr = szSrcDirProperty.Format(L"_DUPSRC_%ls_%u", (LPCWSTR)szSourceDir_, ++i);
				ExitOnFailure(hr, "Failed to format string");

				hr = WcaSetProperty((LPCWSTR)szSrcDirProperty, (LPCWSTR)fileEntry.Path());
				ExitOnFailure(hr, "Failed to set property");

				hr = dirResolver.InsertCreateFolderIfMissing((LPCWSTR)szDstDirProperty, (LPCWSTR)szComponent);
				ExitOnFailure(hr, "Failed to add temporary CreateFolder row table");

				hr = WcaAddTempRecord(&hMoveFileTable, &hMoveFileColumns, L"MoveFile", &dbErr, 1, 7, L"DupFolder", (LPCWSTR)szComponent, L"*", nullptr, (LPCWSTR)szSrcDirProperty, (LPCWSTR)szDstDirProperty, 0);
				ExitOnFailure(hr, "Failed to add temporary MoveFile row table");
			}
		}
	}
	hr = S_OK;

LExit:
	if (hMoveFileTable)
	{
		::MsiCloseHandle(hMoveFileTable);
	}
	if (hMoveFileColumns)
	{
		::MsiCloseHandle(hMoveFileColumns);
	}
	if (hCreateFolderTable)
	{
		::MsiCloseHandle(hCreateFolderTable);
	}
	if (hCreateFolderColumns)
	{
		::MsiCloseHandle(hCreateFolderColumns);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

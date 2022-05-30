#include "stdafx.h"
#include "FileOperations.h"
#include "..\CaCommon\WixString.h"
#include <memutil.h>
#include <pathutil.h>
#include <dirutil.h>

extern "C" UINT __stdcall ExtractPayload(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CWixString szPayloadFolder;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	BYTE* pbData = nullptr;
	DWORD cbData = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	CFileOperations rollbackCAD;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = PathCreateTempDirectory(nullptr, L"PLD%05i", INFINITE, (LPWSTR*)szPayloadFolder);
	ExitOnFailure(hr, "Failed to create payload folder");

	hr = WcaSetProperty(L"PayloadFolder", szPayloadFolder);
	ExitOnFailure(hr, "Failed to set PayloadFolder");

	// Best effort to delete on rollback/commit
	if (SUCCEEDED(rollbackCAD.AddDeleteFile(szPayloadFolder)))
	{
		CWixString szCAD;	
		if (SUCCEEDED(rollbackCAD.GetCustomActionData((LPWSTR*)szCAD)))
		{
			WcaDoDeferredAction(L"ExtractPayloadRollback", szCAD, 0);
			WcaSetProperty(L"ExtractPayloadCommit", szCAD);
		}
	}

	// Ensure table PSW_Payload exists.
	hr = WcaTableExists(L"PSW_Payload");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_Payload'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_Payload'. Have you authored 'PanelSw:Payload' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `BinaryKey_`, `Name` FROM `PSW_Payload`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_XmlSearch'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");
		ReleaseNullMem(pbData);
		cbData = 0;

		// Get fields
		CWixString szBinaryKey;
		CWixString szName;
		CWixString szPayloadDir;
		CWixString szPayloadPath;
		CWixString szSubQuery;
		PMSIHANDLE hSubView;
		PMSIHANDLE hSubRecord;
		DWORD dwRes = 0;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szBinaryKey);
		ExitOnFailure(hr, "Failed to get BinaryKey_.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szName);
		ExitOnFailure(hr, "Failed to get Name.");

		hr = PathConcat(szPayloadFolder, szName, (LPWSTR*)szPayloadPath);
		ExitOnFailure(hr, "Failed to concat payload path");
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Extracting '%ls'", (LPCWSTR)szPayloadPath);

		hr = szSubQuery.Format(L"SELECT `Data` FROM `Binary` WHERE `Name`='%s'", (LPCWSTR)szBinaryKey);
		ExitOnFailure(hr, "Failed to format string");

		hr = WcaOpenExecuteView((LPCWSTR)szSubQuery, &hSubView);
		ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szSubQuery);

		hr = WcaFetchRecord(hSubView, &hSubRecord);
		ExitOnFailure(hr, "Failed to fetch Binary record.");

		hr = WcaGetRecordStream(hSubRecord, 1, &pbData, &cbData);
		ExitOnFailure(hr, "Failed to read Binary.Data for certificate.");

		if (SUCCEEDED(PathGetDirectory(szPayloadPath, (LPWSTR*)szPayloadDir)))
		{
			DirEnsureExists(szPayloadDir, nullptr);
		}

		hFile = ::CreateFile(szPayloadPath, GENERIC_ALL, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed opening file");

		dwRes = ::WriteFile(hFile, pbData, cbData, &cbData, nullptr);
		ExitOnNullWithLastError(dwRes, hr, "Failed writing to file");

		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	hr = ERROR_SUCCESS;

LExit:
	ReleaseMem(pbData);
	if ((hFile != INVALID_HANDLE_VALUE) && (hFile != NULL))
	{
		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}
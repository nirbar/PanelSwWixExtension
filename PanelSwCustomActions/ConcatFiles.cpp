#include "ConcatFiles.h"
#include <fileutil.h>
#include <memutil.h>
#include "../CaCommon/WixString.h"
#include "google\protobuf\any.h"
using namespace com::panelsw::ca;
using namespace google::protobuf;

extern "C" UINT __stdcall ConcatFiles(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	DWORD dwRes = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPWSTR szCustomActionData = nullptr;
	CConcatFiles oDeferred;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure tables exist.
	hr = WcaTableExists(L"PSW_ConcatFiles");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_ConcatFiles'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_ConcatFiles'. Have you authored 'PanelSw:SplitFile' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Component_`, `RootFile_`, `MyFile_` FROM `PSW_ConcatFiles` ORDER BY `RootFile_`, `Order`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szComponent, szRootFileId, szSplitFileId, szFilePathFormat, szRootFilePath, szSplitFilePath;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szRootFileId);
		ExitOnFailure(hr, "Failed to get RootFile_.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)szSplitFileId);
		ExitOnFailure(hr, "Failed to get MyFile_.");

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		if ((compAction != WCA_TODO::WCA_TODO_INSTALL) && (compAction != WCA_TODO::WCA_TODO_REINSTALL))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping file concat '%ls' since component '%ls' is not scheduled for re/install", (LPCWSTR)szRootFileId, (LPCWSTR)szComponent);
			continue;
		}
		if (szRootFileId.Equals(szSplitFileId))
		{
			continue;
		}

		hr = szFilePathFormat.Format(L"[#%ls]", (LPCWSTR)szRootFileId);
		ExitOnFailure(hr, "Failed to format string.");

		hr = szRootFilePath.MsiFormat((LPCWSTR)szFilePathFormat);
		ExitOnFailure(hr, "Failed to format string.");

		hr = szFilePathFormat.Format(L"[#%ls]", (LPCWSTR)szSplitFileId);
		ExitOnFailure(hr, "Failed to format string.");

		hr = szSplitFilePath.MsiFormat((LPCWSTR)szFilePathFormat);
		ExitOnFailure(hr, "Failed to format string.");

		hr = oDeferred.AddConcatFiles((LPCWSTR)szRootFilePath, (LPCWSTR)szSplitFilePath);
		ExitOnFailure(hr, "Failed to add ConcatFile CAD.");
	}

	// Set CAD
	hr = oDeferred.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data.");
	hr = WcaDoDeferredAction(L"ConcatFilesExec", szCustomActionData, oDeferred.GetCost());
	ExitOnFailure(hr, "Failed setting action data.");

LExit:
	ReleaseStr(szCustomActionData);

	dwRes = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(dwRes);
}

HRESULT CConcatFiles::AddConcatFiles(LPCWSTR szRootFile, LPCWSTR szSplitFile)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	ConcatFilesDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CConcatFiles", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new ConcatFilesDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_root_file(szRootFile, WSTR_BYTE_SIZE(szRootFile));
	pDetails->set_split_file(szSplitFile, WSTR_BYTE_SIZE(szSplitFile));

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CConcatFiles::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	LPCWSTR szRootFile = nullptr;
	LPCWSTR szSplitFile = nullptr;
	DWORD bRes = TRUE;
	ConcatFilesDetails details;

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking ConcatFilesDetails");

	szRootFile = (LPCWSTR)(LPVOID)details.root_file().data();
	szSplitFile = (LPCWSTR)(LPVOID)details.split_file().data();

	hr = ExecuteOne(szRootFile, szSplitFile);
	ExitOnFailure(hr, "Failed to concat file");

LExit:
	return hr;
}

HRESULT CConcatFiles::ExecuteOne(LPCWSTR szRootFile, LPCWSTR szSplitFile)
{
	HRESULT hr = S_OK;
	HANDLE hRootFile = INVALID_HANDLE_VALUE;
	HANDLE hSplitFile = INVALID_HANDLE_VALUE;
	DWORD dwBytesRead = 0;
	DWORD dwBytesWritten = 0;
	DWORD dwPos = 0;
	const DWORD dwBuffSize = 1024 * 1024; // 1MB
	BYTE* pBuff = nullptr;
	DWORD bRes = TRUE;

	if (!FileExistsEx(szSplitFile, nullptr))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping concatention of file '%ls' to '%ls' because it does not exist. Assuming the file was not reinstalled", szSplitFile, szRootFile);
		hr = S_FALSE;
		ExitFunction();
	}
	LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, L"Concatenting file '%ls' to '%ls'", szSplitFile, szRootFile);

	pBuff = (BYTE*)MemAlloc(dwBuffSize, FALSE);
	ExitOnNull(pBuff, hr, E_FAIL, "Failed to allocate memory");

	hRootFile = ::CreateFile(szRootFile, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	ExitOnNullWithLastError((hRootFile != INVALID_HANDLE_VALUE), hr, "Failed opening root file '%ls'", szRootFile);

	hSplitFile = ::CreateFile(szSplitFile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	ExitOnNullWithLastError((hSplitFile != INVALID_HANDLE_VALUE), hr, "Failed opening split file '%ls'", szSplitFile);

	while ((bRes = ::ReadFile(hSplitFile, pBuff, dwBuffSize, &dwBytesRead, nullptr)) && (dwBytesRead > 0))
	{
		dwPos = ::SetFilePointer(hRootFile, 0, nullptr, FILE_END);
		ExitOnNullWithLastError((dwPos != INVALID_SET_FILE_POINTER), hr, "Failed setting file pointer at end");

		bRes = ::WriteFile(hRootFile, pBuff, dwBytesRead, &dwBytesWritten, nullptr);
		ExitOnNullWithLastError(bRes, hr, "Failed writing to file");
		ExitOnNull((dwBytesRead == dwBytesWritten), hr, E_FAIL, "Failed writing to file (mismatch size)");
	}
	ExitOnNullWithLastError(bRes, hr, "Failed to read file");

	ReleaseFile(hSplitFile);
	::DeleteFile(szSplitFile);

LExit:
	ReleaseFile(hRootFile);
	ReleaseFile(hSplitFile);
	ReleaseMem(pBuff);

	return hr;
}
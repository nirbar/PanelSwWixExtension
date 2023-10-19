#include "pch.h"
using namespace com::panelsw::ca;

extern "C" UINT __stdcall PromptFileDowngrades(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	DWORD dwDowngrades = 0;
	UINT promptResult = 0;
	CErrorPrompter prompter(PSW_ERROR_MESSAGES::PSW_ERROR_MESSAGES_PROMPTFILEDOWNGRADES, (INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_WARNING | MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONWARNING), S_OK, ErrorHandling::promptAlways);

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `File`.`File`, `File`.`FileName`, `Component`.`Directory_`, `File`.`Version` FROM `File`, `Component` WHERE `File`.`Version` IS NOT NULL AND `Component`.`Component` = `File`.`Component_`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'File' table.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szFileId, szFileName, szDirectoryId, szVersion, szPathFormat, szFullPath;
		DWORD dwNameSeparator = 0;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		ULARGE_INTEGER ullExistingVersion;
		ULARGE_INTEGER ullMsiVersion;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szFileId);
		ExitOnFailure(hr, "Failed to get File.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szFileName);
		ExitOnFailure(hr, "Failed to get FileName.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)szDirectoryId);
		ExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)szVersion);
		ExitOnFailure(hr, "Failed to get Version.");

		hr = FileVersionFromString((LPCWSTR)szVersion, &ullMsiVersion.HighPart, &ullMsiVersion.LowPart);
		if (FAILED(hr) || (hr != S_OK))
		{
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File '%ls' has no version", (LPCWSTR)szFileId);
			continue;
		}

		dwNameSeparator = szFileName.Find(L'|');
		if (dwNameSeparator != INFINITE)
		{
			hr = szFileName.Substring(dwNameSeparator + 1);
			ExitOnFailure(hr, "Failed to get long file name.");
		}

		hr = szPathFormat.Format(L"[%ls]%ls", (LPCWSTR)szDirectoryId, (LPCWSTR)szFileName);
		ExitOnFailure(hr, "Failed to format string.");

		hr = szFullPath.MsiFormat((LPCWSTR)szPathFormat);
		ExitOnFailure(hr, "Failed to msi-format string.");

		if (!::PathFileExistsW((LPCWSTR)szFullPath))
		{
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File '%ls' doesn't exist", (LPCWSTR)szFullPath);
			continue;
		}

		hr = FileVersion((LPCWSTR)szFullPath, &ullExistingVersion.HighPart, &ullExistingVersion.LowPart);
		if (!SUCCEEDED(hr))
		{
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "File '%ls' version couldn't be determined", (LPCWSTR)szFullPath);
			continue;
		}

		if (ullExistingVersion.QuadPart > ullMsiVersion.QuadPart)
		{
			++dwDowngrades;
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File '%ls' might be downgraded from %u.%u.%u.%u to %ls, or deleted altogether", (LPCWSTR)szFullPath
				, (0xFF & (ullExistingVersion.HighPart >> 16))
				, (0xFF & ullExistingVersion.HighPart)
				, (0xFF & (ullExistingVersion.LowPart >> 16))
				, (0xFF & ullExistingVersion.LowPart)
				, (LPCWSTR)szVersion
			);
		}
		else
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File '%ls' has existing version %u.%u.%u.%u. Version %ls may be deployed", (LPCWSTR)szFullPath
				, (0xFF & (ullExistingVersion.HighPart >> 16))
				, (0xFF & ullExistingVersion.HighPart)
				, (0xFF & (ullExistingVersion.LowPart >> 16))
				, (0xFF & ullExistingVersion.LowPart)
				, (LPCWSTR)szVersion
			);
		}
	}
	hr = S_OK;

	if (dwDowngrades > 0)
	{
		hr = prompter.Prompt(dwDowngrades);
		ExitOnFailure(hr, "Aboring on file downgrades")
	}

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

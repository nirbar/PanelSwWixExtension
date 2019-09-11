#include "stdafx.h"
#include "../CaCommon/WixString.h"

extern "C" UINT __stdcall ForceVersion(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_ForceVersion");
	BreakExitOnNull((hr == S_OK), hr, E_FAIL, "Table 'PSW_ForceVersion' does not exist. Have you authored 'PanelSw:ForceVersion' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `File`, `Version` FROM `PSW_ForceVersion`", &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query on 'PSW_ForceVersion'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString file, version;
		CWixString query;
		PMSIHANDLE fileView;
		PMSIHANDLE fileRecord;
		LPCWSTR selectQueryFormat = L"SELECT `File`, `Component_`, `FileName`, `FileSize`, `Version`, `Language`, `Attributes`, `Sequence` FROM `File` WHERE `File`.`File` = '%s'";
		LPCWSTR deleteQueryFormat = L"DELETE FROM `File` WHERE `File`.`File` = '%s'";
		LPCWSTR insertQuery = L"INSERT INTO `File` (`File`, `Component_`, `FileName`, `FileSize`, `Version`, `Language`, `Attributes`, `Sequence`) VALUES (?, ?, ?, ?, ?, ?, ?, ?) TEMPORARY";

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)file);
		BreakExitOnFailure(hr, "Failed to get File.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)version);
		BreakExitOnFailure(hr, "Failed to get Version.");

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Forcing version '%ls' on file '%ls'", (LPCWSTR)version, (LPCWSTR)file);

		// Get file row
		hr = query.Format(selectQueryFormat, (LPCWSTR)file);
		BreakExitOnFailure(hr, "Failed formatting string");

		hr = WcaOpenExecuteView(query, &fileView);
		BreakExitOnFailure(hr, "Failed to execute SELECT query on 'File'.");

		hr = WcaFetchRecord(fileView, &fileRecord);
		BreakExitOnFailure(hr, "Failed getting 'File' record.");

		// Override version
		hr = WcaSetRecordString(fileRecord, 5, (LPCWSTR)version);
		BreakExitOnFailure(hr, "Failed setting 'File' record 'Version' filed.");

		// Delete current row
		hr = query.Format(deleteQueryFormat, (LPCWSTR)file);
		BreakExitOnFailure(hr, "Failed formatting string");

		hr = WcaOpenExecuteView(query, &fileView);
		BreakExitOnFailure(hr, "Failed to execute DELETE query on 'File'.");

		// Insert updated row
		hr = WcaOpenView(insertQuery, &fileView);
		BreakExitOnFailure(hr, "Failed to open INSERT query view on 'File'.");

		hr = WcaExecuteView(fileView, fileRecord);
		BreakExitOnFailure(hr, "Failed to execute INSERT query on 'File'.");
	}
	hr = S_OK;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}
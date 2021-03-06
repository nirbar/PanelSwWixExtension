#include "stdafx.h"
#include "../CaCommon/WixString.h"
#include <fileutil.h>

extern "C" UINT __stdcall VersionCompare(MSIHANDLE hInstall) noexcept
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_VersionCompare");
	ExitOnNull((hr == S_OK), hr, E_INVALIDSTATE, "Table does not exist 'PSW_VersionCompare'. Have you authored 'PanelSw:VersionCompare' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Property_`, `Version1`, `Version2` FROM `PSW_VersionCompare`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_VersionCompare'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString ver1, ver2, property;
		ULARGE_INTEGER ullVer1, ullVer2;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)property);
		ExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)ver1);
		ExitOnFailure(hr, "Failed to get Version1.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)ver2);
		ExitOnFailure(hr, "Failed to get Version2.");

		if (FAILED(FileVersionFromString(ver1, &ullVer1.HighPart, &ullVer1.LowPart)) || FAILED(FileVersionFromString(ver2, &ullVer2.HighPart, &ullVer2.LowPart)))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Failed parsing '%ls' or '%ls' as version. Skipping comparison for property '%ls'", (LPCWSTR)ver1, (LPCWSTR)ver2, (LPCWSTR)property);
			continue;
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Comparing version '%u.%u.%u.%u' to '%u.%u.%u.%u' and placing result in '%ls'"
			, ((ullVer1.HighPart >> 16) & 0xFFFF)
			, (ullVer1.HighPart & 0xFFFF)
			, ((ullVer1.LowPart >> 16) & 0xFFFF)
			, (ullVer1.LowPart & 0xFFFF)
			, ((ullVer2.HighPart >> 16) & 0xFFFF)
			, (ullVer2.HighPart & 0xFFFF)
			, ((ullVer2.LowPart >> 16) & 0xFFFF)
			, (ullVer2.LowPart & 0xFFFF)
			, (LPCWSTR)property
		);
		hr = WcaSetIntProperty(property, (ullVer1.QuadPart > ullVer2.QuadPart) ? 1 : (ullVer1.QuadPart < ullVer2.QuadPart) ? -1 : 0);
		ExitOnFailure(hr, "Failed setting property");
	}

	hr = ERROR_SUCCESS;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}
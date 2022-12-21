#include "pch.h"

extern "C" UINT __stdcall VersionCompare(MSIHANDLE hInstall)
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
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_VersionCompare'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_VersionCompare'. Have you authored 'PanelSw:VersionCompare' entries in WiX code?");

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
		USHORT  rgusVer1[4], rgusVer2[4];
		int nCompareRes = 0;

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

		rgusVer1[3] = ((ullVer1.HighPart >> 16) & 0xFFFF);
		rgusVer1[2] = (ullVer1.HighPart & 0xFFFF);
		rgusVer1[1] = ((ullVer1.LowPart >> 16) & 0xFFFF);
		rgusVer1[0] = (ullVer1.LowPart & 0xFFFF);

		rgusVer2[3] = ((ullVer2.HighPart >> 16) & 0xFFFF);
		rgusVer2[2] = (ullVer2.HighPart & 0xFFFF);
		rgusVer2[1] = ((ullVer2.LowPart >> 16) & 0xFFFF);
		rgusVer2[0] = (ullVer2.LowPart & 0xFFFF);

		for (int i = ARRAYSIZE(rgusVer1) - 1; i >= 0; --i)
		{
			if (rgusVer1[i] > rgusVer2[i])
			{
				nCompareRes = i + 1;
				break;
			}
			if (rgusVer1[i] < rgusVer2[i])
			{
				nCompareRes = -(i + 1);
				break;
			}
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Comparing version '%u.%u.%u.%u' to '%u.%u.%u.%u' and placing result in '%ls'", rgusVer1[3], rgusVer1[2], rgusVer1[1], rgusVer1[0], rgusVer2[3], rgusVer2[2], rgusVer2[1], rgusVer2[0], (LPCWSTR)property);
		hr = WcaSetIntProperty(property, nCompareRes);
		ExitOnFailure(hr, "Failed setting property");
	}
	hr = ERROR_SUCCESS;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

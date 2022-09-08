#include "stdafx.h"

extern "C" UINT __stdcall Rollback(MSIHANDLE hInstall)
{
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Rollback");
	return ERROR_INSTALL_FAILURE;
}

extern "C" UINT __stdcall PromptCancel(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	DWORD dwRes = ERROR_SUCCESS;
	PMSIHANDLE hRec;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hRec = ::MsiCreateRecord(1);
	ExitOnNull(hRec, hr, E_FAIL, "Failed creating record");

	hr = WcaSetRecordInteger(hRec, 1, 1602);
	ExitOnFailure(hr, "Failed setting record string");

	dwRes = WcaProcessMessage((INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_ERROR | MB_YESNO | MB_DEFBUTTON1 | MB_ICONERROR), hRec);
	if (dwRes == IDYES)
	{
		hr = HRESULT_FROM_WIN32(ERROR_INSTALL_USEREXIT);
	}

LExit:

	dwRes = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(dwRes);
}
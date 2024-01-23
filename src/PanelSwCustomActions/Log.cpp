#include "pch.h"

#define CustomActionData L"CustomActionData"
#define LogProperty L"LOGMESSAGE"

extern "C" UINT __stdcall Log(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	LPCWSTR szPropName = LogProperty;
	CWixString szMsg;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");

	// Deferred? Get CustomActionData
	if( ::MsiGetMode( hInstall, MSIRUNMODE_SCHEDULED)
		|| ::MsiGetMode( hInstall, MSIRUNMODE_COMMIT)
		|| ::MsiGetMode( hInstall, MSIRUNMODE_ROLLBACK))
	{
		szPropName = CustomActionData;
	}

	hr = WcaGetProperty( szPropName, (LPWSTR*)szMsg);
	ExitOnFailure(hr, "Failed to get property '%ls'", szPropName);

	WcaLog( LOGLEVEL::LOGMSG_STANDARD, "%ls", (LPCWSTR)szMsg);

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

extern "C" UINT __stdcall Warn(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	LPCWSTR szPropName = LogProperty;
	PMSIHANDLE hRecord;
	CWixString szMsg;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");

	// Deferred? Get CustomActionData
	if( ::MsiGetMode( hInstall, MSIRUNMODE_SCHEDULED)
		|| ::MsiGetMode( hInstall, MSIRUNMODE_COMMIT)
		|| ::MsiGetMode( hInstall, MSIRUNMODE_ROLLBACK))
	{
		szPropName = CustomActionData;
	}

	hr = WcaGetProperty( szPropName, (LPWSTR*)szMsg);
	ExitOnFailure(hr, "Failed to get property '%ls'", szPropName);

	hRecord = ::MsiCreateRecord( 1);
	ExitOnNullWithLastError(hRecord, hr, "Failed to create a warning record.");

	hr = WcaSetRecordString(hRecord, 0, (LPCWSTR)szMsg);
	ExitOnFailure(hr, "Failed to set warning record message");

	WcaProcessMessage( INSTALLMESSAGE::INSTALLMESSAGE_WARNING, hRecord);

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

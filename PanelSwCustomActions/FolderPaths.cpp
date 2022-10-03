#include "stdafx.h"
#include <WixString.h>
#include <ShellAPI.h>
#include <Shlobj.h>
#include <shelutil.h>
#include <map>
using namespace std;

extern "C" UINT __stdcall FolderPaths(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	map<int, LPCWSTR> mapCsidl2Property;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	mapCsidl2Property[CSIDL_DESKTOPDIRECTORY] = L"UserDesktopFolder";

	for (const pair<int, LPCWSTR>& csidlProp : mapCsidl2Property)
	{
		CWixString szFolder;

		hr = ShelGetFolder((LPWSTR*)szFolder, csidlProp.first);
		if (FAILED(hr) || szFolder.IsNullOrEmpty())
		{
			WcaLogError(hr, "Failed to get folder for CSIDL %i", csidlProp.first);
			WcaSetProperty(csidlProp.second, L"");
			hr = S_OK;
			continue;
		}

		hr = WcaSetProperty(csidlProp.second, (LPCWSTR)szFolder);
		ExitOnFailure(hr, "Failed setting property");
	}

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

#include "stdafx.h"

extern "C" UINT __stdcall CheckRebootRequired(MSIHANDLE hInstall)
{
    HRESULT hr = S_OK;
    DWORD er = ERROR_SUCCESS;

    hr = WcaInitialize(hInstall, __FUNCTION__);
    ExitOnFailure(hr, "Failed to initialize");
    WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

    if (WcaDidDeferredActionRequireReboot())
    {
        WcaLog(LOGMSG_STANDARD, "Reboot required by deferred CustomAction.");

        er = ::MsiSetMode(hInstall, MSIRUNMODE_REBOOTATEND, TRUE);
        hr = HRESULT_FROM_WIN32(er);
        ExitOnFailure(hr, "Failed to schedule reboot.");
    }

LExit:

    er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
    return WcaFinalize(er);
}

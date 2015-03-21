#include "stdafx.h"
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/WixString.h"
#include "Telemetry.h"

// ReceiverToExecutorFunc implementation.
HRESULT ReceiverToExecutor(LPCWSTR szReceiver, CDeferredActionBase** ppExecutor)
{
	HRESULT hr = S_OK;
	CWixString szRcvr(szReceiver);

	(*ppExecutor) = NULL;
	if (szRcvr.Equals(L"CTelemetry"))
	{
		(*ppExecutor) = new CTelemetry();
	}
	else
	{
		hr = E_INVALIDARG;
	}
	
	return hr;
}

extern "C" __declspec(dllexport) UINT CommonDeferred(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	hr = CDeferredActionBase::DeferredEntryPoint(ReceiverToExecutor);
	BreakExitOnFailure(hr, "Failed to excute CustomActionData command object");

LExit :
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

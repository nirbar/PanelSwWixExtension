#include "stdafx.h"
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/WixString.h"
#include "Telemetry.h"
#include "ShellExecute.h"

// ReceiverToExecutorFunc implementation.
HRESULT ReceiverToExecutor(LPCWSTR szReceiver, CDeferredActionBase** ppExecutor)
{
	HRESULT hr = S_OK;
	CWixString szRcvr(szReceiver);

	(*ppExecutor) = NULL;
	if (szRcvr.Equals(L"CTelemetry"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating Telemetry handler");
		(*ppExecutor) = new CTelemetry();
	}
	if (szRcvr.Equals(L"CShellExecute"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating ShellExecute handler");
		(*ppExecutor) = new CShellExecute();
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

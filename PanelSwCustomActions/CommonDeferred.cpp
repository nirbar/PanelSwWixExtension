#include "stdafx.h"
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/WixString.h"
#include "Telemetry.h"
#include "ShellExecute.h"
#include "FileRegex.h"
#include "FileOperations.h"
#include "TaskScheduler.h"
#include "ExecOnComponent.h"
#include "ServiceConfig.h"

// ReceiverToExecutorFunc implementation.
HRESULT ReceiverToExecutor(LPCSTR szReceiver, CDeferredActionBase** ppExecutor)
{
	HRESULT hr = S_OK;

	(*ppExecutor) = nullptr;
	if (0 == ::strcmp(szReceiver, "CTelemetry"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating Telemetry handler");
		(*ppExecutor) = new CTelemetry();
	}
	else if (0 == ::strcmp(szReceiver, "CShellExecute"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating ShellExecute handler");
		(*ppExecutor) = new CShellExecute();
	}
	else if (0 == ::strcmp(szReceiver, "CFileRegex"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating FileRegex handler");
		(*ppExecutor) = new CFileRegex();
	}
	else if (0 == ::strcmp(szReceiver, "CFileOperations"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating FileOperations handler");
		(*ppExecutor) = new CFileOperations();
	}
	else if (0 == ::strcmp(szReceiver, "CTaskScheduler"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating TaskScheduler handler");
		(*ppExecutor) = new CTaskScheduler();
	}
	else if (0 == ::strcmp(szReceiver, "CExecOnComponent"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating ExecOnComponent handler");
		(*ppExecutor) = new CExecOnComponent();
	}
	else if (0 == ::strcmp(szReceiver, "CServiceConfig"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating ServiceConfig handler");
		(*ppExecutor) = new CServiceConfig();
	}
	else
	{
		hr = E_INVALIDARG;
	}
	
	return hr;
}

extern "C" UINT __stdcall CommonDeferred(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = CDeferredActionBase::DeferredEntryPoint(ReceiverToExecutor);
	BreakExitOnFailure(hr, "Failed to excute CustomActionData command object");

LExit :
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

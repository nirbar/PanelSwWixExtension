#include "pch.h"
#include "../CaCommon/DeferredActionBase.h"
#include "Telemetry.h"
#include "ShellExecute.h"
#include "FileRegex.h"
#include "FileOperations.h"
#include "TaskScheduler.h"
#include "ExecOnComponent.h"
#include "ServiceConfig.h"
#include "TopShelfService.h"
#include "Unzip.h"
#include "SqlScript.h"
#include "XslTransform.h"
#include "RestartLocalResources.h"
#include "ConcatFiles.h"

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
	else if (0 == ::strcmp(szReceiver, "CTopShelfService"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating TopShelfService handler");
		(*ppExecutor) = new CTopShelfService();
	}
	else if (0 == ::strcmp(szReceiver, "CUnzip"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating Unzip handler");
		(*ppExecutor) = new CUnzip(false);
	}
	else if (0 == ::strcmp(szReceiver, "CZip"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating Zip handler");
		(*ppExecutor) = new CUnzip(true);
	}
	else if (0 == ::strcmp(szReceiver, "CSqlScript"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating SqlScript handler");
		(*ppExecutor) = new CSqlScript();
	}
	else if (0 == ::strcmp(szReceiver, "CXslTransform"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating XslTransform handler");
		(*ppExecutor) = new CXslTransform();
	}
	else if (0 == ::strcmp(szReceiver, "CRestartLocalResources"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating RestartLocalResources handler");
		(*ppExecutor) = new CRestartLocalResources();
	}
	else if (0 == ::strcmp(szReceiver, "CConcatFiles"))
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating ConcatFiles handler");
		(*ppExecutor) = new CConcatFiles();
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

	hr = CDeferredActionBase::DeferredEntryPoint(hInstall, ReceiverToExecutor);
	ExitOnFailure(hr, "Failed to excute CustomActionData command object");

LExit :
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

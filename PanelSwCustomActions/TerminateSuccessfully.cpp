#include "pch.h"

extern "C" UINT __stdcall TerminateSuccessfully(MSIHANDLE hInstall)
{
	WcaInitialize(hInstall, __FUNCTION__);
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Terminating Successfully");	
	return WcaFinalize(ERROR_NO_MORE_ITEMS);
}

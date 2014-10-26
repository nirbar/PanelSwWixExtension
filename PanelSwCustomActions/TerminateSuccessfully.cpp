#include "stdafx.h"

extern "C" __declspec( dllexport ) UINT TerminateSuccessfully(MSIHANDLE hInstall)
{
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Terminating Successfully");
	return ERROR_NO_MORE_ITEMS;
}

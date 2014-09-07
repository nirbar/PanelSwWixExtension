#include "stdafx.h"

UINT __stdcall TerminateSuccessfully(MSIHANDLE hInstall)
{
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Terminating Successfully");
	return ERROR_NO_MORE_ITEMS;
}

#include "stdafx.h"

extern "C" UINT __stdcall TerminateSuccessfully(MSIHANDLE hInstall) noexcept
{
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Terminating Successfully");
	return ERROR_NO_MORE_ITEMS;
}

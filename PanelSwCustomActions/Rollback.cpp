#include "stdafx.h"

extern "C" UINT __stdcall Rollback(MSIHANDLE hInstall)
{
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Rollback");
	return ERROR_INSTALL_FAILURE;
}

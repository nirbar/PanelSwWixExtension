// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"
#include "../CaCommon/WixString.h"

enum PSW_ERROR_MESSAGES
{
	PSW_ERROR_MESSAGES_TOPSHELFFAILURE = 27000,
	PSW_ERROR_MESSAGES_EXECONFAILURE = 27001,
	PSW_ERROR_MESSAGES_SERVICECONFIGFAILURE = 27002,
	PSW_ERROR_MESSAGES_DISMPACKAGEFAILURE = 27003,
	PSW_ERROR_MESSAGES_DISMFEATUREFAILURE = 27004,
	PSW_ERROR_MESSAGES_PSW_SQLSCRIPTFAILURE = 27005,
	PSW_ERROR_MESSAGES_EXECONCONSOLEFAILURE = 27006,
	PSW_ERROR_MESSAGES_WEBSITECONFIGFAILURE = 27007,
	PSW_ERROR_MESSAGES_PSW_SQLSEARCHFAILURE = 27008,
	PSW_ERROR_MESSAGES_JSONJPATHFAILURE = 27009,
	PSW_ERROR_MESSAGES_DISMUNWANTEDFEATUREFAILURE = 27010,
	PSW_ERROR_MESSAGES_EXECONPROMPTALWAYS = 27011,
	PSW_ERROR_MESSAGES_PROMPTFILEDOWNGRADES = 27012,
};

#endif //PCH_H

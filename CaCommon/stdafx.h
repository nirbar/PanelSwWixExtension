// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows 2000.
#define _WIN32_WINNT 0x0501
#endif

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <MsiQuery.h>

// WiX Header Files:
#include <wcautil.h>

// TODO: reference additional headers your program requires here

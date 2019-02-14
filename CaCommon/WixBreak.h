#pragma once
#include "MsiBreak.h"

#ifdef _DEBUG

#define BreakExitOnFailure(x, s, ...)				if (FAILED(x)) { MsiDebugBreak(); } ExitOnFailure(x, s, __VA_ARGS__)				 
#define BreakExitOnNull(p, x, e, s, ...)			if (NULL == p) { MsiDebugBreak(); } ExitOnNull(p, x, e, s, __VA_ARGS__)			 
#define BreakExitOnWin32Error(e, x, s, ...)			if (ERROR_SUCCESS != e) { MsiDebugBreak(); } ExitOnWin32Error(e, x, s, __VA_ARGS__)		
#define BreakExitOnNullWithLastError(p, x, s, ...)	if (NULL == p) { DWORD Dutil_er = ::GetLastError(); x = HRESULT_FROM_WIN32(Dutil_er); if (!FAILED(x)) { x = E_FAIL; } MsiDebugBreak(); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace(x, s, __VA_ARGS__); goto LExit; }

#else

#define BreakExitOnFailure(x, s, ...)				ExitOnFailure(x, s, __VA_ARGS__)
#define BreakExitOnNull(p, x, e, s, ...)			ExitOnNull(p, x, e, s, __VA_ARGS__)
#define BreakExitOnWin32Error(e, x, s, ...)			ExitOnWin32Error(e, x, s, __VA_ARGS__)
#define BreakExitOnNullWithLastError(p, x, s, ...)	ExitOnNullWithLastError(p, x, s, __VA_ARGS__)

#endif
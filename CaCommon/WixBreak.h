#pragma once
#include "MsiBreak.h"

#ifdef _DEBUG

#define BreakExitOnFailure(x, s)			if (FAILED(x)) { MsiDebugBreak(); } ExitOnFailure(x, s)				 
#define BreakExitOnFailure1(x, f, s)		if (FAILED(x)) { MsiDebugBreak(); } ExitOnFailure1(x, f, s)			 
#define BreakExitOnFailure2(x, f, s, t)		if (FAILED(x)) { MsiDebugBreak(); } ExitOnFailure2(x, f, s, t)		 
#define BreakExitOnFailure3(x, f, s, t, u)	if (FAILED(x)) { MsiDebugBreak(); } ExitOnFailure3(x, f, s, t, u) 
#define BreakExitOnNull(p, x, e, s)			if (NULL == p) { MsiDebugBreak(); } ExitOnNull(p, x, e, s)			 
#define BreakExitOnNull1(p, x, e, f, s)		if (NULL == p) { MsiDebugBreak(); } ExitOnNull1(p, x, e, f, s)		 
#define BreakExitOnNull2(p, x, e, f, s, t)  if (NULL == p) { MsiDebugBreak(); } ExitOnNull2(p, x, e, f, s, t)  

#define BreakExitOnWin32Error(e, x, s)			if (ERROR_SUCCESS != e) { MsiDebugBreak(); } ExitOnWin32Error(e, x, s)		
#define BreakExitOnWin32Error1(e, x, f, s)		if (ERROR_SUCCESS != e) { MsiDebugBreak(); } ExitOnWin32Error1(e, x, f, s)	
#define BreakExitOnWin32Error2(e, x, f, s, t)	if (ERROR_SUCCESS != e) { MsiDebugBreak(); } ExitOnWin32Error2(e, x, f, s, t)

#define BreakExitOnNullWithLastError(p, x, s)	if (NULL == p) { DWORD Dutil_er = ::GetLastError(); x = HRESULT_FROM_WIN32(Dutil_er); if (!FAILED(x)) { x = E_FAIL; } MsiDebugBreak(); Dutil_RootFailure(__FILE__, __LINE__, x); ExitTrace(x, s); goto LExit; }

#else

#define BreakExitOnFailure(x, s, ...)			ExitOnFailure(x, s, __VA_ARGS__)
#define BreakExitOnFailure1(x, f, s)		ExitOnFailure1(x, f, s)			
#define BreakExitOnFailure2(x, f, s, t)		ExitOnFailure2(x, f, s, t)		
#define BreakExitOnFailure3(x, f, s, t, u)	ExitOnFailure3(x, f, s, t, u)
#define BreakExitOnNull(p, x, e, s, ...)			ExitOnNull(p, x, e, s, __VA_ARGS__)
#define BreakExitOnNull1(p, x, e, f, s)		ExitOnNull1(p, x, e, f, s)		
#define BreakExitOnNull2(p, x, e, f, s, t)	ExitOnNull2(p, x, e, f, s, t) 

#define BreakExitOnWin32Error(e, x, s, ...)			ExitOnWin32Error(e, x, s, __VA_ARGS__)
#define BreakExitOnWin32Error1(e, x, f, s)		ExitOnWin32Error1(e, x, f, s)	
#define BreakExitOnWin32Error2(e, x, f, s, t)	ExitOnWin32Error2(e, x, f, s, t)

#define BreakExitOnNullWithLastError(p, x, s, ...)	ExitOnNullWithLastError(p, x, s, __VA_ARGS__)

#endif
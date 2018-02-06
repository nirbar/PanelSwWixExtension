#include "MsiBreak.h"
#include <strsafe.h>

HRESULT MsiDebugBreak()
{
	HRESULT hr = S_OK;
	WCHAR szTest[10];
	DWORD dwRes = 0;

	dwRes = ::GetEnvironmentVariable(L"MSI_BREAK", szTest, 10);
	if (dwRes > 0)
	{
		MsiBreak();
		ExitFunction();
	}

	dwRes = ::GetLastError();
	if (dwRes == ERROR_ENVVAR_NOT_FOUND)
	{
		hr = S_FALSE;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(dwRes);
	}

LExit:
	return hr;
}

HRESULT MsiBreak()
{
	HRESULT hr = S_OK;
	DWORD dwPrcId = 0;
	WCHAR szMsg[MAX_PATH + 100];

	dwPrcId = ::GetCurrentProcessId();
	::StringCbPrintf(szMsg, ARRAYSIZE(szMsg), L"MsiBreak process %u", dwPrcId);

	::MessageBox(nullptr, szMsg, L"MsiBreak", MB_OK);

	return hr;
}
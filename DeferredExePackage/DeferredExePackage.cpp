#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <Rpc.h>
#include <tchar.h>
#include <stdlib.h>
#include <sddl.h>
#pragma comment (lib, "Rpcrt4.lib")
#pragma comment (lib, "Ole32.lib")

#define PRINT_INFO			    TEXT("--print-info")
#define SKIP_UNTIL_HERE_FLAG    TEXT("--skip-until-here")
#define IGNORE_ME_FLAG			TEXT("--ignore-me")
#define USER_FLAG				TEXT("--user")
#define DOMAIN_FLAG		        TEXT("--domain")
#define OUTPUT_BUFFER_SIZE		1024

#define ExitIf(cond, op, fmt, ...)	if (cond) {op; _tprintf(TEXT(fmt) TEXT("\n"), __VA_ARGS__); goto LExit; }

static void PrintDebugInfo();
static DWORD CreatePipes(HANDLE* phStdOutRd, HANDLE* phStdOutWr, HANDLE* phStdErr, HANDLE* phStdIn);
static DWORD LogProcessOutput(HANDLE hProcess, HANDLE hStdErrOut);

int _tmain(int argc, _TCHAR* argv[])
{
    DWORD dwExitCode = ERROR_SUCCESS;
    STARTUPINFO startupInfo = {};
    PROCESS_INFORMATION processInfo = {};
    _TCHAR* szCmdLine = nullptr;
    _TCHAR* szSkipArg = nullptr;
    _TCHAR* szIgnoreArg = nullptr;
    const _TCHAR* szDomain = nullptr;
    const _TCHAR* szUser = nullptr;
    _TCHAR* szPassword = nullptr;
    BOOL bRes = TRUE;
	HANDLE hStdout = ::GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdin = ::GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdinRd = INVALID_HANDLE_VALUE;

	// Parse logon user
	for (int i = 0; i < argc; ++i)
	{
		if ((_tcscmp(argv[i], USER_FLAG) == 0) && (++i < argc))
		{
			szUser = argv[i];
			continue;
		}
		if ((_tcscmp(argv[i], DOMAIN_FLAG) == 0) && (++i < argc))
		{
			szDomain = argv[i];
			continue;
		}
		if (_tcscmp(argv[i], PRINT_INFO) == 0)
		{
			PrintDebugInfo();
			goto LExit;
		}
		if ((_tcscmp(argv[i], SKIP_UNTIL_HERE_FLAG) == 0) || (_tcscmp(argv[i], IGNORE_ME_FLAG) == 0))
		{
			break;
		}
	}
	if (szUser && *szUser)
	{
		DWORD dwSize = 1000; // oversized password
		
		szPassword = new _TCHAR[dwSize + 1];
		ExitIf(!szPassword, dwExitCode = ERROR_OUTOFMEMORY, "Failed to allocate buffer");
		ZeroMemory(szPassword, sizeof(_TCHAR) * (dwSize + 1));

		bRes = ::ReadFile(hStdin, szPassword, dwSize * sizeof(_TCHAR), &dwSize, nullptr);
		ExitIf(!bRes, dwExitCode = ::GetLastError(), "Failed to read password from stdin. Error %u", dwExitCode);

		if (!dwSize)
		{
			delete[]szPassword;
			szPassword = nullptr;
		}
	}

	// Parse ignore/skip command line args
    szCmdLine = ::GetCommandLine();
    if ((szCmdLine == nullptr) || (szCmdLine[0] == NULL))
    {
        goto LExit;
    }

    // Search for first arg in full command line
    szSkipArg = _tcsstr(szCmdLine, SKIP_UNTIL_HERE_FLAG);
    szIgnoreArg = _tcsstr(szCmdLine, IGNORE_ME_FLAG);
    if (szIgnoreArg && (!szSkipArg || (szIgnoreArg < szSkipArg)))
    {
        _tprintf(TEXT("Empty run requested: %s\n"), ::GetCommandLine());
        goto LExit;
    }
	ExitIf(!szSkipArg, dwExitCode = ERROR_BAD_ARGUMENTS, "Unidentified command line: '%s'", szCmdLine);

    szCmdLine = szSkipArg + ::_tcsclen(SKIP_UNTIL_HERE_FLAG);
    while ((szCmdLine[0] != NULL) && ::_istspace(szCmdLine[0]))
    {
        ++szCmdLine;
    }
	ExitIf(szCmdLine[0] == NULL, dwExitCode = ERROR_BAD_ARGUMENTS, "Command line after skip is empty: '%s'", ::GetCommandLine());

	::memset(&processInfo, 0, sizeof(processInfo));
	::memset(&startupInfo, 0, sizeof(startupInfo));

	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.dwFlags = STARTF_USESTDHANDLES;
	dwExitCode = CreatePipes(&hStdinRd, &startupInfo.hStdOutput, &startupInfo.hStdError, &startupInfo.hStdInput);
	ExitIf(dwExitCode, dwExitCode, "Failed to create pipes. Error %u", dwExitCode);

	if (szUser && *szUser)
	{
		bRes = ::CreateProcessWithLogonW(szUser, szDomain, szPassword, LOGON_WITH_PROFILE, nullptr, szCmdLine, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo);
		if (szPassword && *szPassword)
		{
			ZeroMemory(szPassword, wcslen(szPassword) * sizeof(szPassword[0]));
		}
	}
	else 
	{
		bRes = ::CreateProcess(nullptr, szCmdLine, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo);
	}
	ExitIf(!bRes, dwExitCode = ::GetLastError(), "Failed to launch process with command line '%s'. Exit code is %u", szCmdLine, dwExitCode);

	dwExitCode = LogProcessOutput(processInfo.hProcess, hStdinRd);
	ExitIf(dwExitCode, dwExitCode, "Failed to log process stdout. Error %u", dwExitCode);

	dwExitCode = ::WaitForSingleObject(processInfo.hProcess, INFINITE);
	ExitIf(dwExitCode == WAIT_FAILED, dwExitCode = ::GetLastError(), "Failed to wait for process to complete. Error %u", dwExitCode);

    bRes = ::GetExitCodeProcess(processInfo.hProcess, &dwExitCode);
	ExitIf(!bRes, dwExitCode = ::GetLastError(), "Failed to get exit code of process with command line '%s'. Error %u", szCmdLine, dwExitCode);

LExit:
    if (processInfo.hThread != NULL)
    {
        ::CloseHandle(processInfo.hThread);
    }
    if (processInfo.hProcess != NULL)
    {
        ::CloseHandle(processInfo.hProcess);
    }
	if (szPassword)
	{
		delete[]szPassword;
	}
	if (startupInfo.hStdError && startupInfo.hStdError != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(startupInfo.hStdError);
	}
	if (startupInfo.hStdInput && startupInfo.hStdInput != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(startupInfo.hStdInput);
	}
	if (startupInfo.hStdOutput && startupInfo.hStdOutput != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(startupInfo.hStdOutput);
	}
	if (hStdinRd && hStdinRd != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hStdinRd);
	}

    return dwExitCode;
}

static DWORD CreatePipes(HANDLE* phStdOutRd, HANDLE* phStdOutWr, HANDLE* phStdErr, HANDLE* phStdIn)
{
	DWORD dwRes = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	SECURITY_ATTRIBUTES sa;
	RPC_STATUS rs = RPC_S_OK;
	UUID guid = {};
	WCHAR wzGuid[39];
	TCHAR szStdInPipeName[100];
	TCHAR szStdOutPipeName[100];
	HANDLE hOutTemp = INVALID_HANDLE_VALUE;
	HANDLE hInTemp = INVALID_HANDLE_VALUE;
	HANDLE hOutRead = INVALID_HANDLE_VALUE;
	HANDLE hOutWrite = INVALID_HANDLE_VALUE;
	HANDLE hErrWrite = INVALID_HANDLE_VALUE;
	HANDLE hInRead = INVALID_HANDLE_VALUE;
	HANDLE hInWrite = INVALID_HANDLE_VALUE;

	ZeroMemory(&sa, sizeof(sa));

	// Generate unique pipe names
	rs = ::UuidCreate(&guid);
	ExitIf(rs < 0, dwRes = ((rs & 0xFFFF) | 0x1), "Failed to generate UUID. Error %i", rs);

	ZeroMemory(wzGuid, ARRAYSIZE(wzGuid) * sizeof(wzGuid[0]));
	for (int i = 0; i < sizeof(guid); ++i)
	{
		LPWSTR sz = wzGuid + i * 2;
		const BYTE g = ((const BYTE*)&guid)[i];
		wsprintfW(sz, L"%02X", g);
	}

	wsprintf(szStdInPipeName, TEXT("\\\\.\\pipe\\%ls-stdin"), wzGuid);
	wsprintf(szStdOutPipeName, TEXT("\\\\.\\pipe\\%ls-stdout"), wzGuid);

	// Fill out security structure so we can inherit handles
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	// Create pipes
	hOutTemp = ::CreateNamedPipe(szStdOutPipeName, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, OUTPUT_BUFFER_SIZE, OUTPUT_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, &sa);
	ExitIf(hOutTemp == INVALID_HANDLE_VALUE, dwRes = ::GetLastError(), "Failed to create pipe. Error %u", dwRes);

	hOutWrite = ::CreateFile(szStdOutPipeName, FILE_WRITE_DATA | SYNCHRONIZE | FILE_FLAG_OVERLAPPED, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	ExitIf(hOutWrite == INVALID_HANDLE_VALUE, dwRes = ::GetLastError(), "Failed to open pipe. Error %u", dwRes);

	bRes = ::DuplicateHandle(::GetCurrentProcess(), hOutWrite, ::GetCurrentProcess(), &hErrWrite, 0, TRUE, DUPLICATE_SAME_ACCESS);
	ExitIf(!bRes, dwRes = ::GetLastError(), "Failed to duplicate pipe handle. Error %u", dwRes);

	bRes = ::DuplicateHandle(::GetCurrentProcess(), hOutTemp, ::GetCurrentProcess(), &hOutRead, 0, FALSE, DUPLICATE_SAME_ACCESS);
	ExitIf(!bRes, dwRes = ::GetLastError(), "Failed to duplicate pipe handle. Error %u", dwRes);

	hInTemp = ::CreateNamedPipe(szStdInPipeName, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, 1, OUTPUT_BUFFER_SIZE, OUTPUT_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, &sa);
	ExitIf(hInTemp == INVALID_HANDLE_VALUE, dwRes = ::GetLastError(), "Failed to create pipe. Error %u", dwRes);

	hInRead = ::CreateFile(szStdInPipeName, FILE_READ_DATA, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	ExitIf(hInRead == INVALID_HANDLE_VALUE, dwRes = ::GetLastError(), "Failed to open pipe. Error %u", dwRes);

	bRes = ::DuplicateHandle(::GetCurrentProcess(), hInTemp, ::GetCurrentProcess(), &hInWrite, 0, FALSE, DUPLICATE_SAME_ACCESS);
	ExitIf(!bRes, dwRes = ::GetLastError(), "Failed to duplicate pipe handle. Error %u", dwRes);

	if (phStdErr)
	{
		*phStdErr = hErrWrite;
		hErrWrite = INVALID_HANDLE_VALUE;
	}
	if (phStdIn)
	{
		*phStdIn = hInRead;
		hInRead = INVALID_HANDLE_VALUE;
	}
	if (phStdOutWr)
	{
		*phStdOutWr = hOutWrite;
		hOutWrite = INVALID_HANDLE_VALUE;
	}
	if (phStdOutRd)
	{
		*phStdOutRd = hOutRead;
		hOutRead = INVALID_HANDLE_VALUE;
	}

LExit:
	if (hOutTemp && hOutTemp != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hOutTemp);
	}
	if (hInTemp && hInTemp != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hInTemp);
	}
	if (hOutRead != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hOutRead);
	}
	if (hOutWrite != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hOutWrite);
	}
	if (hErrWrite != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hErrWrite);
	}
	if (hInRead != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hInRead);
	}
	if (hInWrite != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hInWrite);
	}
	return dwRes;
}

static DWORD LogProcessOutput(HANDLE hProcess, HANDLE hStdErrOut)
{
	BOOL bRes = TRUE;
	DWORD dwBufferSize = 0;
	BYTE* pBuffer = nullptr;
	OVERLAPPED overlapped = {};
	HANDLE rghHandles[2] = { NULL,NULL };
	DWORD dwRes = ERROR_SUCCESS;

	bRes = ::GetNamedPipeInfo(hStdErrOut, nullptr, &dwBufferSize, nullptr, nullptr);
	ExitIf(!bRes, dwRes = ::GetLastError(), "Failed to get pipe size. Error %u", dwRes);

	pBuffer = (LPBYTE)malloc(dwBufferSize);
	ExitIf(!pBuffer, dwRes = ERROR_NOT_ENOUGH_MEMORY, "Failed to allocate memory");

	ZeroMemory(&overlapped, sizeof(overlapped));
	overlapped.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	ExitIf(!overlapped.hEvent, dwRes = ::GetLastError(), "Failed to create event. Error %u", dwRes);

	rghHandles[0] = overlapped.hEvent;
	rghHandles[1] = hProcess;

	bRes = ::ConnectNamedPipe(hStdErrOut, &overlapped);
	if (!bRes)
	{
		dwRes = ::GetLastError();
		if (dwRes == ERROR_IO_PENDING)
		{
			dwRes = ::WaitForSingleObject(overlapped.hEvent, INFINITE);
			ExitIf(dwRes != WAIT_OBJECT_0, dwRes, "Failed to wait for process to connect to stdout. Error %u", dwRes);
			bRes = TRUE;
		}
		else if (dwRes != ERROR_PIPE_CONNECTED)
		{
			ExitIf(dwRes, dwRes, "Failed to connect to stdout. Error %u", dwRes);
		}
		bRes = TRUE;
		dwRes = ERROR_SUCCESS;
	}

	while (true)
	{
		DWORD dwWrittenBytes = 0;
		DWORD dwBytes = 0;

		bRes = ::ResetEvent(overlapped.hEvent);
		ExitIf(!bRes, dwRes = ::GetLastError(), "Failed to reset event. Error %u", dwRes);

		bRes = ::ReadFile(hStdErrOut, pBuffer, dwBufferSize, nullptr, &overlapped);
		if (!bRes)
		{
			dwRes = ::GetLastError();
			if (dwRes == ERROR_BROKEN_PIPE)
			{
				dwRes = ERROR_SUCCESS;
				break;
			}
			ExitIf(dwRes != ERROR_IO_PENDING, dwRes = ::GetLastError(), "Failed to wait for stdout data. Error %u", dwRes);
			dwRes = ERROR_SUCCESS;
		}

		dwRes = ::WaitForMultipleObjects(ARRAYSIZE(rghHandles), rghHandles, FALSE, INFINITE);
		// Process terminated, or pipe abandoned
		if ((dwRes == (WAIT_OBJECT_0 + 1)) || (dwRes == WAIT_ABANDONED_0) || (dwRes == (WAIT_ABANDONED_0 + 1)))
		{
			dwRes = ERROR_SUCCESS;
			break;
		}
		ExitIf(dwRes != WAIT_OBJECT_0, dwRes, "Failed to wait for process to terminate or write to stdout. Error %u", dwRes);

		bRes = ::GetOverlappedResult(hStdErrOut, &overlapped, &dwBytes, FALSE);
		if (!bRes)
		{
			dwRes = ::GetLastError();
			ExitIf(dwRes != ERROR_BROKEN_PIPE, dwRes, "Failed to read stdout. Error %u", dwRes);

			dwRes = ERROR_SUCCESS;
			break;
		}

		bRes = ::WriteFile(::GetStdHandle(STD_OUTPUT_HANDLE), pBuffer, dwBytes, &dwWrittenBytes, nullptr);
		ExitIf(!bRes, dwRes = ::GetLastError(), "Failed to write to stdout. Error %u", dwRes);
		ExitIf(dwWrittenBytes != dwBytes, dwRes = ERROR_WRITE_FAULT, "Failed to write sufficient data to stdout");
	}

LExit:
	if (overlapped.hEvent)
	{
		::CloseHandle(overlapped.hEvent);
	}
	if (pBuffer)
	{
		free(pBuffer);
	}

	return dwRes;
}

static void PrintDebugInfo()
{
	LPTSTR szEnvBlock = ::GetEnvironmentStrings();
	BOOL bRes = TRUE;
	HANDLE hProcessToken = NULL;
	TOKEN_USER* pTokenUser = nullptr;
	DWORD dwSize1 = 0;
	DWORD dwSize2 = 0;
	LPTSTR szUserName = nullptr;
	LPTSTR szDomain = nullptr;
	SID_NAME_USE sidName;
	LPTSTR szSid = nullptr;

	if (szEnvBlock)
	{
		_tprintf(TEXT("Environment:\n"));
		for (LPCTSTR sz = szEnvBlock; sz && *sz; sz += 1 + _tcslen(sz))
		{
			_tprintf(TEXT("\t%s\n"), sz);
		}
	}

	bRes = ::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY | TOKEN_QUERY_SOURCE | TOKEN_READ, &hProcessToken);
	ExitIf(!bRes, NULL, "Failed to get process token. Error %u", ::GetLastError());

	dwSize1 = 0;
	bRes = ::GetTokenInformation(hProcessToken, TOKEN_INFORMATION_CLASS::TokenUser, pTokenUser, dwSize1, &dwSize1);
	if (dwSize1)
	{
		pTokenUser = (TOKEN_USER*)malloc(dwSize1);
		ExitIf(!pTokenUser, NULL, "Failed to allocate %u bytes", dwSize1);

		bRes = ::GetTokenInformation(hProcessToken, TOKEN_INFORMATION_CLASS::TokenUser, pTokenUser, dwSize1, &dwSize1);
	}
	ExitIf(!bRes, NULL, "Failed to get token's user. Error %u", ::GetLastError());

	dwSize1 = 0;
	dwSize2 = 0;
	if (pTokenUser)
	{
		bRes = ::LookupAccountSid(nullptr, pTokenUser->User.Sid, szUserName, &dwSize1, szDomain, &dwSize2, &sidName);
		if (dwSize1 || dwSize2)
		{
			if (dwSize1)
			{
				szUserName = (LPTSTR)malloc(dwSize1 * sizeof(TCHAR));
				ExitIf(!szUserName, NULL, "Failed to allocate %u bytes", dwSize1);
			}
			if (dwSize2)
			{
				szDomain = (LPTSTR)malloc(dwSize2 * sizeof(TCHAR));
				ExitIf(!szDomain, NULL, "Failed to allocate %u bytes", dwSize2);
			}

			bRes = ::LookupAccountSid(nullptr, pTokenUser->User.Sid, szUserName, &dwSize1, szDomain, &dwSize2, &sidName);
		}
		ExitIf(!bRes, NULL, "Failed to get user name. Error %u", ::GetLastError());
		_tprintf(TEXT("User '%s\\%s', user type %u\n"), szDomain, szUserName, sidName);

		bRes = ::ConvertSidToStringSid(pTokenUser->User.Sid, &szSid);
		ExitIf(!bRes, NULL, "Failed to get user SID string. Error %u", ::GetLastError());
		_tprintf(TEXT("User SID '%s'\n"), szSid);
	}

LExit:
	if (szEnvBlock)
	{
		::FreeEnvironmentStrings(szEnvBlock);
		szEnvBlock = nullptr;
	}
	if (hProcessToken)
	{
		::CloseHandle(hProcessToken);
		hProcessToken = NULL;
	}
	if (pTokenUser)
	{
		free(pTokenUser);
		pTokenUser = nullptr;
	}
	if (szUserName)
	{
		free(szUserName);
		szUserName = nullptr;
	}
	if (szDomain)
	{
		free(szDomain);
		szDomain = nullptr;
	}
	if (szSid)
	{
		::LocalFree(szSid);
	}

	return;
}

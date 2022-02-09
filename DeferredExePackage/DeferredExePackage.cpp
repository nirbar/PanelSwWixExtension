#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <tchar.h>

#define SKIP_UNTIL_HERE     TEXT("--skip-until-here")
#define IGNORE_ME           TEXT("--ignore-me")

int _tmain()
{
    DWORD dwExitCode = ERROR_SUCCESS;
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;
    _TCHAR* szCmdLine = nullptr;
    _TCHAR* szSkipArg = nullptr;
    _TCHAR* szIgnoreArg = nullptr;
    BOOL bRes = TRUE;

    ::memset(&processInfo, 0, sizeof(processInfo));
    ::memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    szCmdLine = ::GetCommandLine();
    if ((szCmdLine == nullptr) || (szCmdLine[0] == NULL))
    {
        goto LExit;
    }
    _tprintf(TEXT("\nParsing command line '%s'\n"), szCmdLine); //TODO Does that reveal passwords? It isn't logged by burn anyway

    // Search for first arg in full command line
    szSkipArg = _tcsstr(szCmdLine, SKIP_UNTIL_HERE);
    szIgnoreArg = _tcsstr(szCmdLine, IGNORE_ME);
    if (szIgnoreArg && (!szSkipArg || (szIgnoreArg < szSkipArg)))
    {
        _tprintf(TEXT("Empty run requested: %s\n"), ::GetCommandLine());
        goto LExit;
    }
    if (!szSkipArg)
    {
        _tprintf(TEXT("Unidentified command line: '%s'\n"), szCmdLine);
        dwExitCode = ERROR_BAD_ARGUMENTS;
        goto LExit;
    }

    szCmdLine = szSkipArg + ARRAYSIZE(SKIP_UNTIL_HERE);
    while (::_istspace(szCmdLine[0]))
    {
        ++szCmdLine;
    }

    bRes = ::CreateProcess(nullptr, szCmdLine, nullptr, nullptr, FALSE, CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &startupInfo, &processInfo);
    if (!bRes)
    {
        dwExitCode = ::GetLastError();
        goto LExit;
    }

    dwExitCode = ::WaitForSingleObject(processInfo.hProcess, INFINITE);
    if (dwExitCode != WAIT_OBJECT_0)
    {
        goto LExit;
    }

    bRes = ::GetExitCodeProcess(processInfo.hProcess, &dwExitCode);
    if (!bRes)
    {
        dwExitCode = ::GetLastError();
        goto LExit;
    }

LExit:
    if (processInfo.hThread != NULL)
    {
        ::CloseHandle(processInfo.hThread);
    }
    if (processInfo.hProcess != NULL)
    {
        ::CloseHandle(processInfo.hProcess);
    }

    return dwExitCode;
}
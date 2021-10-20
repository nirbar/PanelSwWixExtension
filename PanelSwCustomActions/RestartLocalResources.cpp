#include "stdafx.h"
#include "RestartLocalResources.h"
#include "FileOperations.h"
#include <memutil.h>
#include <strutil.h>
#include <fileutil.h>
#include <pathutil.h>
#include <TlHelp32.h>
#include <RestartManager.h>
#include <rmutil.h>
#include <procutil.h>
#include <list>
#include <algorithm>
#include <Psapi.h>
#include "../CaCommon/WixString.h"
#pragma comment (lib, "Psapi.lib")
using namespace std;
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

// Helper struct to find a folder that contains the exe by path
struct
{
    WCHAR szFullExePath[MAX_PATH + 1];

    bool operator()(const CWixString& folder) const
    {
        HRESULT hr = PathDirectoryContainsPath((LPCWSTR)folder, szFullExePath);
        return (hr == S_OK);
    }
} visInFolder;

extern "C" UINT __stdcall RestartLocalResources(MSIHANDLE hInstall)
{
    HRESULT hr = S_OK;
    UINT er = ERROR_SUCCESS;
    PMSIHANDLE hView;
    PMSIHANDLE hRecord;
    HANDLE hSnap = INVALID_HANDLE_VALUE;
    PROCESSENTRY32W peData;
    CWixString szSessionKey;
    PRMU_SESSION pSession = nullptr;
    HANDLE hProc = nullptr;
    std::list<CWixString> lstFolders;
    CRestartLocalResources cad;

    hr = WcaInitialize(hInstall, __FUNCTION__);
    ExitOnFailure(hr, "Failed to initialize");
    WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

    hr = WcaTableExists(L"PSW_RestartLocalResources");
    ExitOnFailure(hr, "Failed to check if table exists 'PSW_RestartLocalResources'");
    ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_RestartLocalResources'. Have you authored 'PanelSw:RestartLocalResources' entries in WiX code?");

    // Execute view
    hr = WcaOpenExecuteView(L"SELECT `Path`, `Condition` FROM `PSW_RestartLocalResources`", &hView);
    ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_RestartLocalResources'.");

    // Iterate records
    while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
    {
        ExitOnFailure(hr, "Failed to fetch record.");

        // Get fields
        CWixString szPath;
        CWixString szCondition;
        CWixString szDevicePath;

        hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)szPath);
        ExitOnFailure(hr, "Failed to get Property_.");
        hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)szCondition);
        ExitOnFailure(hr, "Failed to get Condition.");

        if (!szCondition.IsNullOrEmpty())
        {
            MSICONDITION condRes = MSICONDITION::MSICONDITION_NONE;

            condRes = ::MsiEvaluateCondition(hInstall, (LPCWSTR)szCondition);
            ExitOnNullWithLastError((condRes != MSICONDITION::MSICONDITION_ERROR), hr, "Failed evaluating condition '%ls'", (LPCWSTR)szCondition);

            hr = (condRes == MSICONDITION::MSICONDITION_FALSE) ? S_FALSE : S_OK;
            WcaLog(LOGMSG_STANDARD, "Condition '%ls' evaluated to %i", (LPCWSTR)szCondition, (1 - (int)hr));
            if (hr == S_FALSE)
            {
                continue;
            }
        }

        hr = CFileOperations::PathToDevicePath(szPath, (LPWSTR*)szDevicePath);
        ExitOnFailure(hr, "Failed to get target folder in device path form");

        lstFolders.push_back(szDevicePath);

        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will Enumerate processes in '%ls'", (LPCWSTR)szDevicePath);
    }
    hr = S_OK;

    // Nothing to do?
    if (lstFolders.size() <= 0)
    {
        ExitFunction();
    }

    // Best effort to register the processes with restart manager
    hr = WcaGetProperty(L"MsiRestartManagerSessionKey", (LPWSTR*)szSessionKey);
    ExitOnFailure(hr, "Failed to get the MsiRestartManagerSessionKey property.");

    if (szSessionKey.IsNullOrEmpty())
    {
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Can't join RestartManager session because key is null");
    }
    else
    {
        hr = RmuJoinSession(&pSession, szSessionKey);
        if (FAILED(hr))
        {
            WcaLogError(hr, "Failed to join RestartManager session '%ls'.", (LPCWSTR)szSessionKey);
            hr = S_OK;
        }
    }

    hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    ExitOnNullWithLastError((hSnap && (hSnap != INVALID_HANDLE_VALUE)), hr, "Failed creating processes snapshot");

    ZeroMemory(&peData, sizeof(peData));
    peData.dwSize = sizeof(peData);

    for (BOOL fContinue = ::Process32FirstW(hSnap, &peData); fContinue; fContinue = ::Process32NextW(hSnap, &peData))
    {
        ReleaseHandle(hProc);
        visInFolder.szFullExePath[0] = NULL;

        hProc = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, peData.th32ProcessID);
        if (hProc)
        {
            if (::GetProcessImageFileNameW(hProc, visInFolder.szFullExePath, ARRAYSIZE(visInFolder.szFullExePath)))
            {
                // Executable is within the folder?
                if (std::any_of(lstFolders.begin(), lstFolders.end(), visInFolder))
                {
                    if (pSession)
                    {
                        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Registering process '%ls' (%u) with RestartManager", visInFolder.szFullExePath, peData.th32ProcessID);

                        hr = RmuAddProcessById(pSession, peData.th32ProcessID);
                        ExitOnFailure(hr, "Failed adding process to RestartManager");
                    }

                    hr = cad.AddRestartLocalResources(visInFolder.szFullExePath, peData.th32ProcessID);
                    ExitOnFailure(hr, "Failed to enlist process '%ls' for termination", visInFolder.szFullExePath)
                }
            }
        }
    }

    er = ::GetLastError();
    if (er != ERROR_NO_MORE_FILES)
    {
        ExitOnWin32Error(er, hr, "Failed enumerating processes");
    }

    if (cad.HasActions())
    {
        CWixString szCustomActionData;

        hr = cad.GetCustomActionData((LPWSTR*)szCustomActionData);
        ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
        
        hr = WcaSetProperty(L"RestartLocalResourcesExec", (LPCWSTR)szCustomActionData);
        ExitOnFailure(hr, "Failed to set property.");
    }

LExit:
    ReleaseFile(hSnap);
    ReleaseHandle(hProc);
    ReleaseMem(pSession);

    er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
    return WcaFinalize(er);
}

HRESULT CRestartLocalResources::AddRestartLocalResources(LPCWSTR szFilePath, DWORD dwProcId)
{
    HRESULT hr = S_OK;
    ::com::panelsw::ca::Command* pCmd = nullptr;
    RestartLocalResourcesDetails* pDetails = nullptr;
    ::std::string* pAny = nullptr;
    bool bRes = true;

    hr = AddCommand("CRestartLocalResources", &pCmd);
    ExitOnFailure(hr, "Failed to add command");

    pDetails = new RestartLocalResourcesDetails();
    ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

    pDetails->set_file(szFilePath, WSTR_BYTE_SIZE(szFilePath));
    pDetails->set_process_id(dwProcId);

    pAny = pCmd->mutable_details();
    ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

    bRes = pDetails->SerializeToString(pAny);
    ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
    return hr;
}

HRESULT CRestartLocalResources::DeferredExecute(const ::std::string& command)
{
    HRESULT hr = S_OK;
    BOOL bRes = TRUE;
    RestartLocalResourcesDetails details;
    LPCWSTR szFile = nullptr;

    bRes = details.ParseFromString(command);
    ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking RestartLocalResourcesDetails");

    szFile = (LPCWSTR)details.file().data();

    hr = Execute(szFile, details.process_id());
    ExitOnFailure(hr, "Failed to terminate process");

LExit:
    return hr;
}

HRESULT CRestartLocalResources::Execute(LPCWSTR szFilePath, DWORD dwProcId)
{
    HRESULT hr = S_OK;
    HANDLE hProcess = NULL;
    DWORD er = ERROR_SUCCESS;
    BOOL bRes = TRUE;

    // Get windows for all processes, kill only those in the list
    ::EnumWindows(KillWindowsProc, static_cast<LPARAM>(dwProcId));

    // Now kill the process if it is still running
    hProcess = ::OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, dwProcId);
    ExitOnNullWithLastError(hProcess, hr, "Failed to open process '%ls' (%u)", szFilePath, dwProcId);

    // Process still running?
    er = ::WaitForSingleObject(hProcess, 0);
    if (er == WAIT_TIMEOUT)
    {
        er = ERROR_SUCCESS;
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Terminating process '%ls' (%u)", szFilePath, dwProcId);
        
        ::TerminateProcess(hProcess, 0);
        er = ::WaitForSingleObject(hProcess, 1000);
    }
    ExitOnWin32Error(er, hr, "Failed to wait for process to terminate");

LExit:
    ReleaseFile(hProcess);

    return hr;
}

BOOL CALLBACK CRestartLocalResources::KillWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD dwMyProcId = static_cast<DWORD>(lParam);
    DWORD dwOtherProcId = 0;

    ::GetWindowThreadProcessId(hwnd, &dwOtherProcId);

    if (dwMyProcId == dwOtherProcId)
    {
        BOOL bRes = TRUE;
        DWORD dwRes = 0;
        HRESULT hr = S_OK;

        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Closing window belonging to process %u", dwMyProcId);

        bRes = ::SendMessageTimeoutW(hwnd, WM_QUERYENDSESSION, 0, ENDSESSION_CLOSEAPP | ENDSESSION_CRITICAL, SMTO_BLOCK | SMTO_ABORTIFHUNG, 10000, &dwRes);
        ExitOnNullWithLastError(bRes, hr, "Process did not respond timely to query-end-session message");
        ExitOnNull(dwRes, hr, E_FAIL, "Process refused on query to shutdown");

        dwRes = 0;
        bRes = ::SendMessageTimeoutW(hwnd, WM_ENDSESSION, TRUE, ENDSESSION_CLOSEAPP | ENDSESSION_CRITICAL, SMTO_BLOCK | SMTO_ABORTIFHUNG, 10000, &dwRes);
        ExitOnNullWithLastError(bRes, hr, "Process did not respond timely to end-session message");
        ExitOnNull(!dwRes, hr, E_FAIL, "Process refused to shutdown");
    }

LExit:
    return TRUE;
}

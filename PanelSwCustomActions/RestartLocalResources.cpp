#include "stdafx.h"
#include "RestartLocalResources.h"
#include "FileOperations.h"
#include <memutil.h>
#include <strutil.h>
#include <fileutil.h>
#include <TlHelp32.h>
#include <RestartManager.h>
#include <rmutil.h>
#include <procutil.h>
#include <list>
#include <algorithm>
#include "../CaCommon/WixString.h"
using namespace std;
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

extern "C" UINT __stdcall RestartLocalResources(MSIHANDLE hInstall)
{
    HRESULT hr = S_OK;
    UINT er = ERROR_SUCCESS;
    PMSIHANDLE hView;
    PMSIHANDLE hRecord;
    CWixString szSessionKey;
    PRMU_SESSION pSession = nullptr;
    std::list<LPWSTR> lstFolders;
    std::map<DWORD, LPWSTR> mapProcId;
    CRestartLocalResources cad;
    LPWSTR szDevicePath;
    CWixString szCustomActionData;

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

        hr = CFileOperations::PathToDevicePath((LPCWSTR)szPath, &szDevicePath);
        ExitOnFailure(hr, "Failed to get target folder in device path form");
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will Enumerate processes in '%ls'", szDevicePath);
        
        lstFolders.push_back(szDevicePath);
        szDevicePath = nullptr;
    }
    hr = S_OK;

    // Nothing to do?
    if (lstFolders.size() <= 0)
    {
        ExitFunction();
    }

    hr = cad.AddRestartLocalResources(lstFolders);
    ExitOnFailure(hr, "Failed to prepare custom action data");

    // Best effort to register the processes with restart manager
    hr = WcaGetProperty(L"MsiRestartManagerSessionKey", (LPWSTR*)szSessionKey);
    ExitOnFailure(hr, "Failed to get the MsiRestartManagerSessionKey property.");

    if (szSessionKey.IsNullOrEmpty())
    {
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Can't join RestartManager session because key is null");
    }
    else
    {
        hr = RmuJoinSession(&pSession, (LPCWSTR)szSessionKey);
        if (FAILED(hr))
        {
            WcaLogError(hr, "Failed to join RestartManager session '%ls'.", (LPCWSTR)szSessionKey);
            hr = S_OK;
        }
    }

    hr = cad.EnumerateLocalProcesses(lstFolders, mapProcId);
    ExitOnFailure(hr, "Failed to enumerate processes");

    for (const std::pair<DWORD, LPWSTR>& prcId : mapProcId)
    {
        if (pSession)
        {
            WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Registering process '%ls' (%u) with RestartManager", prcId.second, prcId.first);

            hr = RmuAddProcessById(pSession, prcId.first);
            ExitOnFailure(hr, "Failed adding process to RestartManager");
        }
    }


    hr = cad.GetCustomActionData((LPWSTR*)szCustomActionData);
    ExitOnFailure(hr, "Failed getting custom action data for deferred action.");

    hr = WcaSetProperty(L"RestartLocalResourcesExec", (LPCWSTR)szCustomActionData);
    ExitOnFailure(hr, "Failed to set property.");

LExit:
    // Release map, list
    for (LPWSTR f : lstFolders)
    {
        ReleaseStr(f);
    }
    for (std::pair<DWORD, LPWSTR> f : mapProcId)
    {
        ReleaseStr(f.second);
    }

    ReleaseStr(szDevicePath);
    ReleaseMem(pSession);

    er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
    return WcaFinalize(er);
}

HRESULT CRestartLocalResources::AddRestartLocalResources(const std::list<LPWSTR>& lstFolders)
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

    for (LPCWSTR szFolder : lstFolders)
    {
        pDetails->add_folders((void*)szFolder, WSTR_BYTE_SIZE(szFolder));
    }

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
    std::list<LPWSTR> lstFolders;

    bRes = details.ParseFromString(command);
    ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking RestartLocalResourcesDetails");

    for (const std::string& fldr : details.folders())
    {
        lstFolders.push_back(const_cast<LPWSTR>((LPCWSTR)(LPVOID)fldr.data()));
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will Enumerate processes in '%ls'", lstFolders.back());
    }

    hr = Execute(lstFolders);
    ExitOnFailure(hr, "Failed to terminate process");

LExit:
    return SUCCEEDED(hr) ? hr : S_FALSE; // Not failing install on this
}

HRESULT CRestartLocalResources::Execute(const std::list<LPWSTR>& lstFolders)
{
    HRESULT hr = S_OK;
    HANDLE hProcess = NULL;
    INT er = ERROR_SUCCESS;
    BOOL bRes = TRUE;
    PMSIHANDLE hActionData;
    std::map<DWORD, LPWSTR> mapProcId;

    hr = EnumerateLocalProcesses(lstFolders, mapProcId);
    ExitOnFailure(hr, "Failed enumerating local processes");
    if (mapProcId.size() <= 0)
    {
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No processes found in any of the folders");
        ExitFunction();
    }

    // Prompt files in use - best effort
    er = PromptFilesInUse(mapProcId);
    switch (er)
    {
    case IDCANCEL:
    case IDABORT:
        hr = HRESULT_FROM_WIN32(ERROR_INSTALL_USEREXIT);
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User aborted installation");
        ExitFunction();

    case IDRETRY:
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User opted to retry files in use");
        hr = Execute(lstFolders);
        ExitFunction();

    case IDIGNORE:
        hr = S_FALSE;
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User ignored files in use");
        ExitFunction();

    default:
        LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, "Aborting processes");
        break;
    }

    for (const std::pair<DWORD, LPWSTR>& prcId : mapProcId)
    {
        // ActionData: "Closing [1]"
        hActionData = ::MsiCreateRecord(1);
        if (hActionData && SUCCEEDED(WcaSetRecordString(hActionData, 1, prcId.second)))
        {
            WcaProcessMessage(INSTALLMESSAGE::INSTALLMESSAGE_ACTIONDATA, hActionData);
        }

        // Get windows for all processes, kill only those in the list
        ::EnumWindows(KillWindowsProc, static_cast<LPARAM>(prcId.first));

        // Now kill the process if it is still running
        hProcess = ::OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, prcId.first);
        ExitOnNullWithLastError(hProcess, hr, "Failed to open process '%ls' (%u)", prcId.second, prcId.first);

        // Process still running?
        er = ::WaitForSingleObject(hProcess, 0);
        if (er == WAIT_TIMEOUT)
        {
            er = ERROR_SUCCESS;
            LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, "Terminating process '%ls' (%u)", prcId.second, prcId.first);

            ::TerminateProcess(hProcess, 0);
            er = ::WaitForSingleObject(hProcess, 1000);
        }
        ExitOnWin32Error(er, hr, "Failed to wait for process to terminate");

        ReleaseHandle(hProcess);
        hProcess = NULL;
    }

LExit:
    // Release map
    for (std::pair<DWORD, LPWSTR> f : mapProcId)
    {
        ReleaseStr(f.second);
    }
    
    ReleaseHandle(hProcess);

    return hr;
}

INT CRestartLocalResources::PromptFilesInUse(const std::map<DWORD, LPWSTR> mapProcId)
{
    PMSIHANDLE hFilesInUse;
    HRESULT hr = S_OK;
    INT er = IDOK;
    DWORD i = 0;

    // Prompt files in use - best effort
    WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Prompting to close %u processes", mapProcId.size());
    hFilesInUse = ::MsiCreateRecord(1 + 2 * mapProcId.size());
    ExitOnNull(hFilesInUse, hr, E_FAIL, "Failed to allocate record for files-in-use message. Closing processes");
        
    for (const std::pair<DWORD, LPWSTR>& prcId : mapProcId)
    {
        hr = WcaSetRecordInteger(hFilesInUse, ++i, prcId.first);
        ExitOnFailure(hr, "Failed setting record");
        
        hr = WcaSetRecordString(hFilesInUse, ++i, prcId.second);
        ExitOnFailure(hr, "Failed setting record");
    }

    er = ::MsiProcessMessage(WcaGetInstallHandle(), /*INSTALLMESSAGE_RMFILESINUSE*/ (INSTALLMESSAGE)0x19000000L, hFilesInUse);
    if (er <= 0)
    {
        er = ::MsiProcessMessage(WcaGetInstallHandle(), INSTALLMESSAGE::INSTALLMESSAGE_FILESINUSE, hFilesInUse);
    }

LExit:

    return FAILED(hr) ? IDOK : er;
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

        LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, true, "Closing window belonging to process %u", dwMyProcId);

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

HRESULT CRestartLocalResources::EnumerateLocalProcesses(const std::list<LPWSTR>& lstFolders, std::map<DWORD, LPWSTR>& mapProcId)
{
    HRESULT hr = S_OK;
    HANDLE hSnap = INVALID_HANDLE_VALUE;
    PROCESSENTRY32W peData;
    HANDLE hProc = nullptr;
    LPWSTR szProcessName = nullptr;

    hr = Initialize();
    ExitOnFailure(hr, "Failed to initialize");

    hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    ExitOnNullWithLastError((hSnap && (hSnap != INVALID_HANDLE_VALUE)), hr, "Failed creating processes snapshot");

    ZeroMemory(&peData, sizeof(peData));
    peData.dwSize = sizeof(peData);

    for (BOOL fContinue = ::Process32FirstW(hSnap, &peData); fContinue; fContinue = ::Process32NextW(hSnap, &peData))
    {
        ReleaseHandle(hProc);
        visInFolder_.szFullExePath[0] = NULL;

        hProc = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, peData.th32ProcessID);
        if (!hProc)
        {
            WcaLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to open process '%ls' (process ID %u). Ignoring error", peData.szExeFile, peData.th32ProcessID);
            continue;
        }

        if (!pGetProcessImageFileNameW_(hProc, visInFolder_.szFullExePath, ARRAYSIZE(visInFolder_.szFullExePath)))
        {
            WcaLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to get process full path for '%ls' (process ID %u). Ignoring error", peData.szExeFile, peData.th32ProcessID);
            continue;
        }

        // Executable is within the folder?
        if (std::any_of(lstFolders.begin(), lstFolders.end(), visInFolder_))
        {
            hr = StrAllocString(&szProcessName, peData.szExeFile, 0);
            ExitOnFailure(hr, "Failed to allocate memory");

            mapProcId[peData.th32ProcessID] = szProcessName;
            szProcessName = nullptr;
        }
    }

LExit:
    ReleaseStr(szProcessName);
    ReleaseFile(hSnap);
    ReleaseHandle(hProc);

    return hr;
}

HRESULT CRestartLocalResources::Initialize()
{
    HRESULT hr = S_OK;
    if (hGetProcessImageFileNameDll_ && pGetProcessImageFileNameW_)
    {
        ExitFunction1(hr = S_FALSE);
    }

    hGetProcessImageFileNameDll_ = ::LoadLibrary(L"Kernel32.dll");
    ExitOnNullWithLastError(hGetProcessImageFileNameDll_, hr, "Failed loading Kernel32.dll");

    pGetProcessImageFileNameW_ = (decltype(::GetProcessImageFileNameW)*)::GetProcAddress(hGetProcessImageFileNameDll_, "GetProcessImageFileNameW");
    if (pGetProcessImageFileNameW_ == nullptr)
    {
        ::FreeLibrary(hGetProcessImageFileNameDll_);

        hGetProcessImageFileNameDll_ = ::LoadLibrary(L"Psapi.dll");
        ExitOnNullWithLastError(hGetProcessImageFileNameDll_, hr, "Failed loading Psapi.dll");

        pGetProcessImageFileNameW_ = (decltype(::GetProcessImageFileNameW)*)::GetProcAddress(hGetProcessImageFileNameDll_, "GetProcessImageFileNameW");
    }
    ExitOnNullWithLastError(pGetProcessImageFileNameW_, hr, "Failed loading function GetProcessImageFileNameW from Kernel32.dll / Psapi.dll");

LExit:
    if (hGetProcessImageFileNameDll_ && !pGetProcessImageFileNameW_)
    {
        Uninitialize();
    }

    return hr;
}

void CRestartLocalResources::Uninitialize()
{
    pGetProcessImageFileNameW_ = nullptr;
    if (hGetProcessImageFileNameDll_)
    {
        ::FreeLibrary(hGetProcessImageFileNameDll_);
        hGetProcessImageFileNameDll_ = NULL;
    }
}

CRestartLocalResources::~CRestartLocalResources()
{
    Uninitialize();
}

#include "ShellExecute.h"
#include "../CaCommon/WixString.h"
#include <Shellapi.h>
#include "shellExecDetails.pb.h"
using namespace ::com::panelsw::ca;
using namespace google::protobuf;
#pragma comment (lib, "Shell32.lib")


#define ShellExecute_QUERY L"SELECT `Id`, `Target`, `Args`, `Verb`, `WorkingDir`, `Show`, `Wait`, `Flags`, `Condition` FROM `PSW_ShellExecute`"
enum ShellExecuteQuery { Id=1, Target=2, Args=3, Verb=4, WorkingDir=5, Show=6, Wait=7, Flags=8, Condition=9 };

enum ShellExecuteFlags
{
	None = 0,
	OnExecute = 1,
	OnCommit = 2,
	OnRollback = 4
};

extern "C" UINT __stdcall PSW_ShellExecute(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CShellExecute oDeferredShellExecute;
	CShellExecute oRollbackShellExecute;
	CShellExecute oCommitShellExecute;
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_ShellExecute exists.
	hr = WcaTableExists(L"PSW_ShellExecute");
	ExitOnFailure(hr, "Table does not exist 'PSW_ShellExecute'. Have you authored 'PanelSw:ShellExecute' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(ShellExecute_QUERY, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", ShellExecute_QUERY);

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szTarget, szArgs, szVerb, szWorkingDir, szCondition;
		int nShow = 0;
		int nFlags = ShellExecuteFlags::OnExecute;
		int nWait = 0;

		hr = WcaGetRecordString(hRecord, ShellExecuteQuery::Id, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, ShellExecuteQuery::Target, (LPWSTR*)szTarget);
		ExitOnFailure(hr, "Failed to get Target.");
		hr = WcaGetRecordFormattedString(hRecord, ShellExecuteQuery::Args, (LPWSTR*)szArgs);
		ExitOnFailure(hr, "Failed to get Args.");
		hr = WcaGetRecordFormattedString(hRecord, ShellExecuteQuery::Verb, (LPWSTR*)szVerb);
		ExitOnFailure(hr, "Failed to get Verb.");
		hr = WcaGetRecordFormattedString(hRecord, ShellExecuteQuery::WorkingDir, (LPWSTR*)szWorkingDir);
		ExitOnFailure(hr, "Failed to get WorkingDir.");
		hr = WcaGetRecordInteger(hRecord, ShellExecuteQuery::Show, &nShow);
		ExitOnFailure(hr, "Failed to get Show.");
		hr = WcaGetRecordInteger(hRecord, ShellExecuteQuery::Wait, &nWait);
		ExitOnFailure(hr, "Failed to get Wait.");
		hr = WcaGetRecordInteger(hRecord, ShellExecuteQuery::Flags, &nFlags);
		ExitOnFailure(hr, "Failed to get Flags.");
		hr = WcaGetRecordString(hRecord, ShellExecuteQuery::Condition, (LPWSTR*)szCondition);
		ExitOnFailure(hr, "Failed to get Condition.");

		// Test condition
		MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, szCondition);
		switch (condRes)
		{
		case MSICONDITION::MSICONDITION_NONE:
		case MSICONDITION::MSICONDITION_TRUE:
			WcaLog(LOGMSG_STANDARD, "Condition evaluated to true / none.");
			break;

		case MSICONDITION::MSICONDITION_FALSE:
			WcaLog(LOGMSG_STANDARD, "Skipping. Condition evaluated to false");
			continue;

		case MSICONDITION::MSICONDITION_ERROR:
			hr = E_FAIL;
			ExitOnFailure(hr, "Bad Condition field");
		}

		if ((nFlags & ShellExecuteFlags::OnExecute) != 0)
		{
			hr = oDeferredShellExecute.AddShellExec( szTarget, szArgs, szVerb, szWorkingDir, nShow, nWait != 0);
			ExitOnFailure(hr, "Failed creating custom action data for deferred action.");
		}
		if ((nFlags & ShellExecuteFlags::OnCommit) != 0)
		{
			hr = oCommitShellExecute.AddShellExec( szTarget, szArgs, szVerb, szWorkingDir, nShow, nWait != 0);
			ExitOnFailure(hr, "Failed creating custom action data for deferred action.");
		}
		if ((nFlags & ShellExecuteFlags::OnRollback) != 0)
		{
			hr = oRollbackShellExecute.AddShellExec( szTarget, szArgs, szVerb, szWorkingDir, nShow, nWait != 0);
			ExitOnFailure(hr, "Failed creating custom action data for deferred action.");
		}
	}
	
	// Schedule actions.
	hr = oRollbackShellExecute.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for rollback action.");
	hr = WcaDoDeferredAction(L"ShellExecute_rollback", szCustomActionData, oRollbackShellExecute.GetCost());
	ExitOnFailure(hr, "Failed scheduling rollback action.");

	ReleaseNullStr(szCustomActionData);
	hr = oDeferredShellExecute.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"ShellExecute_deferred", szCustomActionData, oDeferredShellExecute.GetCost());
	ExitOnFailure(hr, "Failed scheduling deferred action.");

	ReleaseNullStr(szCustomActionData);
	hr = oCommitShellExecute.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for commit action.");
	hr = WcaDoDeferredAction(L"ShellExecute_commit", szCustomActionData, oCommitShellExecute.GetCost());
	ExitOnFailure(hr, "Failed scheduling commit action.");

LExit:
	ReleaseStr(szCustomActionData);
	
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CShellExecute::AddShellExec(LPCWSTR szTarget, LPCWSTR szArgs, LPCWSTR szVerb, LPCWSTR szWorkingDir, int nShow, bool bWait)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	ShellExecDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CShellExecute", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new ShellExecDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_target(szTarget, WSTR_BYTE_SIZE(szTarget));
	pDetails->set_args(szArgs, WSTR_BYTE_SIZE(szArgs));
	pDetails->set_verb(szVerb, WSTR_BYTE_SIZE(szVerb));
	pDetails->set_workdir(szWorkingDir, WSTR_BYTE_SIZE(szWorkingDir));

	pDetails->set_wait(bWait);
	pDetails->set_show(nShow);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CShellExecute::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	ShellExecDetails details;
	LPCWSTR szTarget = nullptr;
	LPCWSTR szArgs = nullptr;
	LPCWSTR szVerb = nullptr;
	LPCWSTR szWorkingDir = nullptr;
	int nShow;
	bool bWait;

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking ShellExecDetails");

	szTarget = (LPCWSTR)details.target().data();
	szArgs = (LPCWSTR)details.args().data();
	szVerb = (LPCWSTR)details.verb().data();
	nShow = details.show();
	bWait = details.wait();

	WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "ShellExecute: Target='%ls' Args='%ls' Verb='%ls' WorkingDir='%ls' Show=%i Wait=%i"
		, szTarget, szArgs, szVerb, szWorkingDir, nShow, bWait);
	
	hr = Execute(szTarget, szArgs, szVerb, szWorkingDir, nShow, bWait);
	ExitOnFailure(hr, "Failed to execute \"%ls\" %ls", szTarget, szArgs);

LExit:
	return hr;
}

HRESULT CShellExecute::Execute(LPCWSTR szTarget, LPCWSTR szArgs, LPCWSTR szVerb, LPCWSTR szWorkingDir, int nShow, bool bWait)
{
    HRESULT hr = S_OK;
    SHELLEXECUTEINFO shExecInfo = {};
	PMSIHANDLE hRecord;

	// Notify progress data
	hRecord = ::MsiCreateRecord(4);
	WcaSetRecordString( hRecord, 1, szTarget);
	WcaSetRecordString( hRecord, 2, szArgs);
	WcaSetRecordString( hRecord, 3, szVerb);
	WcaSetRecordString( hRecord, 4, szWorkingDir);
	WcaProcessMessage( INSTALLMESSAGE::INSTALLMESSAGE_ACTIONDATA, hRecord);


    shExecInfo.cbSize = sizeof(shExecInfo);
    shExecInfo.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
	if( bWait)
	{
		shExecInfo.fMask |= SEE_MASK_NOCLOSEPROCESS;
	}

    shExecInfo.lpVerb = szVerb;
    shExecInfo.lpFile = szTarget;
    shExecInfo.lpParameters = szArgs;
    shExecInfo.lpDirectory = szWorkingDir;
    shExecInfo.nShow = nShow;

	if (! ::ShellExecuteEx(&shExecInfo))
    {
        switch (reinterpret_cast<DWORD_PTR>(shExecInfo.hInstApp))
        {
        case SE_ERR_FNF:
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            break;
        case SE_ERR_PNF:
            hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
            break;
        case ERROR_BAD_FORMAT:
            hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            break;
        case SE_ERR_ASSOCINCOMPLETE:
        case SE_ERR_NOASSOC:
            hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
            break;
        case SE_ERR_DDEBUSY: __fallthrough;
        case SE_ERR_DDEFAIL: __fallthrough;
        case SE_ERR_DDETIMEOUT:
            hr = HRESULT_FROM_WIN32(ERROR_DDE_FAIL);
            break;
        case SE_ERR_DLLNOTFOUND:
            hr = HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND);
            break;
        case SE_ERR_OOM:
            hr = E_OUTOFMEMORY;
            break;
        case SE_ERR_ACCESSDENIED:
            hr = E_ACCESSDENIED;
            break;
        default:
            hr = E_FAIL;
        }
        ExitOnFailure1(hr, "ShellExecEx failed with return code %d", reinterpret_cast<DWORD_PTR>(shExecInfo.hInstApp));
    }

	if (bWait)
    {
		::WaitForSingleObject( shExecInfo.hProcess, INFINITE);
		::CloseHandle( shExecInfo.hProcess);
		shExecInfo.hProcess = NULL;
    }

LExit:

    return hr;
}
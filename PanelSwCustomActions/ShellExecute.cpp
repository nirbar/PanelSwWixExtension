#include "ShellExecute.h"
#include "../CaCommon/WixString.h"
#include <Shellapi.h>
#pragma comment (lib, "Shell32.lib")


#define ShellExecute_QUERY L"SELECT `Id`, `Target`, `Args`, `Verb`, `WorkingDir`, `Show`, `Wait`, `Condition` FROM `PSW_ShellExecute`"
enum ShellExecuteQuery { Id=1, Target=2, Args=3, Verb=4, WorkingDir=5, Show=6, Wait=7, Condition=8 };

extern "C" __declspec(dllexport) UINT PSW_ShellExecute(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CShellExecute oDeferredShellExecute;
	CComBSTR szCustomActionData;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_ShellExecute exists.
	hr = WcaTableExists(L"PSW_ShellExecute");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_ShellExecute'. Have you authored 'PanelSw:ShellExecute' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(ShellExecute_QUERY, &hView);
	BreakExitOnFailure1(hr, "Failed to execute SQL query '%ls'.", ShellExecute_QUERY);
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szTarget, szArgs, szVerb, szWorkingDir, szCondition;
		int nShow = 0;
		int nWait = 0;

		hr = WcaGetRecordString(hRecord, ShellExecuteQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, ShellExecuteQuery::Target, (LPWSTR*)szTarget);
		BreakExitOnFailure(hr, "Failed to get Target.");
		hr = WcaGetRecordFormattedString(hRecord, ShellExecuteQuery::Args, (LPWSTR*)szArgs);
		BreakExitOnFailure(hr, "Failed to get Args.");
		hr = WcaGetRecordFormattedString(hRecord, ShellExecuteQuery::Verb, (LPWSTR*)szVerb);
		BreakExitOnFailure(hr, "Failed to get Verb.");
		hr = WcaGetRecordFormattedString(hRecord, ShellExecuteQuery::WorkingDir, (LPWSTR*)szWorkingDir);
		BreakExitOnFailure(hr, "Failed to get WorkingDir.");
		hr = WcaGetRecordInteger(hRecord, ShellExecuteQuery::Show, &nShow);
		BreakExitOnFailure(hr, "Failed to get Show.");
		hr = WcaGetRecordInteger(hRecord, ShellExecuteQuery::Wait, &nWait);
		BreakExitOnFailure(hr, "Failed to get Wait.");
		hr = WcaGetRecordString(hRecord, ShellExecuteQuery::Condition, (LPWSTR*)szCondition);
		BreakExitOnFailure(hr, "Failed to get Condition.");

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
			BreakExitOnFailure(hr, "Bad Condition field");
		}

		hr = oDeferredShellExecute.AddShellExec( szTarget, szArgs, szVerb, szWorkingDir, nShow, nWait != 0);
		BreakExitOnFailure(hr, "Failed creating custom action data for deferred action.");
	}
	
	// Schedule actions.
	szCustomActionData.Empty();
	hr = oDeferredShellExecute.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"ShellExecute_deferred", szCustomActionData, oDeferredShellExecute.GetCost());
	BreakExitOnFailure(hr, "Failed scheduling deferred action.");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CShellExecute::AddShellExec(LPCWSTR szTarget, LPCWSTR szArgs, LPCWSTR szVerb, LPCWSTR szWorkingDir, int nShow, bool bWait)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"ShellExecute", L"CShellExecute", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("Target"), CComVariant(szTarget));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Target'");

	hr = pElem->setAttribute(CComBSTR("Args"), CComVariant(szArgs));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Args'");

	hr = pElem->setAttribute(CComBSTR("Verb"), CComVariant(szVerb));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Verb'");

	hr = pElem->setAttribute(CComBSTR("WorkingDir"), CComVariant(szWorkingDir));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'WorkingDir'");

	hr = pElem->setAttribute(CComBSTR("Show"), CComVariant(nShow));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Show'");

	hr = pElem->setAttribute(CComBSTR("Wait"), CComVariant(bWait ? 1: 0));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Wait'");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CShellExecute::DeferredExecute(IXMLDOMElement* pElem)
{
	HRESULT hr = S_OK;
	CComVariant vTarget;
	CComVariant vArgs;
	CComVariant vVerb;
	CComVariant vWorkingDir;
	CComVariant vShow;
	CComVariant vWait;
	int nShow;
	int nWait;

	// Get Parameters:
	hr = pElem->getAttribute(CComBSTR(L"Target"), &vTarget);
	BreakExitOnFailure(hr, "Failed to get Target");

	// Get Args
	hr = pElem->getAttribute(CComBSTR(L"Args"), &vArgs);
	BreakExitOnFailure(hr, "Failed to get Args");

	// Get Args
	hr = pElem->getAttribute(CComBSTR(L"Verb"), &vVerb);
	BreakExitOnFailure(hr, "Failed to get Verb");

	// Get Args
	hr = pElem->getAttribute(CComBSTR(L"WorkingDir"), &vWorkingDir);
	BreakExitOnFailure(hr, "Failed to get WorkingDir");

	// Get Args
	hr = pElem->getAttribute(CComBSTR(L"Show"), &vShow);
	BreakExitOnFailure(hr, "Failed to get Show");

	// Get Args
	hr = pElem->getAttribute(CComBSTR(L"Wait"), &vWait);
	BreakExitOnFailure(hr, "Failed to get Wait");

	WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "ShellExecute: Target='%ls' Args='%ls' Verb='%ls' WorkingDir='%ls' Show=%ls Wait=%ls"
		, vTarget.bstrVal, vArgs.bstrVal, vVerb.bstrVal, vWorkingDir.bstrVal, vShow.bstrVal, vWait.bstrVal);
	
	nShow = _wtoi( vShow.bstrVal);
	nWait = _wtoi( vWait.bstrVal);
	
	hr = Execute( vTarget.bstrVal, vArgs.bstrVal, vVerb.bstrVal, vWorkingDir.bstrVal, nShow, nWait != 0);
	BreakExitOnFailure2(hr, "Failed to execute \"%ls\" %ls", vTarget.bstrVal, vArgs.bstrVal);

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
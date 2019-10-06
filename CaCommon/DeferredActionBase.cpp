#include "DeferredActionBase.h"
#include "WixString.h"
#include <memutil.h>
using namespace ::com::panelsw::ca;

static void FirstLog(MSIHANDLE hInstall, LPCSTR szMessage);

HRESULT CDeferredActionBase::DeferredEntryPoint(MSIHANDLE hInstall, ReceiverToExecutorFunc mapFunc)
{
	HRESULT hr = S_OK;
	HRESULT hrErr = S_OK;
	LPWSTR szCustomActionData = nullptr;
	BYTE *pData = nullptr;
	DWORD dwDataSize = 0;
	BOOL bRes = TRUE;
	CDeferredActionBase* pExecutor = nullptr;
	CustomActionData cad;
	BOOL bIsRollback = FALSE;

	// Get CustomActionData
	UINT er = ERROR_SUCCESS;
	DWORD_PTR cch = 0;
	WCHAR szEmpty[1] = L"";
	er = ::MsiGetPropertyW(hInstall, L"CustomActionData", szEmpty, (DWORD *)&cch);
	if ((er != ERROR_MORE_DATA) && (er != ERROR_SUCCESS))
	{
		hr = E_FAIL;
		FirstLog(hInstall, "Failed getting CustomActionData");
		ExitFunction();
	}

	hr = StrAlloc(&szCustomActionData, ++cch);
	if (FAILED(hr))
	{
		FirstLog(hInstall, "Failed getting CustomActionData");
		ExitFunction();
	}

	er = ::MsiGetPropertyW(hInstall, L"CustomActionData", szCustomActionData, (DWORD *)&cch);
	if (er != ERROR_SUCCESS)
	{
		hr = E_FAIL;
		FirstLog(hInstall, "Failed getting CustomActionData");
		ExitFunction();
	}

	if (!(szCustomActionData && *szCustomActionData))
	{
		FirstLog(hInstall, "Nothing to do");
		ExitFunction();
	}

	hr = StrAllocBase85Decode(szCustomActionData, &pData, &dwDataSize);
	if (FAILED(hr))
	{
		FirstLog(hInstall, "Failed decoding CustomActionData");
		ExitFunction();
	}

	bRes = cad.ParseFromArray(pData, dwDataSize);
	if (!bRes)
	{
		hr = E_FAIL;
		FirstLog(hInstall, "Failed parsing CustomActionData");
		ExitFunction();
	}

	hr = WcaInitialize(hInstall, cad.id().c_str());
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// During rollback we don't exit on failure before completing cleanup.
	bIsRollback = ::MsiGetMode(WcaGetInstallHandle(), MSIRUNMODE::MSIRUNMODE_ROLLBACK);

	// Iterate elements
	for (const Command &cmd : cad.commands())
	{
		if (cmd.handler().empty())
		{
			continue;
		}
		
		// Get receiver
		hr = mapFunc(cmd.handler().c_str(), &pExecutor);
		if (bIsRollback && FAILED(hr))
		{
			WcaLogError(hr, "Failed to get CDeferredActionBase for '%s'", cmd.handler().c_str());
			hrErr = hr;
			hr = S_OK;
			goto LContinue;
		}
		BreakExitOnFailure(hr, "Failed to get CDeferredActionBase for '%s'", cmd.handler().c_str());

		// Execute
		hr = pExecutor->DeferredExecute(cmd.details());
		if (bIsRollback && FAILED(hr))
		{
			WcaLogError(hr, "Failed");
			hrErr = hr;
			hr = S_OK;
			goto LContinue;
		}
		BreakExitOnFailure(hr, "Failed");

		hr = WcaProgressMessage(cmd.cost(), FALSE);
		if (bIsRollback && FAILED(hr))
		{
			WcaLogError(hr, "Failed to report progress by cost");
			hrErr = hr;
			hr = S_OK;
			goto LContinue;
		}
		BreakExitOnFailure(hr, "Failed to report progress by cost");

	LContinue:

		// Release CDeferredActionBase.
		if (pExecutor)
		{
			delete pExecutor;
			pExecutor = nullptr;
		}
	}

LExit:

	ReleaseStr(szCustomActionData);
	ReleaseMem(pData);
	if (pExecutor)
	{
		delete pExecutor;
	}

	return SUCCEEDED(hr) ? hrErr : hr;
}

CDeferredActionBase::CDeferredActionBase(LPCSTR szId)
	: _uCost( 0)
	, _cad()
{
	_cad.set_id(szId);
}

CDeferredActionBase::~CDeferredActionBase()
{
}

HRESULT CDeferredActionBase::AddCommand(LPCSTR szHandler, Command **ppCommand)
{
	HRESULT hr = S_OK;
	Command *pCmd = nullptr;

	pCmd = _cad.add_commands();

	pCmd->set_handler(szHandler);

	*ppCommand = pCmd;
	pCmd = nullptr;

	if (pCmd)
	{
		_cad.mutable_commands()->RemoveLast();
	}

	return hr;
}

bool CDeferredActionBase::HasActions() const
{
	return (_cad.commands_size() > 0);
}

HRESULT CDeferredActionBase::GetCustomActionData(LPWSTR *pszCustomActionData)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	std::string srlz;
	LPWSTR szCustomActionData = nullptr;

	bRes = _cad.SerializeToString(&srlz);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing CustomActionData");

	hr = StrAllocBase85Encode((const BYTE*)srlz.data(), srlz.size(), &szCustomActionData);
	BreakExitOnFailure(hr, "Failed encode CustomActionData");

	*pszCustomActionData = szCustomActionData;
	szCustomActionData = nullptr;

LExit:
	ReleaseStr(szCustomActionData);

	return hr;
}

HRESULT CDeferredActionBase::Prepend(CDeferredActionBase* pOther)
{
	HRESULT hr = S_OK;
	CustomActionData mergedCad;
	mergedCad.set_id(_cad.id());

	for (const Command &cmd : pOther->_cad.commands())
	{
		Command *pNewCmd = mergedCad.add_commands();
		BreakExitOnNull(pNewCmd, hr, E_FAIL, "Failed to allocate command");

		pNewCmd->CopyFrom(cmd);
	}

	for (const Command &cmd : _cad.commands())
	{
		Command *pNewCmd = mergedCad.add_commands();
		BreakExitOnNull(pNewCmd, hr, E_FAIL, "Failed to allocate command");

		pNewCmd->CopyFrom(cmd);
	}

	_cad.Clear();
	_cad.CopyFrom(mergedCad);

LExit:
	return hr;
}

static void FirstLog(MSIHANDLE hInstall, LPCSTR szMessage)
{
	HRESULT hr = S_OK;

	if (!WcaIsInitialized())
	{
		hr = WcaInitialize(hInstall, "CommonDeferred");
	}

	if (SUCCEEDED(hr))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, szMessage);
	}
}

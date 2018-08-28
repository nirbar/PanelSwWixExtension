#include "DeferredActionBase.h"
#include "WixString.h"
#include <memutil.h>
using namespace ::com::panelsw::ca;
HRESULT CDeferredActionBase::DeferredEntryPoint(ReceiverToExecutorFunc mapFunc)
{
	HRESULT hr = S_OK;
	HRESULT hrErr = S_OK;
	CWixString szCustomActionData;
	BYTE *pData = nullptr;
	DWORD dwDataSize = 0;
	BOOL bRes = TRUE;
	CDeferredActionBase* pExecutor = nullptr;
	CustomActionData cad;
	BOOL bIsRollback = FALSE;

	// Get CustomActionData
	hr = WcaGetProperty(L"CustomActionData", (LPWSTR*)szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting CustomActionData");
	if (szCustomActionData.IsNullOrEmpty())
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Nothing to do");
		ExitFunction();
	}

	hr = StrAllocBase85Decode(szCustomActionData, &pData, &dwDataSize);
	BreakExitOnFailure(hr, "Failed decoding CustomActionData");

	bRes = cad.ParseFromArray(pData, dwDataSize);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed parsing CustomActionData");

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

	ReleaseMem(pData);
	if (pExecutor)
	{
		delete pExecutor;
	}

	return SUCCEEDED(hr) ? hrErr : hr;
}

CDeferredActionBase::CDeferredActionBase()
	: _uCost( 0)
	, _cad()
{
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

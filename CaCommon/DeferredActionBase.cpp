#include "DeferredActionBase.h"
#include "WixString.h"
#include <memutil.h>
#include <wcawow64.h>
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
	BOOL bIsWow64Initialized = FALSE;
	BOOL bRedirected = FALSE;

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
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	bIsWow64Initialized = (WcaInitializeWow64() == S_OK);
	if (bIsWow64Initialized)
	{
		hr = WcaDisableWow64FSRedirection();
		bRedirected = SUCCEEDED(hr);
		if (!bRedirected)
		{
			WcaLogError(hr, "Failed to enable filesystem redirection.");
			hr = S_OK;
		}
	}

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
		ExitOnFailure(hr, "Failed to get CDeferredActionBase for '%s'", cmd.handler().c_str());

		// Execute
		hr = pExecutor->DeferredExecute(cmd.details());
		if (bIsRollback && FAILED(hr))
		{
			WcaLogError(hr, "Failed");
			hrErr = hr;
			hr = S_OK;
			goto LContinue;
		}
		ExitOnFailure(hr, "Failed");

		hr = WcaProgressMessage(cmd.cost(), FALSE);
		if (bIsRollback && FAILED(hr))
		{
			WcaLogError(hr, "Failed to report progress by cost");
			hrErr = hr;
			hr = S_OK;
			goto LContinue;
		}
		ExitOnFailure(hr, "Failed to report progress by cost");

	LContinue:

		// Release CDeferredActionBase.
		if (pExecutor)
		{
			delete pExecutor;
			pExecutor = nullptr;
		}
	}

LExit:

	if (bRedirected)
	{
		WcaRevertWow64FSRedirection();
	}
	if (bIsWow64Initialized)
	{
		WcaFinalizeWow64();
	}

	ReleaseStr(szCustomActionData);
	ReleaseMem(pData);
	if (pExecutor)
	{
		delete pExecutor;
	}

	return SUCCEEDED(hr) ? hrErr : hr;
}

CDeferredActionBase::CDeferredActionBase(LPCSTR szId)
{
	_cad.set_id(szId);
}

CDeferredActionBase::~CDeferredActionBase()
{
}

UINT CDeferredActionBase::GetCost() const
{
	UINT cost = 0;
	for each (const Command cmd in _cad.commands())
	{
		cost += cmd.cost();
	}
	return cost;
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
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing CustomActionData");

	hr = StrAllocBase85Encode((const BYTE*)srlz.data(), srlz.size(), &szCustomActionData);
	ExitOnFailure(hr, "Failed encode CustomActionData");

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
		ExitOnNull(pNewCmd, hr, E_FAIL, "Failed to allocate command");

		pNewCmd->CopyFrom(cmd);
	}

	for (const Command &cmd : _cad.commands())
	{
		Command *pNewCmd = mergedCad.add_commands();
		ExitOnNull(pNewCmd, hr, E_FAIL, "Failed to allocate command");

		pNewCmd->CopyFrom(cmd);
	}

	_cad.Clear();
	_cad.CopyFrom(mergedCad);

LExit:
	return hr;
}

void CDeferredActionBase::LogUnformatted(LOGLEVEL level, PCSTR szFormat, ...)
{
	HRESULT hr = S_OK;
	int nRes = 0;
	LPSTR szMessage = nullptr;
	LPCSTR szLogName = nullptr;
	va_list va;
	size_t nSize = 0;
	PMSIHANDLE hRec;

	va_start(va, szFormat);

	nSize = ::_vscprintf(szFormat, va);
	if (nSize == 0)
	{
		ExitFunction();
	}
	++nSize;

	szMessage = reinterpret_cast<char*>(MemAlloc(nSize, TRUE));
	if (szMessage == nullptr)
	{
		ExitFunction();
	}

	nRes = ::vsprintf_s(szMessage, nSize, szFormat, va);
	if (nRes < 0)
	{
		ExitFunction();
	}

	// Replace non-printable characters with spaces
	for (LPSTR szCurr = szMessage; szCurr && *szCurr; ++szCurr)
	{
		int chr = ((int)szCurr[0]) & 0x7F;
		if (::iscntrl(chr) || ::isspace(chr) || !::isprint(chr))
		{
			*szCurr = L' ';
		}
	}

	hRec = MsiCreateRecord(3);
	if (static_cast<MSIHANDLE>(hRec) == NULL)
	{
		ExitFunction();
	}

	szLogName = WcaGetLogName();
	if (szLogName && *szLogName)
	{
		::MsiRecordSetStringA(hRec, 0, "[1]:  [2]");
		::MsiRecordSetStringA(hRec, 1, szLogName);
		::MsiRecordSetStringA(hRec, 2, szMessage);
	}
	else
	{
		::MsiRecordSetStringA(hRec, 0, "[1]");
		::MsiRecordSetStringA(hRec, 1, szMessage);
	}
	WcaProcessMessage(INSTALLMESSAGE_INFO, hRec);

LExit:

	ReleaseMem(szMessage);
	va_end(va);
	return;
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

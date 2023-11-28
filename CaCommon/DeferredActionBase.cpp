#include "DeferredActionBase.h"
#include "WixString.h"
#include <strsafe.h>
#include <memutil.h>
#include <wcawow64.h>
using namespace ::com::panelsw::ca;

static void FirstLog(MSIHANDLE hInstall, LPCSTR szMessage);

HRESULT CDeferredActionBase::DeferredEntryPoint(MSIHANDLE hInstall, ReceiverToExecutorFunc mapFunc)
{
	HRESULT hr = S_OK;
	HRESULT hrErr = S_OK;
	LPWSTR szCustomActionData = nullptr;
	BYTE* pData = nullptr;
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
	er = ::MsiGetPropertyW(hInstall, L"CustomActionData", szEmpty, (DWORD*)&cch);
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

	er = ::MsiGetPropertyW(hInstall, L"CustomActionData", szCustomActionData, (DWORD*)&cch);
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
	for (const Command& cmd : cad.commands())
	{
		if (cmd.handler().empty())
		{
			continue;
		}

		// Get receiver
		hr = mapFunc(cmd.handler().c_str(), &pExecutor);
		if (bIsRollback && FAILED(hr))
		{
			WcaLogError(hr, "Failed to get CDeferredActionBase for '%hs'", cmd.handler().c_str());
			hrErr = hr;
			hr = S_OK;
			goto LContinue;
		}
		ExitOnFailure(hr, "Failed to get CDeferredActionBase for '%hs'", cmd.handler().c_str());

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

HRESULT CDeferredActionBase::DoDeferredAction(LPCWSTR szCustomActionName)
{
	HRESULT hr = S_OK;
	CWixString szCustomActionData;

	if (HasActions())
	{
		hr = GetCustomActionData((LPWSTR*)szCustomActionData);
		ExitOnFailure(hr, "Failed getting custom action data for commit action.");

		hr = WcaDoDeferredAction(szCustomActionName, (LPCWSTR)szCustomActionData, GetCost());
		ExitOnFailure(hr, "Failed scheduling deferred action '%ls'.", szCustomActionName);
	}

LExit:
	return hr;
}

HRESULT CDeferredActionBase::SetCustomActionData(LPCWSTR szPropertyName)
{
	HRESULT hr = S_OK;
	CWixString szCustomActionData;

	if (HasActions())
	{
		hr = GetCustomActionData((LPWSTR*)szCustomActionData);
		ExitOnFailure(hr, "Failed getting custom action data for commit action.");

		hr = WcaSetProperty(szPropertyName, (LPCWSTR)szCustomActionData);
		ExitOnFailure(hr, "Failed setting CustomActionData to property '%ls'.", szPropertyName);
	}

LExit:
	return hr;
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

HRESULT CDeferredActionBase::AddCommand(LPCSTR szHandler, Command** ppCommand)
{
	HRESULT hr = S_OK;
	Command* pCmd = nullptr;

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

HRESULT CDeferredActionBase::GetCustomActionData(LPWSTR* pszCustomActionData)
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

	for (const Command& cmd : pOther->_cad.commands())
	{
		Command* pNewCmd = mergedCad.add_commands();
		ExitOnNull(pNewCmd, hr, E_FAIL, "Failed to allocate command");

		pNewCmd->CopyFrom(cmd);
	}

	for (const Command& cmd : _cad.commands())
	{
		Command* pNewCmd = mergedCad.add_commands();
		ExitOnNull(pNewCmd, hr, E_FAIL, "Failed to allocate command");

		pNewCmd->CopyFrom(cmd);
	}

	_cad.Clear();
	_cad.CopyFrom(mergedCad);

LExit:
	return hr;
}

void CDeferredActionBase::LogUnformatted(LOGLEVEL level, bool bShowTime, LPCWSTR szFormat, ...)
{
	HRESULT hr = S_OK;
	int nRes = 0;
	LPWSTR szTime = nullptr;
	LPWSTR szFormattedTime = nullptr;
	LPWSTR szMessage = nullptr;
	LPCSTR szLogName = nullptr;
	va_list va = nullptr;
	size_t nSize = 0;
	PMSIHANDLE hRec;

	nSize = 256;
	do
	{
		nSize *= 2;
		if (nSize > STRSAFE_MAX_CCH) // Print a truncated message
		{
			hr = S_OK;
			break;
		}

		if (va)
		{
			va_end(va);
		}

		va_start(va, szFormat);
		ReleaseNullStr(szMessage);

		hr = StrAlloc(&szMessage, nSize);
		ExitOnFailure(hr, "Failed to allocate memory");

		hr = ::StringCchVPrintfW(szMessage, nSize, szFormat, va);
	} while (hr == STRSAFE_E_INSUFFICIENT_BUFFER);
	ExitOnFailure(hr, "Failed to format log message");

	// Replace non-printable characters with spaces
	for (LPWSTR szCurr = szMessage; szCurr && *szCurr; ++szCurr)
	{
		int chr = ((int)szCurr[0]) & 0xFFFF;
		if (::iswcntrl(chr) || ::iswspace(chr) || !::iswprint(chr))
		{
			szCurr[0] = L' ';
		}
	}

	if (bShowTime)
	{
		SYSTEMTIME sysTime;

		::GetLocalTime(&sysTime); // Grab the time once to ensure both calls to GetTimeFormatW() require the same buffer size

		nRes = ::GetTimeFormatW(LOCALE_INVARIANT, TIME_FORCE24HOURFORMAT, &sysTime, nullptr, nullptr, 0);
		if (nRes)
		{
			if (SUCCEEDED(StrAlloc(&szTime, nRes)))
			{
				nRes = ::GetTimeFormatW(LOCALE_INVARIANT, TIME_FORCE24HOURFORMAT, &sysTime, nullptr, szTime, nRes);
				if (nRes)
				{
					StrAllocFormatted(&szFormattedTime, L" (%ls)", szTime);
				}
			}
		}
	}

	hRec = ::MsiCreateRecord(4);
	ExitOnNullWithLastError(hRec, hr, "Failed to create record");

	szLogName = WcaGetLogName();
	if (szLogName && *szLogName)
	{
		hr = WcaSetRecordString(hRec, 0, L"[1][2]:  [3]");
		ExitOnFailure(hr, "Failed to set log record");

		nRes = ::MsiRecordSetStringA(hRec, 1, szLogName);
		ExitOnWin32Error(nRes, hr, "Failed to set log record");

		hr = WcaSetRecordString(hRec, 2, szFormattedTime ? szFormattedTime : L"");
		ExitOnFailure(hr, "Failed to set log record");

		hr = WcaSetRecordString(hRec, 3, szMessage);
		ExitOnFailure(hr, "Failed to set log record");
	}
	else
	{
		hr = WcaSetRecordString(hRec, 0, L"[1][2]");
		ExitOnFailure(hr, "Failed to set log record");

		hr = WcaSetRecordString(hRec, 1, szFormattedTime ? szFormattedTime : L"");
		ExitOnFailure(hr, "Failed to set log record");

		hr = WcaSetRecordString(hRec, 2, szMessage);
		ExitOnFailure(hr, "Failed to set log record");
	}

	WcaProcessMessage(INSTALLMESSAGE_INFO, hRec);

LExit:

	ReleaseStr(szTime);
	ReleaseStr(szFormattedTime);
	ReleaseStr(szMessage);
	if (va)
	{
		va_end(va);
	}
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

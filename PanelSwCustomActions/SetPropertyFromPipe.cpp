#include "pch.h"
#include "setPropertyFromPipeDetails.pb.h"
using namespace ::com::panelsw::ca;

#define SetPropertyFromPipeQuery L"SELECT `PipeName`, `Timeout` FROM `PSW_SetPropertyFromPipe`"
enum eSetPropertyFromPipeQuery { PipeName = 1, Timeout = 2};

static HRESULT ReadPropertiesFromPipe(LPCWSTR szPipeName, UINT nTimeout);

extern "C" UINT __stdcall SetPropertyFromPipe(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_SetPropertyFromPipe");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_SetPropertyFromPipe'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_SetPropertyFromPipe'. Have you authored 'PanelSw:SetPropertyFromPipe' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(SetPropertyFromPipeQuery, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_SetPropertyFromPipe'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString pipeName;
		int timeout = 0;

		hr = WcaGetRecordFormattedString(hRecord, eSetPropertyFromPipeQuery::PipeName, (LPWSTR*)pipeName);
		ExitOnFailure(hr, "Failed to get PipeName.");
		hr = WcaGetRecordInteger(hRecord, eSetPropertyFromPipeQuery::Timeout, &timeout);
		ExitOnFailure(hr, "Failed to get Timeout.");

		// Sec to mili-sec.
		timeout *= 1000;
		if (!timeout)
		{
			timeout = INFINITE;
		}

		if (pipeName.IsNullOrEmpty())
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Pipe name is empty");
			continue;
		}

		hr = ReadPropertiesFromPipe(pipeName, timeout);
		ExitOnFailure(hr, "Failed to read properties from pipe '%ls'.", (LPCWSTR)pipeName);
	}
	hr = S_OK;

LExit:

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT ReadPropertiesFromPipe(LPCWSTR szPipeName, UINT nTimeout)
{
	HRESULT hr = S_OK;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	DWORD dwPipeMode = PIPE_READMODE_MESSAGE;
	BOOL bRes = TRUE;
	OVERLAPPED ovrlp;
	DWORD dwSize = 0;
	DWORD dwRes = ERROR_SUCCESS;
	BYTE* msgBuffer = nullptr;
	SetPropertyFromPipeDetails details;
	PMSIHANDLE hDummy;

	// Based on https://stackoverflow.com/questions/2084535/how-to-get-the-length-of-data-to-be-read-reliably-in-named-pipes

	// Wait to connect to pipe.
	do {
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Reading property values from pipe '%ls'", szPipeName);

		bRes = ::WaitNamedPipe(szPipeName, nTimeout);
		ExitOnNullWithLastError(bRes, hr, "Failed waiting for pipe");

		hPipe = ::CreateFile(szPipeName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_WRITE_ATTRIBUTES | FILE_FLAG_OVERLAPPED, 0);

	} while ((hPipe == INVALID_HANDLE_VALUE) && (::GetLastError() == ERROR_PIPE_BUSY));
	ExitOnNullWithLastError((hPipe != INVALID_HANDLE_VALUE), hr, "Failed openning pipe");

	//TODO: Figure out why this results in ERROR_ACCESS_DENIED. Anyways, it seems to work OK without it.
//	bRes = ::SetNamedPipeHandleState(hPipe, &dwPipeMode, nullptr, nullptr);
//	ExitOnNullWithLastError(bRes, hr, "Failed setting mode for pipe");

	// Wait for server to write message.
	do {
		bRes = ::PeekNamedPipe(hPipe, nullptr, 0, nullptr, &dwSize, nullptr);
		ExitOnNullWithLastError(bRes, hr, "Failed getting message size for pipe");

		// Test if user cancelled.
		if (!dwSize)
		{
			if (!hDummy)
			{
				hDummy = ::MsiCreateRecord(1);
				ExitOnNull(hDummy, hr, E_FAIL, "Failed creating record");

				hr = WcaSetRecordInteger(hDummy, 0, 0);
				ExitOnFailure(hr, "Failed setting record");
			}

			dwRes = WcaProcessMessage(INSTALLMESSAGE::INSTALLMESSAGE_ACTIONDATA, hDummy);
			ExitOnWin32Error(dwRes, hr, "Instructed to cancel");
		}
	} while (dwSize == 0);

	msgBuffer = (BYTE*)MemAlloc(dwSize, FALSE);
	ExitOnNull(msgBuffer, hr, E_FAIL, "Failed allocating memory");

	ZeroMemory(&ovrlp, sizeof(ovrlp));

	ovrlp.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	ExitOnNullWithLastError(ovrlp.hEvent, hr, "Failed creating event");

	bRes = ::ReadFile(hPipe, msgBuffer, dwSize, nullptr, &ovrlp);
	ExitOnNullWithLastError((bRes || (::GetLastError() == ERROR_IO_PENDING)), hr, "Failed reading from pipe");

	if (!bRes)
	{
		dwRes = WaitForSingleObject(ovrlp.hEvent, nTimeout);
		ExitOnWin32Error(dwRes, hr, "Failed waiting for pipe read");
	}

	// Parse message.
	bRes = details.ParseFromArray(msgBuffer, dwSize);
	ExitOnNull(bRes, hr, E_FAIL, "Failed parsing message");

	for (int i = 0; i < details.properties_size(); ++i)
	{
		LPCWSTR szName = (LPCWSTR)(LPVOID)details.properties(i).name().data();
		LPCWSTR szValue = (LPCWSTR)(LPVOID)details.properties(i).value().data();

		if (szName && *szName)
		{
			hr = WcaSetProperty(szName, szValue);
			ExitOnFailure(hr, "Failed to set property.");
		}
	}

LExit:
	if (hPipe != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hPipe);
	}
	ReleaseMem(msgBuffer);

	return hr;
}

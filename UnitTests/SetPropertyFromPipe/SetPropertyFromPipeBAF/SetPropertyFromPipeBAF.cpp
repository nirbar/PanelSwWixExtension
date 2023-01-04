#include <Windows.h>
#include <Msi.h>
#include <dutil.h>
#include <BootstrapperEngine.h>
#include <BootstrapperApplication.h>
#include <IBootstrapperApplication.h>
#include <BalBaseBAFunctions.h>
#include <BalBaseBAFunctionsProc.h>
#include <rpc.h>
#include <strutil.h>
#include <balutil.h>
#include <string>
#include "setPropertyFromPipeDetails.pb.h"
#pragma comment (lib, "Rpcrt4.lib")
using namespace ::com::panelsw::ca;

#define PROP_NAME	L"MY_PROP"
#define PROP_VAL	L"MY_VALUE"
static HRESULT CreateBAFunctions(HMODULE hModule, const BA_FUNCTIONS_CREATE_ARGS* pArgs, BA_FUNCTIONS_CREATE_RESULTS* pResults);
static HINSTANCE vhInstance = nullptr;

class SetPropertyFromPipeBAF : public CBalBaseBAFunctions
{
public:

	HRESULT SendProperty()
	{
		HRESULT hr = S_OK;
		BOOL bRes = TRUE;
		SetPropertyFromPipeDetails details;
		property *prop;
		std::string buffer;
		DWORD dwSize = 0;

		prop = details.add_properties();
		BalExitOnNullWithLastError(prop, hr, "Failed allocating property for pipe message");

		prop->set_name(PROP_NAME, (1 + ::wcslen(PROP_NAME)) * sizeof(WCHAR));
		prop->set_value(PROP_VAL, (1 + ::wcslen(PROP_VAL)) * sizeof(WCHAR));

		bRes = details.SerializeToString(&buffer);
		BalExitOnNullWithLastError(bRes, hr, "Failed serializing message");

		bRes = ::ConnectNamedPipe(hPipe_, nullptr);
		BalExitOnNullWithLastError(bRes, hr, "Failed connecting client to pipe");

		// Enable testing 'Cancel' functionality.
		::Sleep(60 * 1000);

		bRes = ::WriteFile(hPipe_, buffer.data(), buffer.size(), &dwSize, nullptr);
		BalExitOnNullWithLastError(bRes, hr, "Failed writing to pipe");
		BalExitOnNullWithLastError((dwSize == buffer.size()), hr, "Failed writing to pipe (incompatible size)");

	LExit:

		return hr;
	}

	STDMETHODIMP OnDetectBegin(
		__in BOOL /*fCached*/,
		__in BOOTSTRAPPER_REGISTRATION_TYPE /*registrationType*/,
		__in DWORD /*cPackages*/,
		__inout BOOL* /*pfCancel*/
	) override
	{
		HRESULT hr = S_OK;
		UUID pipeId;
		RPC_WSTR szPipeUuid = nullptr;
		LPWSTR szLocalPipeName = nullptr;

		// Pipe name
		hr = ::UuidCreateSequential(&pipeId);
		BalExitOnFailure(hr, "Failed creating pipe ID");

		hr = ::UuidToString(&pipeId, &szPipeUuid);
		BalExitOnFailure(hr, "Failed converting pipe ID to string");

		hr = StrAllocFormatted(&szLocalPipeName, L"\\\\.\\pipe\\%s", szPipeUuid);
		BalExitOnFailure(hr, "Failed formatting string");

		hPipe_ = ::CreateNamedPipe(szLocalPipeName, PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 0, 0, 0, nullptr);
		BalExitOnNullWithLastError((hPipe_ != INVALID_HANDLE_VALUE), hr, "Failed creating pipe '%ls'", szLocalPipeName);

		hr = m_pEngine->SetVariableString(L"PIPE_NAME", szLocalPipeName, TRUE);
		BalExitOnFailure(hr, "Failed setting variable");

		hThread_ = ::CreateThread(nullptr, 0, ThreadFunc, this, 0, nullptr);
		BalExitOnNullWithLastError(hThread_, hr, "Failed creating thread");

	LExit:
		ReleaseStr(szLocalPipeName);
		if (szPipeUuid)
		{
			::RpcStringFree(&szPipeUuid);
		}

		return hr;
	}

	static DWORD WINAPI ThreadFunc(LPVOID lpThreadParameter)
	{
		((SetPropertyFromPipeBAF*)lpThreadParameter)->SendProperty();
		return ERROR_SUCCESS;
	}

	SetPropertyFromPipeBAF(HMODULE hModule, IBootstrapperEngine* pEngine, const BA_FUNCTIONS_CREATE_ARGS* pArgs) 
		: CBalBaseBAFunctions(hModule, pEngine, pArgs)
		, hPipe_(INVALID_HANDLE_VALUE)
		, hThread_(NULL)
	{
	}

	~SetPropertyFromPipeBAF()
	{
		if (hPipe_ != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(hPipe_);
		}
		if (hThread_)
		{
			TerminateThread(hThread_, 0);
			::CloseHandle(hThread_);
		}
	}

private:
	HANDLE hPipe_;
	HANDLE hThread_;
};

extern "C" BOOL WINAPI DllMain(
	IN HINSTANCE hInstance,
	IN DWORD dwReason,
	IN LPVOID /* pvReserved */
)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hInstance);
		vhInstance = hInstance;
		break;

	case DLL_PROCESS_DETACH:
		vhInstance = NULL;
		break;
	}

	return TRUE;
}

extern "C" HRESULT WINAPI BAFunctionsCreate(
	__in const BA_FUNCTIONS_CREATE_ARGS * pArgs,
	__inout BA_FUNCTIONS_CREATE_RESULTS * pResults
)
{
	HRESULT hr = S_OK;

	hr = CreateBAFunctions(vhInstance, pArgs, pResults);
	BalExitOnFailure(hr, "Failed to create BAFunctions interface.");

LExit:
	return hr;
}

extern "C" void WINAPI BAFunctionsDestroy(
	__in const BA_FUNCTIONS_DESTROY_ARGS* /*pArgs*/,
	__inout BA_FUNCTIONS_DESTROY_RESULTS* /*pResults*/
)
{
	BalUninitialize();
}

static HRESULT CreateBAFunctions(HMODULE hModule, const BA_FUNCTIONS_CREATE_ARGS * pArgs, BA_FUNCTIONS_CREATE_RESULTS * pResults)
{
	HRESULT hr = S_OK;
	SetPropertyFromPipeBAF* pBAFunction = nullptr;
	IBootstrapperEngine* pEngine = nullptr;

	hr = BalInitializeFromCreateArgs(pArgs->pBootstrapperCreateArgs, &pEngine);
	ExitOnFailure(hr, "Failed to initialize Bal.");

	pBAFunction = new SetPropertyFromPipeBAF(hModule, pEngine, pArgs);
	BalExitOnNullWithLastError(pBAFunction, hr, "Failed instantiating IBootstrapperBAFunction");

	pResults->pfnBAFunctionsProc = BalBaseBAFunctionsProc;
	pResults->pvBAFunctionsProcContext = pBAFunction;
	pBAFunction = nullptr;

LExit:
	ReleaseObject(pBAFunction);
	ReleaseObject(pEngine);

	return hr;
}

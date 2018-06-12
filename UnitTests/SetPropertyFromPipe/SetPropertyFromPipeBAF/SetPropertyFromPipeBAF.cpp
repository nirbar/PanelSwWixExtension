
#include "BalBaseBootstrapperApplication.h"
#include "IBootstrapperBAFunction.h"
#include <rpc.h>
#include <strutil.h>
#include <string>
#include "setPropertyFromPipeDetails.pb.h"
#pragma comment (lib, "balutil.lib")
#pragma comment (lib, "dutil.lib")
#pragma comment (lib, "Rpcrt4.lib")
using namespace ::com::panelsw::ca;

#define PROP_NAME	L"MY_PROP"
#define PROP_VAL	L"MY_VALUE"


class SetPropertyFromPipeBAF : public IBootstrapperBAFunction
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

		bRes = ::WriteFile(hPipe_, buffer.data(), buffer.size(), &dwSize, nullptr);
		BalExitOnNullWithLastError(bRes, hr, "Failed writing to pipe");
		BalExitOnNullWithLastError((dwSize == buffer.size()), hr, "Failed writing to pipe (incompatible size)");

	LExit:

		return hr;
	}

	STDMETHODIMP OnDetect() 
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
		BalExitOnNullWithLastError1((hPipe_ != INVALID_HANDLE_VALUE), hr, "Failed creating pipe '%ls'", szLocalPipeName);

		hr = pEngine_->SetVariableString(L"PIPE_NAME", szLocalPipeName);
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

	STDMETHODIMP OnDetectComplete() { return S_OK; }
	STDMETHODIMP OnPlan() { return S_OK; }
	STDMETHODIMP OnPlanComplete() { return S_OK; }

	static DWORD WINAPI ThreadFunc(LPVOID lpThreadParameter)
	{
		((SetPropertyFromPipeBAF*)lpThreadParameter)->SendProperty();
		return ERROR_SUCCESS;
	}

	SetPropertyFromPipeBAF(IBootstrapperEngine* pEngine)
		: pEngine_(pEngine)
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
	IBootstrapperEngine * pEngine_;
	HANDLE hPipe_;
	HANDLE hThread_;
};


extern "C" __declspec(dllexport) HRESULT WINAPI CreateBootstrapperBAFunction(IBootstrapperEngine* pEngine, HMODULE, IBootstrapperBAFunction** ppBAFunction)
{
	HRESULT hr = S_OK;
	SetPropertyFromPipeBAF* pBAFunction = nullptr;

	BalInitialize(pEngine);

	pBAFunction = new SetPropertyFromPipeBAF(pEngine);
	BalExitOnNullWithLastError(pBAFunction, hr, "Failed instantiating IBootstrapperBAFunction");

	*ppBAFunction = pBAFunction;
    
LExit:

	return hr;
}

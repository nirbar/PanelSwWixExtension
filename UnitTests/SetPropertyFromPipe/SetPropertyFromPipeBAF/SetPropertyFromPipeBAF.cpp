
#include "BalBaseBootstrapperApplication.h"
#include "IBootstrapperBAFunction.h"
#include <rpc.h>
#include <strutil.h>
#pragma comment (lib, "balutil.lib")
#pragma comment (lib, "dutil.lib")
#pragma comment (lib, "Rpcrt4.lib")

class SetPropertyFromPipeBAF : public IBootstrapperBAFunction
{
public:

	HRESULT SendProperty()
	{
		return S_OK;
	}

	STDMETHODIMP OnDetect() 
	{ 
		HRESULT hr = S_OK;
		UUID pipeId;
		RPC_WSTR szPipeUuid = nullptr;
		LPWSTR szLocalPipeName = nullptr;

		// Pipe name
		hr = ::UuidCreateSequential(&pipeId);
		if ((hr != RPC_S_OK) && (hr != RPC_S_UUID_LOCAL_ONLY))
		{
			BalExitOnFailure(hr, "Failed creating pipe ID");
		}

		hr = ::UuidToStringW(&pipeId, &szPipeUuid);
		BalExitOnFailure(hr, "Failed converting pipe ID to string");

		hr = StrAllocFormatted(&szLocalPipeName, L"\\\\.\\pipe\\%s", szPipeUuid);
		BalExitOnFailure(hr, "Failed formatting string");

		hr = pEngine_->SetVariableString(L"PIPE_NAME", szLocalPipeName);
		BalExitOnFailure(hr, "Failed setting variable");

		hThread_ = ::CreateThread(nullptr, 0, ThreadFunc, this, 0, nullptr);
		BalExitOnNullWithLastError(hThread_, hr, "Failed creating thread");

	LExit:
		ReleaseStr(szLocalPipeName);
		if (szPipeUuid)
		{
			::RpcStringFreeW(&szPipeUuid);
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

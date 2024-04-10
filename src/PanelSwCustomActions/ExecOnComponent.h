#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/WixString.h"
#include "../CaCommon/ErrorPrompter.h"
#include "execOnDetails.pb.h"
#include <map>
#include <vector>

class CExecOnComponent :
	public CDeferredActionBase
{
public:
    typedef std::map<int, int> ExitCodeMap;
	typedef std::map<std::string, com::panelsw::ca::ObfuscatedString> EnvironmentMap;
	typedef EnvironmentMap::const_iterator EnvironmentMapItr;

	CExecOnComponent();

	HRESULT AddExec(const CWixString &szCommand, LPCWSTR szWorkingDirectory, LPCWSTR szDomain, LPCWSTR szUser, LPCWSTR szPassword, ExitCodeMap *pExitCodeMap, std::vector<com::panelsw::ca::ConsoleOuputRemap> *pConsoleOuput, EnvironmentMap *pEnv, int nFlags, com::panelsw::ca::ErrorHandling errorHandling);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:

	struct IMPERSONATION_CONTEXT
	{
		HANDLE hUserToken = NULL;
		BOOL bImpersonated = FALSE;
		HANDLE hProfile = NULL;
		HWINSTA hWinStation = NULL;
		HDESK hDesktop = NULL;

		LPWSTR szExeWrapper = nullptr;
		LPWSTR szDomain = nullptr;
		LPWSTR szUser = nullptr;
		LPWSTR szPassword = nullptr;
	};

	HRESULT ExecuteOne(const com::panelsw::ca::ExecOnDetails &details);

	HRESULT Impersonate(BOOL bImpersonate, LPCWSTR szDomain, LPCWSTR szUser, LPCWSTR szPassword, DWORD dwSessionId, CWixString* pszEnvironmentMultiSz, IMPERSONATION_CONTEXT* pctxImpersonation);
	void Unimpersonate(IMPERSONATION_CONTEXT* pctxImpersonation);

	HRESULT SetEnvironment(CWixString *pszEnvironmentMultiSz, const ::google::protobuf::Map<std::string, com::panelsw::ca::ObfuscatedString> &customEnv);

	HRESULT LogProcessOutput(HANDLE hProc, HANDLE hStdErrOut, LPWSTR *pszText);

	HRESULT LaunchProcess(IMPERSONATION_CONTEXT* pctxImpersonation, LPWSTR szCommand, LPCWSTR szWorkingDirectory, LPCWSTR rgszEnvironment, HANDLE* phProcess, HANDLE* phStdOut);

	// S_FALSE: Had no matches, go on with error handling.
	// S_OK: Ignore errors and continue
	// E_RETRY: Retry
	// E_FAIL: Abort
	HRESULT SearchStdOut(LPCWSTR szStdOut, const com::panelsw::ca::ExecOnDetails &details);

	CErrorPrompter _errorPrompter;
	CErrorPrompter _alwaysPrompter;
	CErrorPrompter _stdoutPrompter;
};


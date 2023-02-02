#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/WixString.h"
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

	DECLSPEC_NOTHROW CExecOnComponent() : CDeferredActionBase("ExecOn") { }

	HRESULT AddExec(const CWixString &szCommand, LPCWSTR szWorkingDirectory, LPCWSTR szDomain, LPCWSTR szUser, LPCWSTR szPassword, ExitCodeMap *pExitCodeMap, std::vector<com::panelsw::ca::ConsoleOuputRemap> *pConsoleOuput, EnvironmentMap *pEnv, int nFlags, com::panelsw::ca::ErrorHandling errorHandling);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT SetEnvironment(const ::google::protobuf::Map<std::string, com::panelsw::ca::ObfuscatedString> &customEnv);

	HRESULT LogProcessOutput(HANDLE hProc, HANDLE hStdErrOut, LPWSTR *pszText);

	// S_FALSE: Had no matches, go on with error handling.
	// S_OK: Ignore errors and continue
	// E_RETRY: Retry
	// E_FAIL: Abort
	HRESULT SearchStdOut(LPCWSTR szStdOut, const com::panelsw::ca::ExecOnDetails &details);
};


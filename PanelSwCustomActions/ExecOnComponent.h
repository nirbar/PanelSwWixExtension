#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "execOnDetails.pb.h"
#include <map>

class CExecOnComponent :
	public CDeferredActionBase
{
public:
    typedef std::map<int, int> ExitCodeMap;
	typedef std::map<std::string, std::string> EnvironmentMap;
	typedef EnvironmentMap::const_iterator EnvironmentMapItr;

	HRESULT AddExec(LPCWSTR szCommand, LPCWSTR szObfuscatedCommand, LPCWSTR szWorkingDirectory, ExitCodeMap *pExitCodeMap, EnvironmentMap *pEnv, int nFlags, com::panelsw::ca::ErrorHandling errorHandling);

protected:
	// Execute the command object (XML element)
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT SetEnvironment(const ::google::protobuf::Map<std::string, std::string> &customEnv);

};


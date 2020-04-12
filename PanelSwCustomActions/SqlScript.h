#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "sqlScriptDetails.pb.h"

class CSqlScript :
	public CDeferredActionBase
{
public:

	CSqlScript() : CDeferredActionBase("SqlScript") { }

	HRESULT AddExec(LPCWSTR szServer, LPCWSTR szInstance, USHORT nPort, bool bEncrypted, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, LPCWSTR szScript, com::panelsw::ca::ErrorHandling errorHandling);

protected:
	// Execute the command object (XML element)
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT ExecuteOne(LPCWSTR szServer, LPCWSTR szInstance, USHORT nPort, bool bEncrypted, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, LPCWSTR szScript, LPWSTR *pszError);

	HRESULT SplitScript(com::panelsw::ca::SqlScriptDetails *pDetails, LPCWSTR szScript);
};
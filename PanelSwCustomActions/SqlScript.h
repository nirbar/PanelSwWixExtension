#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/SqlConnection.h"
#include "sqlScriptDetails.pb.h"

class CSqlScript :
	public CDeferredActionBase
{
public:

	CSqlScript() : CDeferredActionBase("SqlScript") { }

	HRESULT AddExec(LPCWSTR szConnectionString, LPCWSTR szServer, LPCWSTR szInstance, USHORT nPort, bool bEncrypted, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, LPCWSTR szScript, com::panelsw::ca::ErrorHandling errorHandling);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT ExecuteOne(const CSqlConnection& sqlConn, LPCWSTR szScript, LPWSTR* pszError);

	HRESULT SplitScript(com::panelsw::ca::SqlScriptDetails* pDetails, LPCWSTR szScript);
};
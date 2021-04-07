#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/SqlConnection.h"
#include "sqlScriptDetails.pb.h"

class CSqlScript :
	public CDeferredActionBase
{
public:

	CSqlScript() noexcept : CDeferredActionBase("SqlScript") { }

	HRESULT AddExec(LPCWSTR szConnectionString, LPCWSTR szServer, LPCWSTR szInstance, USHORT nPort, bool bEncrypted, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, LPCWSTR szScript, com::panelsw::ca::ErrorHandling errorHandling) noexcept;

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) noexcept override;

private:
	HRESULT ExecuteOne(const CSqlConnection& sqlConn, LPCWSTR szScript, LPWSTR* pszError) noexcept;

	HRESULT SplitScript(com::panelsw::ca::SqlScriptDetails* pDetails, LPCWSTR szScript) noexcept;
};
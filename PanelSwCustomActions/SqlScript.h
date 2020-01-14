#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "sqlScriptDetails.pb.h"
#include <atlbase.h>
#include <oledb.h>

class CSqlScript :
	public CDeferredActionBase
{
public:

	CSqlScript() : CDeferredActionBase("SqlScript") { }

	static HRESULT SqlConnect(LPCWSTR wzServer, LPCWSTR wzInstance, LPCWSTR wzDatabase, LPCWSTR wzUser, LPCWSTR wzPassword, IDBCreateSession** ppidbSession);

	HRESULT AddExec(LPCWSTR szServer, LPCWSTR szInstance, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, LPCWSTR szScript, com::panelsw::ca::ErrorHandling errorHandling);

protected:
	// Execute the command object (XML element)
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT ExecuteOne(LPCWSTR szServer, LPCWSTR szInstance, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, LPCWSTR szScript, LPWSTR *pszError);

	HRESULT SplitScript(com::panelsw::ca::SqlScriptDetails *pDetails, LPCWSTR szScript);

	static HRESULT GetLastErrorText(IUnknown* pObjectWithError, REFIID IID_InterfaceWithError, LPWSTR* pszErrorDescription);
};
#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CShellExecute :
	public CDeferredActionBase
{
public:

	CShellExecute() : CDeferredActionBase("ShellExec") { }

	HRESULT AddShellExec(LPCWSTR szTarget, LPCWSTR szArgs, LPCWSTR szVerb, LPCWSTR szWorkingDir, int nShow, bool bWait);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT Execute(LPCWSTR szTarget, LPCWSTR szArgs, LPCWSTR szVerb, LPCWSTR szWorkingDir, int nShow, bool bWait);
};
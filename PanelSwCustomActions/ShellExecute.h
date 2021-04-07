#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CShellExecute :
	public CDeferredActionBase
{
public:

	CShellExecute() noexcept : CDeferredActionBase("ShellExec") { }

	HRESULT AddShellExec(LPCWSTR szTarget, LPCWSTR szArgs, LPCWSTR szVerb, LPCWSTR szWorkingDir, int nShow, bool bWait) noexcept;

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) noexcept override;

private:
	HRESULT Execute(LPCWSTR szTarget, LPCWSTR szArgs, LPCWSTR szVerb, LPCWSTR szWorkingDir, int nShow, bool bWait) noexcept;
};
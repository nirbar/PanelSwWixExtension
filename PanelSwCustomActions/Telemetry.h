#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CTelemetry :
	public CDeferredActionBase
{
public:

	CTelemetry() : CDeferredActionBase("Telemetry") { }


	HRESULT AddPost(LPCWSTR szUrl, LPCWSTR szPage, LPCWSTR szMethod, LPCWSTR szData, BOOL bSecure);

protected:

	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT Post(LPCWSTR szUrl, LPCWSTR szPage, LPCWSTR szMethod, LPCWSTR szData, BOOL bSecure);
};
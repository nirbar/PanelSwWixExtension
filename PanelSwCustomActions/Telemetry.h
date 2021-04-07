#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CTelemetry :
	public CDeferredActionBase
{
public:

	CTelemetry() noexcept : CDeferredActionBase("Telemetry") { }


	HRESULT AddPost(LPCWSTR szUrl, LPCWSTR szPage, LPCWSTR szMethod, LPCWSTR szData, BOOL bSecure) noexcept;

protected:

	HRESULT DeferredExecute(const ::std::string& command) noexcept override;

private:
	HRESULT Post(LPCWSTR szUrl, LPCWSTR szPage, LPCWSTR szMethod, LPCWSTR szData, BOOL bSecure) noexcept;
};
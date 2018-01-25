#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CTelemetry :
	public CDeferredActionBase
{
public:

	HRESULT AddPost(LPCWSTR szUrl, LPCWSTR szPage, LPCWSTR szMethod, LPCWSTR szData, BOOL bSecure);

protected:
	// Execute the command object (XML element)
	HRESULT DeferredExecute(const ::google::protobuf::Any* pCommand) override;

private:
	HRESULT Post(LPCWSTR szUrl, LPCWSTR szPage, LPCWSTR szMethod, LPCWSTR szData, BOOL bSecure);
};


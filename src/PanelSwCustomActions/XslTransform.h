#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CXslTransform :
	public CDeferredActionBase
{
public:

	CXslTransform() : CDeferredActionBase("XslTransform") { }

	HRESULT AddExec(LPCWSTR szXmlFilePath, LPCWSTR szXslt);

protected:

	HRESULT DeferredExecute(const ::std::string& command) override;
};
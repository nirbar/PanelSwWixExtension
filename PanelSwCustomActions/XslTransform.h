#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CXslTransform :
	public CDeferredActionBase
{
public:

	CXslTransform() noexcept : CDeferredActionBase("XslTransform") { }

	HRESULT AddExec(LPCWSTR szXmlFilePath, LPCWSTR szXslt) noexcept;

protected:

	HRESULT DeferredExecute(const ::std::string& command) noexcept override;
};
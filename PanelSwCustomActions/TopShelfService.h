#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/WixString.h"
#include "topShelfServiceDetails.pb.h"

class CTopShelfService :
	public CDeferredActionBase
{
public:

	CTopShelfService() noexcept : CDeferredActionBase("TopShelf") { }

	HRESULT AddInstall(LPCWSTR file, LPCWSTR serviceName, LPCWSTR displayName, LPCWSTR description, LPCWSTR instance, LPCWSTR userName, LPCWSTR passowrd, ::com::panelsw::ca::TopShelfServiceDetails_HowToStart howToStart, ::com::panelsw::ca::TopShelfServiceDetails_ServiceAccount account, ::com::panelsw::ca::ErrorHandling promptOnError) noexcept;
	HRESULT AddUninstall(LPCWSTR file, LPCWSTR instance) noexcept;

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) noexcept override;

	HRESULT BuildCommandLine(const ::com::panelsw::ca::TopShelfServiceDetails *pDetails, CWixString *pCommandLine) noexcept;
};


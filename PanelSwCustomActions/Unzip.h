#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "..\poco\Zip\include\Poco\Zip\ZipLocalFileHeader.h"
#include <string>

class CUnzip :
	public CDeferredActionBase
{
public:

	HRESULT AddUnzip(LPCWSTR zipFile, LPCWSTR targetFolder);

protected:
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	void OnDecompressError(const void* pSender, std::pair<const Poco::Zip::ZipLocalFileHeader, const std::string>& info);

	bool hadErrors_ = false;
};


#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "..\poco\Zip\include\Poco\Zip\ZipLocalFileHeader.h"
#include "unzipDetails.pb.h"
#include <string>

class CUnzip :
	public CDeferredActionBase
{
public:

	HRESULT AddUnzip(LPCWSTR zipFile, LPCWSTR targetFolder, ::com::panelsw::ca::UnzipDetails_OverwriteMode overwriteMode);

protected:
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT ShouldOverwriteFile(LPCSTR szFile, ::com::panelsw::ca::UnzipDetails_OverwriteMode overwriteMode);
};


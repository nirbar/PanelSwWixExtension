#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "..\poco\Zip\include\Poco\Zip\ZipLocalFileHeader.h"
#include <string>

class CUnzip :
	public CDeferredActionBase
{
public:

	HRESULT AddUnzip(LPCWSTR zipFile, LPCWSTR targetFolder, bool overwrite);

protected:
	HRESULT DeferredExecute(const ::std::string& command) override;
};


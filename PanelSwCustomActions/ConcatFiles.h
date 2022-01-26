#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "concatFilesDetails.pb.h"

class CConcatFiles :
	public CDeferredActionBase
{
public:

	CConcatFiles() : CDeferredActionBase("ConcatFiles") { }

	HRESULT AddConcatFiles(LPCWSTR szRootFile, LPCWSTR szSplitFile);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT ExecuteOne(LPCWSTR szRootFile, LPCWSTR szSplitFile);
};
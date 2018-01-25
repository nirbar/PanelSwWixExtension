#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CFileOperations :
	public CDeferredActionBase
{
public:

	HRESULT AddCopyFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0);
	HRESULT AddMoveFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0);
	HRESULT AddDeleteFile(LPCWSTR szFile, int flags = 0);

    static unsigned long long FileSize(LPCWSTR szPath);

protected:
	// Execute the command object (XML element)
	HRESULT DeferredExecute(const ::google::protobuf::Any* pCommand) override;

private:
	enum DeletePathAttributes
	{
		IgnoreMissingPath = 1
		, IgnoreErrors = 2 * IgnoreMissingPath
	};

	HRESULT CopyPath(LPCWSTR szFrom, LPCWSTR szTo, bool bMove, bool bIgnoreMissing, bool bIgnoreErrors);
	HRESULT DeletePath(LPCWSTR szFrom, bool bIgnoreMissing, bool bIgnoreErrors);
};


#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "fileRegexDetails.pb.h"

class CFileOperations :
	public CDeferredActionBase
{
public:
	enum FileOperationsAttributes
	{
		IgnoreMissingPath = 1
		, IgnoreErrors = 2 * IgnoreMissingPath
	};

	CFileOperations() : CDeferredActionBase("FileOperations") { }

	HRESULT AddCopyFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0);
	HRESULT AddMoveFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0);
	HRESULT AddDeleteFile(LPCWSTR szFile, int flags = 0);

	HRESULT CopyPath(LPCWSTR szFrom, LPCWSTR szTo, bool bMove, bool bIgnoreMissing, bool bIgnoreErrors);
	HRESULT DeletePath(LPCWSTR szFrom, bool bIgnoreMissing, bool bIgnoreErrors);

	static ::com::panelsw::ca::FileRegexDetails::FileEncoding DetectEncoding(const void* pFileContent, DWORD dwSize);
	static HRESULT PathToDevicePath(LPCWSTR szPath, LPWSTR* pszDevicePath);
	static HRESULT ListFiles(LPCWSTR szFolder, LPCWSTR szPattern, bool bRecursive, LPWSTR** pszFiles, UINT* pcFiles);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;
};


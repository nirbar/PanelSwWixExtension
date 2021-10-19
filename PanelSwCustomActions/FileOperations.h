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

	CFileOperations() noexcept : CDeferredActionBase("FileOperations") { }

	HRESULT AddCopyFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0) noexcept;
	HRESULT AddMoveFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0) noexcept;
	HRESULT AddDeleteFile(LPCWSTR szFile, int flags = 0) noexcept;

	HRESULT CopyPath(LPCWSTR szFrom, LPCWSTR szTo, bool bMove, bool bIgnoreMissing, bool bIgnoreErrors) noexcept;
	HRESULT DeletePath(LPCWSTR szFrom, bool bIgnoreMissing, bool bIgnoreErrors) noexcept;

	static ::com::panelsw::ca::FileRegexDetails::FileEncoding DetectEncoding(const void* pFileContent, DWORD dwSize) noexcept;
	static HRESULT PathToDevicePath(LPCWSTR szPath, LPWSTR* pszDevicePath) noexcept;

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) noexcept override;
};


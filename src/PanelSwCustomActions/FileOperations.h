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
		, OnlyIfEmpty = 2 * IgnoreErrors
		, AllowReboot = 2 * OnlyIfEmpty
	};

	typedef struct _FILE_ENTRY
	{
		LPWSTR szPath = nullptr;
		DWORD dwAttributes = INVALID_FILE_ATTRIBUTES;

		_FILE_ENTRY* pSubEntries = nullptr;
		UINT cSubEntries = 0;
	} FILE_ENTRY, * PFILE_ENTRY;

	CFileOperations() : CDeferredActionBase("FileOperations") { }

	HRESULT AddCopyFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0);
	HRESULT AddMoveFile(LPCWSTR szFrom, LPCWSTR szTo, int flags = 0);
	HRESULT AddDeleteFile(LPCWSTR szFile, int flags = 0);

	HRESULT CopyPath(LPCWSTR szFrom, LPCWSTR szTo, bool bMove, bool bIgnoreMissing, bool bIgnoreErrors, bool bOnlyIfEmpty, bool bAllowReboot);
	HRESULT DeletePath(LPCWSTR szFrom, bool bIgnoreMissing, bool bIgnoreErrors, bool bOnlyIfEmpty, bool bAllowReboot);

	static HRESULT PathToDevicePath(LPCWSTR szPath, LPWSTR* pszDevicePath);

	static ::com::panelsw::ca::FileRegexDetails::FileEncoding DetectEncoding(const void* pFileContent, DWORD dwSize);

	static HRESULT MakeTemporaryName(LPCWSTR szBackupOf, LPCWSTR szPrefix, bool bIsFolder, LPWSTR* pszTempName);

	static HRESULT ListFileEntries(CFileOperations::FILE_ENTRY* pRootFolder, LPCWSTR szPattern, bool bRecursive);
	static void ReleaseFileEntries(CFileOperations::FILE_ENTRY* pRootFolder);
	static HRESULT FilterFileEntries(CFileOperations::FILE_ENTRY* pRootFolder, DWORD dwAttributesInclude, DWORD dwAttributesExclude, LPWSTR** pszFiltered, UINT* pcFiltered);

	static HRESULT ListSubFolders(LPCWSTR szBaseFolder, LPWSTR** pszFolders, UINT* pcFolder);
	static HRESULT ListFiles(LPCWSTR szFolder, LPCWSTR szPattern, bool bRecursive, LPWSTR** pszFiles, UINT* pcFiles);
	static HRESULT ListReparsePoints(LPCWSTR szFolder, LPWSTR** pszReparsePoints, UINT* pcReparsePoints);

protected:

	HRESULT DeferredExecute(const ::std::string& command) override;
};

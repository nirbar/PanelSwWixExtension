#pragma once

class CDirectoryTableResolver
{
public:
	~CDirectoryTableResolver();

	HRESULT Initialize();

	HRESULT ResolvePath(LPCWSTR szDirecotoryId, LPWSTR* pszPath);

	HRESULT InsertHierarchy(LPCWSTR szParentId, LPCWSTR szRelativeHierarchy, LPWSTR* pszDirectoryId);

	HRESULT InsertCreateFolderIfMissing(LPCWSTR szDirectory, LPCWSTR szComponent);

private:

	HRESULT InsertDirectoryIfMissing(LPCWSTR szDirectory, LPCWSTR szParent, LPCWSTR szName);

	PMSIHANDLE _hQueryDirView;
	PMSIHANDLE _hInsertDirQueryView;
	PMSIHANDLE _hQueryCreateFolderView;
	MSIHANDLE _hDirectoryTable = NULL;
	MSIHANDLE _hDirectoryColumns = NULL;
	MSIHANDLE _hCreateFolderTable = NULL;
	MSIHANDLE _hCreateFolderColumns = NULL;
};

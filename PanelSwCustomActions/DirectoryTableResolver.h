#pragma once

class CDirectoryTableResolver
{
public:

	HRESULT Initialize();

	HRESULT ResolvePath(LPCWSTR szDirecotoryId, LPWSTR* pszPath);

	HRESULT InsertHierarchy(LPCWSTR szParentId, LPCWSTR szRelativeHierarchy, LPWSTR* pszDirectoryId);

private:
	PMSIHANDLE _hQueryDirView;
	PMSIHANDLE _hInsertDirQueryView;
};

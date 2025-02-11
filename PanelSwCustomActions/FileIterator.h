#pragma once
#include "FileEntry.h"
#include "IFileFilter.h"
#include "../CaCommon/WixString.h"

#ifndef E_NOMOREFILES
	#define E_NOMOREFILES  HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES)
#endif

class CFileIterator
{
public:

	~CFileIterator();
	void Release();

	CFileEntry Find(LPCWSTR szBasePath, const IFileFilter* pIncludeFilter, const IFileFilter* pExcludeFilter, bool bRecursive);
	CFileEntry Next();

	HRESULT Status() const;
	bool IsEnd() const;

private:

	struct FolderIterator
	{
		HANDLE _hFind = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATA _findData = {};
		CWixString _szBasePath;
	};

	CFileEntry ProcessFindData(bool* pbSkip);
	void ReleaseOne();
	HRESULT AppendFolder(LPCWSTR szPath);

	CWixString _szBasePath;
	bool _bRecursive = false;
	FolderIterator* _pFolders = nullptr;
	UINT _cFolders = 0;
	HRESULT _hrStatus = S_OK;
	const IFileFilter* _pIncludeFilter = nullptr;
	const IFileFilter* _pExcludeFilter = nullptr;
};

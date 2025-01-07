#pragma once
#include "FileEntry.h"
#include "../CaCommon/WixString.h"

#ifndef E_NOMOREFILES
	#define E_NOMOREFILES  HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES)
#endif

class CFileIterator
{
public:

	~CFileIterator();
	void Release();

	CFileEntry Find(LPCWSTR szBasePath, LPCWSTR szPattern, bool bRecursive);
	CFileEntry Next();

	HRESULT Status() const;
	bool IsEnd() const;

private:

	CFileEntry ProcessFindData(bool* pbSkip);
	void ReleaseOne();
	HRESULT AppendFolder(LPCWSTR szPath);

	struct FolderIterator
	{
		HANDLE _hFind = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATA _findData = {};
		CWixString _szBasePath;
	};

	bool _bRecursive = false;
	CWixString _szPattern;
	FolderIterator* _pFolders = nullptr;
	UINT _cFolders = 0;
	HRESULT _hrStatus = S_OK;
};

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

	CFileEntry Find(LPCWSTR szBasePath, LPCWSTR szPattern, bool bRecursive, bool bIncludeSelf);
	CFileEntry ProcessFindData();
	CFileEntry ProcessEntry(CFileEntry entry);

	HANDLE _hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA _findData = {};
	CWixString _szBasePath;
	CWixString _szPattern;
	bool _bIncludeSelf = false;
	bool _bRecursive = false;
	CFileIterator* _pCurrSubDir = nullptr;
	HRESULT _hrStatus = S_OK;
};

#include "FileIterator.h"
#include <memutil.h>

CFileIterator::~CFileIterator()
{
	Release();
}

void CFileIterator::Release()
{
	while (_cFolders)
	{
		ReleaseOne();
	}
	ReleaseNullMem(_pFolders);
	_hrStatus = S_OK;
	_szPattern.Release();
}

CFileEntry CFileIterator::Find(LPCWSTR szBasePath, LPCWSTR szPattern, bool bRecursive)
{
	Release();

	_bRecursive = bRecursive;

	if (szPattern && *szPattern)
	{
		_hrStatus = _szPattern.Copy(szPattern);
		ExitOnFailure(_hrStatus, "Failed to copy string");
	}

	_hrStatus = AppendFolder(szBasePath);
	ExitOnFailure(_hrStatus, "Failed to initlize find");

	return Next();

LExit:
	return CFileEntry::InvalidFileEntry();
}

HRESULT CFileIterator::AppendFolder(LPCWSTR szPath)
{
	_hrStatus = MemEnsureArraySize((LPVOID*)&_pFolders, ++_cFolders, sizeof(FolderIterator), 10);
	ExitOnFailure(_hrStatus, "Failed to allocate memory");

	_pFolders[_cFolders - 1] = FolderIterator();

	_hrStatus = _pFolders[_cFolders - 1]._szBasePath.Copy(szPath);
	ExitOnFailure(_hrStatus, "Failed to copy string");
	::PathRemoveBackslash((LPWSTR)_pFolders[_cFolders - 1]._szBasePath);

LExit:
	return _hrStatus;
}

CFileEntry CFileIterator::Next()
{
	CFileEntry entry = CFileEntry::InvalidFileEntry();

	while (_pFolders && _cFolders)
	{
		bool bSkip = false;
		BOOL bRes = TRUE;
		FolderIterator* pFolder = &_pFolders[_cFolders - 1];

		if (pFolder->_hFind == INVALID_HANDLE_VALUE)
		{
			CWixString szBasePattern;

			// We're searching without filespec wildcard pattern so we can match subfolders
			_hrStatus = szBasePattern.AppnedFormat(L"%ls\\*", (LPCWSTR)pFolder->_szBasePath);
			ExitOnFailure(_hrStatus, "Failed to copy string");

			pFolder->_hFind = FindFirstFile(szBasePattern, &pFolder->_findData);
			bRes = (pFolder->_hFind != INVALID_HANDLE_VALUE);
		}
		else
		{
			bRes = ::FindNextFile(pFolder->_hFind, &pFolder->_findData);
		}

		if (!bRes)
		{
			DWORD dwError = ::GetLastError();
			if ((dwError == ERROR_FILE_NOT_FOUND) || (dwError == ERROR_PATH_NOT_FOUND) || (dwError == ERROR_NO_MORE_FILES))
			{
				ReleaseOne();
				continue;
			}
			ExitOnWin32Error(dwError, _hrStatus, "Failed to find files in '%ls'", (LPCWSTR)pFolder->_szBasePath);
		}

		entry = ProcessFindData(&bSkip);
		if (!bSkip)
		{
			return entry;
		}
	}
	_hrStatus = E_NOMOREFILES;

LExit:
	return CFileEntry::InvalidFileEntry();
}

CFileEntry CFileIterator::ProcessFindData(bool* pbSkip)
{
	FolderIterator* pFolder = &_pFolders[_cFolders - 1];
	*pbSkip = false;

	if (wcscmp(pFolder->_findData.cFileName, L".") == 0)
	{
		CFileEntry entry(pFolder->_findData, (LPCWSTR)pFolder->_szBasePath);
		return entry;
	}
	if (wcscmp(pFolder->_findData.cFileName, L"..") == 0)
	{
		*pbSkip = true;
		return CFileEntry::InvalidFileEntry();
	}

	// File?
	if (!CFileEntry::IsDirectory(pFolder->_findData.dwFileAttributes))
	{
		// Skip files that don't match the wildcard pattern
		if (_szPattern.IsNullOrEmpty() || !::PathMatchSpec(pFolder->_findData.cFileName, _szPattern))
		{
			*pbSkip = true;
			return CFileEntry::InvalidFileEntry();
		}

		CFileEntry entry(pFolder->_findData, (LPCWSTR)pFolder->_szBasePath);
		return entry;
	}
	// Skip symlink folders
	if (CFileEntry::IsSymlink(pFolder->_findData.dwFileAttributes, pFolder->_findData.dwReserved0))
	{
		// Skip files that don't match the wildcard pattern
		if (_szPattern.IsNullOrEmpty() || !::PathMatchSpec(pFolder->_findData.cFileName, _szPattern))
		{
			*pbSkip = true;
			return CFileEntry::InvalidFileEntry();
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping files under symbolic link folder '%ls\\%ls'", (LPCWSTR)pFolder->_szBasePath, pFolder->_findData.cFileName);
		CFileEntry entry(pFolder->_findData, (LPCWSTR)pFolder->_szBasePath);
		return entry;
	}
	// Skip mount folders
	if (CFileEntry::IsMountPoint(pFolder->_findData.dwFileAttributes, pFolder->_findData.dwReserved0))
	{
		// Skip files that don't match the wildcard pattern
		if (_szPattern.IsNullOrEmpty() || !::PathMatchSpec(pFolder->_findData.cFileName, _szPattern))
		{
			*pbSkip = true;
			return CFileEntry::InvalidFileEntry();
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping files under mount point folder '%ls\\%ls'", (LPCWSTR)pFolder->_szBasePath, pFolder->_findData.cFileName);
		CFileEntry entry(pFolder->_findData, (LPCWSTR)pFolder->_szBasePath);
		return entry;
	}

	if (_bRecursive)
	{
		CWixString szSubDirPath;

		_hrStatus = szSubDirPath.AppnedFormat(L"%ls\\%ls", (LPCWSTR)pFolder->_szBasePath, pFolder->_findData.cFileName);
		ExitOnFailure(_hrStatus, "Failed to format fodler path");

		_hrStatus = AppendFolder(szSubDirPath);
		ExitOnFailure(_hrStatus, "Failed to initialize find in fodler '%ls'", (LPCWSTR)szSubDirPath);
	}

	// Return the subfolder name too?
	if (!_bRecursive && !_szPattern.IsNullOrEmpty() && ::PathMatchSpec(pFolder->_findData.cFileName, _szPattern))
	{
		CFileEntry entry(pFolder->_findData, (LPCWSTR)pFolder->_szBasePath);
		return entry;
	}

	*pbSkip = true;

LExit:
	return CFileEntry::InvalidFileEntry();
}

HRESULT CFileIterator::Status() const
{
	return _hrStatus;
}
bool CFileIterator::IsEnd() const
{
	return (_hrStatus == E_NOMOREFILES) || !_cFolders;
}

void CFileIterator::ReleaseOne()
{
	if (_pFolders && _cFolders)
	{
		--_cFolders;
		if (_pFolders[_cFolders]._hFind != INVALID_HANDLE_VALUE)
		{
			::FindClose(_pFolders[_cFolders]._hFind);
			_pFolders[_cFolders]._hFind = INVALID_HANDLE_VALUE;
		}
		memset(&_pFolders[_cFolders]._findData, 0, sizeof(_pFolders[_cFolders]._findData));
		_pFolders[_cFolders]._szBasePath.Release();
	}
}

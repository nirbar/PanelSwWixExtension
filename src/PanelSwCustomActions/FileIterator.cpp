#include "pch.h"
#include "FileIterator.h"

CFileIterator::~CFileIterator()
{
	Release();
}

void CFileIterator::Release()
{
	ReleaseFileFindHandle(_hFind);
	memset(&_findData, 0, sizeof(_findData));
	_findData.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	_szBasePath.Release();
	_szPattern.Release();
	_bRecursive = false;
	if (_pCurrSubDir)
	{
		delete _pCurrSubDir;
		_pCurrSubDir = nullptr;
	}
	_hrStatus = S_OK;
}

CFileEntry CFileIterator::Find(LPCWSTR szBasePath, LPCWSTR szPattern, bool bRecursive)
{
	return Find(szBasePath, szPattern, bRecursive, true);
}

CFileEntry CFileIterator::Find(LPCWSTR szBasePath, LPCWSTR szPattern, bool bRecursive, bool bIncludeSelf)
{
	Release();
	DWORD dwAttributes = INVALID_FILE_ATTRIBUTES;
	CWixString szBasePattern;
	bool bSkip = true;
	CFileEntry entry = CFileEntry::InvalidFileEntry();

	_hrStatus = _szBasePath.Copy(szBasePath);
	ExitOnFailure(_hrStatus, "Failed to copy string");
	::PathRemoveBackslash((LPWSTR)_szBasePath);

	_hrStatus = szBasePattern.AppnedFormat(L"%ls\\*", (LPCWSTR)_szBasePath);
	ExitOnFailure(_hrStatus, "Failed to copy string");

	if (szPattern && *szPattern)
	{
		_hrStatus = _szPattern.Copy(szPattern);
		ExitOnFailure(_hrStatus, "Failed to copy string");
	}

	_bRecursive = bRecursive;
	_bIncludeSelf = bIncludeSelf;

	// We're searching without filespec wildcard pattern so we can match subfolders
	_hFind = ::FindFirstFile(szBasePattern, &_findData);
	if (_hFind == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = ::GetLastError();
		if ((dwError == ERROR_FILE_NOT_FOUND) || (dwError == ERROR_PATH_NOT_FOUND) || (dwError == ERROR_NO_MORE_FILES))
		{
			_hrStatus = E_NOMOREFILES;
			ExitFunction();
		}
		ExitOnWin32Error(dwError, _hrStatus, "Failed to find files in '%ls'", szBasePath);
	}

	entry = ProcessFindData(&bSkip);
	if (bSkip)
	{
		return Next();
	}
	return entry;

LExit:
	return CFileEntry::InvalidFileEntry();
}

CFileEntry CFileIterator::Next()
{
	CFileEntry entry = CFileEntry::InvalidFileEntry();

	while (true)
	{
		bool bSkip = false;

		// Sub dir is enumerating?
		if (_pCurrSubDir)
		{
			HRESULT hrSubDir = S_OK;
			// First entry of the subdir?
			if (_pCurrSubDir->_szBasePath.IsNullOrEmpty())
			{
				CWixString szSubDirPath;

				_hrStatus = szSubDirPath.AppnedFormat(L"%ls\\%ls", (LPCWSTR)_szBasePath, _findData.cFileName);
				ExitOnFailure(_hrStatus, "Failed to format fodler path");

				CFileEntry entry = _pCurrSubDir->Find((LPCWSTR)szSubDirPath, _szPattern, _bRecursive, false);
				hrSubDir = _pCurrSubDir->Status();
				if (SUCCEEDED(hrSubDir))
				{
					return entry;
				}
			}
			else
			{
				CFileEntry entry = _pCurrSubDir->Next();
				hrSubDir = _pCurrSubDir->Status();
				if (SUCCEEDED(hrSubDir))
				{
					return entry;
				}
			}

			delete _pCurrSubDir;
			_pCurrSubDir = nullptr;

			// Bad failure
			if (hrSubDir != E_NOMOREFILES)
			{
				_hrStatus = hrSubDir;
				ExitOnFailure(_hrStatus, "Failed to enumerate entries in %ls\\%ls", (LPCWSTR)_szBasePath, _findData.cFileName);
			}
		}

		if (!::FindNextFile(_hFind, &_findData))
		{
			_hrStatus = HRESULT_FROM_WIN32(::GetLastError());
			if (_hrStatus == E_NOMOREFILES)
			{
				return CFileEntry::InvalidFileEntry();
			}

			ExitOnFailure(_hrStatus, "Failed to find next entry in %ls", (LPCWSTR)_szBasePath);
		}

		entry = ProcessFindData(&bSkip);
		if (!bSkip)
		{
			return entry;
		}
	}

LExit:
	return CFileEntry::InvalidFileEntry();
}

CFileEntry CFileIterator::ProcessFindData(bool* pbSkip)
{
	*pbSkip = false;

	if (_bIncludeSelf && (wcscmp(_findData.cFileName, L".") == 0))
	{
		CFileEntry entry(_findData, (LPCWSTR)_szBasePath);
		return ProcessEntry(entry, pbSkip);
	}
	if ((wcscmp(_findData.cFileName, L".") == 0) || (wcscmp(_findData.cFileName, L"..") == 0))
	{
		*pbSkip = true;
		return CFileEntry::InvalidFileEntry();
	}

	// File?
	if (!CFileEntry::IsDirectory(_findData.dwFileAttributes))
	{
		// Skip files that don't match the wildcard pattern
		if (_szPattern.IsNullOrEmpty() || !::PathMatchSpec(_findData.cFileName, _szPattern))
		{
			*pbSkip = true;
			return CFileEntry::InvalidFileEntry();
		}

		CFileEntry entry(_findData, (LPCWSTR)_szBasePath);
		return ProcessEntry(entry, pbSkip);
	}
	// Skip symlink folders
	if (CFileEntry::IsSymlink(_findData.dwFileAttributes, _findData.dwReserved0))
	{
		// Skip files that don't match the wildcard pattern
		if (_szPattern.IsNullOrEmpty() || !::PathMatchSpec(_findData.cFileName, _szPattern))
		{
			*pbSkip = true;
			return CFileEntry::InvalidFileEntry();
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping files under symbolic link folder '%ls\\%ls'", (LPCWSTR)_szBasePath, _findData.cFileName);
		CFileEntry entry(_findData, (LPCWSTR)_szBasePath);
		return ProcessEntry(entry, pbSkip);
	}
	// Skip mount folders
	if (CFileEntry::IsMountPoint(_findData.dwFileAttributes, _findData.dwReserved0))
	{
		// Skip files that don't match the wildcard pattern
		if (_szPattern.IsNullOrEmpty() || !::PathMatchSpec(_findData.cFileName, _szPattern))
		{
			*pbSkip = true;
			return CFileEntry::InvalidFileEntry();
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping files under mount point folder '%ls\\%ls'", (LPCWSTR)_szBasePath, _findData.cFileName);
		CFileEntry entry(_findData, (LPCWSTR)_szBasePath);
		return ProcessEntry(entry, pbSkip);
	}

	// Return the subfolder name too?
	if (_bRecursive || (!_szPattern.IsNullOrEmpty() && ::PathMatchSpec(_findData.cFileName, _szPattern)))
	{
		// When recursive, Next() will handle subfolder
		if (_bRecursive)
		{
			_pCurrSubDir = new CFileIterator();
			ExitOnNull(_pCurrSubDir, _hrStatus, E_OUTOFMEMORY, "Failed to allocate CFileIterator");
		}

		CFileEntry entry(_findData, (LPCWSTR)_szBasePath);
		return ProcessEntry(entry, pbSkip);
	}

	*pbSkip = true;
	return CFileEntry::InvalidFileEntry();

LExit:
	return CFileEntry::InvalidFileEntry();
}

CFileEntry CFileIterator::ProcessEntry(CFileEntry entry, bool* pbSkip)
{
	if (entry.IsValid())
	{
		return entry;
	}

	if (_pCurrSubDir)
	{
		if (_pCurrSubDir->IsEnd())
		{
			delete _pCurrSubDir;
			_pCurrSubDir = nullptr;

			*pbSkip = true;
			return CFileEntry::InvalidFileEntry();
		}

		if (FAILED(_pCurrSubDir->Status()))
		{
			_hrStatus = _pCurrSubDir->Status();
			ExitOnFailure(_hrStatus, "Failed to find in folder '%ls'", (LPCWSTR)_szBasePath);
		}
	}

	if (SUCCEEDED(_hrStatus))
	{
		_hrStatus = E_FAIL;
	}
	ExitOnFailure(_hrStatus, "Failed to find");

LExit:
	return CFileEntry::InvalidFileEntry();
}

HRESULT CFileIterator::Status() const
{
	return _hrStatus;
}
bool CFileIterator::IsEnd() const
{
	return (_hrStatus == E_NOMOREFILES);
}

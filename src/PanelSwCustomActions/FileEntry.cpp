#include "pch.h"
#include "FileEntry.h"
#include <strsafe.h>

CFileEntry::CFileEntry(LPCWSTR szPath)
{
	HRESULT hr = S_OK;
	WIN32_FIND_DATA findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD i = 0;

	hr = _szPath.Copy(szPath);
	ExitOnFailure(hr, "Failed to allocate string");

	::PathRemoveBackslash((LPWSTR)_szPath);

	hr = _szParentPath.Copy(_szPath);
	ExitOnFailure(hr, "Failed to allocate string");

	i = _szParentPath.RFind(L'\\');
	if (i < _szParentPath.StrLen())
	{
		hr = _szParentPath.Substring(0, i);
		ExitOnFailure(hr, "Failed to cut string");
	}

	hFind = ::FindFirstFile((LPCWSTR)_szPath, &findData);
	ExitOnInvalidHandleWithLastError(hFind, hr, "Failed to find data for path '%ls'", (LPCWSTR)_szPath);

	hr = _szFileName.Copy(findData.cFileName);
	ExitOnFailure(hr, "Failed to allocate string");

	_dwFileAttributes = findData.dwFileAttributes;
	_ftCreationTime = findData.ftCreationTime;
	_ftLastAccessTime = findData.ftLastAccessTime;
	_ftLastWriteTime = findData.ftLastWriteTime;
	_ulFileSize.HighPart = findData.nFileSizeHigh;
	_ulFileSize.LowPart = findData.nFileSizeLow;
	_dwReserved0 = findData.dwReserved0;

LExit:
	if (hFind != INVALID_HANDLE_VALUE)
	{
		::FindClose(hFind);
	}
}

CFileEntry::CFileEntry(const WIN32_FIND_DATA& findData, LPCWSTR szBasePath)
{
	HRESULT hr = S_OK;

	if ((findData.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && szBasePath && *szBasePath && findData.cFileName[0])
	{
		DWORD dwStrLen = wcslen(szBasePath) + wcslen(findData.cFileName);

		hr = _szPath.Allocate(dwStrLen + 2); // Make room for backslash and NULL
		ExitOnFailure(hr, "Failed to allocate string");

		hr = ::StringCchCopy((LPWSTR)_szPath, dwStrLen, szBasePath);
		ExitOnFailure(hr, "Failed to copy base path");

		if (wcscmp(findData.cFileName, L".") == 0)
		{
			::PathRemoveBackslash((LPWSTR)_szPath);

			DWORD i = _szPath.RFind(L'\\');
			if (i < _szPath.StrLen())
			{
				hr = _szParentPath.Copy((LPCWSTR)_szPath, i);
				ExitOnFailure(hr, "Failed to copy base path");

				hr = _szFileName.Copy((LPCWSTR)_szPath + i + 1);
				ExitOnFailure(hr, "Failed to copy file name");
			}
		}
		else
		{
			// Ensure backslash
			dwStrLen = _szPath.StrLen();
			if (dwStrLen && (_szPath[dwStrLen - 1] != L'\\'))
			{
				_szPath[dwStrLen] = L'\\';
				_szPath[++dwStrLen] = NULL;
			}

			hr = ::StringCchCopy(((LPWSTR)_szPath) + dwStrLen, _szPath.Capacity() - dwStrLen, findData.cFileName);
			ExitOnFailure(hr, "Failed to append file name to path");

			hr = _szParentPath.Copy(szBasePath);
			ExitOnFailure(hr, "Failed to allocate string");

			hr = _szFileName.Copy(findData.cFileName);
			ExitOnFailure(hr, "Failed to allocate string");
		}
	}

	_dwFileAttributes = findData.dwFileAttributes;
	_ftCreationTime = findData.ftCreationTime;
	_ftLastAccessTime = findData.ftLastAccessTime;
	_ftLastWriteTime = findData.ftLastWriteTime;
	_ulFileSize.HighPart = findData.nFileSizeHigh;
	_ulFileSize.LowPart = findData.nFileSizeLow;
	_dwReserved0 = findData.dwReserved0;

LExit:
	if (FAILED(hr))
	{
		_dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	}
}

CFileEntry::CFileEntry(const CFileEntry& other)
{
	HRESULT hr = S_OK;

	hr = _szPath.Copy(other._szPath);
	ExitOnFailure(hr, "Failed to copy string");

	hr = _szParentPath.Copy(other._szParentPath);
	ExitOnFailure(hr, "Failed to allocate string");

	hr = _szFileName.Copy(other._szFileName);
	ExitOnFailure(hr, "Failed to allocate string");

	_dwFileAttributes = other._dwFileAttributes;
	_ftCreationTime = other._ftCreationTime;
	_ftLastAccessTime = other._ftLastAccessTime;
	_ftLastWriteTime = other._ftLastWriteTime;
	_ulFileSize = other._ulFileSize;
	_dwReserved0 = other._dwReserved0;

LExit:
	if (FAILED(hr))
	{
		_dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	}
}

CFileEntry::CFileEntry(CFileEntry&& other) noexcept
{
	_szPath.Attach(other._szPath.Detach());
	_szParentPath.Attach(other._szParentPath.Detach());
	_szFileName.Attach(other._szFileName.Detach());
	_dwFileAttributes = other._dwFileAttributes;
	_ftCreationTime = other._ftCreationTime;
	_ftLastAccessTime = other._ftLastAccessTime;
	_ftLastWriteTime = other._ftLastWriteTime;
	_ulFileSize = other._ulFileSize;
	_dwReserved0 = other._dwReserved0;
}

CFileEntry& CFileEntry::operator=(CFileEntry& other)
{
	HRESULT hr = S_OK;

	hr = _szPath.Copy(other._szPath);
	ExitOnFailure(hr, "Failed to copy string");

	hr = _szParentPath.Copy(other._szParentPath);
	ExitOnFailure(hr, "Failed to copy string");

	hr = _szFileName.Copy(other._szFileName);
	ExitOnFailure(hr, "Failed to copy string");

	_dwFileAttributes = other._dwFileAttributes;
	_ftCreationTime = other._ftCreationTime;
	_ftLastAccessTime = other._ftLastAccessTime;
	_ftLastWriteTime = other._ftLastWriteTime;
	_ulFileSize = other._ulFileSize;
	_dwReserved0 = other._dwReserved0;

LExit:
	if (FAILED(hr))
	{
		_dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	}

	return *this;
}

CFileEntry& CFileEntry::operator=(CFileEntry&& other) noexcept
{
	_szPath.Attach(other._szPath.Detach());
	_szParentPath.Attach(other._szParentPath.Detach());
	_szFileName.Attach(other._szFileName.Detach());
	_dwFileAttributes = other._dwFileAttributes;
	_ftCreationTime = other._ftCreationTime;
	_ftLastAccessTime = other._ftLastAccessTime;
	_ftLastWriteTime = other._ftLastWriteTime;
	_ulFileSize = other._ulFileSize;
	_dwReserved0 = other._dwReserved0;
	return *this;
}

DWORD CFileEntry::Attributes() const
{
	return _dwFileAttributes;
}
bool CFileEntry::IsValid() const
{
	return (_dwFileAttributes != INVALID_FILE_ATTRIBUTES);
}

bool CFileEntry::IsDirectory() const
{
	return (IsValid() && (_dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}
/*static*/ bool CFileEntry::IsDirectory(DWORD dwFileAttributes)
{
	return ((dwFileAttributes != INVALID_FILE_ATTRIBUTES) && (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool CFileEntry::IsReparsePoint() const
{
	return (IsValid() && (_dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT));
}

bool CFileEntry::IsSymlink() const
{
	return (IsReparsePoint() && (_dwReserved0 == IO_REPARSE_TAG_SYMLINK));
}
/*static*/ bool CFileEntry::IsSymlink(DWORD dwFileAttributes, DWORD dwReserved0)
{
	return ((dwFileAttributes != INVALID_FILE_ATTRIBUTES) && (dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && (dwReserved0 == IO_REPARSE_TAG_SYMLINK));
}

bool CFileEntry::IsMountPoint() const
{
	return (IsReparsePoint() && (_dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT));
}
/*static*/ bool CFileEntry::IsMountPoint(DWORD dwFileAttributes, DWORD dwReserved0)
{
	return ((dwFileAttributes != INVALID_FILE_ATTRIBUTES) && (dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && (dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT));
}

DWORD CFileEntry::ReparsePointTag() const
{
	return (IsValid() && IsReparsePoint()) ? _dwReserved0 : 0;
}

const CWixString& CFileEntry::Path() const
{
	return _szPath;
}

const CWixString& CFileEntry::ParentPath() const
{
	return _szParentPath;
}

const CWixString& CFileEntry::FileName() const
{
	return _szFileName;
}

FILETIME CFileEntry::CreationTime() const
{
	return IsValid() ? _ftCreationTime : FILETIME{};
}
FILETIME CFileEntry::LastWriteTime() const
{
	return IsValid() ? _ftLastWriteTime : FILETIME{};
}

/*static*/ CFileEntry CFileEntry::InvalidFileEntry()
{
	WIN32_FIND_DATA invalidData;

	memset(&invalidData, 0, sizeof(invalidData));
	invalidData.dwFileAttributes = INVALID_FILE_ATTRIBUTES;

	return CFileEntry(invalidData, nullptr);
}

#pragma once
#include "../CaCommon/WixString.h"

class CFileEntry
{
public:

	static CFileEntry InvalidFileEntry();

	// Copy and move c'tor and assignment operators
	CFileEntry(LPCWSTR szPath);
	CFileEntry(const WIN32_FIND_DATA& findData, LPCWSTR szBasePath);
	CFileEntry(const CFileEntry& other);
	CFileEntry(CFileEntry&& other);
	CFileEntry& operator=(CFileEntry& other);
	CFileEntry& operator=(CFileEntry&& other);

	// Attribute tests
	DWORD Attributes() const;
	bool IsValid() const;
	bool IsDirectory() const;
	bool IsReparsePoint() const;
	bool IsSymlink() const;
	bool IsMountPoint() const;

	static bool IsDirectory(DWORD dwFileAttributes);
	static bool IsSymlink(DWORD dwFileAttributes, DWORD dwReserved0);
	static bool IsMountPoint(DWORD dwFileAttributes, DWORD dwReserved0);

	DWORD ReparsePointTag() const;

	const CWixString& ParentPath() const;
	const CWixString& Path() const;
	const CWixString& FileName() const;

	FILETIME CreationTime() const;
	FILETIME LastWriteTime() const;

private:
	DWORD _dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	FILETIME _ftCreationTime = {};
	FILETIME _ftLastAccessTime = {};
	FILETIME _ftLastWriteTime = {};
	ULARGE_INTEGER _ulFileSize = {};
	DWORD _dwReserved0 = 0;
	CWixString _szPath;
	CWixString _szFileName;
	CWixString _szParentPath;
};

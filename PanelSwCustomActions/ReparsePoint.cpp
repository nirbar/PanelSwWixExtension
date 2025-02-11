#include "FileOperations.h"
#include "ReparsePoint.h"
#include "reparsePointDetails.pb.h"
#include "../CaCommon/WixString.h"
#include "FileEntry.h"
#include "FileIterator.h"
#include "FileSpecFilter.h"
#include <memutil.h>
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

// Extract from WDK's ntifs.h
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define METHOD_BUFFERED                 0
#define FILE_ANY_ACCESS                 0
#define FILE_SPECIAL_ACCESS    (FILE_ANY_ACCESS)
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define FSCTL_SET_REPARSE_POINT         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // REPARSE_DATA_BUFFER,
#define FSCTL_GET_REPARSE_POINT         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS) // REPARSE_DATA_BUFFER
#define FSCTL_DELETE_REPARSE_POINT      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // REPARSE_DATA_BUFFER,

typedef struct _REPARSE_DATA_BUFFER {
	ULONG  ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;

	_Field_size_bytes_(ReparseDataLength)
		union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		} SymbolicLinkReparseBuffer;
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;
		struct {
			UCHAR  DataBuffer[1];
		} GenericReparseBuffer;
	} DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, * PREPARSE_DATA_BUFFER;

#define REPARSE_DATA_BUFFER_HEADER_SIZE   FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
// End extract from WDK's ntifs.h

#define E_NOTAREPARSEPOINT				HRESULT_FROM_WIN32(ERROR_NOT_A_REPARSE_POINT)

typedef struct _REPARSE_DATA_COMMON_HEADER {
	ULONG  ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
} REPARSE_DATA_COMMON_HEADER;

enum RemoveFileInstallMode
{
	RemoveFileInstallMode_Unknown = 0,
	RemoveFileInstallMode_Install = 1,
	RemoveFileInstallMode_Uninstall = 2,
	RemoveFileInstallMode_Both = 3,
};

extern "C" UINT __stdcall RemoveReparseDataSched(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	DWORD dwRes = 0;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CReparsePoint execCAD;
	CReparsePoint rollbackCAD;
	CFileSpecFilter* pFileFilter = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_RemoveFolderEx exists.
	hr = WcaTableExists(L"RemoveFile");
	ExitOnFailure(hr, "Failed to check if table exists 'RemoveFile'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'RemoveFile'. Have you authored 'PanelSw:RemoveFolderEx' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Component_`, `FileName`, `DirProperty`, `InstallMode` FROM `RemoveFile`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");
		if (pFileFilter)
		{
			delete pFileFilter;
			pFileFilter = nullptr;
		}

		// Get fields
		CWixString szComponent, szFileName, szDirProperty, szBasePath;
		RemoveFileInstallMode flags = RemoveFileInstallMode::RemoveFileInstallMode_Unknown;
		WCA_TODO componentAction = WCA_TODO::WCA_TODO_UNKNOWN;
		CFileIterator fileFinder;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component_.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szFileName);
		ExitOnFailure(hr, "Failed to get FileName.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)szDirProperty);
		ExitOnFailure(hr, "Failed to get DirProperty.");
		hr = WcaGetRecordInteger(hRecord, 4, (int*)&flags);
		ExitOnFailure(hr, "Failed to get InstallMode.");

		componentAction = WcaGetComponentToDo(szComponent);
		if ((componentAction != WCA_TODO_INSTALL) && (componentAction != WCA_TODO_REINSTALL) && (componentAction != WCA_TODO_UNINSTALL))
		{
			CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Skipping RemoveReparseData for property '%ls' because component isn't executing", (LPCWSTR)szDirProperty);
			continue;
		}
		if (((componentAction == WCA_TODO_INSTALL) || (componentAction == WCA_TODO_REINSTALL)) && !(flags & RemoveFileInstallMode_Install))
		{
			CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Skipping RemoveReparseData for property '%ls' because component action is (re)install", (LPCWSTR)szDirProperty);
			continue;
		}
		if ((componentAction == WCA_TODO_UNINSTALL) && !(flags & RemoveFileInstallMode_Uninstall))
		{
			CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Skipping RemoveReparseData for property '%ls' because component action is uninstall", (LPCWSTR)szDirProperty);
			continue;
		}

		hr = WcaGetProperty(szDirProperty, (LPWSTR*)szBasePath);
		ExitOnFailure(hr, "Failed to get property");

		if (szBasePath.IsNullOrEmpty())
		{
			CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Skipping RemoveReparseData for property '%ls' because it is empty", (LPCWSTR)szDirProperty);
			continue;
		}

		if (!szFileName.IsNullOrEmpty())
		{
			pFileFilter = new CFileSpecFilter();
			ExitOnNull(pFileFilter, hr, E_OUTOFMEMORY, "Failed to instantiate file filter");

			hr = pFileFilter->Initialize(szBasePath, szFileName, false);
			ExitOnFailure(hr, "Failed to initialize file filter");
		}

		for (CFileEntry fileEntry = fileFinder.Find(szBasePath, pFileFilter, nullptr, false); !fileFinder.IsEnd(); fileEntry = fileFinder.Next())
		{
			ExitOnNull(fileEntry.IsValid(), hr, fileFinder.Status(), "Failed to find files in '%ls'", (LPCWSTR)szBasePath);

			if (fileEntry.IsSymlink() || fileEntry.IsMountPoint())
			{
				hr = rollbackCAD.AddRestoreReparsePoint(fileEntry.Path());
				ExitOnFailure(hr, "Failed to get reparse point data for '%ls'", (LPCWSTR)fileEntry.Path());

				hr = execCAD.AddDeleteReparsePoint(fileEntry.Path());
				ExitOnFailure(hr, "Failed to add exec data for reparse point of '%ls'", (LPCWSTR)fileEntry.Path());
			}
		}
	}
	hr = S_OK;

	hr = rollbackCAD.DoDeferredAction(L"RemoveReparseDataRollback");
	ExitOnFailure(hr, "Failed to do action");

	hr = execCAD.DoDeferredAction(L"RemoveReparseDataExec");
	ExitOnFailure(hr, "Failed to do action");

LExit:
	if (pFileFilter)
	{
		delete pFileFilter;
		pFileFilter = nullptr;
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CReparsePoint::AddRestoreReparsePoint(LPCWSTR szPath)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	ReparsePointDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;
	void* pBuffer = nullptr;
	DWORD dwSize = 0;
	FILETIME ftCreateTime = {}, ftWriteTime = {};
	ULARGE_INTEGER ulCreateTime = {}, ulWriteTime = {};

	hr = AddCommand("CReparsePoint", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new ReparsePointDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	hr = GetReparsePointData(szPath, &pBuffer, &dwSize, &ftCreateTime, &ftWriteTime);
	ExitOnFailure(hr, "Failed to read reparse point data for '%ls'", szPath);

	ulCreateTime.HighPart = ftCreateTime.dwHighDateTime;
	ulCreateTime.LowPart = ftCreateTime.dwLowDateTime;
	ulWriteTime.HighPart = ftWriteTime.dwHighDateTime;
	ulWriteTime.LowPart = ftWriteTime.dwLowDateTime;

	pDetails->set_action(::com::panelsw::ca::ReparsePointAction::restore);
	pDetails->set_path(szPath, WSTR_BYTE_SIZE(szPath));
	pDetails->set_reparsedata(pBuffer, dwSize);
	pDetails->set_createtime(ulCreateTime.QuadPart);
	pDetails->set_writetime(ulWriteTime.QuadPart);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	ReleaseMem(pBuffer);

	return hr;
}

HRESULT CReparsePoint::AddDeleteReparsePoint(LPCWSTR szPath)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	ReparsePointDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;
	void* pBuffer = nullptr;
	DWORD dwSize = 0;

	hr = AddCommand("CReparsePoint", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new ReparsePointDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	hr = GetReparsePointData(szPath, &pBuffer, &dwSize, nullptr, nullptr);
	ExitOnFailure(hr, "Failed to read reparse point data for '%ls'", szPath);

	pDetails->set_action(::com::panelsw::ca::ReparsePointAction::delete_);
	pDetails->set_path(szPath, WSTR_BYTE_SIZE(szPath));
	pDetails->set_reparsedata(pBuffer, dwSize);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	ReleaseMem(pBuffer);

	return hr;
}

HRESULT CReparsePoint::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	ReparsePointDetails details;
	LPCWSTR szPath = nullptr;
	LPVOID pData = nullptr;
	DWORD dwSize = 0;
	FILETIME ftCreateTime = {}, ftWriteTime = {};
	FILETIME *pftCreateTime = nullptr, *pftWriteTime = nullptr;
	ULARGE_INTEGER ulCreateTime = {}, ulWriteTime = {};

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking ReparsePointDetails");

	szPath = (LPCWSTR)details.path().c_str();
	pData = const_cast<char*>(details.reparsedata().data());
	dwSize = details.reparsedata().size();

	switch (details.action())
	{
	case ::com::panelsw::ca::ReparsePointAction::delete_:
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Removing reparse point data from '%ls'", szPath);
		hr = DeleteReparsePoint(szPath, pData, dwSize);
		ExitOnFailure(hr, "Failed to delete reparse point in '%ls'", szPath);
		break;
	case ::com::panelsw::ca::ReparsePointAction::restore:
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Restoring reparse point data to '%ls'", szPath);
		ulCreateTime.QuadPart = details.createtime();
		if (ulCreateTime.QuadPart)
		{
			ftCreateTime.dwHighDateTime = ulCreateTime.HighPart;
			ftCreateTime.dwLowDateTime = ulCreateTime.LowPart;
			pftCreateTime = &ftCreateTime;
		}
		ulWriteTime.QuadPart = details.writetime();
		if (ulWriteTime.QuadPart)
		{
			ftWriteTime.dwHighDateTime = ulWriteTime.HighPart;
			ftWriteTime.dwLowDateTime = ulWriteTime.LowPart;
			pftWriteTime = &ftWriteTime;
		}

		hr = CreateReparsePoint(szPath, pData, dwSize, pftCreateTime, pftWriteTime);
		ExitOnFailure(hr, "Failed to set reparse point in '%ls'", szPath);
		break;
	default:
		hr = E_INVALIDDATA;
		ExitOnFailure(hr, "Illegal reparse point action %i requested for '%ls'", details.action(), (LPCWSTR)details.path().c_str());
		break;
	}

LExit:
	return S_OK; // Since the whole point is to *help* Windows Installer, we never fail the install
}

/*static*/ bool CReparsePoint::IsSymbolicLinkOrMount(LPCWSTR szPath)
{
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA wfaData;
	HRESULT hr = S_OK;

	hFind = ::FindFirstFile(szPath, &wfaData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		hr = S_FALSE;
		DWORD dwErr = ::GetLastError();
		ExitOnNullWithLastError(((dwErr == ERROR_FILE_NOT_FOUND) || (dwErr == ERROR_PATH_NOT_FOUND)), hr, "Path '%ls' can't be checked for reparse point tag", szPath);
	}

LExit:
	if (hFind && (hFind != INVALID_HANDLE_VALUE))
	{
		::FindClose(hFind);
	}

	return (hr == S_OK) ? IsSymbolicLinkOrMount(&wfaData) : false;
}

/*static*/ bool CReparsePoint::IsSymbolicLinkOrMount(const WIN32_FIND_DATA* pFindFileData)
{
	return ((pFindFileData->dwFileAttributes != INVALID_FILE_ATTRIBUTES)
		&& (((pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT)
			&& ((pFindFileData->dwReserved0 == IO_REPARSE_TAG_SYMLINK) || (pFindFileData->dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT))));
}

HRESULT CReparsePoint::GetReparsePointData(LPCWSTR szPath, void** ppBuffer, DWORD* pdwSize, FILETIME* pftCreateTime, FILETIME* pftLastWriteTime)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	void* pBuffer = nullptr;
	DWORD dwReparseDataSize = 0;
	DWORD dwBufferSize = 0;
	DWORD dwFlags = FILE_FLAG_OPEN_REPARSE_POINT;
	REPARSE_DATA_COMMON_HEADER* pReparseCommon = nullptr;

	if (::PathIsDirectory(szPath))
	{
		dwFlags |= FILE_FLAG_BACKUP_SEMANTICS;
	}

	hFile = ::CreateFile(szPath, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, dwFlags, NULL);
	ExitOnInvalidHandleWithLastError(hFile, hr, "Failed to open file '%ls'", szPath);

	::GetFileTime(hFile, pftCreateTime, nullptr, pftLastWriteTime);

	dwBufferSize = MAX_PATH;
	do
	{
		ReleaseNullMem(pBuffer);
		dwReparseDataSize = 0;

		dwBufferSize *= 2;
		pBuffer = MemAlloc(dwBufferSize, TRUE);
		ExitOnNull(pBuffer, hr, E_OUTOFMEMORY, "Failed to allocate buffer");

		bRes = ::DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, nullptr, 0, pBuffer, dwBufferSize, &dwReparseDataSize, nullptr);

	} while (!bRes && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER));
	ExitOnNullWithLastError(bRes, hr, "Failed to get reparse point data of '%ls'", szPath);

	if (dwReparseDataSize)
	{
		ExitOnNull(dwReparseDataSize >= sizeof(REPARSE_DATA_COMMON_HEADER), hr, E_INVALIDDATA, "Reparse data size is lesser than header size");
		REPARSE_DATA_COMMON_HEADER* pReparseCommon = (REPARSE_DATA_COMMON_HEADER*)pBuffer;

		if (IsReparseTagMicrosoft(pReparseCommon->ReparseTag))
		{
			ExitOnNull((dwReparseDataSize >= REPARSE_DATA_BUFFER_HEADER_SIZE), hr, E_INVALIDDATA, "Reparse data size is lesser than MS header size");
		}
		else
		{
			ExitOnNull((dwReparseDataSize >= REPARSE_GUID_DATA_BUFFER_HEADER_SIZE), hr, E_INVALIDDATA, "Reparse data size is lesser than GUID header size");
		}

		*ppBuffer = pBuffer;
		*pdwSize = dwReparseDataSize;
		pBuffer = nullptr;
	}

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}
	ReleaseMem(pBuffer);

	return hr;
}

HRESULT CReparsePoint::CreateReparsePoint(LPCWSTR szPath, LPVOID pBuffer, DWORD dwSize, FILETIME* pftCreateTime, FILETIME* pftLastWriteTime)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwFlags = FILE_FLAG_OPEN_REPARSE_POINT;

	if (::PathIsDirectory(szPath))
	{
		dwFlags |= FILE_FLAG_BACKUP_SEMANTICS;
	}

	hFile = ::CreateFile(szPath, GENERIC_ALL, FILE_SHARE_READ, nullptr, OPEN_EXISTING, dwFlags, NULL);
	ExitOnInvalidHandleWithLastError(hFile, hr, "Failed to open file '%ls'", szPath);

	bRes = ::DeviceIoControl(hFile, FSCTL_SET_REPARSE_POINT, pBuffer, dwSize, nullptr, 0, nullptr, nullptr);
	ExitOnNullWithLastError(bRes, hr, "Failed to set reparse point data of '%ls'", szPath);

	// Now that the file has a reparse point, reopen it to set file times on the link
	::CloseHandle(hFile);
	hFile = ::CreateFile(szPath, GENERIC_ALL, FILE_SHARE_READ, nullptr, OPEN_EXISTING, dwFlags, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::SetFileTime(hFile, pftCreateTime, nullptr, pftLastWriteTime);
	}

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}

	return hr;
}

HRESULT CReparsePoint::DeleteReparsePoint(LPCWSTR szPath, LPVOID pBuffer, DWORD dwSize)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwFlags = FILE_FLAG_OPEN_REPARSE_POINT;
	REPARSE_DATA_COMMON_HEADER *pReparseCommon = nullptr;
	void* pCurrReparseData = nullptr;
	DWORD cCurrReparseData = 0;

	// Check if reparse data was already deleted, as may happen if multiple RemoveFile entries match on the same file
	hr = GetReparsePointData(szPath, &pCurrReparseData, &cCurrReparseData, nullptr, nullptr);
	if (hr == E_NOTAREPARSEPOINT)
	{
		hr = S_FALSE;
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Reparse data for '%ls' already removed", szPath);
		ExitFunction();
	}
	ExitOnFailure(hr, "Failed to get current reparse data for '%ls'", szPath);

	if (::PathIsDirectory(szPath))
	{
		dwFlags |= FILE_FLAG_BACKUP_SEMANTICS;
	}

	pReparseCommon = (REPARSE_DATA_COMMON_HEADER*)pBuffer;
	pReparseCommon->ReparseDataLength = 0;
	if (IsReparseTagMicrosoft(pReparseCommon->ReparseTag))
	{
		dwSize = REPARSE_DATA_BUFFER_HEADER_SIZE;
	}
	else
	{
		dwSize = REPARSE_GUID_DATA_BUFFER_HEADER_SIZE;
	}

	hFile = ::CreateFile(szPath, GENERIC_ALL, FILE_SHARE_READ, nullptr, OPEN_EXISTING, dwFlags, NULL);
	ExitOnInvalidHandleWithLastError(hFile, hr, "Failed to open file '%ls'", szPath);

	bRes = ::DeviceIoControl(hFile, FSCTL_DELETE_REPARSE_POINT, pBuffer, dwSize, nullptr, 0, nullptr, nullptr);
	ExitOnNullWithLastError(bRes, hr, "Failed to delete reparse point data of '%ls'", szPath);

LExit:
	if (hFile && (hFile != INVALID_HANDLE_VALUE))
	{
		::CloseHandle(hFile);
	}
	ReleaseMem(pCurrReparseData)

	return hr;
}

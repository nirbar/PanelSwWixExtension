#include "Unzip.h"
#include "..\CaCommon\WixString.h"
#include "..\poco\Zip\include\Poco\Zip\ZipArchive.h"
#include "..\poco\Zip\include\Poco\Zip\ZipStream.h"
#include "..\poco\Foundation\include\Poco\UnicodeConverter.h"
#include "..\poco\Foundation\include\Poco\Delegate.h"
#include "..\poco\Foundation\include\Poco\StreamCopier.h"
#include "unzipDetails.pb.h"
#include <fileutil.h>
#include <Windows.h>
#include <fstream>
#include <shlwapi.h>
#pragma comment (lib, "Iphlpapi.lib")
#pragma comment (lib, "Shlwapi.lib")
using namespace ::com::panelsw::ca;
using namespace google::protobuf;
using namespace Poco::Zip;

/*TODO
- Migrate Zip to native code
- Support rollback (copy current target-folder to temp before decompressing over it).
*/

extern "C" UINT __stdcall Unzip(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CUnzip cad;
	DWORD dwRes = 0;
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_Unzip");
	BreakExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_Unzip'. Have you authored 'PanelSw:Unzip' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `ZipFile`, `TargetFolder`, `Flags`, `Condition` FROM `PSW_Unzip`", &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString zip, folder, condition;
		int flags = 0;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)zip);
		BreakExitOnFailure(hr, "Failed to get zip file.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)folder);
		BreakExitOnFailure(hr, "Failed to get folder.");
		hr = WcaGetRecordInteger(hRecord, 3, &flags);
		BreakExitOnFailure(hr, "Failed to get flags.");
		hr = WcaGetRecordString(hRecord, 4, (LPWSTR*)condition);
		BreakExitOnFailure(hr, "Failed to get condition.");

		MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, (LPCWSTR)condition);
		switch (condRes)
		{
		case MSICONDITION::MSICONDITION_NONE:
		case MSICONDITION::MSICONDITION_TRUE:
			break;

		case MSICONDITION::MSICONDITION_FALSE:
			WcaLog(LOGMSG_STANDARD, "Skipping. Condition evaluated to false");
			continue;

		case MSICONDITION::MSICONDITION_ERROR:
			hr = E_FAIL;
			BreakExitOnFailure(hr, "Bad Condition field");
		}

		ExitOnNull(!zip.IsNullOrEmpty(), hr, E_INVALIDARG, "ZIP file path is empty");
		ExitOnNull(!folder.IsNullOrEmpty(), hr, E_INVALIDARG, "ZIP target path is empty");

		hr = cad.AddUnzip(zip, folder, (UnzipDetails_UnzipFlags)flags);
		BreakExitOnFailure(hr, "Failed scheduling zip file extraction");
	}

	hr = cad.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"UnzipExec", szCustomActionData, 0);
	BreakExitOnFailure(hr, "Failed setting property");

LExit:
	ReleaseStr(szCustomActionData);
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CUnzip::AddUnzip(LPCWSTR zipFile, LPCWSTR targetFolder, UnzipDetails_UnzipFlags flags)
{
	HRESULT hr = S_OK;
	Command *pCmd = nullptr;
	UnzipDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CUnzip", &pCmd);
	BreakExitOnFailure(hr, "Failed to add command");

	pDetails = new UnzipDetails();
	BreakExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_zipfile(zipFile, WSTR_BYTE_SIZE(zipFile));
	pDetails->set_targetfolder(targetFolder, WSTR_BYTE_SIZE(targetFolder));
	pDetails->set_flags(flags);

	pAny = pCmd->mutable_details();
	BreakExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	BreakExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CUnzip::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	bool bRes = true;
	UnzipDetails details;
	ZipArchive *archive = nullptr;
	LPCWSTR zipFileW = nullptr;
	LPCWSTR targetFolderW = nullptr;
	std::string zipFileA;
	std::string targetFolderA;
	std::istream *zipFileStream = nullptr;
	LPWSTR szDstFile = nullptr;

	bRes = details.ParseFromString(command);
	BreakExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking UnzipDetails");

	zipFileW = (LPCWSTR)details.zipfile().c_str();
	targetFolderW = (LPCWSTR)details.targetfolder().c_str();

	Poco::UnicodeConverter::toUTF8(targetFolderW, targetFolderA);
	Poco::UnicodeConverter::toUTF8(zipFileW, zipFileA);
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Extracting zip file '%s' to '%s'", zipFileA.c_str(), targetFolderA.c_str());

	zipFileStream = new std::ifstream(zipFileA, std::ios::binary);
	BreakExitOnNull(zipFileStream, hr, E_FAIL, "Failed allocating file stream");

	archive = new ZipArchive(*zipFileStream);
	BreakExitOnNull(zipFileStream, hr, E_FAIL, "Failed allocating ZIP archive");

	for (ZipArchive::FileHeaders::const_iterator it = archive->headerBegin(), endIt = archive->headerEnd(); it != endIt; ++it)
	{
		std::string file = it->second.getFileName();
		Poco::Path path(targetFolderA);
		path.append(file);
		std::string pathA = path.toString(Poco::Path::Style::PATH_WINDOWS).c_str();

		hr = StrAllocStringAnsi(&szDstFile, pathA.c_str(), 0, CP_UTF8);
		ExitOnFailure(hr, "Failed converting ANSI string to wide")

		// Create missing folders
		while (!::PathIsDirectoryA(path.parent().toString(Poco::Path::Style::PATH_WINDOWS).c_str()))
		{
			// Find first missing folder.
			Poco::Path dir = path.parent();
			while (!::PathIsDirectoryA(dir.parent().toString(Poco::Path::Style::PATH_WINDOWS).c_str()) && !::PathIsRootA(dir.parent().toString(Poco::Path::Style::PATH_WINDOWS).c_str()))
			{
				dir = dir.parent();
			}

			std::string dirA = dir.toString(Poco::Path::Style::PATH_WINDOWS);
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Creating sub-folder '%s'", dirA.c_str());
			bRes = ::CreateDirectoryA(dirA.c_str(), nullptr);
			BreakExitOnNullWithLastError((bRes || (::GetLastError() == ERROR_ALREADY_EXISTS)), hr, "Failed creating folder '%s'", dirA.c_str());
		}

		if (FileExistsEx(szDstFile, nullptr))
		{
			hr = ShouldOverwriteFile(szDstFile, details.flags());
			BreakExitOnFailure(hr, "Failed determining whether or not to overwrite file '%s'", pathA.c_str());

			if (hr == S_FALSE)
			{
				continue;
			}

			bRes = ::SetFileAttributesA(pathA.c_str(), FILE_ATTRIBUTE_NORMAL);
			BreakExitOnNullWithLastError(bRes, hr, "Failed clearing attributes of '%s'", pathA.c_str());

			bRes = ::DeleteFileA(pathA.c_str());
			BreakExitOnNullWithLastError(bRes, hr, "Failed deleting '%s'", pathA.c_str());
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Extracting '%s'", pathA.c_str());

		{ // Scope ZipInputStream to release the file
			ZipInputStream zipin(*zipFileStream, it->second);
			std::ofstream out(pathA.c_str(), std::ios::binary);

			std::streamsize bytes = Poco::StreamCopier::copyStream(zipin, out);
			BreakExitOnNull((bytes == it->second.getUncompressedSize()), hr, E_FAIL, "Error extracting file '%s' from zip '%ls': %i / %i bytes written", file.c_str(), zipFileW, bytes, it->second.getUncompressedSize());
		}

		// Set file times, ignore failures.
		if (it->second.hasExtraField())
		{
			SetFileTimes(pathA.c_str(), it->second.getExtraField());
		}
		ReleaseNullStr(szDstFile);
	}

	if ((details.flags() & UnzipDetails_UnzipFlags::UnzipDetails_UnzipFlags_delete_) == UnzipDetails_UnzipFlags::UnzipDetails_UnzipFlags_delete_)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Deleting ZIP archive '%ls'", zipFileW);
		if (::DeleteFileW(zipFileW))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Failed deleting ZIP archive '%ls'. Error %i", zipFileW, ::GetLastError());
		}
	}

LExit:
	if (archive)
	{
		delete archive;
	}
	if (zipFileStream)
	{
		delete zipFileStream;
	}
	ReleaseStr(szDstFile);
	return hr;
}

HRESULT CUnzip::SetFileTimes(LPCSTR szFilePath, const std::string &extradField)
{
	HRESULT hr = S_OK;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BOOL bRes = TRUE;
	FILETIME createTime{ 0, 0 };
	FILETIME accessTime{ 0, 0 };
	FILETIME modifyTime{ 0, 0 };
	FILETIME *pCreateTime = nullptr;
	FILETIME *pAccessTime = nullptr;
	FILETIME *pModifyTime = nullptr;

	hr = FindTimeInEntry(extradField, &createTime, &accessTime, &modifyTime);
	ExitOnFailure(hr, "Failed finding time in entry for %s", szFilePath);

	if ((createTime.dwHighDateTime != 0) || (createTime.dwLowDateTime != 0))
	{
		pCreateTime = &createTime;
	}
	if ((accessTime.dwHighDateTime != 0) || (accessTime.dwLowDateTime != 0))
	{
		pAccessTime = &accessTime;
	}
	if ((modifyTime.dwHighDateTime != 0) || (modifyTime.dwLowDateTime != 0))
	{
		pModifyTime = &modifyTime;
	}

	if (!pCreateTime && !pAccessTime && !pModifyTime)
	{
		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "No time fields found in entry for file '%s'", szFilePath);
		ExitFunction();
	}

	hFile = ::CreateFileA(szFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ExitOnNullWithLastError((hFile && (hFile != INVALID_HANDLE_VALUE)), hr, "Failed openning file '%s' to set create/access/modify times", szFilePath);

	bRes = ::SetFileTime(hFile, pCreateTime, pAccessTime, pModifyTime);
	ExitOnNullWithLastError(bRes, hr, "Failed to set create/access/modify times on '%s'", szFilePath);

LExit:
	if (hFile && (hFile != INVALID_HANDLE_VALUE))
	{
		::CloseHandle(hFile);
	}

	return hr;
}

HRESULT CUnzip::FindTimeInEntry(const std::string &extradField, FILETIME *createTime, FILETIME *accessTime, FILETIME *modifyTime)
{
	HRESULT hr = S_OK;

	for (int i = 0; i <= extradField.size() - 4; )
	{
		const short *pTag = reinterpret_cast<const short*>(extradField.data() + i);
		const short *pSize = reinterpret_cast<const short*>(extradField.data() + i + sizeof(short));
		i += (2 * sizeof(short));
		ExitOnNull((extradField.size() >= (i + (*pSize))), hr, E_INVALIDARG, "Zip entry extra-data field does not adhere to format.");

		if ((*pTag == 0xA) && (*pSize == sizeof(ExtraDataWindowsTime)))
		{
			const ExtraDataWindowsTime *winTime = reinterpret_cast<const ExtraDataWindowsTime*>(extradField.data() + i);
			if ((winTime->size == 24) && (winTime->tag == 1))
			{
				WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Detected Windows time entries for file");
				*createTime = winTime->creationTime;
				*accessTime = winTime->accessTime;
				*modifyTime = winTime->modificationTime;
			}
			break;
		}
		else if ((*pTag == 0x5455) && (*pSize >= sizeof(char)) && (*pSize <= sizeof(ExtraDataUnixTime)))
		{
			const ExtraDataUnixTime *unixTime = reinterpret_cast<const ExtraDataUnixTime*>(extradField.data() + i);
			int j = 0;

			// https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime
			ULARGE_INTEGER ll;
			unsigned int time = 0;

			// Modify
			if ((unixTime->flags & 1) && (*pSize >= (sizeof(ExtraDataUnixTime::flags) + ((j + 1) * sizeof(ExtraDataUnixTime::times[0])))))
			{
				WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Detected Unix modification time entry for file");
				time = unixTime->times[j++];
				ll.QuadPart = Int32x32To64(time, 10000000) + 116444736000000000;
				modifyTime->dwHighDateTime = ll.HighPart;
				modifyTime->dwLowDateTime = ll.LowPart;
			}
			if ((unixTime->flags & 2) && (*pSize >= (sizeof(ExtraDataUnixTime::flags) + ((j + 1) * sizeof(ExtraDataUnixTime::times[0])))))
			{
				WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Detected Unix access time entry for file");
				time = unixTime->times[j++];
				ll.QuadPart = Int32x32To64(time, 10000000) + 116444736000000000;
				accessTime->dwHighDateTime = ll.HighPart;
				accessTime->dwLowDateTime = ll.LowPart;
			}
			if ((unixTime->flags & 4) && (*pSize >= (sizeof(ExtraDataUnixTime::flags) + ((j + 1) * sizeof(ExtraDataUnixTime::times[0])))))
			{
				WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Detected Unix creation time entry for file");
				time = unixTime->times[j++];
				ll.QuadPart = Int32x32To64(time, 10000000) + 116444736000000000;
				createTime->dwHighDateTime = ll.HighPart;
				createTime->dwLowDateTime = ll.LowPart;
			}
			break;
		}
		
		i += (*pSize);
	}

LExit:

	return hr;
}

HRESULT CUnzip::ShouldOverwriteFile(LPCWSTR szFile, UnzipDetails_UnzipFlags flags)
{
	HRESULT hr = S_OK;
	bool bRes = true;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	FILETIME ftCreate;
	FILETIME ftModify;
	ULARGE_INTEGER ulCreate;
	ULARGE_INTEGER ulModify;

	switch (flags & 0x3)
	{
	case UnzipDetails::UnzipFlags::UnzipDetails_UnzipFlags_never:
	default:
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will not overwrite '%s' due to NeverOverwrite flag", szFile);
		hr = S_FALSE;
		break;

	case UnzipDetails::UnzipFlags::UnzipDetails_UnzipFlags_always:
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will overwrite '%s' due to AlwaysOverwrite flag", szFile);
		hr = S_OK;
		break;

	case UnzipDetails::UnzipFlags::UnzipDetails_UnzipFlags_unmodified:
		hFile = ::CreateFileW(szFile, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		BreakExitOnNullWithLastError(((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)), hr, "Failed opening file '%s' to read times", szFile);

		bRes = ::GetFileTime(hFile, &ftCreate, nullptr, &ftModify);
		BreakExitOnNullWithLastError(bRes, hr, "Failed getting file times for '%s'", szFile);

		ulCreate.HighPart = ftCreate.dwHighDateTime;
		ulCreate.LowPart = ftCreate.dwLowDateTime;

		ulModify.HighPart = ftModify.dwHighDateTime;
		ulModify.LowPart = ftModify.dwLowDateTime;

		if (ulModify.QuadPart > ulCreate.QuadPart)
		{
			hr = S_FALSE;
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will not overwrite '%s' due to OverwriteUnmodified flag- file has been modified since created", szFile);
		}
		else
		{
			hr = S_OK;
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will overwrite '%s' due to OverwriteUnmodified flag- file isn't modified", szFile);
		}

		break;
	}

LExit:
	if (hFile && (hFile != INVALID_HANDLE_VALUE))
	{
		::CloseHandle(hFile);
	}

	return hr;
}
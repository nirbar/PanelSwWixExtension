#include "Unzip.h"
#include "FileOperations.h"
#include "..\CaCommon\WixString.h"
#include "..\poco\Zip\include\Poco\Zip\ZipArchive.h"
#include "..\poco\Zip\include\Poco\Zip\ZipStream.h"
#include "..\poco\Zip\include\Poco\Zip\Compress.h"
#include "..\poco\Foundation\include\Poco\UnicodeConverter.h"
#include "..\poco\Foundation\include\Poco\Delegate.h"
#include "..\poco\Foundation\include\Poco\StreamCopier.h"
#include "unzipDetails.pb.h"
#include "zipDetails.pb.h"
#include <memutil.h>
#include <pathutil.h>
#include <fileutil.h>
#include <Windows.h>
#include <fstream>
#include <shlwapi.h>
#pragma comment (lib, "Iphlpapi.lib")
#pragma comment (lib, "Shlwapi.lib")
#pragma comment (lib, "PocoZipmt.lib")
using namespace ::com::panelsw::ca;
using namespace google::protobuf;
using namespace Poco::Zip;

/*TODO
- Support rollback (copy current target-folder to temp before decompressing over it).
*/

extern "C" UINT __stdcall Unzip(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CUnzip rlbkCad(false);
	CUnzip cad(false);
	DWORD dwRes = 0;
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_Unzip");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_Unzip'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_Unzip'. Have you authored 'PanelSw:Unzip' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `ZipFile`, `TargetFolder`, `Flags`, `Condition` FROM `PSW_Unzip`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString zip, folder, condition;
		int flags = 0;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)zip);
		ExitOnFailure(hr, "Failed to get zip file.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)folder);
		ExitOnFailure(hr, "Failed to get folder.");
		hr = WcaGetRecordInteger(hRecord, 3, &flags);
		ExitOnFailure(hr, "Failed to get flags.");
		hr = WcaGetRecordString(hRecord, 4, (LPWSTR*)condition);
		ExitOnFailure(hr, "Failed to get condition.");

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
			ExitOnFailure(hr, "Bad Condition field");
		}

		ExitOnNull(!zip.IsNullOrEmpty(), hr, E_INVALIDARG, "ZIP file path is empty");
		ExitOnNull(!folder.IsNullOrEmpty(), hr, E_INVALIDARG, "ZIP target path is empty");

		if ((flags & UnzipDetails_UnzipFlags::UnzipDetails_UnzipFlags_onRollback) == UnzipDetails_UnzipFlags::UnzipDetails_UnzipFlags_onRollback)
		{
			hr = rlbkCad.AddUnzip(zip, folder, (UnzipDetails_UnzipFlags)flags);
			ExitOnFailure(hr, "Failed scheduling zip file extraction");
		}
		else
		{
			hr = cad.AddUnzip(zip, folder, (UnzipDetails_UnzipFlags)flags);
			ExitOnFailure(hr, "Failed scheduling zip file extraction");
		}
	}
	hr = S_OK;

	if (rlbkCad.HasActions())
	{
		hr = rlbkCad.GetCustomActionData(&szCustomActionData);
		ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
		hr = WcaDoDeferredAction(L"UnzipRollback", szCustomActionData, 0);
		ExitOnFailure(hr, "Failed setting property");
		ReleaseNullStr(szCustomActionData);
	}

	if (cad.HasActions())
	{
		hr = cad.GetCustomActionData(&szCustomActionData);
		ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
		hr = WcaDoDeferredAction(L"UnzipExec", szCustomActionData, 0);
		ExitOnFailure(hr, "Failed setting property");
	}

LExit:
	ReleaseStr(szCustomActionData);
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

extern "C" UINT __stdcall ZipFileSched(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CUnzip cad(true);
	DWORD dwRes = 0;
	LPWSTR szCustomActionData = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_DeletePath exists.
	hr = WcaTableExists(L"PSW_ZipFile");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_ZipFile'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_ZipFile'. Have you authored 'PanelSw:ZipFile' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `ZipFile`, `CompressFolder`, `FilePattern`, `Recursive`, `Condition` FROM `PSW_ZipFile`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString zip, folder, pattern, condition;
		int recursive = 0;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)zip);
		ExitOnFailure(hr, "Failed to get ZipFile.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)folder);
		ExitOnFailure(hr, "Failed to get CompressFolder.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)pattern);
		ExitOnFailure(hr, "Failed to get FilePattern.");
		hr = WcaGetRecordInteger(hRecord, 4, &recursive);
		ExitOnFailure(hr, "Failed to get Recursive.");
		hr = WcaGetRecordString(hRecord, 5, (LPWSTR*)condition);
		ExitOnFailure(hr, "Failed to get Condition.");

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
			ExitOnFailure(hr, "Bad Condition field");
		}

		ExitOnNull(!zip.IsNullOrEmpty(), hr, E_INVALIDARG, "ZIP file path is empty");
		ExitOnNull(!folder.IsNullOrEmpty(), hr, E_INVALIDARG, "ZIP source folder is empty");

		if (folder.RFind(L'\\') != (folder.StrLen() - 1))
		{
			hr = folder.AppnedFormat(L"\\");
			ExitOnFailure(hr, "Failed appending backslash");
		}

		hr = cad.AddZip(zip, folder, pattern, recursive);
		ExitOnFailure(hr, "Failed scheduling zip file compression");
	}

	hr = cad.GetCustomActionData(&szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaDoDeferredAction(L"ZipFileExec", szCustomActionData, 0);
	ExitOnFailure(hr, "Failed setting property");

LExit:
	ReleaseStr(szCustomActionData);
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CUnzip::AddUnzip(LPCWSTR zipFile, LPCWSTR targetFolder, UnzipDetails_UnzipFlags flags)
{
	HRESULT hr = S_OK;
	Command* pCmd = nullptr;
	UnzipDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	ExitOnNull(!isZip_, hr, E_INVALIDSTATE, "Using Zip to unzip");

	hr = AddCommand("CUnzip", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new UnzipDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_zipfile(zipFile, WSTR_BYTE_SIZE(zipFile));
	pDetails->set_targetfolder(targetFolder, WSTR_BYTE_SIZE(targetFolder));
	pDetails->set_flags(flags);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CUnzip::AddZip(LPCWSTR zipFile, LPCWSTR sourceFolder, LPCWSTR szPattern, bool bRecursive)
{
	HRESULT hr = S_OK;
	Command* pCmd = nullptr;
	ZipDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	ExitOnNull(isZip_, hr, E_INVALIDSTATE, "Using Unzip to zip");

	hr = AddCommand("CZip", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new ZipDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_zipfile(zipFile, WSTR_BYTE_SIZE(zipFile));
	pDetails->set_srcfolder(sourceFolder, WSTR_BYTE_SIZE(sourceFolder));
	pDetails->set_pattern(szPattern, WSTR_BYTE_SIZE(szPattern));
	pDetails->set_recursive(bRecursive);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}


HRESULT CUnzip::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	UnzipDetails unzipDetails;
	ZipDetails zipDetails;

	if (!isZip_ && unzipDetails.ParseFromString(command))
	{
		hr = ExecuteOneUnzip(&unzipDetails);
		ExitOnFailure(hr, "Failed executing UnzipDetails");
	}
	else if (isZip_ && zipDetails.ParseFromString(command))
	{
		hr = ExecuteOneZip(&zipDetails);
		ExitOnFailure(hr, "Failed executing ZipDetails");
	}
	else
	{
		hr = E_INVALIDARG;
		ExitOnFailure(hr, "Failed unpacking command");
	}

LExit:
	return hr;
}

HRESULT CUnzip::ExecuteOneZip(::com::panelsw::ca::ZipDetails* pDetails)
{
	HRESULT hr = S_OK;
	bool bRes = true;
	LPCWSTR zipFileW = nullptr;
	LPCWSTR srcFolderW = nullptr;
	LPCWSTR szPattern = nullptr;
	LPWSTR* pszFiles = nullptr;
	LPSTR szEntryName = nullptr;
	UINT cFiles = 0;
	std::string zipFileA;
	std::ostream* zipFileStream = nullptr;
	Compress* pZip = nullptr;
	Poco::Path file, fileName;
	PMSIHANDLE hActionData;

	zipFileW = (LPCWSTR)(LPVOID)pDetails->zipfile().data();
	srcFolderW = (LPCWSTR)(LPVOID)pDetails->srcfolder().data();
	szPattern = (LPCWSTR)(LPVOID)pDetails->pattern().data();

	try
	{
		hr = CFileOperations::ListFiles(srcFolderW, szPattern, pDetails->recursive(), &pszFiles, &cFiles);
		ExitOnFailure(hr, "Failed listing files in '%ls'", srcFolderW);
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Compressing %u files matching '%ls%ls' to zip file '%ls'", cFiles, srcFolderW, szPattern, zipFileW);
		
		// ActionData: "Compressing from [1] to [2]"
		hActionData = ::MsiCreateRecord(2);
		if (hActionData 
			&& SUCCEEDED(WcaSetRecordString(hActionData, 1, srcFolderW))
			&& SUCCEEDED(WcaSetRecordString(hActionData, 2, zipFileW)))
		{
			WcaProcessMessage(INSTALLMESSAGE::INSTALLMESSAGE_ACTIONDATA, hActionData);
		}

		if (cFiles > 0)
		{
			Poco::UnicodeConverter::toUTF8(zipFileW, zipFileA);

			zipFileStream = new std::ofstream(zipFileA, std::ios::binary);
			ExitOnNull(zipFileStream, hr, E_OUTOFMEMORY, "Failed creating zip file '%ls'", zipFileW);

			pZip = new Compress(*zipFileStream, true);
			ExitOnNull(zipFileStream, hr, E_OUTOFMEMORY, "Failed creating zip archive for '%ls'", zipFileW);

			for (UINT i = 0; i < cFiles; ++i)
			{
				hr = StrAnsiAllocString(&szEntryName, pszFiles[i] + ::wcslen(srcFolderW), 0, CP_UTF8);
				ExitOnFailure(hr, "Failed allocating string");

				while (LPSTR szBackslah = ::strchr(szEntryName, '\\'))
				{
					*szBackslah = '/';
				}

				fileName.setFileName(szEntryName);
				ReleaseNullMem(szEntryName);

				hr = StrAnsiAllocString(&szEntryName, pszFiles[i], 0, CP_UTF8);
				ExitOnFailure(hr, "Failed allocating string");

				file.setFileName(szEntryName);
				ReleaseNullMem(szEntryName);

				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Adding '%s' as '%s' to zip file '%s'", file.getFileName().c_str(), fileName.getFileName().c_str(), zipFileA.c_str());
				pZip->addFile(file, fileName);

				//TODO Set extra time fields, once POCO support getting the entry of the added file.
			}

			pZip->close();
			zipFileStream->flush();
		}
	}
	catch (Poco::Exception ex)
	{
		hr = HRESULT_FROM_WIN32(ex.code());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
		ExitOnFailure(hr, "Failed zipping file. %s", ex.displayText().c_str());
	}
	catch (std::exception ex)
	{
		hr = E_FAIL;
		ExitOnFailure(hr, "Failed zipping file. %s", ex.what());
	}
	catch (...)
	{
		hr = E_FAIL;
		ExitOnFailure(hr, "Failed zipping file");
	}

LExit:
	ReleaseNullMem(szEntryName);
	if (cFiles && pszFiles)
	{
		StrArrayFree(pszFiles, cFiles);
	}
	if (pZip)
	{
		delete pZip;
	}
	if (zipFileStream)
	{
		delete zipFileStream;
	}

	return hr;
}

HRESULT CUnzip::ExecuteOneUnzip(::com::panelsw::ca::UnzipDetails* pDetails)
{
	HRESULT hr = S_OK;
	bool bRes = true;
	ZipArchive* archive = nullptr;
	LPCWSTR zipFileW = nullptr;
	LPCWSTR targetFolderW = nullptr;
	std::string zipFileA;
	std::string targetFolderA;
	std::istream* zipFileStream = nullptr;
	LPSTR szSrcFileA = nullptr;
	LPWSTR szSrcFile = nullptr;
	LPWSTR szDstFile = nullptr;
	PMSIHANDLE hActionData;

	zipFileW = (LPCWSTR)(LPVOID)pDetails->zipfile().data();
	targetFolderW = (LPCWSTR)(LPVOID)pDetails->targetfolder().data();

	// ActionData: "Extracting from [1] to [2]"
	hActionData = ::MsiCreateRecord(2);
	if (hActionData
		&& SUCCEEDED(WcaSetRecordString(hActionData, 1, zipFileW))
		&& SUCCEEDED(WcaSetRecordString(hActionData, 2, targetFolderW)))
	{
		WcaProcessMessage(INSTALLMESSAGE::INSTALLMESSAGE_ACTIONDATA, hActionData);
	}

	try
	{
		Poco::UnicodeConverter::toUTF8(targetFolderW, targetFolderA);
		Poco::UnicodeConverter::toUTF8(zipFileW, zipFileA);
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Extracting zip file '%s' to '%s'", zipFileA.c_str(), targetFolderA.c_str());

		zipFileStream = new std::ifstream(zipFileA, std::ios::binary);
		ExitOnNull(zipFileStream, hr, E_FAIL, "Failed allocating file stream");

		archive = new ZipArchive(*zipFileStream);
		ExitOnNull(zipFileStream, hr, E_FAIL, "Failed allocating ZIP archive");

		for (ZipArchive::FileHeaders::const_iterator it = archive->headerBegin(), endIt = archive->headerEnd(); it != endIt; ++it)
		{
			std::string file = it->second.getFileName();
			if ((pDetails->flags() & UnzipDetails::UnzipFlags::UnzipDetails_UnzipFlags_createRoot) == 0)
			{
				size_t i1 = file.find_first_of('/');
				size_t i2 = file.find_first_of('\\');
				if ((i1 > 0) && (i1 < file.length() - 1) && ((i1 < i2) || (i2 <= 0)))
				{
					file = file.substr(i1 + 1);
				}
				else if ((i2 > 0) && (i2 < file.length() - 1) && ((i2 < i1) || (i1 <= 0)))
				{
					file = file.substr(i2 + 1);
				}
				else
				{
					WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File '%s' is directly in ZIP root so 'SkipRoot' does not apply to it", file.c_str());
				}
			}

			Poco::Path path(targetFolderA);
			path.append(file);
			std::string pathA = path.toString(Poco::Path::Style::PATH_WINDOWS).c_str();

			hr = StrAllocStringAnsi(&szDstFile, pathA.c_str(), 0, CP_UTF8);
			ExitOnFailure(hr, "Failed converting ANSI string to wide");

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
				ExitOnNullWithLastError((bRes || (::GetLastError() == ERROR_ALREADY_EXISTS)), hr, "Failed creating folder '%s'", dirA.c_str());
			}

			if (it->second.isDirectory())
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Creating folder '%s'", pathA.c_str());
				::CreateDirectoryA(pathA.c_str(), nullptr); // Ignore failure to create empty folders. Folders with files will fail when extracting the files.
				continue;
			}

			if (FileExistsEx(szDstFile, nullptr))
			{
				hr = ShouldOverwriteFile(szDstFile, pDetails->flags());
				ExitOnFailure(hr, "Failed determining whether or not to overwrite file '%s'", pathA.c_str());

				if (hr == S_FALSE)
				{
					continue;
				}

				bRes = ::SetFileAttributesA(pathA.c_str(), FILE_ATTRIBUTE_NORMAL);
				ExitOnNullWithLastError(bRes, hr, "Failed clearing attributes of '%s'", pathA.c_str());

				bRes = ::DeleteFileA(pathA.c_str());
				if (!bRes && ((::GetLastError() == ERROR_ACCESS_DENIED) || (::GetLastError() == ERROR_SHARING_VIOLATION) || (::GetLastError() == ERROR_LOCK_VIOLATION)))
				{
					WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Extraction of '%s' requires reboot", pathA.c_str());
					bRes = true;
					
					hr = CFileOperations::MakeTemporaryName(szDstFile, L"UZP%05i.tmp", false, &szSrcFile);
					ExitOnFailure(hr, "Failed getting temporary path for '%hs'", pathA.c_str());

					hr = StrAnsiAllocString(&szSrcFileA, szSrcFile, 0, CP_UTF8);
					ExitOnFailure(hr, "Failed copying UTF-8 string");

					pathA = szSrcFileA;

					bRes = ::MoveFileExW(szSrcFile, szDstFile, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
					ExitOnNullWithLastError(bRes, hr, "Failed deferring file copy to after reboot");

					hr = WcaDeferredActionRequiresReboot();
					ExitOnFailure(hr, "Failed requiring reboot");
				}
				ExitOnNullWithLastError(bRes, hr, "Failed deleting '%s'", pathA.c_str());
			}

			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Extracting '%s'", pathA.c_str());

			{ // Scope ZipInputStream to release the file
				ZipInputStream zipin(*zipFileStream, it->second);
				std::ofstream out(pathA.c_str(), std::ios::binary);

				std::streamsize bytes = Poco::StreamCopier::copyStream(zipin, out);
				ExitOnNull((bytes == it->second.getUncompressedSize()), hr, E_FAIL, "Error extracting file '%s' from zip '%ls': %i / %i bytes written", file.c_str(), zipFileW, bytes, it->second.getUncompressedSize());
			}

			// Set file times, ignore failures.
			if (it->second.hasExtraField())
			{
				SetFileTimes(pathA.c_str(), it->second.getExtraField());
			}
			ReleaseNullStr(szDstFile);
			ReleaseNullStr(szSrcFile);
			ReleaseNullMem(szSrcFileA);
		}

		// Release stream so we can delete the zip file
		delete archive;
		archive = nullptr;

		delete zipFileStream;
		zipFileStream = nullptr;

		if ((pDetails->flags() & UnzipDetails_UnzipFlags::UnzipDetails_UnzipFlags_delete_) == UnzipDetails_UnzipFlags::UnzipDetails_UnzipFlags_delete_)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Deleting ZIP archive '%ls'", zipFileW);
			FileEnsureDelete(zipFileW);// Ignoring result
		}
	}
	catch (Poco::Exception ex)
	{
		hr = HRESULT_FROM_WIN32(ex.code());
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;
		}
		ExitOnFailure(hr, "Failed unzipping file. %s", ex.displayText().c_str());
	}
	catch (std::exception ex)
	{
		hr = E_FAIL;
		ExitOnFailure(hr, "Failed unzipping file. %s", ex.what());
	}
	catch (...)
	{
		hr = E_FAIL;
		ExitOnFailure(hr, "Failed unzipping file");
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
	ReleaseStr(szSrcFile);
	ReleaseStr(szDstFile);
	ReleaseNullMem(szSrcFileA);
	
	return hr;
}

HRESULT CUnzip::SetFileTimes(LPCSTR szFilePath, const std::string& extradField)
{
	HRESULT hr = S_OK;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BOOL bRes = TRUE;
	FILETIME createTime{ 0, 0 };
	FILETIME accessTime{ 0, 0 };
	FILETIME modifyTime{ 0, 0 };
	FILETIME* pCreateTime = nullptr;
	FILETIME* pAccessTime = nullptr;
	FILETIME* pModifyTime = nullptr;

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

HRESULT CUnzip::FindTimeInEntry(const std::string& extradField, FILETIME* createTime, FILETIME* accessTime, FILETIME* modifyTime)
{
	HRESULT hr = S_OK;

	for (int i = 0; i <= extradField.size() - 4; )
	{
		const short* pTag = reinterpret_cast<const short*>(extradField.data() + i);
		const short* pSize = reinterpret_cast<const short*>(extradField.data() + i + sizeof(short));
		i += (2 * sizeof(short));
		ExitOnNull((extradField.size() >= (i + (*pSize))), hr, E_INVALIDARG, "Zip entry extra-data field does not adhere to format.");

		if ((*pTag == 0xA) && (*pSize == sizeof(ExtraDataWindowsTime)))
		{
			const ExtraDataWindowsTime* winTime = reinterpret_cast<const ExtraDataWindowsTime*>(extradField.data() + i);
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
			const ExtraDataUnixTime* unixTime = reinterpret_cast<const ExtraDataUnixTime*>(extradField.data() + i);
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
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will not overwrite '%ls' due to NeverOverwrite flag", szFile);
		hr = S_FALSE;
		break;

	case UnzipDetails::UnzipFlags::UnzipDetails_UnzipFlags_always:
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will overwrite '%ls' due to AlwaysOverwrite flag", szFile);
		hr = S_OK;
		break;

	case UnzipDetails::UnzipFlags::UnzipDetails_UnzipFlags_unmodified:
		hFile = ::CreateFileW(szFile, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		ExitOnNullWithLastError(((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)), hr, "Failed opening file '%ls' to read times", szFile);

		bRes = ::GetFileTime(hFile, &ftCreate, nullptr, &ftModify);
		ExitOnNullWithLastError(bRes, hr, "Failed getting file times for '%ls'", szFile);

		ulCreate.HighPart = ftCreate.dwHighDateTime;
		ulCreate.LowPart = ftCreate.dwLowDateTime;

		ulModify.HighPart = ftModify.dwHighDateTime;
		ulModify.LowPart = ftModify.dwLowDateTime;

		if (ulModify.QuadPart > ulCreate.QuadPart)
		{
			hr = S_FALSE;
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will not overwrite '%ls' due to OverwriteUnmodified flag- file has been modified since created", szFile);
		}
		else
		{
			hr = S_OK;
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will overwrite '%ls' due to OverwriteUnmodified flag- file isn't modified", szFile);
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
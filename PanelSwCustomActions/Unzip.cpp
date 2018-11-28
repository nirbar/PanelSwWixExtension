#include "Unzip.h"
#include "..\CaCommon\WixString.h"
#include "..\poco\Zip\include\Poco\Zip\ZipArchive.h"
#include "..\poco\Zip\include\Poco\Zip\ZipStream.h"
#include "..\poco\Foundation\include\Poco\UnicodeConverter.h"
#include "..\poco\Foundation\include\Poco\Delegate.h"
#include "..\poco\Foundation\include\Poco\StreamCopier.h"
#include "unzipDetails.pb.h"
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

enum UnzipFlags
{
	None = 0,
	Overwrite = 1
};

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
		hr = WcaGetRecordFormattedInteger(hRecord, 3, &flags);
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

		hr = cad.AddUnzip(zip, folder, flags & UnzipFlags::Overwrite);
		BreakExitOnFailure(hr, "Failed scheduling zip file extraction");
	}

	hr = cad.GetCustomActionData(&szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaSetProperty(L"UnzipExec", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting property");

LExit:
	ReleaseStr(szCustomActionData);
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CUnzip::AddUnzip(LPCWSTR zipFile, LPCWSTR targetFolder, bool overwrite)
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
	pDetails->set_overwrite(overwrite);

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

		if (details.overwrite() && ::PathFileExistsA(pathA.c_str()))
		{
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Deleting '%s'", pathA.c_str());

			bRes = ::SetFileAttributesA(pathA.c_str(), FILE_ATTRIBUTE_NORMAL);
			BreakExitOnNullWithLastError(bRes, hr, "Failed clearing attributes of '%s'", pathA.c_str());

			bRes = ::DeleteFileA(pathA.c_str());
			BreakExitOnNullWithLastError(bRes, hr, "Failed deleting '%s'", pathA.c_str());
		}

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

		if (!::PathFileExistsA(pathA.c_str()))
		{
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Extracting '%s'", pathA.c_str());

			{ // Scope ZipInputStream to release the file
				ZipInputStream zipin(*zipFileStream, it->second);
				std::ofstream out(pathA.c_str(), std::ios::binary);

				std::streamsize bytes = Poco::StreamCopier::copyStream(zipin, out);
				BreakExitOnNull((bytes == it->second.getUncompressedSize()), hr, E_FAIL, "Error extracting file '%s' from zip '%ls': %i / %i bytes written", file.c_str(), zipFileW, bytes, it->second.getUncompressedSize());
			}

			// Set file times, if created by panelsw:Zip custom action
			if (it->second.hasExtraField())
			{
				const std::string times = it->second.getExtraField();
				if (times.size() == (3 * sizeof(FILETIME)))
				{
					HANDLE hFile = INVALID_HANDLE_VALUE;
					const FILETIME *fileTimes = (const FILETIME*)times.c_str(); // Create, access, write

					hFile = ::CreateFileA(pathA.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if ((hFile == NULL) || (hFile == INVALID_HANDLE_VALUE))
					{
						WcaLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed openning file '%s' to set create/access/modify times", pathA.c_str());
						continue;
					}

					bRes = ::SetFileTime(hFile, fileTimes, fileTimes + 1, fileTimes + 2);
					if (!bRes)
					{
						WcaLogError(HRESULT_FROM_WIN32(::GetLastError()), "Failed to set create/access/modify times on '%s'", pathA.c_str());
					}

					::CloseHandle(hFile);
				}
			}
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
	return hr;
}

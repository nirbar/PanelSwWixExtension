#include "Unzip.h"
#include "..\CaCommon\WixString.h"
#include "..\poco\Zip\include\Poco\Zip\Decompress.h"
#include "..\poco\Foundation\include\Poco\UnicodeConverter.h"
#include "..\poco\Foundation\include\Poco\Delegate.h"
#include "unzipDetails.pb.h"
#include <fstream>
#pragma comment (lib, "Iphlpapi.lib")
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

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
	hr = WcaOpenExecuteView(L"SELECT `ZipFile`, `TargetFolder`, `Condition` FROM `PSW_Unzip`", &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString zip, folder, condition;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)zip);
		BreakExitOnFailure(hr, "Failed to get zip file.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)folder);
		BreakExitOnFailure(hr, "Failed to get folder.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)condition);
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

		hr = cad.AddUnzip(zip, folder);
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

HRESULT CUnzip::AddUnzip(LPCWSTR zipFile, LPCWSTR targetFolder)
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
	Poco::Zip::Decompress *zip = nullptr;
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

	zip = new Poco::Zip::Decompress(*zipFileStream, targetFolderA);
	zip->EError += Poco::Delegate<CUnzip, std::pair<const Poco::Zip::ZipLocalFileHeader, const std::string>>(this, &CUnzip::OnDecompressError);

	zip->decompressAllFiles();
	BreakExitOnNull(!hadErrors_, hr, E_FAIL, "Failed decompressing '%ls' to '%ls'", zipFileW, targetFolderW);

LExit:
	if (zip)
	{
		delete zip;
	}
	if (zipFileStream)
	{
		delete zipFileStream;
	}
	return hr;
}

void CUnzip::OnDecompressError(const void* pSender, std::pair<const Poco::Zip::ZipLocalFileHeader, const std::string>& info)
{
	hadErrors_ = true;
	WcaLogError(E_FAIL, "Failed decompressing ZIP file '%s': %s", info.first.getFileName().c_str(), info.second.c_str());
}
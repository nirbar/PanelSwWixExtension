#include "XslTransform.h"
#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include "XslTransformDetails.pb.h"
#include <wcautil.h>
#include <memutil.h>
#include <fileutil.h>
#include "google\protobuf\any.h"
#include <objbase.h>
#include <msxml6.h>
#include <atlbase.h>
#pragma comment( lib, "msxml6.lib")
using namespace com::panelsw::ca;
using namespace google::protobuf;

static HRESULT ReadBinary(LPCWSTR szBinaryKey, LPCWSTR szQueryId, CWixString *pszQuery);
static HRESULT ReplaceStrings(CWixString * pszXsl, LPCWSTR szXslId);

extern "C" UINT __stdcall XslTransform(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	DWORD dwRes = 0;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPWSTR szCustomActionData = nullptr;
	CXslTransform deferredCA;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure tables exist.
	hr = WcaTableExists(L"PSW_XslTransform");
	ExitOnFailure((hr == S_OK), "Table does not exist 'PSW_XslTransform'. Have you authored 'PanelSw:XslTransform' entries in WiX code?");
    hr = WcaTableExists(L"PSW_XslTransform_Replacements");
    ExitOnFailure((hr == S_OK), "Table does not exist 'PSW_XslTransform_Replacements'. Have you authored 'PanelSw:XslTransform' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `PSW_XslTransform`.`Id`, `File`.`Component_`, `PSW_XslTransform`.`File_`, `PSW_XslTransform`.`XslBinary_` FROM `PSW_XslTransform`, `File` ORDER BY `PSW_XslTransform`.`Order` WHERE `PSW_XslTransform`.`File_` = `File`.`File`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{        
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
        PMSIHANDLE hSubView;
        PMSIHANDLE hSubRecord;
		CWixString szId, szComponent, szFileId, szXslBinaryId;
		CWixString szXsl, szFileFmt, szFilePath;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component_.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)szFileId);
		ExitOnFailure(hr, "Failed to get File_.");
		hr = WcaGetRecordString(hRecord, 4, (LPWSTR*)szXslBinaryId);
		ExitOnFailure(hr, "Failed to get XslBinary_.");

		ExitOnNull(!szXslBinaryId.IsNullOrEmpty(), hr, E_INVALIDARG, "Binary key is empty");
		ExitOnNull(!szFileId.IsNullOrEmpty(), hr, E_INVALIDARG, "File key is empty");
		ExitOnNull(!szComponent.IsNullOrEmpty(), hr, E_INVALIDARG, "Component is empty");

		hr = szFileFmt.Format(L"[#%s]", (LPCWSTR)szFileId);
		ExitOnFailure(hr, "Failed formatting string");

		hr = szFilePath.MsiFormat((LPCWSTR)szFileFmt);
		ExitOnFailure(hr, "Failed formatting string");

		if (szFilePath.IsNullOrEmpty())
		{
			WcaLog(LOGMSG_STANDARD, "Skipping execution of XSLT '%ls' since component '%ls' is not being installed or reinstalled", (LPCWSTR)szXslBinaryId, (LPCWSTR)szComponent);
			continue;
		}

		hr = ReadBinary(szXslBinaryId, szId, &szXsl);
		ExitOnFailure(hr, "Failed reading XSL transform");

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
		case WCA_TODO::WCA_TODO_REINSTALL:
			hr = deferredCA.AddExec(szFilePath, szXsl);
			ExitOnFailure(hr, "Failed scheduling '%ls' SQL script", (LPCWSTR)szXslBinaryId);
			break;

		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGMSG_STANDARD, "Skipping execution of XSLT '%ls' since component '%ls' is not being installed or reinstalled", (LPCWSTR)szXslBinaryId, (LPCWSTR)szComponent);
			break;
		}
	}
	hr = S_OK;

	// Deferred action
	if (deferredCA.HasActions())
	{
		hr = deferredCA.GetCustomActionData(&szCustomActionData);
		ExitOnFailure(hr, "Failed getting custom action data for rollback.");
		hr = WcaDoDeferredAction(L"PSW_XslTransformExec", szCustomActionData, 1);
		ExitOnFailure(hr, "Failed setting rollback action data.");
		ReleaseNullStr(szCustomActionData);
	}

LExit:
	ReleaseMem(szCustomActionData);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT ReadBinary(LPCWSTR szBinaryKey, LPCWSTR szQueryId, CWixString *pszQuery)
{
	HRESULT hr = S_OK;
	CWixString szMsiQuery;
	std::map<std::string, std::string> replacements;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	BYTE* pbData = nullptr;
	DWORD cbData = 0;
	FileRegexDetails::FileEncoding encoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None;

	hr = szMsiQuery.Format(L"SELECT `Data` FROM `Binary` WHERE `Name`='%s'", szBinaryKey);
	ExitOnFailure(hr, "Failed to format string");

	hr = WcaOpenExecuteView((LPCWSTR)szMsiQuery, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szMsiQuery);

	hr = WcaFetchSingleRecord(hView, &hRecord);
	ExitOnFailure(hr, "Failed to fetch binary record.");

	hr = WcaGetRecordStream(hRecord, 1, &pbData, &cbData);
	ExitOnFailure(hr, "Failed to ready Binary.Data for certificate.");

	// Ensure null-termination. scoped for local use of pbData1
	{
		cbData += 2;
		BYTE* pbData1 = (LPBYTE)MemReAlloc(pbData, cbData, TRUE);
		ExitOnNull(pbData1, hr, E_FAIL, "Failed reallocating memory");
		pbData = pbData1;
	}

	encoding = CFileOperations::DetectEncoding(pbData, cbData);
	if (encoding == FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte)
	{
		hr = pszQuery->Format(L"%hs", pbData);
		ExitOnFailure(hr, "Failed to copy SQL script to string. Is binary file '%ls' multibyte-encoded?", szBinaryKey);
	}
	else
	{
		hr = pszQuery->Copy((LPCWSTR)pbData);
		ExitOnFailure(hr, "Failed to copy SQL script to string. Is binary file '%ls' unicode-encoded?", szBinaryKey);
	}

	hr = ReplaceStrings(pszQuery, szQueryId);
	ExitOnFailure(hr, "Failed to replacing strings in SQL script '%ls'", szBinaryKey);

LExit:
	ReleaseMem(pbData);

	return hr;
}

static HRESULT ReplaceStrings(CWixString *pszXsl, LPCWSTR szXslId)
{
	HRESULT hr = S_OK;
	CWixString szMsiQuery;
	std::map<std::string, std::string> replacements;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = szMsiQuery.Format(L"SELECT `Text`, `Replacement` FROM `PSW_XslTransform_Replacements` WHERE `XslTransform_`='%s' ORDER BY `Order`", szXslId);
	ExitOnFailure(hr, "Failed to format string");

	hr = WcaOpenExecuteView((LPCWSTR)szMsiQuery, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szMsiQuery);

	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		CWixString szText, szReplacement;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)szText);
		ExitOnFailure(hr, "Failed to get Text.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)szReplacement);
		ExitOnFailure(hr, "Failed to get Replacement.");

		if (!szText.IsNullOrEmpty())
		{
			hr = pszXsl->ReplaceAll(szText, szReplacement);
			ExitOnFailure(hr, "Failed to replace strings in SQL script.");
		}
	}
	hr = S_OK;

LExit:

	return hr;
}

HRESULT CXslTransform::AddExec(LPCWSTR szXmlFilePath, LPCWSTR szXslt)
{
    HRESULT hr = S_OK;
	::com::panelsw::ca::Command *pCmd = nullptr;
	XslTransformDetails *pDetails = nullptr;
	::std::string *pAny = nullptr;
	bool bRes = true;

    hr = AddCommand("CXslTransform", &pCmd);
    ExitOnFailure(hr, "Failed to add command");

	pDetails = new XslTransformDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_xslt(szXslt, WSTR_BYTE_SIZE(szXslt));
	pDetails->set_xml_path(szXmlFilePath, WSTR_BYTE_SIZE(szXmlFilePath));

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
    return hr;
}

// Execute the command object (XML element)
HRESULT CXslTransform::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	XslTransformDetails details;
	LPCWSTR szXsl = nullptr;
	LPCWSTR szXmlPath = nullptr;
	CComPtr<IXMLDOMDocument2> pXmlDoc;
	CComPtr<IXMLDOMDocument2> pXsl;
	CComPtr<IXMLDOMNodeList> pNodeList;
	CComVariant filePath;
	CComBSTR xslText;
	CComBSTR xmlTransformed;
	VARIANT_BOOL isXmlSuccess;

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking XslTransformDetails");

	szXmlPath = (LPCWSTR)details.xml_path().c_str();
	szXsl = (LPCWSTR)details.xslt().c_str();

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing XSL transform on file '%ls'", szXmlPath);

	// Create XML documents.
	hr = ::CoCreateInstance(CLSID_DOMDocument, nullptr, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXmlDoc);
	ExitOnFailure(hr, "Failed to CoCreateInstance CLSID_DOMDocument");
	hr = ::CoCreateInstance(CLSID_DOMDocument, nullptr, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXsl);
	ExitOnFailure(hr, "Failed to CoCreateInstance CLSID_DOMDocument");

	// Load XML document
	filePath = szXmlPath;
	hr = pXmlDoc->load(filePath, &isXmlSuccess);
	ExitOnFailure(hr, "Failed to load XML");
	if (!isXmlSuccess)
	{
		hr = E_FAIL;
		ExitOnFailure(hr, "Failed to load XML");
	}

	// Load XSL document
	xslText = szXsl;
	hr = pXsl->loadXML(xslText, &isXmlSuccess);
	ExitOnFailure(hr, "Failed to load XML");
	if (!isXmlSuccess)
	{
		hr = E_FAIL;
		BreakExitOnFailure(hr, "Failed to load XSL");
	}

	hr = pXmlDoc->transformNode(pXsl, &xmlTransformed);
	ExitOnFailure(hr, "Failed to apply XSL transform");

	hr = FileWrite(szXmlPath, FILE_ATTRIBUTE_NORMAL, (BYTE*)(LPWSTR)xmlTransformed, xmlTransformed.ByteLength(), nullptr);
	ExitOnFailure(hr, "Failed writing XML file '%ls'", szXmlPath);

LExit:
	return hr;
}
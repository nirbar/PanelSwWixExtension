#include "XslTransform.h"
#include "FileOperations.h"
#include "FileRegex.h"
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

static HRESULT ReadBinary(LPCWSTR szBinaryKey, LPCWSTR szQueryId, LPWSTR *pszXslPath);
static HRESULT ReplaceStrings(LPCWSTR szXslPath, LPCWSTR szXslId);

enum InstallUninstallType : int
{
	NotSet = -1,
	install = 0,
	uninstall = 1,
	both = 2,
	IllegalValue = INT_MAX
};

extern "C" UINT __stdcall XslTransform(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	DWORD dwRes = 0;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	CXslTransform deferredCA;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure tables exist.
	hr = WcaTableExists(L"PSW_XslTransform");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_XslTransform'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_XslTransform'. Have you authored 'PanelSw:XslTransform' entries in WiX code?");
	hr = WcaTableExists(L"PSW_XslTransform_Replacements");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_XslTransform_Replacements'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_XslTransform_Replacements'. Have you authored 'PanelSw:XslTransform' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `Component_`, `File_`, `FilePath`, `XslBinary_`, `On` FROM `PSW_XslTransform` ORDER BY `Order`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId, szComponent, szFileId, szXslBinaryId;
		CWixString szXslPath, szFileFmt, szFilePath;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		InstallUninstallType eOn;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component_.");
		hr = WcaGetRecordString(hRecord, 3, (LPWSTR*)szFileId);
		ExitOnFailure(hr, "Failed to get File_.");
		hr = WcaGetRecordString(hRecord, 4, (LPWSTR*)szFileFmt);
		ExitOnFailure(hr, "Failed to get File_.");
		hr = WcaGetRecordString(hRecord, 5, (LPWSTR*)szXslBinaryId);
		ExitOnFailure(hr, "Failed to get XslBinary_.");
		hr = WcaGetRecordInteger(hRecord, 6, (int*)&eOn);
		ExitOnFailure(hr, "Failed to get On.");

		ExitOnNull(!szXslBinaryId.IsNullOrEmpty(), hr, E_INVALIDARG, "Binary key is empty");
		ExitOnNull(!szComponent.IsNullOrEmpty(), hr, E_INVALIDARG, "Component is empty");
		ExitOnNull((szFileFmt.IsNullOrEmpty() != szFileId.IsNullOrEmpty()), hr, E_INVALIDARG, "Either FileId or FilePath must be specified");

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
		case WCA_TODO::WCA_TODO_REINSTALL:
			if ((eOn == InstallUninstallType::install) || (eOn == InstallUninstallType::both))
			{
				break;
			}
			WcaLog(LOGMSG_STANDARD, "Skipping execution of XSLT '%ls' since component '%ls' is being installed or reinstalled", (LPCWSTR)szXslBinaryId, (LPCWSTR)szComponent);
			continue;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			if ((eOn == InstallUninstallType::uninstall) || (eOn == InstallUninstallType::both))
			{
				break;
			}
			WcaLog(LOGMSG_STANDARD, "Skipping execution of XSLT '%ls' since component '%ls' is being uninstalled", (LPCWSTR)szXslBinaryId, (LPCWSTR)szComponent);
			continue;

		default:
			WcaLog(LOGMSG_STANDARD, "Skipping execution of XSLT '%ls'", (LPCWSTR)szXslBinaryId);
			continue;
		}

		if (!szFileId.IsNullOrEmpty())
		{
			hr = szFileFmt.Format(L"[#%ls]", (LPCWSTR)szFileId);
			ExitOnFailure(hr, "Failed formatting string");
		}

		hr = szFilePath.MsiFormat((LPCWSTR)szFileFmt);
		ExitOnFailure(hr, "Failed formatting string");

		if (szFilePath.IsNullOrEmpty())
		{
			WcaLog(LOGMSG_STANDARD, "Skipping execution of XSLT '%ls' since component '%ls' is not being installed or reinstalled", (LPCWSTR)szXslBinaryId, (LPCWSTR)szComponent);
			continue;
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will apply XSL transform '%ls' on file '%ls'", (LPCWSTR)szXslBinaryId, (LPCWSTR)szFilePath);
		hr = ReadBinary(szXslBinaryId, szId, (LPWSTR*)szXslPath);
		ExitOnFailure(hr, "Failed reading XSL transform");

		hr = ReplaceStrings(szXslPath, szId);
		ExitOnFailure(hr, "Failed replacing strings in temp XSL file");

		//TODO Add rollback + commit to delete the XSL file and rollback to revert the XML file
		hr = deferredCA.AddExec(szFilePath, szXslPath);
		ExitOnFailure(hr, "Failed scheduling '%ls' XSL transform", (LPCWSTR)szXslBinaryId);
	}
	hr = S_OK;

	// Deferred action
	hr = deferredCA.DoDeferredAction(L"PSW_XslTransformExec");
	ExitOnFailure(hr, "Failed setting custom action data.");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT ReadBinary(LPCWSTR szBinaryKey, LPCWSTR szQueryId, LPWSTR* pszXslFile)
{
	HRESULT hr = S_OK;
	CWixString szMsiQuery;
	std::map<std::string, std::string> replacements;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	BYTE* pbData = nullptr;
	DWORD cbData = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	hr = szMsiQuery.Format(L"SELECT `Data` FROM `Binary` WHERE `Name`='%ls'", szBinaryKey);
	ExitOnFailure(hr, "Failed to format string");

	hr = WcaOpenExecuteView((LPCWSTR)szMsiQuery, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szMsiQuery);

	hr = WcaFetchRecord(hView, &hRecord);
	ExitOnFailure(hr, "Failed to fetch binary record.");

	hr = WcaGetRecordStream(hRecord, 1, &pbData, &cbData);
	ExitOnFailure(hr, "Failed to ready Binary.Data for certificate.");

	// Load XSL document
	hr = FileCreateTempW(L"XSL", L"xsl", pszXslFile, &hFile);
	ExitOnFailure(hr, "Failed creating temp XSL file");

	hr = FileWriteHandle(hFile, pbData, cbData);
	ExitOnFailure(hr, "Failed writing temp XSL file");

	::FlushFileBuffers(hFile);

LExit:
	ReleaseMem(pbData);
	ReleaseFile(hFile);

	return hr;
}

static HRESULT ReplaceStrings(LPCWSTR szXslPath, LPCWSTR szXslId)
{
	HRESULT hr = S_OK;
	CWixString szMsiQuery;
	std::map<std::string, std::string> replacements;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = szMsiQuery.Format(L"SELECT `Text`, `Replacement` FROM `PSW_XslTransform_Replacements` WHERE `XslTransform_`='%ls' ORDER BY `Order`", szXslId);
	ExitOnFailure(hr, "Failed to format string");

	hr = WcaOpenExecuteView((LPCWSTR)szMsiQuery, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szMsiQuery);

	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		CWixString szTextFmt, szReplacementFmt;
		CWixString szText, szReplacement;
		CFileRegex fileRegex;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szTextFmt);
		ExitOnFailure(hr, "Failed to get Text.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szReplacementFmt);
		ExitOnFailure(hr, "Failed to get Replacement.");

		hr = szText.MsiFormat((LPCWSTR)szTextFmt);
		ExitOnFailure(hr, "Failed formatting string");

		hr = szReplacement.MsiFormat((LPCWSTR)szReplacementFmt);
		ExitOnFailure(hr, "Failed formatting string");

		if (!szText.IsNullOrEmpty())
		{
			CDeferredActionBase::LogUnformatted(LOGLEVEL::LOGMSG_STANDARD, false, L"Will replace matches of regex '%ls' with '%ls' in file '%ls'", szText.Obfuscated(), szReplacement.Obfuscated(), szXslPath);

			hr = fileRegex.Execute(szXslPath, szText, szReplacement, com::panelsw::ca::FileRegexDetails_FileEncoding::FileRegexDetails_FileEncoding_None, false);
			ExitOnFailure(hr, "Failed to replace strings in SQL script.");
		}
	}
	hr = S_OK;

LExit:

	return hr;
}

HRESULT CXslTransform::AddExec(LPCWSTR szXmlFilePath, LPCWSTR szXsltPath)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	XslTransformDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CXslTransform", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new XslTransformDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_xsl_path(szXsltPath, WSTR_BYTE_SIZE(szXsltPath));
	pDetails->set_xml_path(szXmlFilePath, WSTR_BYTE_SIZE(szXmlFilePath));

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CXslTransform::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	XslTransformDetails details;
	LPCWSTR szXmlPath = nullptr;
	LPCWSTR szXslPath = nullptr;

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking XslTransformDetails");

	szXmlPath = (LPCWSTR)(LPVOID)details.xml_path().data();
	szXslPath = (LPCWSTR)(LPVOID)details.xsl_path().data();

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing XSL transform on file '%ls'", szXmlPath);

	try
	{
		CComPtr<IXMLDOMDocument2> pXmlDoc;
		CComPtr<IXMLDOMDocument2> pOutXmlDoc;
		CComPtr<IXMLDOMDocument2> pXsl;
		CComPtr<IXMLDOMParseError2> pError;
		CComPtr<IXMLDOMNodeList> pNodeList;
		CComVariant filePath;
		CComBSTR szErrorReason;
		CComBSTR szError;
		CComBSTR xmlTransformed;
		VARIANT_BOOL isXmlSuccess;

		// Create XML documents.
		hr = ::CoCreateInstance(CLSID_DOMDocument, nullptr, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXmlDoc);
		ExitOnFailure(hr, "Failed to CoCreateInstance CLSID_DOMDocument");
		hr = ::CoCreateInstance(CLSID_DOMDocument, nullptr, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXsl);
		ExitOnFailure(hr, "Failed to CoCreateInstance CLSID_DOMDocument");
		hr = ::CoCreateInstance(CLSID_DOMDocument, nullptr, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pOutXmlDoc);
		ExitOnFailure(hr, "Failed to CoCreateInstance CLSID_DOMDocument");

		// Lifting security limitations since XSL are from known source (the developer) and anyway it has local system priveleges
		hr = pXmlDoc->setProperty(L"AllowXsltScript", CComVariant(true));
		if (FAILED(hr))
		{
			WcaLogError(hr, "Failed setting AllowXsltScript");
		}
		hr = pXmlDoc->setProperty(L"AllowDocumentFunction", CComVariant(true));
		if (FAILED(hr))
		{
			WcaLogError(hr, "Failed setting AllowDocumentFunction");
		}
		hr = pXmlDoc->setProperty(L"ProhibitDTD", CComVariant(false));
		if (FAILED(hr))
		{
			WcaLogError(hr, "Failed resetting ProhibitDTD");
		}
		hr = pXsl->setProperty(L"AllowXsltScript", CComVariant(true));
		if (FAILED(hr))
		{
			WcaLogError(hr, "Failed setting AllowXsltScript");
		}
		hr = pXsl->setProperty(L"AllowDocumentFunction", CComVariant(true));
		if (FAILED(hr))
		{
			WcaLogError(hr, "Failed setting AllowDocumentFunction");
		}
		hr = pXsl->setProperty(L"ProhibitDTD", CComVariant(false));
		if (FAILED(hr))
		{
			WcaLogError(hr, "Failed resetting ProhibitDTD");
		}

		// Load XML document
		filePath = szXmlPath;
		hr = pXmlDoc->load(filePath, &isXmlSuccess);
		if (FAILED(hr) || !isXmlSuccess)
		{
			if (SUCCEEDED(hr))
			{
				hr = E_FAIL;
			}
			if (SUCCEEDED(pXsl->get_parseError((IXMLDOMParseError**)&pError)) && SUCCEEDED(pError->get_srcText(&szError)) && SUCCEEDED(pError->get_reason(&szErrorReason)))
			{
				ExitOnFailure(hr, "Failed to load XML. %ls: %ls", (LPWSTR)szErrorReason, (LPWSTR)szError);
			}
			else
			{
				ExitOnFailure(hr, "Failed to load XML");
			}
		}

		filePath = szXslPath;
		hr = pXsl->load(filePath, &isXmlSuccess);
		if (FAILED(hr) || !isXmlSuccess)
		{
			if (SUCCEEDED(hr))
			{
				hr = E_FAIL;
			}
			if (SUCCEEDED(pXsl->get_parseError((IXMLDOMParseError**)&pError)) && SUCCEEDED(pError->get_srcText(&szError)) && SUCCEEDED(pError->get_reason(&szErrorReason)))
			{
				ExitOnFailure(hr, "Failed to load XSL. %ls: %ls", (LPWSTR)szErrorReason, (LPWSTR)szError);
			}
			else
			{
				ExitOnFailure(hr, "Failed to load XSL");
			}
		}

		hr = pXmlDoc->transformNodeToObject(pXsl, CComVariant(pOutXmlDoc.p));
		if (FAILED(hr))
		{
			if (SUCCEEDED(pXmlDoc->get_parseError((IXMLDOMParseError**)&pError)) && SUCCEEDED(pError->get_srcText(&szError)) && SUCCEEDED(pError->get_reason(&szErrorReason)))
			{
				ExitOnFailure(hr, "Failed to apply XSL transform. %ls: %ls", (LPWSTR)szErrorReason, (LPWSTR)szError);
			}
		}
		ExitOnFailure(hr, "Failed to apply XSL transform");

		hr = pOutXmlDoc->save(CComVariant(szXmlPath));
		ExitOnFailure(hr, "Failed saving XML file '%ls'", szXmlPath);
	}
	catch (CAtlException ex)
	{
		hr = (HRESULT)ex;
		ExitOnFailure(hr, "Failed applying XSLT");
	}
	catch (...)
	{
		hr = E_FAIL;
		ExitOnFailure(hr, "Failed applying XSLT");
	}

LExit:

	return hr;
}

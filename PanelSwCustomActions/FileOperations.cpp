#include "FileOperations.h"

HRESULT CFileOperations::AddCopyFile(LPCWSTR szFrom, LPCWSTR szTo)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"CopyFile", L"CFileOperations", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("From"), CComVariant(szFrom));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'From'");

	hr = pElem->setAttribute(CComBSTR("To"), CComVariant(szTo));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'To'");

LExit:
	return hr;
}

HRESULT CFileOperations::AddMoveFile(LPCWSTR szFrom, LPCWSTR szTo)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"MoveFile", L"CFileOperations", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("From"), CComVariant(szFrom));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'From'");

	hr = pElem->setAttribute(CComBSTR("To"), CComVariant(szTo));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'To'");

LExit:
	return hr;
}

HRESULT CFileOperations::AddDeleteFile(LPCWSTR szPath)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMElement> pElem;

	hr = AddElement(L"DeleteFile", L"CFileOperations", 1, &pElem);
	BreakExitOnFailure(hr, "Failed to add XML element");

	hr = pElem->setAttribute(CComBSTR("Path"), CComVariant(szPath));
	BreakExitOnFailure(hr, "Failed to add XML attribute 'Path'");

LExit:
	return hr;
}

// Execute the command object (XML element)
HRESULT CFileOperations::DeferredExecute(IXMLDOMElement* pElem)
{
	HRESULT hr = S_OK;
	CComBSTR vTag;

	hr = pElem->get_tagName( &vTag);
	BreakExitOnFailure(hr, "Failed to get tag name");

	if( vTag == L"CopyFile")
	{
		hr = CopyFile(pElem);
		BreakExitOnFailure(hr, "Failed to copy file");
	}
	else if( vTag == L"MoveFile")
	{
		hr = MoveFile(pElem);
		BreakExitOnFailure(hr, "Failed to move file");
	}
	else if( vTag == L"DeleteFile")
	{
		hr = DeleteFile(pElem);
		BreakExitOnFailure(hr, "Failed to delete file");
	}
	else
	{
		hr = E_INVALIDARG;
		BreakExitOnFailure1(hr, "Invalid tag: '%ls'", (LPWSTR)vTag);
	}

LExit:
	return hr;
}

HRESULT CFileOperations::CopyFile(IXMLDOMElement* pElem)
{
	CComVariant vFrom;
	CComVariant vTo;
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;

	hr = pElem->getAttribute(CComBSTR(L"From"), &vFrom);
	BreakExitOnFailure(hr, "Failed getting 'From' attribute");

	hr = pElem->getAttribute(CComBSTR(L"To"), &vTo);
	BreakExitOnFailure(hr, "Failed getting 'To' attribute");

	bRes = ::CopyFile(vFrom.bstrVal, vTo.bstrVal, FALSE);
	BreakExitOnNullWithLastError(bRes, hr, "Failed copying file");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Copied '%ls' to '%ls'", vFrom.bstrVal, vTo.bstrVal);

LExit:
	return hr;
}

HRESULT CFileOperations::MoveFile(IXMLDOMElement* pElem)
{
	CComVariant vFrom;
	CComVariant vTo;
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;

	hr = pElem->getAttribute(CComBSTR(L"From"), &vFrom);
	BreakExitOnFailure(hr, "Failed getting 'From' attribute");

	hr = pElem->getAttribute(CComBSTR(L"To"), &vTo);
	BreakExitOnFailure(hr, "Failed getting 'To' attribute");

	bRes = ::MoveFileEx(vFrom.bstrVal, vTo.bstrVal, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
	BreakExitOnNullWithLastError(bRes, hr, "Failed moving file");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Moved '%ls' to '%ls'", vFrom.bstrVal, vTo.bstrVal);

LExit:
	return hr;
}

HRESULT CFileOperations::DeleteFile(IXMLDOMElement* pElem)
{
	CComVariant vPath;
	CComVariant vTo;
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;

	hr = pElem->getAttribute(CComBSTR(L"Path"), &vPath);
	BreakExitOnFailure(hr, "Failed getting 'Path' attribute");

	bRes = ::DeleteFile(vPath.bstrVal);
	BreakExitOnNullWithLastError(bRes, hr, "Failed deleting file");
	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Deleted '%ls'", vPath.bstrVal);

LExit:
	return hr;
}

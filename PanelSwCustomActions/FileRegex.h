#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CFileRegex :
	public CDeferredActionBase
{
public:

	enum FileEncoding
	{
		None,
		MultiByte,
		Unicode,
		ReverseUnicode
	};

	HRESULT AddFileRegex(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, FileEncoding eEncoding, bool bIgnoreCase);

protected:
	// Execute the command object (XML element)
	virtual HRESULT DeferredExecute(IXMLDOMElement* pElem);

private:

	FileEncoding DetectEncoding(const void* pFileContent, DWORD dwSize);

	HRESULT Execute(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, FileEncoding eEncoding, bool bIgnoreCase);

	HRESULT ExecuteMultibyte(LPCWSTR szFilePath, LPCSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase);

	HRESULT ExecuteUnicode(LPCWSTR szFilePath, LPCWSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase);
};


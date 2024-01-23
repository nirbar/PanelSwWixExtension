#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/WixString.h"
#include "fileRegexDetails.pb.h"

class CFileRegex :
	public CDeferredActionBase
{
public:

	CFileRegex() : CDeferredActionBase("FileRegex") { }

	HRESULT AddFileRegex(LPCWSTR szFilePath, const CWixString &szRegex, const CWixString& szReplacement, ::com::panelsw::ca::FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase);

	HRESULT Execute(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, ::com::panelsw::ca::FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:

	HRESULT ExecuteMultibyte(LPCWSTR szFilePath, LPCSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase);

	HRESULT ExecuteUnicode(LPCWSTR szFilePath, LPCWSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase);
};

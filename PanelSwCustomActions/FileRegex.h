#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "fileRegexDetails.pb.h"

class CFileRegex :
	public CDeferredActionBase
{
public:

	CFileRegex() noexcept : CDeferredActionBase("FileRegex") { }

	HRESULT AddFileRegex(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, ::com::panelsw::ca::FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase) noexcept;

	HRESULT Execute(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, ::com::panelsw::ca::FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase) noexcept;

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) noexcept override;

private:

	HRESULT ExecuteMultibyte(LPCWSTR szFilePath, LPCSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase) noexcept;

	HRESULT ExecuteUnicode(LPCWSTR szFilePath, LPCWSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase) noexcept;
};
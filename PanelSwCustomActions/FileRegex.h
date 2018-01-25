#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "fileRegexDetails.pb.h"

class CFileRegex :
	public CDeferredActionBase
{
public:

	HRESULT AddFileRegex(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, ::com::panelsw::ca::FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase);

protected:
	// Execute the command object (XML element)
	HRESULT DeferredExecute(const ::google::protobuf::Any* pCommand) override;

private:

	::com::panelsw::ca::FileRegexDetails::FileEncoding DetectEncoding(const void* pFileContent, DWORD dwSize);

	HRESULT Execute(LPCWSTR szFilePath, LPCWSTR szRegex, LPCWSTR szReplacement, ::com::panelsw::ca::FileRegexDetails::FileEncoding eEncoding, bool bIgnoreCase);

	HRESULT ExecuteMultibyte(LPCWSTR szFilePath, LPCSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase);

	HRESULT ExecuteUnicode(LPCWSTR szFilePath, LPCWSTR szFileContent, LPCWSTR szRegex, LPCWSTR szReplacement, bool bIgnoreCase);
};


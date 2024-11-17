#pragma once
#include "../CaCommon/DeferredActionBase.h"

class CReparsePoint :
	public CDeferredActionBase
{
public:

	CReparsePoint() : CDeferredActionBase("ReparsePoint") { }

	HRESULT AddRestoreReparsePoint(LPCWSTR szPath);

	HRESULT AddDeleteReparsePoint(LPCWSTR szPath);

	static bool IsSymbolicLinkOrMount(LPCWSTR szPath);
	static bool IsSymbolicLinkOrMount(const WIN32_FIND_DATA *pFindFileData);

protected:
	
	HRESULT DeferredExecute(const ::std::string& command) override;

private:

	HRESULT DeleteReparsePoint(LPCWSTR szPath, LPVOID pBuffer, DWORD dwSize);

	HRESULT CreateReparsePoint(LPCWSTR szPath, LPVOID pBuffer, DWORD dwSize, FILETIME* pftCreateTime, FILETIME* pftLastWriteTime);

	HRESULT GetReparsePointData(LPCWSTR szPath, void** ppBuffer, DWORD* pdwSize, FILETIME* pftCreateTime, FILETIME* pftLastWriteTime);
};


#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "registryOperationsDetails.pb.h"

class CRegistryOperations
	: public CDeferredActionBase
{
public:

	CRegistryOperations() : CDeferredActionBase("RegistryOperations") { }

	HRESULT AddDeleteKey(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey);
	HRESULT AddDeleteValue(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey, LPCWSTR szName);

	HRESULT AddCreateKey(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey);
	HRESULT AddCreateValue(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey, LPCWSTR szName, DWORD dwType, LPCBYTE pbData, DWORD cbData);

	HRESULT AddRecreateHierarchy(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey);

protected:

	HRESULT DeferredExecute(const ::std::string& command) override;

private:

	/* Supported data types:
	  REG_NONE = 0ul		// No value type
	  REG_SZ = 1ul			// Unicode nul terminated string
	  REG_EXPAND_SZ = 2ul	// Unicode nul terminated string
	  REG_DWORD = 4ul		// 32-bit number
	  REG_QWORD = 11ul		// 64-bit number
	*/

	HRESULT DeleteKey(HKEY hkRoot, DWORD dwView, LPCWSTR szKey);
	HRESULT CreateKey(HKEY hkRoot, DWORD dwView, LPCWSTR szKey);

	HRESULT DeleteValue(HKEY hkRoot, DWORD dwView, LPCWSTR szKey, LPCWSTR szName);
	HRESULT SetValue(HKEY hkRoot, DWORD dwView, LPCWSTR szKey, LPCWSTR szName, DWORD dwType, LPCBYTE pbData, DWORD cbData);

	REG_KEY_BITNESS View2Bitness(DWORD dwView) const;
};

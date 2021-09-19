#pragma once
#include "stdafx.h"
#include <windows.h>
#include "google\protobuf\message_lite.h"
#include "command.pb.h"
#include "customActionData.pb.h"

#define WSTR_BYTE_SIZE(sz)		((sz) ? (sizeof(WCHAR) * (1 + ::wcslen(sz))) : 0)
#define E_RETRY					__HRESULT_FROM_WIN32(ERROR_RETRY)

class CDeferredActionBase
{
public:
	CDeferredActionBase(LPCSTR szId) noexcept;
	virtual ~CDeferredActionBase() noexcept;

	// Function that maps a receiver name to a CDeferredActionBase inheritor.
	typedef HRESULT(*ReceiverToExecutorFunc)(LPCSTR szReceiver, CDeferredActionBase** ppExecutor);
	static HRESULT DeferredEntryPoint(MSIHANDLE hInstall, ReceiverToExecutorFunc mapFunc) noexcept;

	UINT GetCost() const noexcept;

	HRESULT GetCustomActionData(LPWSTR *pszCustomActionData) noexcept;

	HRESULT Prepend(CDeferredActionBase* pOther) noexcept;

	bool HasActions() const noexcept;

	static void LogUnformatted(LOGLEVEL level, PCSTR szFormat, ...) noexcept;

protected:

	// Overriden by inheriting classes. Execute the command object (XML element)
	virtual HRESULT DeferredExecute(const ::std::string& command) noexcept = 0;

	HRESULT AddCommand(LPCSTR szHandler, ::com::panelsw::ca::Command **ppCommand) noexcept;

private:
	::com::panelsw::ca::CustomActionData _cad;
};


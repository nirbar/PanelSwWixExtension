#pragma once
#include "stdafx.h"
#include <windows.h>
#include "google\protobuf\message_lite.h"
#include "command.pb.h"
#include "customActionData.pb.h"

#define WSTR_BYTE_SIZE(sz)		((sz) ? (sizeof(WCHAR) * (1 + ::wcslen(sz))) : 0)

class CDeferredActionBase
{
public:
	CDeferredActionBase(LPCSTR szId);
	virtual ~CDeferredActionBase();

	// Function that maps a receiver name to a CDeferredActionBase inheritor.
	typedef HRESULT(*ReceiverToExecutorFunc)(LPCSTR szReceiver, CDeferredActionBase** ppExecutor);
	static HRESULT DeferredEntryPoint(MSIHANDLE hInstall, ReceiverToExecutorFunc mapFunc);

	UINT GetCost() const;

	HRESULT GetCustomActionData(LPWSTR *pszCustomActionData);

	HRESULT Prepend(CDeferredActionBase* pOther);

	bool HasActions() const;

	static void LogUnformatted(LOGLEVEL level, bool bShowTime, LPCWSTR szFormat, ...);

protected:

	// Overriden by inheriting classes. Execute the command object (XML element)
	virtual HRESULT DeferredExecute(const ::std::string& command) = 0;

	HRESULT AddCommand(LPCSTR szHandler, ::com::panelsw::ca::Command **ppCommand);

private:
	::com::panelsw::ca::CustomActionData _cad;
};


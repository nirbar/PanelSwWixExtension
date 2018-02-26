#pragma once
#include "stdafx.h"
#include <windows.h>
#include "google\protobuf\message_lite.h"
#include "command.pb.h"
#include "customActionData.pb.h"

#define WSTR_BYTE_SIZE(sz)		(sizeof(WCHAR) * (1 + ::wcslen(sz)))

class CDeferredActionBase
{
public:
	CDeferredActionBase();
	virtual ~CDeferredActionBase();

	// Function that maps a receiver name to a CDeferredActionBase inheritor.
	typedef HRESULT(*ReceiverToExecutorFunc)(LPCSTR szReceiver, CDeferredActionBase** ppExecutor);
	static HRESULT DeferredEntryPoint(ReceiverToExecutorFunc mapFunc);

	UINT GetCost() const { return _uCost; }

	HRESULT GetCustomActionData(LPWSTR *pszCustomActionData);

	HRESULT Prepend(CDeferredActionBase* pOther);

protected:

	// Overriden by inheriting classes. Execute the command object (XML element)
	virtual HRESULT DeferredExecute(const ::std::string& command) = 0;

	HRESULT AddCommand(LPCSTR szHandler, ::com::panelsw::ca::Command **ppCommand);

private:
	::com::panelsw::ca::CustomActionData _cad;
	UINT _uCost;
};


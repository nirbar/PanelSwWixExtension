#include "stdafx.h"
#include <Objbase.h>
#include <atlbase.h>
#include <Wbemidl.h>
#include "../CaCommon/WixString.h"
#include "errorHandling.pb.h"

#pragma comment(lib, "wbemuuid.lib")
using namespace ::com::panelsw::ca;

static HRESULT ExecuteOne(LPCWSTR szNamespace, LPCWSTR szQuery, LPCWSTR szResultProperty, LPCWSTR szProperty) noexcept;

extern "C" UINT __stdcall WmiSearch(MSIHANDLE hInstall) noexcept
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bComInit = false;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_WmiSearch");
	ExitOnFailure(hr, "Table does not exist 'PSW_WmiSearch'. Have you authored 'PanelSw:WmiSearch' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Property_`, `Condition`, `Namespace`, `Query`, `ResultProperty` FROM `PSW_WmiSearch` ORDER BY `Order`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'PSW_WmiSearch'.");

	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	ExitOnFailure(hr, "Failed to initialize COM");
	bComInit = true;

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szProperty;
		CWixString szCondition;
		CWixString szNamespace;
		CWixString szQuery;
		CWixString szResultProperty;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szProperty);
		ExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)szCondition);
		ExitOnFailure(hr, "Failed to get Condition.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)szNamespace);
		ExitOnFailure(hr, "Failed to get Namespace.");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)szQuery);
		ExitOnFailure(hr, "Failed to get Query.");
		hr = WcaGetRecordFormattedString(hRecord, 5, (LPWSTR*)szResultProperty);
		ExitOnFailure(hr, "Failed to get ResultProperty.");

		if (!szCondition.IsNullOrEmpty())
		{
			MSICONDITION condRes = MSICONDITION::MSICONDITION_NONE;

			condRes = ::MsiEvaluateCondition(hInstall, szCondition);
			ExitOnNullWithLastError((condRes != MSICONDITION::MSICONDITION_ERROR), hr, "Failed evaluating condition '%ls'", szCondition);

			hr = (condRes == MSICONDITION::MSICONDITION_FALSE) ? S_FALSE : S_OK;
			WcaLog(LOGMSG_STANDARD, "Condition '%ls' evaluated to %i", (LPCWSTR)szCondition, (1 - (int)hr));
			if (hr == S_FALSE)
			{
				continue;
			}
		}

		if (szNamespace.IsNullOrEmpty())
		{
			hr = szNamespace.Copy(L"ROOT\\CIMV2");
			ExitOnFailure(hr, "Failed copying string");
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing WMI query '%ls' in namespace '%ls'. Will place result's '%ls' in property '%ls'", (LPCWSTR)szQuery, (LPCWSTR)szNamespace, (LPCWSTR)szResultProperty, (LPCWSTR)szProperty);

		hr = ExecuteOne(szNamespace, szQuery, szResultProperty, szProperty);
		ExitOnFailure(hr, "Failed executing WQL search");
	}
	hr = S_OK;

LExit:
	if (bComInit)
	{
		CoUninitialize();
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT ExecuteOne(LPCWSTR szNamespace, LPCWSTR szQuery, LPCWSTR szResultProperty, LPCWSTR szProperty) noexcept
{
	HRESULT hr = S_OK;
	CComPtr<IWbemLocator> wbemLocator;
	CComPtr<IWbemServices> wbemSvc;
	CComPtr<IEnumWbemClassObject> wqlEnumerator;

	hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&wbemLocator);
	ExitOnFailure(hr, "Failed initializing IWbemLocator");

	hr = wbemLocator->ConnectServer(CComBSTR(szNamespace), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &wbemSvc);
	ExitOnFailure(hr, "Failed connecting to WMI namespace");

	hr = CoSetProxyBlanket(wbemSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
	ExitOnFailure(hr, "Failed setting proxy blanket");

	hr = wbemSvc->ExecQuery(CComBSTR("WQL"), CComBSTR(szQuery), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &wqlEnumerator);
	ExitOnFailure(hr, "Failed executing WQL query");

	while (true)
	{
		CComPtr<IWbemClassObject> pclsObj;
		ULONG nCnt = 0;

		HRESULT hr = wqlEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &nCnt);
		if ((hr == WBEM_S_FALSE) || (nCnt == 0))
		{
			hr = S_OK;
			break;
		}
		ExitOnFailure(hr, "Failed getting next result");

		// Get the value
		if (szResultProperty && *szResultProperty)
		{
			CComVariant vrValue;

			hr = pclsObj->Get(szResultProperty, 0, &vrValue, nullptr, 0);
			ExitOnFailure(hr, "Failed getting property '%ls' from WQL result", szResultProperty);

			if ((vrValue.vt == VARENUM::VT_EMPTY) || (vrValue.vt == VARENUM::VT_NULL) || (vrValue.vt == VARENUM::VT_VOID))
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Value is empty");
				continue;
			}

			if (vrValue.vt != VARENUM::VT_BSTR)
			{
				hr = VariantChangeType(&vrValue, &vrValue, VARIANT_ALPHABOOL | VARIANT_NOUSEROVERRIDE, VARENUM::VT_BSTR);
				ExitOnFailure(hr, "Failed converting value to string");
				ExitOnNull((vrValue.vt == VARENUM::VT_BSTR), hr, E_INVALIDDATA, "Failed converting value to string. It is %u", vrValue.vt);
			}

			hr = WcaSetProperty(szProperty, vrValue.bstrVal);
			ExitOnFailure(hr, "Failed setting property");

		}
		else // Get full object text
		{
			CComBSTR szVal;

			hr = pclsObj->GetObjectText(0, &szVal);
			ExitOnFailure(hr, "Failed getting object text");

			hr = WcaSetProperty(szProperty, szVal);
			ExitOnFailure(hr, "Failed setting property");
		}
	}

LExit:
	return hr;
}
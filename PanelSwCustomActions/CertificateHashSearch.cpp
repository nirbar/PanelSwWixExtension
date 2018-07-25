#include "stdafx.h"
#include <WixString.h>
#include <memutil.h>
#include <Wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

extern "C" UINT __stdcall CertificateHashSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	PCCERT_CONTEXT pCertContext = nullptr;
	HCERTSTORE hMachineStore = NULL;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_CertificateHashSearch");
	BreakExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_CertificateHashSearch'. Have you authored 'PanelSw:CertificateHashSearch' entries in WiX code?");
	
	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `CertName` FROM `PSW_CertificateHashSearch`", &hView);
	BreakExitOnFailure(hr, "Failed to execute MSI SQL query");

	hMachineStore = ::CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"MY");
	BreakExitOnNullWithLastError(hMachineStore, hr, "Failed opening certificate store");

	// Loop
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");
		
		CWixString id; // Property name i.e. "SELF_SIGNED_PFX"
		CWixString certName;
		BYTE certHash[20];
		DWORD hashSize = 20;
		WCHAR hashHex[20 * 2 + 1];

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)id);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)certName);
		BreakExitOnFailure(hr, "Failed to get certificate name.");

		pCertContext = ::CertFindCertificateInStore(hMachineStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, (LPCWSTR)certName, pCertContext);
		if (!pCertContext && (HRESULT_FROM_WIN32(::GetLastError()) == CRYPT_E_NOT_FOUND))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Did not find certificate with sybject '%ls' in machine MY store.", (LPCWSTR)certName);
			continue;
		}
		BreakExitOnNullWithLastError(pCertContext, hr, "Failed finding certificate with name '%ls'", (LPCWSTR)certName);

		bRes = ::CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, certHash, &hashSize);
		BreakExitOnNullWithLastError(bRes, hr, "Failed getting certificate hash for '%ls'", (LPCWSTR)certName);

		hr = StrHexEncode(certHash, hashSize, hashHex, countof(hashHex));
		BreakExitOnFailure(hr, "Failed encoding certificate hash for '%ls'", (LPCWSTR)certName);

		hr = WcaSetProperty(id, hashHex);
		BreakExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)id);

		bRes = ::CertFreeCertificateContext(pCertContext);
		BreakExitOnNullWithLastError(bRes, hr, "Failed releasing certificate");
		pCertContext = nullptr;
	}
	hr = S_OK;

LExit:
	if (hMachineStore)
	{
		::CertCloseStore(hMachineStore, 0);
	}
	if (pCertContext)
	{
		::CertFreeCertificateContext(pCertContext);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

#include "stdafx.h"
#include <WixString.h>
#include <memutil.h>
#include <Wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

static HRESULT RegBinaryToMem(CWixString &binaryStr, BYTE** ppBytes, DWORD *pSize);

extern "C" UINT __stdcall CertificateHashSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	PCCERT_CONTEXT pCertContext = nullptr;
	HCERTSTORE hMachineStore = NULL;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	BYTE *buffer1 = nullptr;
	BYTE *buffer2 = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_CertificateHashSearch");
	BreakExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_CertificateHashSearch'. Have you authored 'PanelSw:CertificateHashSearch' entries in WiX code?");
	
	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `CertName`, `Issuer`, `SerialNumber` FROM `PSW_CertificateHashSearch`", &hView);
	BreakExitOnFailure(hr, "Failed to execute MSI SQL query");

	hMachineStore = ::CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE, L"MY");
	BreakExitOnNullWithLastError(hMachineStore, hr, "Failed opening certificate store");

	// Loop
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");
		
		CWixString id; // Property name i.e. "SELF_SIGNED_PFX"
		CWixString certName; // Subject
		CWixString issuer; // Issuer, binary format
		CWixString serial; // Serial, binary format
		BYTE certHash[20];
		DWORD hashSize = 20;
		WCHAR hashHex[20 * 2 + 1];

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)id);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)certName);
		BreakExitOnFailure(hr, "Failed to get CertName.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)issuer);
		BreakExitOnFailure(hr, "Failed to get Issuer.");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)serial);
		BreakExitOnFailure(hr, "Failed to get Serial.");

		if (certName.IsNullOrEmpty() && issuer.IsNullOrEmpty() && serial.IsNullOrEmpty())
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "All certificate parameters are empty for '%ls'", (LPCWSTR)id);
			continue;
		}
		if (!certName.IsNullOrEmpty())
		{
			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Searching certificate with subject '%ls'", (LPCWSTR)certName);
			pCertContext = ::CertFindCertificateInStore(hMachineStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, (LPCWSTR)certName, pCertContext);
		}
		else // Expecting both Issuer and serial.
		{
			CERT_INFO certInfo;
			DWORD buffSize = 0;

			WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Searching certificate with issuer '%ls' and serial '%ls'", (LPCWSTR)issuer, (LPCWSTR)serial);

			::ZeroMemory(&certInfo, sizeof(certInfo));

			hr = RegBinaryToMem(issuer, &buffer1, &buffSize);
			BreakExitOnFailure(hr, "Failed parsing issuer as binary");

			certInfo.Issuer.cbData = buffSize;
			certInfo.Issuer.pbData = buffer1;

			buffSize = 0;
			hr = RegBinaryToMem(serial, &buffer2, &buffSize);
			BreakExitOnFailure(hr, "Failed parsing serial as binary");

			certInfo.SerialNumber.cbData = buffSize;
			certInfo.SerialNumber.pbData = buffer2;

			pCertContext = ::CertFindCertificateInStore(hMachineStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, &certInfo, pCertContext);

			ReleaseNullMem(buffer1);
			ReleaseNullMem(buffer2);
		}

		if (!pCertContext && (HRESULT_FROM_WIN32(::GetLastError()) == CRYPT_E_NOT_FOUND))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Did not find matching certificate in machine MY store.");
			continue;
		}
		BreakExitOnNullWithLastError(pCertContext, hr, "Failed finding matching certificate");

		bRes = ::CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, certHash, &hashSize);
		BreakExitOnNullWithLastError(bRes, hr, "Failed getting certificate hash");

		hr = StrHexEncode(certHash, hashSize, hashHex, countof(hashHex));
		BreakExitOnFailure(hr, "Failed encoding certificate hash");

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
	ReleaseMem(buffer1);
	ReleaseMem(buffer2);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT RegBinaryToMem(CWixString &binaryStr, BYTE** ppBytes, DWORD *pSize)
{
	HRESULT hr = S_OK;
	
	hr = binaryStr.ReplaceAll(L"#x", L"");
	ExitOnFailure(hr, "Failed trimming embedded hex specifiers");

	hr = binaryStr.ReplaceAll(L" ", L"");
	ExitOnFailure(hr, "Failed trimming embedded spaces");

	hr = StrAllocHexDecode(binaryStr, ppBytes, pSize);
	ExitOnFailure(hr, "Failed getting hexadecimal numbers from '%ls'", (LPCWSTR)binaryStr);

LExit:
	return hr;
}
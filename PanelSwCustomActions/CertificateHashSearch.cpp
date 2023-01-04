::CertCloseStore(hTmpStore, 0);
#include "pch.h"
#include <list>
#include <Wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

static HRESULT RegBinaryToMem(CWixString &binaryStr, BYTE** ppBytes, DWORD *pSize);
static HRESULT SearchIssuerAndSerial(HCERTSTORE hMachineStore, CWixString &issuer, CWixString &serial, PCCERT_CONTEXT *ppCertContext);
static HRESULT SearchByFriendlyName(HCERTSTORE hMachineStore, LPCWSTR friendlyName, PCCERT_CONTEXT *ppCertContext);

extern "C" UINT __stdcall CertificateHashSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	PCCERT_CONTEXT pCertContext = nullptr;
	HCERTSTORE hStoreCollection = NULL;
	HCERTSTORE hTmpStore = NULL;
	std::list<HCERTSTORE> allStores;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPCWSTR pszStores[] = { L"MY", L"Root", L"AddressBook", L"AuthRoot", L"CertificateAuthority", L"TrustedPeople", L"TrustedPublisher" };

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_CertificateHashSearch");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_CertificateHashSearch'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_CertificateHashSearch'. Have you authored 'PanelSw:CertificateHashSearch' entries in WiX code?");
	
	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `CertName`, `FriendlyName`, `Issuer`, `SerialNumber` FROM `PSW_CertificateHashSearch`", &hView);
	ExitOnFailure(hr, "Failed to execute MSI SQL query");

	hStoreCollection = ::CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, NULL, 0, nullptr);
	ExitOnNullWithLastError(hStoreCollection, hr, "Failed opening certificate store");

	for (int i = 0; i < countof(pszStores); ++i)
	{
		hTmpStore = ::CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE, pszStores[i]);
		ExitOnNullWithLastError(hTmpStore, hr, "Failed opening '%ls' certificate store", pszStores[i]);

		bRes = ::CertAddStoreToCollection(hStoreCollection, hTmpStore, 0, 0);
		ExitOnNull(hTmpStore, hr, E_FAIL, "Failed adding '%ls' certificate store to collection", pszStores[i]);

		allStores.push_back(hTmpStore);
		hTmpStore = NULL;
	}

	// Loop
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");
		
		CWixString id; // Property name i.e. "SELF_SIGNED_PFX"
		CWixString certName; // Subject
		CWixString friendlyName; // Subject
		CWixString issuer; // Issuer, binary format
		CWixString serial; // Serial, binary format
		BYTE certHash[20];
		DWORD hashSize = 20;
		WCHAR hashHex[20 * 2 + 1];

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)id);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)certName);
		ExitOnFailure(hr, "Failed to get CertName.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)friendlyName);
		ExitOnFailure(hr, "Failed to get CertName.");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)issuer);
		ExitOnFailure(hr, "Failed to get Issuer.");
		hr = WcaGetRecordFormattedString(hRecord, 5, (LPWSTR*)serial);
		ExitOnFailure(hr, "Failed to get Serial.");

		if (certName.IsNullOrEmpty() && (issuer.IsNullOrEmpty() || serial.IsNullOrEmpty()) && friendlyName.IsNullOrEmpty())
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "All certificate parameters are empty for '%ls'", (LPCWSTR)id);
			continue;
		}

		if (!certName.IsNullOrEmpty())
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Searching certificate with subject '%ls'", (LPCWSTR)certName);
			pCertContext = ::CertFindCertificateInStore(hStoreCollection, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, (LPCWSTR)certName, pCertContext);
			if (!pCertContext)
			{
				hr = HRESULT_FROM_WIN32(::GetLastError());
			}
		}
		if (!pCertContext && !issuer.IsNullOrEmpty() && !serial.IsNullOrEmpty()) // Expecting both Issuer and serial.
		{
			hr = SearchIssuerAndSerial(hStoreCollection, issuer, serial, &pCertContext);
			ExitOnFailure(hr, "Failed searching certificate by issuer and serial number");
		}
		if (!pCertContext && !friendlyName.IsNullOrEmpty())
		{
			hr = SearchByFriendlyName(hStoreCollection, friendlyName, &pCertContext);
			ExitOnFailure(hr, "Failed searching certificate by friendly name '%ls'", (LPCWSTR)friendlyName);
		}

		if (!pCertContext && (SUCCEEDED(hr) || (hr == CRYPT_E_NOT_FOUND)))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Did not find matching certificate in machine MY store.");
			hr = S_FALSE;
			continue;
		}
		ExitOnNull(pCertContext, hr, hr, "Failed finding matching certificate");

		bRes = ::CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, certHash, &hashSize);
		ExitOnNullWithLastError(bRes, hr, "Failed getting certificate hash");

		hr = StrHexEncode(certHash, hashSize, hashHex, countof(hashHex));
		ExitOnFailure(hr, "Failed encoding certificate hash");

		hr = WcaSetProperty(id, hashHex);
		ExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)id);

		bRes = ::CertFreeCertificateContext(pCertContext);
		ExitOnNullWithLastError(bRes, hr, "Failed releasing certificate");
		pCertContext = nullptr;
	}
	hr = S_OK;

LExit:
	for (HCERTSTORE &hStore: allStores)
	{
		::CertCloseStore(hStore, 0);
	}
	if (hTmpStore)
	{
		::CertCloseStore(hTmpStore, 0);
	}
	if (hStoreCollection)
	{
		::CertCloseStore(hStoreCollection, 0);
	}
	if (pCertContext)
	{
		::CertFreeCertificateContext(pCertContext);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT SearchIssuerAndSerial(HCERTSTORE hMachineStore, CWixString &issuer, CWixString &serial, PCCERT_CONTEXT *ppCertContext)
{
	HRESULT hr = S_OK;
	CERT_INFO certInfo;
	DWORD buffSize = 0;
	BYTE *buffer1 = nullptr;
	BYTE *buffer2 = nullptr;

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Searching certificate with issuer '%ls' and serial '%ls'", (LPCWSTR)issuer, (LPCWSTR)serial);

	::ZeroMemory(&certInfo, sizeof(certInfo));

	hr = RegBinaryToMem(issuer, &buffer1, &buffSize);
	ExitOnFailure(hr, "Failed parsing issuer as binary");

	certInfo.Issuer.cbData = buffSize;
	certInfo.Issuer.pbData = buffer1;

	buffSize = 0;
	hr = RegBinaryToMem(serial, &buffer2, &buffSize);
	ExitOnFailure(hr, "Failed parsing serial as binary");

	certInfo.SerialNumber.cbData = buffSize;
	certInfo.SerialNumber.pbData = buffer2;

	*ppCertContext = ::CertFindCertificateInStore(hMachineStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, &certInfo, nullptr);
	if (!*ppCertContext)		
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		if (hr == CRYPT_E_NOT_FOUND)
		{
			hr = S_FALSE;
			ExitFunction();
		}
	}

LExit:
	ReleaseMem(buffer1);
	ReleaseMem(buffer2);

	return hr;
}

static HRESULT SearchByFriendlyName(HCERTSTORE hMachineStore, LPCWSTR friendlyName, PCCERT_CONTEXT *ppCertContext)
{
	HRESULT hr = S_OK;
	PCCERT_CONTEXT pCertContext = nullptr;
	WCHAR wzFriendlyName[256] = { 0 };
	DWORD cbFriendlyName = sizeof(wzFriendlyName);

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Searching certificate with friendly name '%ls'", friendlyName);
	while (NULL != (pCertContext = ::CertFindCertificateInStore(hMachineStore, PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, pCertContext)))
	{
		wzFriendlyName[0] = NULL;
		cbFriendlyName = sizeof(wzFriendlyName);

		if (::CertGetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, reinterpret_cast<BYTE*>(wzFriendlyName), &cbFriendlyName) &&
			*wzFriendlyName && cbFriendlyName && (::wcscmp(friendlyName, wzFriendlyName) == 0))
		{
			*ppCertContext = pCertContext;
			pCertContext = nullptr;
			ExitFunction();
		}
	}

	// We get here on error on no-match.
	hr = HRESULT_FROM_WIN32(::GetLastError());
	if (hr == CRYPT_E_NOT_FOUND)
	{
		hr = S_FALSE;
		ExitFunction();
	}

LExit:
	if (pCertContext)
	{
		::CertFreeCertificateContext(pCertContext);
	}

	return hr;
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

#include "stdafx.h"
#include <WixString.h>
#include <pathutil.h>
#include <memutil.h>
#include <Wincrypt.h>
#include "FileOperations.h"
#pragma comment (lib, "Crypt32.lib")

#define SelfSignCertificate_QUERY L"SELECT `Id`, `Component_`, `X500`, `SubjectAltNames`, `Expiry`, `Password`, `DeleteOnCommit` FROM `PSW_SelfSignCertificate`"
enum SelfSignCertificateQuery { Id = 1, Component, X500, SubjectAltNames, Expiry, Password, DeleteOnCommit };

#define CERT_CONTAINER		L"Panel-SW.co.il"

extern "C" UINT __stdcall CreateSelfSignCertificate(MSIHANDLE hInstall) noexcept
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	HCRYPTPROV hCrypt = NULL;
	HCRYPTKEY hKey = NULL;
	DWORD nSize = 0;
	LPCWSTR errStr = nullptr;
	BYTE *name = nullptr;
	CERT_NAME_BLOB nameBlob;
	CRYPT_KEY_PROV_INFO keyInfo;
	PCCERT_CONTEXT pCertContext = nullptr;
	HCERTSTORE hStore = NULL;
	CRYPT_DATA_BLOB cryptBlob;
	CERT_ALT_NAME_ENTRY altName;
	CERT_ALT_NAME_INFO altNameInfo;
	CRYPT_DATA_BLOB extBlob;
	CERT_EXTENSION ext;
	CERT_EXTENSIONS extArray;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwRes = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	SYSTEMTIME startTime;
	FILETIME startFileTime;
	CFileOperations commitActions;
	CWixString szCustomActionData;

	::ZeroMemory(&nameBlob, sizeof(nameBlob));
	::ZeroMemory(&keyInfo, sizeof(keyInfo));
	::ZeroMemory(&ext, sizeof(ext));
	::ZeroMemory(&extArray, sizeof(extArray));
	::ZeroMemory(&altName, sizeof(altName));
	::ZeroMemory(&altNameInfo, sizeof(altNameInfo));
	::ZeroMemory(&extBlob, sizeof(extBlob));

	keyInfo.pwszProvName = nullptr;
	keyInfo.dwProvType = PROV_RSA_FULL;
	keyInfo.dwKeySpec = AT_KEYEXCHANGE;
	keyInfo.dwFlags = CRYPT_EXPORTABLE;
	keyInfo.pwszContainerName = CERT_CONTAINER;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_SelfSignCertificate");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_SelfSignCertificate'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_SelfSignCertificate'. Have you authored 'PanelSw:SelfSignCertificate' entries in WiX code?");
	
	// Execute view
	hr = WcaOpenExecuteView(SelfSignCertificate_QUERY, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", SelfSignCertificate_QUERY);

	::GetSystemTime(&startTime);
	bRes = ::SystemTimeToFileTime(&startTime, &startFileTime);
	ExitOnNullWithLastError(bRes, hr, "Failed converting SYSTEMTIME: %i-%i-%i %i:%i:%i", startTime.wYear, startTime.wMonth, startTime.wDay, startTime.wHour, startTime.wMinute, startTime.wMonth);

	::CryptAcquireContext(&hCrypt, CERT_CONTAINER, nullptr, PROV_RSA_FULL, CRYPT_DELETEKEYSET | CRYPT_SILENT); // Make sure the container is removed

	bRes = ::CryptAcquireContext(&hCrypt, CERT_CONTAINER, nullptr, PROV_RSA_FULL, CRYPT_NEWKEYSET | CRYPT_SILENT);
	ExitOnNullWithLastError(bRes, hr, "Failed acquiring crypto context");

	bRes = ::CryptGenKey(hCrypt, AT_KEYEXCHANGE, 0x08000000 | CRYPT_EXPORTABLE, &hKey);
	ExitOnNullWithLastError(bRes, hr, "Failed generating crypto key");

	hStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY, 0, NULL, CERT_STORE_CREATE_NEW_FLAG, nullptr);
	ExitOnNullWithLastError(hStore, hr, "Failed creating certificate memory store");

	// Loop
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");
		
		CWixString szFile;
		SYSTEMTIME endTime;
		FILETIME endFileTime;
		CWixString x500; // Signed to, i.e. "CN=Panel-SW.com"
		CWixString subAltNames; // Subject Alternative Name, i.e. "Panel-SW.com"
		CWixString password;
		CWixString id; // Property name i.e. "SELF_SIGNED_PFX"
		CWixString component;
		int expiryDays = 0;
		int deleteOnCommit = 0;
		ULARGE_INTEGER expiry{ 0, 0 };
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, SelfSignCertificateQuery::Id, (LPWSTR*)id);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, SelfSignCertificateQuery::Component, (LPWSTR*)component);
		ExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, SelfSignCertificateQuery::X500, (LPWSTR*)x500);
		ExitOnFailure(hr, "Failed to get X500.");
		hr = WcaGetRecordFormattedString(hRecord, SelfSignCertificateQuery::SubjectAltNames, (LPWSTR*)subAltNames);
		ExitOnFailure(hr, "Failed to get SubjectAltNames.");
		hr = WcaGetRecordFormattedInteger(hRecord, SelfSignCertificateQuery::Expiry, &expiryDays);
		ExitOnFailure(hr, "Failed to get Expiry.");
		hr = WcaGetRecordFormattedString(hRecord, SelfSignCertificateQuery::Password, (LPWSTR*)password);
		ExitOnFailure(hr, "Failed to get Password.");
		hr = WcaGetRecordFormattedInteger(hRecord, SelfSignCertificateQuery::DeleteOnCommit, &deleteOnCommit);
		ExitOnFailure(hr, "Failed to get DeleteOnCommit.");

		// Test condition
		compAction = WcaGetComponentToDo(component);
		if ((compAction == WCA_TODO::WCA_TODO_UNINSTALL) || (compAction == WCA_TODO::WCA_TODO_UNKNOWN))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skipping creation of self-signed certificate '%ls' since component is neither installed nor repaired", (LPCWSTR)id);
			continue;
		}

		// Calculate endTime
		expiry.HighPart = startFileTime.dwHighDateTime;
		expiry.LowPart = startFileTime.dwLowDateTime;
		expiry.QuadPart += (expiryDays * 24 * 60 * 60 * 10000000ll); // 100 nano-Sec units
		endFileTime.dwHighDateTime = expiry.HighPart;
		endFileTime.dwLowDateTime = expiry.LowPart;

		bRes = ::FileTimeToSystemTime(&endFileTime, &endTime);
		ExitOnNullWithLastError(bRes, hr, "Failed converting FILETIME");

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Creating self-signed certificate '%ls', expired %i-%i-%i %i:%i:%i (%i days from today)", (LPCWSTR)x500, endTime.wYear, endTime.wMonth, endTime.wDay, endTime.wHour, endTime.wMinute, endTime.wSecond, expiryDays);
		
		bRes = ::CertStrToName(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, x500, CERT_X500_NAME_STR, nullptr, nullptr, &nSize, &errStr);
		ExitOnNullWithLastError(bRes, hr, "Failed getting size for converting x500 string to cert name: %ls", errStr);

		name = (BYTE*)MemAlloc(nSize, FALSE);
		ExitOnNull(name, hr, E_FAIL, "Failed allocating memory");

		bRes = ::CertStrToName(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, x500, CERT_X500_NAME_STR, nullptr, name, &nSize, &errStr);
		ExitOnNullWithLastError(bRes, hr, "Failed converting x500 string to cert name: %ls", errStr);

		nameBlob.cbData = nSize;
		nameBlob.pbData = name;

		// Subject Alternative Name
		if (!subAltNames.IsNullOrEmpty())
		{
			altName.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
			altName.pwszDNSName = (LPWSTR)subAltNames;

			altNameInfo.cAltEntry = 1;
			altNameInfo.rgAltEntry = &altName;

			bRes = ::CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_ALTERNATE_NAME, &altNameInfo, 0, nullptr, nullptr, &extBlob.cbData);
			ExitOnNullWithLastError(bRes, hr, "Failed encoding alternate name");

			extBlob.pbData = (BYTE*)MemAlloc(extBlob.cbData, FALSE);
			ExitOnNull(extBlob.pbData, hr, E_FAIL, "Failed allocating memory");

			bRes = ::CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_ALTERNATE_NAME, &altNameInfo, 0, nullptr, extBlob.pbData, &extBlob.cbData);
			ExitOnNullWithLastError(bRes, hr, "Failed encoding alternate name");

			ext.fCritical = FALSE;
			ext.pszObjId = szOID_SUBJECT_ALT_NAME2;
			ext.Value = extBlob;

			extArray.cExtension = 1;
			extArray.rgExtension = &ext;
		}


		pCertContext = ::CertCreateSelfSignCertificate(hCrypt, &nameBlob, 0, &keyInfo, nullptr, nullptr, &endTime, extArray.cExtension ? &extArray : nullptr);
		ExitOnNullWithLastError(pCertContext, hr, "Failed creating self signed certificate");

		bRes = ::CertAddCertificateContextToStore(hStore, pCertContext, CERT_STORE_ADD_ALWAYS, nullptr);
		ExitOnNullWithLastError(bRes, hr, "Failed adding certificate to store");

		::ZeroMemory(&cryptBlob, sizeof(cryptBlob));
		bRes = ::PFXExportCertStoreEx(hStore, &cryptBlob, password, nullptr, EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY | REPORT_NO_PRIVATE_KEY);
		ExitOnNullWithLastError(bRes, hr, "Failed exporting certificate memory store");

		cryptBlob.pbData = (BYTE*)MemAlloc(cryptBlob.cbData, FALSE);
		ExitOnNullWithLastError(cryptBlob.pbData, hr, "Failed allocating memory");

		bRes = ::PFXExportCertStoreEx(hStore, &cryptBlob, password, nullptr, EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY | REPORT_NO_PRIVATE_KEY);
		ExitOnNullWithLastError(bRes, hr, "Failed exporting certificate memory store");

		hr = PathCreateTempFile(nullptr, L"PFX%05i.tmp", INFINITE, FILE_ATTRIBUTE_NORMAL, (LPWSTR*)szFile, nullptr);
		ExitOnFailure(hr, "Failed getting temporary file name");

		if (deleteOnCommit)
		{
			hr = commitActions.AddDeleteFile(szFile, CFileOperations::FileOperationsAttributes::IgnoreErrors | CFileOperations::FileOperationsAttributes::IgnoreMissingPath);
			ExitOnFailure(hr, "Failed adding commit action");
		}

		hFile = ::CreateFile(szFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		ExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed creating file");

		bRes = ::WriteFile(hFile, cryptBlob.pbData, cryptBlob.cbData, &nSize, nullptr);
		ExitOnNullWithLastError(bRes, hr, "Failed writing certificate to file");
		ExitOnNull((cryptBlob.cbData == nSize), hr, E_FAIL, "Failed writing certificate to file- mismatching size");

		hr = WcaSetProperty(id, szFile);
		ExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)id);

		// Clear for next round
		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;

		bRes = ::CertDeleteCertificateFromStore(pCertContext);
		ExitOnNullWithLastError(bRes, hr, "Failed removing certificate from store");

		bRes = ::CertFreeCertificateContext(pCertContext);
		ExitOnNullWithLastError(bRes, hr, "Failed releasing certificate");
		pCertContext = nullptr;

		ReleaseMem(cryptBlob.pbData);
		::ZeroMemory(&cryptBlob, sizeof(cryptBlob));

		ReleaseMem(name);
		name = nullptr;
		::ZeroMemory(&nameBlob, sizeof(nameBlob));

		ReleaseMem(extBlob.pbData);
		::ZeroMemory(&ext, sizeof(ext));
		::ZeroMemory(&extArray, sizeof(extArray));
		::ZeroMemory(&altName, sizeof(altName));
		::ZeroMemory(&altNameInfo, sizeof(altNameInfo));
		::ZeroMemory(&extBlob, sizeof(extBlob));
	}
	hr = S_OK;

	hr = commitActions.GetCustomActionData((LPWSTR*)szCustomActionData);
	ExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaSetProperty(L"CreateSelfSignCertificate_commit", szCustomActionData);
	ExitOnFailure(hr, "Failed setting property.");

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
	}
	if (hStore)
	{
		::CertCloseStore(hStore, 0);
	}
	if (pCertContext)
	{
	}
	if (hKey)
	{
		::CryptDestroyKey(hKey);
	}
	if (hCrypt)
	{
		::CryptReleaseContext(hCrypt, 0);
		hCrypt = NULL;
		::CryptAcquireContext(&hCrypt, CERT_CONTAINER, nullptr, PROV_RSA_FULL, CRYPT_DELETEKEYSET | CRYPT_SILENT); // Make sure the container is removed
	}

	ReleaseMem(cryptBlob.pbData);
	ReleaseMem(name);
	ReleaseMem(extBlob.pbData);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

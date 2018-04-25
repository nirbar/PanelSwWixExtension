#include "stdafx.h"
#include <WixString.h>
#include <memutil.h>
#include <Wincrypt.h>
#include "FileOperations.h"
#pragma comment (lib, "Crypt32.lib")

#define SelfSignCertificate_QUERY L"SELECT `Id`, `Component_`, `X500`, `Expiry`, `Password`, `DeleteOnCommit` FROM `PSW_SelfSignCertificate`"
enum SelfSignCertificateQuery { Id = 1, Component, X500, Expiry, Password, DeleteOnCommit };

#define CERT_CONTAINER		L"Panel-SW.co.il"

extern "C" UINT __stdcall CreateSelfSignCertificate(MSIHANDLE hInstall)
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
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwRes = ERROR_SUCCESS;
	WCHAR szTempPath[MAX_PATH + 1];
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	SYSTEMTIME startTime;
	FILETIME startFileTime;
	CFileOperations commitActions;
	CWixString szCustomActionData;

	::ZeroMemory(&nameBlob, sizeof(nameBlob));
	::ZeroMemory(&keyInfo, sizeof(keyInfo));

	keyInfo.pwszProvName = nullptr;
	keyInfo.dwProvType = PROV_RSA_FULL;
	keyInfo.dwKeySpec = AT_KEYEXCHANGE;
	keyInfo.dwFlags = CRYPT_EXPORTABLE;
	keyInfo.pwszContainerName = CERT_CONTAINER;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_SelfSignCertificate");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_SelfSignCertificate'. Have you authored 'PanelSw:SelfSignCertificate' entries in WiX code?");
	
	// Execute view
	hr = WcaOpenExecuteView(SelfSignCertificate_QUERY, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query '%ls'.", SelfSignCertificate_QUERY);

	::GetSystemTime(&startTime);
	bRes = ::SystemTimeToFileTime(&startTime, &startFileTime);
	BreakExitOnNullWithLastError(bRes, hr, "Failed converting SYSTEMTIME: %i-%i-%i %i:%i:%i", startTime.wYear, startTime.wMonth, startTime.wDay, startTime.wHour, startTime.wMinute, startTime.wMonth);

	dwRes = ::GetTempPath(MAX_PATH, szTempPath);
	BreakExitOnNullWithLastError(dwRes, hr, "Failed getting temporary folder");

	::CryptAcquireContext(&hCrypt, CERT_CONTAINER, nullptr, PROV_RSA_FULL, CRYPT_DELETEKEYSET | CRYPT_SILENT); // Make sure the container is removed

	bRes = ::CryptAcquireContext(&hCrypt, CERT_CONTAINER, nullptr, PROV_RSA_FULL, CRYPT_NEWKEYSET | CRYPT_SILENT);
	BreakExitOnNullWithLastError(bRes, hr, "Failed acquiring crypto context");

	bRes = ::CryptGenKey(hCrypt, AT_KEYEXCHANGE, 0x08000000 | CRYPT_EXPORTABLE, &hKey);
	BreakExitOnNullWithLastError(bRes, hr, "Failed generating crypto key");

	hStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY, 0, NULL, CERT_STORE_CREATE_NEW_FLAG, nullptr);
	BreakExitOnNullWithLastError(hStore, hr, "Failed creating certificate memory store");

	// Loop
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");
		
		WCHAR szFile[MAX_PATH + 1];
		SYSTEMTIME endTime;
		FILETIME endFileTime;
		CWixString x500; // Signed to, i.e. "CN=Panel-SW.com"
		CWixString password;
		CWixString id; // Property name i.e. "SELF_SIGNED_PFX"
		CWixString component;
		int expiryDays = 0;
		int deleteOnCommit = 0;
		ULARGE_INTEGER expiry{ 0, 0 };
		WCA_TODO compAction = WCA_TODO_UNKNOWN;

		hr = WcaGetRecordString(hRecord, SelfSignCertificateQuery::Id, (LPWSTR*)id);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, SelfSignCertificateQuery::Component, (LPWSTR*)component);
		BreakExitOnFailure(hr, "Failed to get Component.");
		hr = WcaGetRecordFormattedString(hRecord, SelfSignCertificateQuery::X500, (LPWSTR*)x500);
		BreakExitOnFailure(hr, "Failed to get X500.");
		hr = WcaGetRecordFormattedInteger(hRecord, SelfSignCertificateQuery::Expiry, &expiryDays);
		BreakExitOnFailure(hr, "Failed to get Expiry.");
		hr = WcaGetRecordFormattedString(hRecord, SelfSignCertificateQuery::Password, (LPWSTR*)password);
		BreakExitOnFailure(hr, "Failed to get Password.");
		hr = WcaGetRecordFormattedInteger(hRecord, SelfSignCertificateQuery::DeleteOnCommit, &deleteOnCommit);
		BreakExitOnFailure(hr, "Failed to get DeleteOnCommit.");

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
		BreakExitOnNullWithLastError(bRes, hr, "Failed converting FILETIME");

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Creating self-signed certificate '%ls', expired %i-%i-%i %i:%i:%i (%i days from today) (%I64u)", (LPCWSTR)x500, endTime.wYear, endTime.wMonth, endTime.wDay, endTime.wHour, endTime.wMinute, endTime.wSecond, expiryDays, expiry.QuadPart);
		
		bRes = ::CertStrToName(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, x500, CERT_X500_NAME_STR, nullptr, nullptr, &nSize, &errStr);
		BreakExitOnNullWithLastError(bRes, hr, "Failed getting size for converting x500 string to cert name: %ls", errStr);

		name = (BYTE*)MemAlloc(nSize, FALSE);
		BreakExitOnNull(name, hr, E_FAIL, "Failed allocating memory");

		bRes = ::CertStrToName(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, x500, CERT_X500_NAME_STR, nullptr, name, &nSize, &errStr);
		BreakExitOnNullWithLastError(bRes, hr, "Failed converting x500 string to cert name: %ls", errStr);

		nameBlob.cbData = nSize;
		nameBlob.pbData = name;

		pCertContext = ::CertCreateSelfSignCertificate(hCrypt, &nameBlob, 0, &keyInfo, nullptr, nullptr, &endTime, nullptr);
		BreakExitOnNullWithLastError(pCertContext, hr, "Failed creating self signed certificate");

		bRes = ::CertAddCertificateContextToStore(hStore, pCertContext, CERT_STORE_ADD_ALWAYS, nullptr);
		BreakExitOnNullWithLastError(bRes, hr, "Failed adding certificate to store");

		::ZeroMemory(&cryptBlob, sizeof(cryptBlob));
		bRes = ::PFXExportCertStoreEx(hStore, &cryptBlob, password, nullptr, EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY | REPORT_NO_PRIVATE_KEY);
		BreakExitOnNullWithLastError(bRes, hr, "Failed exporting certificate memory store");

		cryptBlob.pbData = (BYTE*)MemAlloc(cryptBlob.cbData, FALSE);
		BreakExitOnNullWithLastError(cryptBlob.pbData, hr, "Failed allocating memory");

		bRes = ::PFXExportCertStoreEx(hStore, &cryptBlob, password, nullptr, EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY | REPORT_NO_PRIVATE_KEY);
		BreakExitOnNullWithLastError(bRes, hr, "Failed exporting certificate memory store");

		dwRes = ::GetTempFileName(szTempPath, L"PFX", 0, szFile);
		BreakExitOnNullWithLastError(dwRes, hr, "Failed getting temporary file name");

		if (deleteOnCommit)
		{
			hr = commitActions.AddDeleteFile(szFile, CFileOperations::FileOperationsAttributes::IgnoreErrors | CFileOperations::FileOperationsAttributes::IgnoreMissingPath);
			BreakExitOnFailure(hr, "Failed adding commit action");
		}

		hFile = ::CreateFile(szFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		BreakExitOnNullWithLastError((hFile != INVALID_HANDLE_VALUE), hr, "Failed creating file");

		bRes = ::WriteFile(hFile, cryptBlob.pbData, cryptBlob.cbData, &nSize, nullptr);
		BreakExitOnNullWithLastError(bRes, hr, "Failed writing certificate to file");
		BreakExitOnNull((cryptBlob.cbData == nSize), hr, E_FAIL, "Failed writing certificate to file- mismatching size");

		hr = WcaSetProperty(id, szFile);
		BreakExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)id);

		// Clear for next round
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;

		bRes = ::CertDeleteCertificateFromStore(pCertContext);
		BreakExitOnNullWithLastError(bRes, hr, "Failed removing certificate from store");

		bRes = ::CertFreeCertificateContext(pCertContext);
		BreakExitOnNullWithLastError(bRes, hr, "Failed releasing certificate");
		pCertContext = nullptr;

		MemFree(cryptBlob.pbData);
		::ZeroMemory(&cryptBlob, sizeof(cryptBlob));

		MemFree(name);
		name = nullptr;
		::ZeroMemory(&nameBlob, sizeof(nameBlob));
	}
	hr = S_OK;

	hr = commitActions.GetCustomActionData((LPWSTR*)szCustomActionData);
	BreakExitOnFailure(hr, "Failed getting custom action data for deferred action.");
	hr = WcaSetProperty(L"CreateSelfSignCertificate_commit", szCustomActionData);
	BreakExitOnFailure(hr, "Failed setting property.");

LExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
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
	if (cryptBlob.pbData)
	{
		MemFree(cryptBlob.pbData);
	}
	if (name)
	{
		MemFree(name);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

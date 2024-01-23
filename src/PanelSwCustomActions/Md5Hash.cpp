#include "pch.h"
#include <Wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

#define MD5LEN  16

extern "C" UINT __stdcall Md5Hash(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	HCRYPTPROV hCrypt = NULL;
	HCRYPTPROV hHash = NULL;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPSTR szPlainAscii = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = WcaTableExists(L"PSW_Md5Hash");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_Md5Hash'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_Md5Hash'. Have you authored 'PanelSw:Md5Hash' entries in WiX code?");
	
	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Property_`, `Plain` FROM `PSW_Md5Hash`", &hView);
	ExitOnFailure(hr, "Failed to execute MSI SQL query");

	::CryptAcquireContext(&hCrypt, nullptr, nullptr, PROV_RSA_AES, CRYPT_DELETEKEYSET | CRYPT_SILENT); // Make sure the container is removed

	bRes = ::CryptAcquireContext(&hCrypt, nullptr, nullptr, PROV_RSA_AES, CRYPT_NEWKEYSET | CRYPT_SILENT);
	ExitOnNullWithLastError(bRes, hr, "Failed acquiring crypto context");

	// Loop
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");
		
		CWixString szProperty, szPlain;
		BYTE rgbHash[MD5LEN];
		WCHAR szHashed[2 * MD5LEN + 1];
		DWORD cbHash = 0;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szProperty);
		ExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)szPlain);
		ExitOnFailure(hr, "Failed to get Plain.");

		if (szPlain.IsNullOrEmpty())
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Plain is empty for '%ls'", (LPCWSTR)szProperty);
			continue;
		}

		// Restart hash
		if (hHash)
		{
			::CryptDestroyHash(hHash);
			hHash = NULL;
		}

		ReleaseNullMem(szPlainAscii);
		hr = szPlain.ToAnsiString(&szPlainAscii);
		ExitOnFailure(hr, "Failed allocating ANSI string");

		bRes = ::CryptCreateHash(hCrypt, CALG_MD5, 0, 0, &hHash);
		ExitOnNullWithLastError(bRes, hr, "Failed creating MD5 hash");

		bRes = ::CryptHashData(hHash, (LPCBYTE)szPlainAscii, strlen(szPlainAscii), 0);
		ExitOnNullWithLastError(bRes, hr, "Failed hashing text");

		cbHash = ARRAYSIZE(rgbHash);
		bRes = ::CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0);
		ExitOnNullWithLastError(bRes, hr, "Failed getting hashed text");

		hr = StrHexEncode(rgbHash, cbHash, szHashed, ARRAYSIZE(szHashed));
		ExitOnFailure(hr, "Failed to hex-encode MD5 hash");

		hr = WcaSetProperty((LPCWSTR)szProperty, szHashed);
		ExitOnFailure(hr, "Failed setting property '%ls'", (LPCWSTR)szProperty);
	}
	hr = S_OK;

LExit:
	ReleaseMem(szPlainAscii);
	if (hHash)
	{
		::CryptDestroyHash(hHash);
	}
	if (hCrypt)
	{
		::CryptReleaseContext(hCrypt, 0);
		hCrypt = NULL;
		::CryptAcquireContext(&hCrypt, nullptr, nullptr, PROV_RSA_AES, CRYPT_DELETEKEYSET | CRYPT_SILENT); // Make sure the container is removed
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

#include "stdafx.h"
#include <WixString.h>
#include <Sddl.h>
#include <map>
#pragma comment (lib, "Advapi32.lib")
using namespace std;

extern "C" UINT __stdcall AccountNames(MSIHANDLE hInstall) noexcept
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;
	map<LPCWSTR, LPCWSTR> mapSid2Property;
	PSID pSid = nullptr;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	mapSid2Property[SDDL_BUILTIN_ADMINISTRATORS] = L"BUILTIN_ADMINISTRATORS";
	mapSid2Property[SDDL_BUILTIN_GUESTS] = L"BUILTIN_GUESTS";
	mapSid2Property[SDDL_BUILTIN_USERS] = L"BUILTIN_USERS";
	mapSid2Property[SDDL_LOCAL_ADMIN] = L"LOCAL_ADMIN";
	mapSid2Property[SDDL_LOCAL_GUEST] = L"LOCAL_GUEST";
	mapSid2Property[SDDL_ACCOUNT_OPERATORS] = L"ACCOUNT_OPERATORS";
	mapSid2Property[SDDL_BACKUP_OPERATORS] = L"BACKUP_OPERATORS";
	mapSid2Property[SDDL_PRINTER_OPERATORS] = L"PRINTER_OPERATORS";
	mapSid2Property[SDDL_SERVER_OPERATORS] = L"SERVER_OPERATORS";
	mapSid2Property[SDDL_AUTHENTICATED_USERS] = L"AUTHENTICATED_USERS";
	mapSid2Property[SDDL_PERSONAL_SELF] = L"PERSONAL_SELF";
	mapSid2Property[SDDL_CREATOR_OWNER] = L"CREATOR_OWNER";
	mapSid2Property[SDDL_CREATOR_GROUP] = L"CREATOR_GROUP";
	mapSid2Property[SDDL_LOCAL_SYSTEM] = L"LOCAL_SYSTEM";
	mapSid2Property[SDDL_POWER_USERS] = L"POWER_USERS";
	mapSid2Property[SDDL_EVERYONE] = L"EVERYONE";
	mapSid2Property[SDDL_REPLICATOR] = L"REPLICATOR";
	mapSid2Property[SDDL_INTERACTIVE] = L"INTERACTIVE";
	mapSid2Property[SDDL_NETWORK] = L"NETWORK";
	mapSid2Property[SDDL_SERVICE] = L"SERVICE";
	mapSid2Property[SDDL_RESTRICTED_CODE] = L"RESTRICTED_CODE";
	mapSid2Property[SDDL_WRITE_RESTRICTED_CODE] = L"WRITE_RESTRICTED_CODE";
	mapSid2Property[SDDL_ANONYMOUS] = L"ANONYMOUS";
	mapSid2Property[SDDL_SCHEMA_ADMINISTRATORS] = L"SCHEMA_ADMINISTRATORS";
	mapSid2Property[SDDL_CERT_SERV_ADMINISTRATORS] = L"CERT_SERV_ADMINISTRATORS";
	mapSid2Property[SDDL_RAS_SERVERS] = L"RAS_SERVERS";
	mapSid2Property[SDDL_ENTERPRISE_ADMINS] = L"ENTERPRISE_ADMINS";
	mapSid2Property[SDDL_GROUP_POLICY_ADMINS] = L"GROUP_POLICY_ADMINS";
	mapSid2Property[SDDL_ALIAS_PREW2KCOMPACC] = L"ALIAS_PREW2KCOMPACC";
	mapSid2Property[SDDL_LOCAL_SERVICE] = L"LOCAL_SERVICE";
	mapSid2Property[SDDL_NETWORK_SERVICE] = L"NETWORK_SERVICE";
	mapSid2Property[SDDL_REMOTE_DESKTOP] = L"REMOTE_DESKTOP";
	mapSid2Property[SDDL_NETWORK_CONFIGURATION_OPS] = L"NETWORK_CONFIGURATION_OPS";
	mapSid2Property[SDDL_PERFMON_USERS] = L"PERFMON_USERS";
	mapSid2Property[SDDL_PERFLOG_USERS] = L"PERFLOG_USERS";
	mapSid2Property[SDDL_IIS_USERS] = L"IIS_USERS";
	mapSid2Property[SDDL_CRYPTO_OPERATORS] = L"CRYPTO_OPERATORS";
	mapSid2Property[SDDL_OWNER_RIGHTS] = L"OWNER_RIGHTS";
	mapSid2Property[SDDL_EVENT_LOG_READERS] = L"EVENT_LOG_READERS";
	mapSid2Property[SDDL_ENTERPRISE_RO_DCs] = L"ENTERPRISE_RO_DCs";
	mapSid2Property[SDDL_CERTSVC_DCOM_ACCESS] = L"CERTSVC_DCOM_ACCESS";
	mapSid2Property[SDDL_DOMAIN_ADMINISTRATORS] = L"DOMAIN_ADMINISTRATORS";
	mapSid2Property[SDDL_DOMAIN_GUESTS] = L"DOMAIN_GUESTS";
	mapSid2Property[SDDL_DOMAIN_USERS] = L"DOMAIN_USERS";
	mapSid2Property[SDDL_ENTERPRISE_DOMAIN_CONTROLLERS] = L"ENTERPRISE_DOMAIN_CONTROLLERS";
	mapSid2Property[SDDL_DOMAIN_DOMAIN_CONTROLLERS] = L"DOMAIN_DOMAIN_CONTROLLERS";
	mapSid2Property[SDDL_DOMAIN_COMPUTERS] = L"DOMAIN_COMPUTERS";

	for (const pair<LPCWSTR, LPCWSTR> &sidPair: mapSid2Property)
	{
		CWixString accountName;
		DWORD dwNameLen = 0;
		CWixString domainName;
		DWORD dwDomainLen = 0;
		SID_NAME_USE eUse;
		CWixString fullName;
		CWixString property;

		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "Converting SID '%ls'", sidPair.first);
		bRes = ::ConvertStringSidToSid(sidPair.first, &pSid);
		if (!bRes)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Failed converting SID '%ls': Error %u", sidPair.first, ::GetLastError());
			goto LForContinue;
		}

		bRes = ::LookupAccountSid(nullptr, pSid, (LPWSTR)accountName, &dwNameLen, (LPWSTR)domainName, &dwDomainLen, &eUse);
		if (!bRes && (::GetLastError() != ERROR_INSUFFICIENT_BUFFER))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Failed getting SID for '%ls': Error %u", sidPair.first, ::GetLastError());
			goto LForContinue;
		}

		hr = accountName.Allocate(dwNameLen);
		ExitOnFailure(hr, "Failed allocating memory");

		hr = domainName.Allocate(dwDomainLen);
		ExitOnFailure(hr, "Failed allocating memory");

		bRes = ::LookupAccountSid(nullptr, pSid, (LPWSTR)accountName, &dwNameLen, (LPWSTR)domainName, &dwDomainLen, &eUse);
		ExitOnNullWithLastError(bRes, hr, "Failed looking up SID");

		if (domainName.StrLen() > 0)
		{
			hr = fullName.Format(L"%s\\%s", (LPCWSTR)domainName, (LPCWSTR)accountName);
			ExitOnFailure(hr, "Failed formatting string");

			hr = property.Format(L"%s_DOMAIN", sidPair.second);
			ExitOnFailure(hr, "Failed formatting string");

			hr = WcaSetProperty(property, (LPCWSTR)domainName);
			ExitOnFailure(hr, "Failed setting property");
		}
		else
		{
			hr = fullName.Copy(accountName);
			ExitOnFailure(hr, "Failed copying string");
		}

		hr = property.Format(L"%s_NAME", sidPair.second);
		ExitOnFailure(hr, "Failed formatting string");

		hr = WcaSetProperty(property, (LPCWSTR)accountName);
		ExitOnFailure(hr, "Failed setting property");

		hr = WcaSetProperty(sidPair.second, (LPCWSTR)fullName);
		ExitOnFailure(hr, "Failed setting property");

	LForContinue:

		if (pSid)
		{
			::LocalFree(pSid);
			pSid = nullptr;
		}
	}

LExit:

	if (pSid)
	{
		::LocalFree(pSid);
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}
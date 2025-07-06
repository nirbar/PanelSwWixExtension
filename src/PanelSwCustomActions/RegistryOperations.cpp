#include "pch.h"
#include "RegistryOperations.h"
#include "..\CaCommon\SummaryStream.h"
using namespace ::com::panelsw::ca;
using namespace google::protobuf;

extern "C" UINT __stdcall RemoveRegistryValueSched(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	CRegistryOperations deferredCAD;
	CRegistryOperations rollbackCAD;
	PMSIHANDLE hView;
	PMSIHANDLE hRec;
	PMSIHANDLE hComponentView;
	PMSIHANDLE hComponentExecRec;
	LPBYTE pbData = nullptr;
	SIZE_T cbData = 0;
	HKEY hkKey = NULL;
	bool bIsPackageX64 = false;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure table PSW_RemoveRegistryValue exists.
	hr = WcaTableExists(L"PSW_RemoveRegistryValue");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_RemoveRegistryValue'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_RemoveRegistryValue'. Have you authored 'PanelSw:RemoveRegistryValue' entries in WiX code?");

	hr = WcaOpenExecuteView(L"SELECT `Component_`, `Root`, `Key`, `Name`, `View`, `Condition` FROM `PSW_RemoveRegistryValue`", &hView);
	ExitOnFailure(hr, "Failed to execute view");

	hr = WcaOpenView(L"SELECT `Attributes` FROM `Component` WHERE `Component` = ?", &hComponentView);
	ExitOnFailure(hr, "Failed to open view");

	hComponentExecRec = ::MsiCreateRecord(1);
	ExitOnNullWithLastError(hComponentExecRec, hr, "Failed to create record");

	hr = CSummaryStream::IsPackageX64();
	ExitOnFailure(hr, "Failed to determine package bitness");
	bIsPackageX64 = (hr == S_OK);

	while (E_NOMOREITEMS != (hr = WcaFetchRecord(hView, &hRec)))
	{
		ExitOnFailure(hr, "Failed to fetch record");
		ReleaseRegKey(hkKey);
		ReleaseNullMem(pbData);
		cbData = 0;

		// Get record.
		CWixString szComponent, szKey, szName, szCondition;
		msidbRegistryRoot nRoot = msidbRegistryRootLocalMachine;
		int nView = 0;
		HKEY hkRoot = NULL;
		DWORD dwType = 0;
		LPCWSTR szRegRoot = nullptr;
		LPCWSTR szRegView = nullptr;

		hr = WcaGetRecordString(hRec, 1, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component");
		hr = WcaGetRecordInteger(hRec, 2, (int*)&nRoot);
		ExitOnFailure(hr, "Failed to get Root");
		hr = WcaGetRecordFormattedString(hRec, 3, (LPWSTR*)szKey);
		ExitOnFailure(hr, "Failed to get Key");
		hr = WcaGetRecordFormattedString(hRec, 4, (LPWSTR*)szName);
		ExitOnFailure(hr, "Failed to get Name");
		hr = WcaGetRecordInteger(hRec, 5, &nView);
		ExitOnFailure(hr, "Failed to get View");
		hr = WcaGetRecordString(hRec, 6, (LPWSTR*)szCondition);
		ExitOnFailure(hr, "Failed to get Condition");

		if (!szComponent.IsNullOrEmpty())
		{
			WCA_TODO todo = WcaGetComponentToDo(szComponent);
			if (todo != WCA_TODO::WCA_TODO_UNINSTALL)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Skip removing registry value for component '%ls' because it isn't uninstalled", (LPCWSTR)szComponent);
				continue;
			}
		}
		if (!szCondition.IsNullOrEmpty())
		{
			MSICONDITION cond = ::MsiEvaluateCondition(hInstall, szCondition);
			switch (cond)
			{
			case MSICONDITION::MSICONDITION_NONE:
			case MSICONDITION::MSICONDITION_TRUE:
				break;
			case MSICONDITION::MSICONDITION_FALSE:
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Condition evaluated to false");
				continue;
			case MSICONDITION::MSICONDITION_ERROR:
				hr = E_FAIL;
				ExitOnFailure(hr, "Failed to evaluate condition");
			}
		}

		if (nView < 0)
		{
			if (bIsPackageX64 && !szComponent.IsNullOrEmpty())
			{
				PMSIHANDLE hComponentRec;
				int nAttributes = 0;

				hr = WcaSetRecordString(hComponentExecRec, 1, (LPCWSTR)szComponent);
				ExitOnFailure(hr, "Failed to set record");

				hr = WcaExecuteView(hComponentView, hComponentExecRec);
				ExitOnFailure(hr, "Failed to execute view");

				hr = WcaFetchSingleRecord(hComponentView, &hComponentRec);
				ExitOnFailure(hr, "Failed to fetch record");
				ExitOnNull((hr == S_OK), hr, E_INVALIDDATA, "Component '%ls' not found", (LPCWSTR)szComponent);

				hr = WcaGetRecordInteger(hComponentRec, 1, &nAttributes);
				ExitOnFailure(hr, "Failed to get Attributes");

				nView = (nAttributes & msidbComponentAttributes64bit) ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
			}
			else
			{
				nView = bIsPackageX64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
			}
		}
		szRegView = (nView == KEY_WOW64_64KEY) ? L"x64" : L"x86";

		if (nRoot < 0)
		{
			hr = CSummaryStream::IsUserContext();
			ExitOnFailure(hr, "Failed to get package context");
			nRoot = (hr == S_OK) ? msidbRegistryRootCurrentUser : msidbRegistryRootLocalMachine;
		}
		switch (nRoot)
		{
		case msidbRegistryRootClassesRoot:
			szRegRoot = L"HKEY_CLASSES_ROOT";
			hkRoot = HKEY_CLASSES_ROOT;
			break;
		case msidbRegistryRootCurrentUser:
			szRegRoot = L"HKEY_CURRENT_USER";
			hkRoot = HKEY_CURRENT_USER;
			break;
		case msidbRegistryRootLocalMachine:
			szRegRoot = L"HKEY_LOCAL_MACHINE";
			hkRoot = HKEY_LOCAL_MACHINE;
			break;
		case msidbRegistryRootUsers:
			szRegRoot = L"HKEY_USERS";
			hkRoot = HKEY_USERS;
			break;
		default:
			hr = E_INVALIDARG;
			ExitOnFailure(hr, "Illegal registry root %u", nRoot);
		}

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will delete registry value '%ls\\%ls\\@%ls' in %ls view", szRegRoot, (LPCWSTR)szKey, (LPCWSTR)szName, szRegView);
		hr = deferredCAD.AddDeleteValue(nRoot, nView, szKey, szName);
		ExitOnFailure(hr, "Failed to add data to CustomActionData");

		// Rollback data:
		hr = RegOpenEx(hkRoot, (LPCWSTR)szKey, KEY_READ, (nView == KEY_WOW64_64KEY) ? REG_KEY_64BIT : REG_KEY_32BIT, &hkKey);
		if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND) || (hr == E_NOTFOUND))
		{
			hr = rollbackCAD.AddDeleteKey(nRoot, nView, (LPCWSTR)szKey);
			ExitOnFailure(hr, "Failed to add rollback data");
		}
		else
		{
			DWORD dwCurrType = 0;

			hr = RegReadValue(hkKey, (LPCWSTR)szName, FALSE, &pbData, &cbData, &dwCurrType);
			if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND) || (hr == E_NOTFOUND))
			{
				hr = rollbackCAD.AddDeleteValue(nRoot, nView, (LPCWSTR)szKey, (LPCWSTR)szName);
				ExitOnFailure(hr, "Failed to add rollback data");
			}
			else if (SUCCEEDED(hr))
			{
				hr = rollbackCAD.AddCreateValue(nRoot, nView, (LPCWSTR)szKey, (LPCWSTR)szName, dwCurrType, pbData, cbData);
				ExitOnFailure(hr, "Failed to add rollback data");
			}
			ExitOnFailure(hr, "Failed to query current data");

			ReleaseNullMem(pbData);
			cbData = 0;
		}
	}

	hr = rollbackCAD.DoDeferredAction(L"RemoveRegistryValueRollback");
	ExitOnFailure(hr, "Failed to schedule rollback action");

	hr = deferredCAD.DoDeferredAction(L"RemoveRegistryValueExec");
	ExitOnFailure(hr, "Failed to schedule deferred action");

LExit:
	ReleaseMem(pbData);
	ReleaseRegKey(hkKey);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

HRESULT CRegistryOperations::AddDeleteKey(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	RegistryOperationsDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CRegistryOperations", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new RegistryOperationsDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_action(RegistryAction::deleteKey);
	pDetails->set_root(nRoot);
	pDetails->set_view(dwView);
	pDetails->set_key(szKey, WSTR_BYTE_SIZE(szKey));

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CRegistryOperations::AddDeleteValue(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey, LPCWSTR szName)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	RegistryOperationsDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CRegistryOperations", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new RegistryOperationsDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_action(RegistryAction::deleteValue);
	pDetails->set_root(nRoot);
	pDetails->set_view(dwView);
	pDetails->set_key(szKey, WSTR_BYTE_SIZE(szKey));
	pDetails->set_name(szName, WSTR_BYTE_SIZE(szName));

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CRegistryOperations::AddCreateKey(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	RegistryOperationsDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CRegistryOperations", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new RegistryOperationsDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_action(RegistryAction::createKey);
	pDetails->set_root(nRoot);
	pDetails->set_view(dwView);
	pDetails->set_key(szKey, WSTR_BYTE_SIZE(szKey));

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CRegistryOperations::AddCreateValue(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey, LPCWSTR szName, DWORD dwType, LPCBYTE pbData, DWORD cbData)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	RegistryOperationsDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CRegistryOperations", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new RegistryOperationsDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	pDetails->set_action(RegistryAction::setValue);
	pDetails->set_root(nRoot);
	pDetails->set_view(dwView);
	pDetails->set_key(szKey, WSTR_BYTE_SIZE(szKey));
	pDetails->set_name(szName, WSTR_BYTE_SIZE(szName));
	pDetails->set_type(dwType);
	pDetails->set_value(pbData, cbData);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CRegistryOperations::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	BOOL bRes = TRUE;
	RegistryOperationsDetails details;
	LPCWSTR szFrom = nullptr;
	LPCWSTR szTo = nullptr;
	HKEY hkRoot = NULL;
	DWORD dwView = 0;
	LPCWSTR szKey = nullptr;
	LPCWSTR szName = nullptr;
	DWORD dwType = 0;
	LPCBYTE pbData = nullptr;
	DWORD cbData = 0;
	msidbRegistryRoot nRoot = msidbRegistryRootLocalMachine;
	CWixString szUserName;
	HANDLE hUserToken = NULL;
	PROFILEINFO profileInfo = {};

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking RegistryOperationsDetails");

	dwView = details.view();
	szKey = (LPCWSTR)details.key().data();
	szName = (LPCWSTR)details.name().data();
	dwType = details.type();
	pbData = (LPCBYTE)details.value().data();
	cbData = details.value().size();

	nRoot = (msidbRegistryRoot)details.root();
	switch (nRoot)
	{
	case msidbRegistryRootClassesRoot:
		hkRoot = HKEY_CLASSES_ROOT;
		break;
	case msidbRegistryRootCurrentUser:
		hr = GetUserToken(&hUserToken, (LPWSTR*)szUserName);
		ExitOnFailure(hr, "Failed to get user token");

		profileInfo.dwSize = sizeof(profileInfo);
		profileInfo.lpUserName = (LPWSTR)szUserName;

		bRes = ::LoadUserProfile(hUserToken, &profileInfo);
		ExitOnNullWithLastError(bRes, hr, "Failed to get current user's profile");
		hkRoot = (HKEY)profileInfo.hProfile;
		break;
	case msidbRegistryRootLocalMachine:
		hkRoot = HKEY_LOCAL_MACHINE;
		break;
	case msidbRegistryRootUsers:
		hkRoot = HKEY_USERS;
		break;
	default:
		break;
	}

	switch (details.action())
	{
	case RegistryAction::createKey:
		hr = CreateKey(hkRoot, dwView, szKey);
		ExitOnFailure(hr, "Failed to create key");
		break;
	case RegistryAction::setValue:
		hr = SetValue(hkRoot, dwView, szKey, szName, dwType, pbData, cbData);
		ExitOnFailure(hr, "Failed to set value");
		break;
	case RegistryAction::deleteKey:
		hr = DeleteKey(hkRoot, dwView, szKey);
		ExitOnFailure(hr, "Failed to create key");
		break;
	case RegistryAction::deleteValue:
		hr = DeleteValue(hkRoot, dwView, szKey, szName);
		ExitOnFailure(hr, "Failed to set value");
		break;
	default:
		hr = E_INVALIDSTATE;
		ExitOnFailure(hr, "Illegal registry action");
		break;
	}

LExit:
	if (hUserToken && profileInfo.hProfile)
	{
		::UnloadUserProfile(hUserToken, profileInfo.hProfile);
	}
	ReleaseHandle(hUserToken);
	
	return hr;
}

REG_KEY_BITNESS CRegistryOperations::View2Bitness(DWORD dwView) const
{
	return (dwView == KEY_WOW64_32KEY) ? REG_KEY_BITNESS::REG_KEY_32BIT
		: (dwView == KEY_WOW64_64KEY) ? REG_KEY_BITNESS::REG_KEY_64BIT
		: REG_KEY_BITNESS::REG_KEY_DEFAULT;
}

HRESULT CRegistryOperations::DeleteKey(HKEY hkRoot, DWORD dwView, LPCWSTR szKey)
{
	HRESULT hr = S_OK;
	REG_KEY_BITNESS kbKeyBitness = View2Bitness(dwView);

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Deleting registry key '%ls'", szKey);
	hr = RegDelete(hkRoot, szKey, kbKeyBitness, TRUE);
	ExitOnFailure(hr, "Failed to delete registry key");

LExit:
	return hr;
}

HRESULT CRegistryOperations::CreateKey(HKEY hkRoot, DWORD dwView, LPCWSTR szKey)
{
	REG_KEY_BITNESS kbKeyBitness = View2Bitness(dwView);
	HRESULT hr = S_OK;
	HKEY hkNew = NULL;

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Creating registry key '%ls'", szKey);
	hr = RegCreateEx(hkRoot, szKey, KEY_ALL_ACCESS, kbKeyBitness, FALSE, nullptr, &hkNew, nullptr);
	ExitOnFailure(hr, "Failed to create registry key");

LExit:
	ReleaseRegKey(hkNew);

	return hr;
}

HRESULT CRegistryOperations::DeleteValue(HKEY hkRoot, DWORD dwView, LPCWSTR szKey, LPCWSTR szName)
{
	HRESULT hr = S_OK;
	REG_KEY_BITNESS kbKeyBitness = View2Bitness(dwView);
	HKEY hkSubkey = NULL;

	hr = RegOpenEx(hkRoot, szKey, KEY_ALL_ACCESS, kbKeyBitness, &hkSubkey);
	if ((hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND) || (hr == E_NOTFOUND))
	{
		hr = S_OK;
		ExitFunction();
	}
	ExitOnFailure(hr, "Failed to open registry key");

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Deleting registry value '%ls\\@%ls'", szKey, szName);
	hr = RegWriteString(hkSubkey, szName, nullptr);
	ExitOnFailure(hr, "Failed to delete registry value");

LExit:
	ReleaseRegKey(hkSubkey);
	
	return hr;
}

HRESULT CRegistryOperations::SetValue(HKEY hkRoot, DWORD dwView, LPCWSTR szKey, LPCWSTR szName, DWORD dwType, LPCBYTE pbData, DWORD cbData)
{
	HRESULT hr = S_OK;
	REG_KEY_BITNESS kbKeyBitness = View2Bitness(dwView);
	HKEY hkSubkey = NULL;

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Setting registry value '%ls\\@%ls'", szKey, szName);
	hr = RegCreateEx(hkRoot, szKey, KEY_ALL_ACCESS, kbKeyBitness, FALSE, nullptr, &hkSubkey, nullptr);
	ExitOnFailure(hr, "Failed to create registry key");

	hr = RegSetValueExW(hkSubkey, szName, 0, dwType, pbData, cbData);
	ExitOnFailure(hr, "Failed to create registry value");

LExit:
	ReleaseRegKey(hkSubkey);
	
	return hr;
}

HRESULT CRegistryOperations::AddRecreateHierarchy(msidbRegistryRoot nRoot, DWORD dwView, LPCWSTR szKey)
{
	HRESULT hr = S_OK;
	REG_KEY_BITNESS kbKeyBitness = View2Bitness(dwView);
	HKEY hkRoot = NULL;
	HKEY hkSubkey = NULL;
	CWixString szSubKey;
	CWixString szName;
	BYTE* pbData = nullptr;
	SIZE_T cbData = 0;

	switch (nRoot)
	{
	case msidbRegistryRootClassesRoot:
		hkRoot = HKEY_CLASSES_ROOT;
		break;
	case msidbRegistryRootCurrentUser:
		hkRoot = HKEY_CURRENT_USER;
		break;
	case msidbRegistryRootLocalMachine:
		hkRoot = HKEY_LOCAL_MACHINE;
		break;
	case msidbRegistryRootUsers:
		hkRoot = HKEY_USERS;
		break;
	default:
		hr = E_INVALIDARG;
		ExitOnFailure(hr, "Illegal registry root %u", nRoot);
	}

	hr = AddDeleteKey(nRoot, dwView, szKey);
	ExitOnFailure(hr, "Failed to schedule registry key deletion");

	hr = RegOpenEx(hkRoot, szKey, GENERIC_READ, kbKeyBitness, &hkSubkey);
	if ((hr == E_NOTFOUND) || (hr == E_FILENOTFOUND) || (hr == E_PATHNOTFOUND))
	{
		hr = S_OK;
		ExitFunction();
	}
	ExitOnFailure(hr, "Failed to open registry key");

	for (DWORD i = 0; (hr = RegKeyEnum(hkSubkey, i, (LPWSTR*)szSubKey)) != E_NOMOREITEMS; ++i)
	{
		CWixString szSubFullName;
		ExitOnFailure(hr, "Failed to enumerate subkeys");

		hr = szSubFullName.Format(L"%ls\\%ls", (LPCWSTR)szKey, (LPCWSTR)szSubKey);
		ExitOnFailure(hr, "Failed to format string");

		hr = AddRecreateHierarchy(nRoot, dwView, szSubFullName);
		ExitOnFailure(hr, "Failed to schedule subkey recreation '%ls'", (LPCWSTR)szSubFullName);
	}
	hr = S_OK;

	for (DWORD i = 0; (hr = RegValueEnum(hkSubkey, i, (LPWSTR*)szName, nullptr)) != E_NOMOREITEMS; ++i)
	{
		DWORD dwType = 0;
		ExitOnFailure(hr, "Failed to enumerate subvalues");
		ReleaseNullMem(pbData);
		cbData = 0;

		hr = RegReadValue(hkSubkey, szName, FALSE, &pbData, &cbData, &dwType);
		ExitOnFailure(hr, "Failed to read registry value");

		hr = AddCreateValue(nRoot, dwView, (LPCWSTR)szKey, (LPCWSTR)szName, dwType, pbData, cbData);
		ExitOnFailure(hr, "Failed to schedule value recreation '%ls\\@%ls'", (LPCWSTR)szKey, (LPCWSTR)szName);
	}
	hr = S_OK;

LExit:
	ReleaseRegKey(hkSubkey);
	ReleaseMem(pbData);

	return hr;
}

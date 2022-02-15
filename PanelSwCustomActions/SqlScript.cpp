#include "SqlScript.h"
#include "FileOperations.h"
#include "../CaCommon/WixString.h"
#include "../CaCommon/SqlQuery.h"
#include <wcautil.h>
#include <memutil.h>
#include "google\protobuf\any.h"
using namespace com::panelsw::ca;
using namespace google::protobuf;

enum SqlExecOn
{
	Install = 1
	, InstallRollback = Install * 2
	, Uninstall = InstallRollback * 2
	, UninstallRollback = Uninstall * 2
	, Reinstall = UninstallRollback * 2
	, ReinstallRollback = Reinstall * 2
};

static HRESULT ReadBinary(LPCWSTR szBinaryKey, LPCWSTR szQueryId, CWixString *pszQuery);
static HRESULT ReplaceStrings(CWixString *pszQuery, LPCWSTR szQueryId);

extern "C" UINT __stdcall SqlScript(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	DWORD dwRes = 0;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	LPWSTR szCustomActionData = nullptr;
	CSqlScript deferredCA;
	CSqlScript rollbackCA;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	// Ensure tables exist.
	hr = WcaTableExists(L"PSW_SqlScript");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_SqlScript'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_SqlScript'. Have you authored 'PanelSw:SqlScript' entries in WiX code?");
	hr = WcaTableExists(L"PSW_SqlScript_Replacements");
	ExitOnFailure(hr, "Failed to check if table exists 'PSW_SqlScript_Replacements'");
	ExitOnNull((hr == S_OK), hr, E_FAIL, "Table does not exist 'PSW_SqlScript_Replacements'. Have you authored 'PanelSw:SqlScript' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(L"SELECT `Id`, `Component_`, `Server`, `Instance`, `Database`, `Username`, `Password`, `Binary_`, `On`, `ErrorHandling`, `Port`, `Encrypted`, `ConnectionString` FROM `PSW_SqlScript` ORDER BY `Order`", &hView);
	ExitOnFailure(hr, "Failed to execute SQL query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szConnectionString;
		CWixString szId, szComponent, szServer, szInstance, szDatabase, szUsername, szPassword, szBinary, szEncrypted;
		CWixString szQuery;
		int nOn = 0;
		int errorHandling = ErrorHandling::fail;
		WCA_TODO compAction = WCA_TODO_UNKNOWN;
		int nPort = 0;
		int bEncrypted = 0;

		hr = WcaGetRecordString(hRecord, 1, (LPWSTR*)szId);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, 2, (LPWSTR*)szComponent);
		ExitOnFailure(hr, "Failed to get Component_.");
		hr = WcaGetRecordFormattedString(hRecord, 3, (LPWSTR*)szServer);
		ExitOnFailure(hr, "Failed to get Server.");
		hr = WcaGetRecordFormattedString(hRecord, 4, (LPWSTR*)szInstance);
		ExitOnFailure(hr, "Failed to get Instance.");
		hr = WcaGetRecordFormattedString(hRecord, 5, (LPWSTR*)szDatabase);
		ExitOnFailure(hr, "Failed to get Database.");
		hr = WcaGetRecordFormattedString(hRecord, 6, (LPWSTR*)szUsername);
		ExitOnFailure(hr, "Failed to get Username.");
		hr = WcaGetRecordFormattedString(hRecord, 7, (LPWSTR*)szPassword);
		ExitOnFailure(hr, "Failed to get Password.");
		hr = WcaGetRecordString(hRecord, 8, (LPWSTR*)szBinary);
		ExitOnFailure(hr, "Failed to get Binary_.");
		hr = WcaGetRecordInteger(hRecord, 9, &nOn);
		ExitOnFailure(hr, "Failed to get On.");
		hr = WcaGetRecordInteger(hRecord, 10, &errorHandling);
		ExitOnFailure(hr, "Failed to get ErrorHandling.");
		hr = WcaGetRecordFormattedInteger(hRecord, 11, &nPort);
		ExitOnFailure(hr, "Failed to get Port.");
		hr = WcaGetRecordFormattedString(hRecord, 12, (LPWSTR*)szEncrypted);
		ExitOnFailure(hr, "Failed to get Encrypted.");
		bEncrypted = (szEncrypted.EqualsIgnoreCase(L"true") || szEncrypted.EqualsIgnoreCase(L"yes") || szEncrypted.Equals(L"1"));
		hr = WcaGetRecordFormattedString(hRecord, 13, (LPWSTR*)szConnectionString);
		ExitOnFailure(hr, "Failed to get ConnectionString.");

		ExitOnNull(!szBinary.IsNullOrEmpty(), hr, E_INVALIDARG, "Binary key is empty");
		ExitOnNull(!szId.IsNullOrEmpty(), hr, E_INVALIDARG, "Id is empty");
		ExitOnNull(!szComponent.IsNullOrEmpty(), hr, E_INVALIDARG, "Component is empty");
		ExitOnNull((!szServer.IsNullOrEmpty() || !szConnectionString.IsNullOrEmpty()), hr, E_INVALIDARG, "Server and ConnectionString are both empty");
		if (szUsername.IsNullOrEmpty())
		{
			szPassword.SecureRelease();
		}

		hr = ReadBinary(szBinary, szId, &szQuery);
		ExitOnFailure(hr, "Failed reading SQL script");

		// Handle:
		// Server\Instance
		// Server\...\Instance (multiple backslash)
		// Server\ (default instance)
		if (szInstance.IsNullOrEmpty())
		{
			DWORD dwInstIndex = 0;

			dwInstIndex = szServer.RFind(L'\\');
			if ((dwInstIndex != INFINITE) && (dwInstIndex < szServer.StrLen()))
			{
				// Copy instance name
				hr = szInstance.Copy(dwInstIndex + 1 + (LPCWSTR)szServer);
				ExitOnFailure(hr, "Failed copying instance name.");

				// Terminate server name on first backslash
				dwInstIndex = szServer.Find(L'\\');
				const_cast<LPWSTR>((LPCWSTR)szServer)[dwInstIndex] = NULL;
			}
		}

		// Test condition
		compAction = WcaGetComponentToDo(szComponent);
		switch (compAction)
		{
		case WCA_TODO::WCA_TODO_INSTALL:
			if (nOn & SqlExecOn::Install)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute SQL script '%ls'", (LPCWSTR)szBinary);
				hr = deferredCA.AddExec(szConnectionString, szServer, szInstance, nPort, bEncrypted, szDatabase, szUsername, szPassword, szQuery, (::com::panelsw::ca::ErrorHandling)errorHandling);
				ExitOnFailure(hr, "Failed scheduling '%ls' SQL script", (LPCWSTR)szBinary);
			}
			if (nOn & SqlExecOn::InstallRollback)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute SQL script '%ls' on rollback", (LPCWSTR)szBinary);
				hr = rollbackCA.AddExec(szConnectionString, szServer, szInstance, nPort, bEncrypted, szDatabase, szUsername, szPassword, szQuery, (::com::panelsw::ca::ErrorHandling)errorHandling);
				ExitOnFailure(hr, "Failed scheduling '%ls' SQL script", (LPCWSTR)szBinary);
			}
			break;

		case WCA_TODO::WCA_TODO_REINSTALL:
			if (nOn & SqlExecOn::Reinstall)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute SQL script '%ls'", (LPCWSTR)szBinary);
				hr = deferredCA.AddExec(szConnectionString, szServer, szInstance, nPort, bEncrypted, szDatabase, szUsername, szPassword, szQuery, (::com::panelsw::ca::ErrorHandling)errorHandling);
				ExitOnFailure(hr, "Failed scheduling '%ls' SQL script", (LPCWSTR)szBinary);
			}
			if (nOn & SqlExecOn::ReinstallRollback)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute SQL script '%ls' on rollback", (LPCWSTR)szBinary);
				hr = rollbackCA.AddExec(szConnectionString, szServer, szInstance, nPort, bEncrypted, szDatabase, szUsername, szPassword, szQuery, (::com::panelsw::ca::ErrorHandling)errorHandling);
				ExitOnFailure(hr, "Failed scheduling '%ls' SQL script", (LPCWSTR)szBinary);
			}
			break;

		case WCA_TODO::WCA_TODO_UNINSTALL:
			if (nOn & SqlExecOn::Uninstall)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute SQL script '%ls'", (LPCWSTR)szBinary);
				hr = deferredCA.AddExec(szConnectionString, szServer, szInstance, nPort, bEncrypted, szDatabase, szUsername, szPassword, szQuery, (::com::panelsw::ca::ErrorHandling)errorHandling);
				ExitOnFailure(hr, "Failed scheduling '%ls' SQL script", (LPCWSTR)szBinary);
			}
			if (nOn & SqlExecOn::UninstallRollback)
			{
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Will execute SQL script '%ls' on rollback", (LPCWSTR)szBinary);
				hr = rollbackCA.AddExec(szConnectionString, szServer, szInstance, nPort, bEncrypted, szDatabase, szUsername, szPassword, szQuery, (::com::panelsw::ca::ErrorHandling)errorHandling);
				ExitOnFailure(hr, "Failed scheduling '%ls' SQL script", (LPCWSTR)szBinary);
			}
			break;

		case WCA_TODO::WCA_TODO_UNKNOWN:
			WcaLog(LOGMSG_STANDARD, "Component '%ls' action is unknown. Skipping execution of '%ls'.", (LPCWSTR)szComponent, (LPCWSTR)szBinary);
			break;
		}

		szPassword.SecureRelease();
	}
	hr = S_OK;

	// Rollback actions
	if (rollbackCA.HasActions())
	{
		hr = rollbackCA.GetCustomActionData(&szCustomActionData);
		ExitOnFailure(hr, "Failed getting custom action data for rollback.");
		hr = WcaDoDeferredAction(L"PSW_SqlScriptRollback", szCustomActionData, 1);
		ExitOnFailure(hr, "Failed setting rollback action data.");
		ReleaseNullStr(szCustomActionData);
	}

	// Deferred action
	if (deferredCA.HasActions())
	{
		hr = deferredCA.GetCustomActionData(&szCustomActionData);
		ExitOnFailure(hr, "Failed getting custom action data for rollback.");
		hr = WcaDoDeferredAction(L"PSW_SqlScriptExec", szCustomActionData, 1);
		ExitOnFailure(hr, "Failed setting rollback action data.");
		ReleaseNullStr(szCustomActionData);
	}

LExit:
	ReleaseMem(szCustomActionData);

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT ReadBinary(LPCWSTR szBinaryKey, LPCWSTR szQueryId, CWixString* pszQuery)
{
	HRESULT hr = S_OK;
	CWixString szMsiQuery;
	std::map<std::string, std::string> replacements;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	BYTE* pbData = nullptr;
	DWORD cbData = 0;
	FileRegexDetails::FileEncoding encoding = FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_None;

	hr = szMsiQuery.Format(L"SELECT `Data` FROM `Binary` WHERE `Name`='%s'", szBinaryKey);
	ExitOnFailure(hr, "Failed to format string");

	hr = WcaOpenExecuteView((LPCWSTR)szMsiQuery, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szMsiQuery);

	hr = WcaFetchRecord(hView, &hRecord);
	ExitOnFailure(hr, "Failed to fetch binary record.");

	hr = WcaGetRecordStream(hRecord, 1, &pbData, &cbData);
	ExitOnFailure(hr, "Failed to ready Binary.Data for certificate.");

	// Ensure null-termination. scoped for local use of pbData1
	{
		cbData += 2;
		BYTE* pbData1 = (LPBYTE)MemReAlloc(pbData, cbData, TRUE);
		ExitOnNull(pbData1, hr, E_FAIL, "Failed reallocating memory");
		pbData = pbData1;
	}

	encoding = CFileOperations::DetectEncoding(pbData, cbData);
	if (encoding == FileRegexDetails::FileEncoding::FileRegexDetails_FileEncoding_MultiByte)
	{
		hr = pszQuery->Format(L"%hs", pbData);
		ExitOnFailure(hr, "Failed to copy SQL script to string. Is binary file '%ls' multibyte-encoded?", szBinaryKey);
	}
	else
	{
		hr = pszQuery->Copy((LPCWSTR)pbData);
		ExitOnFailure(hr, "Failed to copy SQL script to string. Is binary file '%ls' unicode-encoded?", szBinaryKey);
	}

	hr = ReplaceStrings(pszQuery, szQueryId);
	ExitOnFailure(hr, "Failed to replacing strings in SQL script '%ls'", szBinaryKey);

LExit:
	ReleaseMem(pbData);

	return hr;
}

static HRESULT ReplaceStrings(CWixString* pszQuery, LPCWSTR szQueryId)
{
	HRESULT hr = S_OK;
	CWixString szMsiQuery;
	std::map<std::string, std::string> replacements;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = szMsiQuery.Format(L"SELECT `Text`, `Replacement` FROM `PSW_SqlScript_Replacements` WHERE `SqlScript_`='%s' ORDER BY `Order`", szQueryId);
	ExitOnFailure(hr, "Failed to format string");

	hr = WcaOpenExecuteView((LPCWSTR)szMsiQuery, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query '%ls'.", (LPCWSTR)szMsiQuery);

	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		CWixString szText, szReplacement;

		hr = WcaGetRecordFormattedString(hRecord, 1, (LPWSTR*)szText);
		ExitOnFailure(hr, "Failed to get Text.");
		hr = WcaGetRecordFormattedString(hRecord, 2, (LPWSTR*)szReplacement);
		ExitOnFailure(hr, "Failed to get Replacement.");

		if (!szText.IsNullOrEmpty())
		{
			hr = pszQuery->ReplaceAll(szText, szReplacement);
			ExitOnFailure(hr, "Failed to replace strings in SQL script.");
		}
	}
	hr = S_OK;

LExit:

	return hr;
}

// Copied from WiX function ScaSqlStrsReadScripts
HRESULT CSqlScript::SplitScript(SqlScriptDetails* pDetails, LPCWSTR pwzScript)
{
	DWORD cchScript = ::wcslen(pwzScript);
	LPCWSTR pwz = nullptr;
	DWORD cch = 0;
	HRESULT hr = S_OK;

	while (cchScript && pwzScript && *pwzScript)
	{
		// strip off leading whitespace
		while (cchScript && *pwzScript && iswspace(*pwzScript))
		{
			++pwzScript;
			--cchScript;
		}

		Assert(0 <= cchScript);

		// if there is a SQL comment remove it
		while (cchScript && L'/' == *pwzScript && L'*' == *(pwzScript + 1))
		{
			// go until end of comment
			while (cchScript && *pwzScript && *(pwzScript + 1) && !(L'*' == *pwzScript && L'/' == *(pwzScript + 1)))
			{
				++pwzScript;
				--cchScript;
			}

			Assert(2 <= cchScript);

			if (2 <= cchScript)
			{
				// to account for */ at end
				pwzScript += 2;
				cchScript -= 2;
			}

			Assert(0 <= cchScript);

			// strip off any new leading whitespace
			while (cchScript && *pwzScript && iswspace(*pwzScript))
			{
				++pwzScript;
				--cchScript;
			}
		}

		while (cchScript && L'-' == *pwzScript && L'-' == *(pwzScript + 1))
		{
			// go past the new line character
			while (cchScript && *pwzScript && L'\n' != *pwzScript)
			{
				++pwzScript;
				--cchScript;
			}

			Assert(0 <= cchScript);

			if (cchScript && L'\n' == *pwzScript)
			{
				++pwzScript;
				--cchScript;
			}

			Assert(0 <= cchScript);

			// strip off any new leading whitespace
			while (cchScript && *pwzScript && iswspace(*pwzScript))
			{
				++pwzScript;
				--cchScript;
			}
		}

		Assert(0 <= cchScript);

		// try to isolate a "GO" keyword and count the characters in the SQL string
		pwz = pwzScript;
		cch = 0;
		CWixString szSubQuery(pwzScript);
		while (cchScript && *pwz)
		{
			//skip past comment lines that might have "go" in them
			//note that these comments are "in the middle" of our function,
			//or possibly at the end of a line
			if (cchScript && L'-' == *pwz && L'-' == *(pwz + 1))
			{
				// skip past chars until the new line character
				while (cchScript && *pwz && (L'\n' != *pwz))
				{
					++pwz;
					++cch;
					--cchScript;
				}
			}

			//skip past comment lines of form /* to */ that might have "go" in them
			//note that these comments are "in the middle" of our function,
			//or possibly at the end of a line
			if (cchScript && L'/' == *pwz && L'*' == *(pwz + 1))
			{
				// skip past chars until the new line character
				while (cchScript && *pwz && *(pwz + 1) && !((L'*' == *pwz) && (L'/' == *(pwz + 1))))
				{
					++pwz;
					++cch;
					--cchScript;
				}

				if (2 <= cchScript)
				{
					// to account for */ at end
					pwz += 2;
					cch += 2;
					cchScript -= 2;
				}
			}

			// Skip past strings that may be part of the SQL statement that might have a "go" in them
			if (cchScript && L'\'' == *pwz)
			{
				++pwz;
				++cch;
				--cchScript;

				// Skip past chars until the end of the string
				while (cchScript && *pwz && !(L'\'' == *pwz))
				{
					++pwz;
					++cch;
					--cchScript;
				}
			}

			// Skip past strings that may be part of the SQL statement that might have a "go" in them
			if (cchScript && L'\"' == *pwz)
			{
				++pwz;
				++cch;
				--cchScript;

				// Skip past chars until the end of the string
				while (cchScript && *pwz && !(L'\"' == *pwz))
				{
					++pwz;
					++cch;
					--cchScript;
				}
			}

			// if "GO" is isolated
			if ((pwzScript == pwz || iswspace(*(pwz - 1))) &&
				(L'G' == *pwz || L'g' == *pwz) &&
				(L'O' == *(pwz + 1) || L'o' == *(pwz + 1)) &&
				(0 == *(pwz + 2) || iswspace(*(pwz + 2))))
			{
				hr = szSubQuery.Copy(pwzScript, pwz - pwzScript);
				ExitOnFailure(hr, "Failed copying string");

				//TODO Nir- was: *pwz = 0; // null terminate the SQL string on the "G"
				pwz += 2;
				cchScript -= 2;
				break;   // found "GO" now add SQL string to list
			}

			++pwz;
			++cch;
			--cchScript;
		}

		Assert(0 <= cchScript);

		if (0 < cch) //don't process if there's nothing to process
		{
			// replace tabs with spaces
			hr = szSubQuery.ReplaceAll(L"\t", L" ");
			ExitOnFailure(hr, "Failed replacing tabs with spaces");

			// strip off whitespace at the end of the script string
			for (LPCWSTR pwzErase = pwzScript + cch - 1; pwzScript < pwzErase && iswspace(*pwzErase); pwzErase--)
			{
				//TODO Nir- was: *(pwzErase) = 0;
				cch--;
			}
		}

		if (0 < cch)
		{
			pDetails->add_scripts((LPCWSTR)szSubQuery, WSTR_BYTE_SIZE((LPCWSTR)szSubQuery));
		}

		pwzScript = pwz;
	}

LExit:
	return hr;
}

HRESULT CSqlScript::AddExec(LPCWSTR szConnectionString, LPCWSTR szServer, LPCWSTR szInstance, USHORT nPort, bool bEncrypted, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, LPCWSTR szScript, com::panelsw::ca::ErrorHandling errorHandling)
{
	HRESULT hr = S_OK;
	::com::panelsw::ca::Command* pCmd = nullptr;
	SqlScriptDetails* pDetails = nullptr;
	::std::string* pAny = nullptr;
	bool bRes = true;

	hr = AddCommand("CSqlScript", &pCmd);
	ExitOnFailure(hr, "Failed to add command");

	pDetails = new SqlScriptDetails();
	ExitOnNull(pDetails, hr, E_FAIL, "Failed allocating details");

	hr = SplitScript(pDetails, szScript);
	ExitOnFailure(hr, "Failed splitting script");

	pDetails->set_connectionstring(szConnectionString, WSTR_BYTE_SIZE(szConnectionString));
	pDetails->set_server(szServer, WSTR_BYTE_SIZE(szServer));
	pDetails->set_instance(szInstance, WSTR_BYTE_SIZE(szInstance));
	pDetails->set_port(nPort);
	pDetails->set_encrypted(bEncrypted);
	pDetails->set_database(szDatabase, WSTR_BYTE_SIZE(szDatabase));
	pDetails->set_username(szUser, WSTR_BYTE_SIZE(szUser));
	pDetails->set_password(szPassword, WSTR_BYTE_SIZE(szPassword));
	pDetails->set_errorhandling(errorHandling);

	pAny = pCmd->mutable_details();
	ExitOnNull(pAny, hr, E_FAIL, "Failed allocating any");

	bRes = pDetails->SerializeToString(pAny);
	ExitOnNull(bRes, hr, E_FAIL, "Failed serializing command details");

LExit:
	return hr;
}

HRESULT CSqlScript::DeferredExecute(const ::std::string& command)
{
	HRESULT hr = S_OK;
	DWORD exitCode = 0;
	BOOL bRes = TRUE;
	SqlScriptDetails details;
	LPWSTR szError = nullptr;
	CSqlConnection sqlConn;

	bRes = details.ParseFromString(command);
	ExitOnNull(bRes, hr, E_INVALIDARG, "Failed unpacking SqlScriptDetails");

	WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing %i SQL scripts", details.scripts_size());
	for (int i = 0; i < details.scripts_size(); ++i)
	{
	LRetry:
		ReleaseNullStr(szError);

		// Failure and error handling on either connection or query failure
		if (!sqlConn.IsConnected())
		{
			LPCWSTR szConnStr = (LPCWSTR)details.connectionstring().data();
			if (szConnStr && *szConnStr)
			{
				hr = sqlConn.Connect(szConnStr, &szError);
			}
			else
			{
				hr = sqlConn.Connect((LPCWSTR)details.server().data(), (LPCWSTR)details.instance().data(), details.port(), (LPCWSTR)details.database().data(), (LPCWSTR)details.username().data(), (LPCWSTR)details.password().data(), details.encrypted(), &szError);
			}
		}
		if (sqlConn.IsConnected())
		{
			hr = ExecuteOne(sqlConn, (LPCWSTR)details.scripts(i).data(), &szError);
		}

		if (FAILED(hr))
		{
			switch (details.errorhandling())
			{
			case ErrorHandling::fail:
			default:
				// Will fail downstairs.
				break;

			case ErrorHandling::ignore:
				WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Ignoring SQL script failure 0x%08X", hr);
				hr = S_FALSE;
				break;

			case ErrorHandling::prompt:
			{
				HRESULT hrOp = hr;
				PMSIHANDLE hRec;
				UINT promptResult = IDABORT;

				hRec = ::MsiCreateRecord(3);
				ExitOnNull(hRec, hr, E_FAIL, "Failed creating record");

				hr = WcaSetRecordInteger(hRec, 1, 27005);
				ExitOnFailure(hr, "Failed setting record integer");

				if (szError && *szError)
				{
					hr = WcaSetRecordString(hRec, 2, szError);
					ExitOnFailure(hr, "Failed setting record integer");
				}

				promptResult = WcaProcessMessage((INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_ERROR | MB_ABORTRETRYIGNORE | MB_DEFBUTTON1 | MB_ICONERROR), hRec);
				switch (promptResult)
				{
				case IDABORT:
				case IDCANCEL:
				default: // Probably silent (result 0)
					WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User aborted on SQL failure (error code 0x%08X)", hrOp);
					hr = hrOp;
					break;

				case IDRETRY:
					WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User chose to retry on SQL failure (error code 0x%08X)", hrOp);
					hr = S_OK;
					goto LRetry;

				case IDIGNORE:
					WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User ignored SQL failure (error code 0x%08X)", hrOp);
					hr = S_FALSE;
					break;
				}
				break;
			}
			}
			ExitOnFailure(hr, "Failed to execute SQL query");
		}
	}

LExit:
	ReleaseStr(szError);

	return hr;
}

HRESULT CSqlScript::ExecuteOne(const CSqlConnection& sqlConn, LPCWSTR szScript, LPWSTR* pszError)
{
	HRESULT hr = S_OK;
	LPWSTR szError = nullptr;
	CSqlQuery sqlQuery;

	hr = sqlQuery.ExecuteQuery(sqlConn, szScript, nullptr, &szError);
	ExitOnFailure(hr, "Failed to execute SQL string");

LExit:
	if (szError && *szError && pszError)
	{
		*pszError = szError;
		szError = nullptr;
	}

	ReleaseStr(szError);

	return hr;
}
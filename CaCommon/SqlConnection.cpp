#include "SqlConnection.h"

CSqlConnection::CSqlConnection()
{
	SQLRETURN sr = SQL_SUCCESS;
    HRESULT hr = S_OK;

	sr = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv_);
	ExitOnSqlError(sr, hr, "Failed allocating SQL environment handle");

    sr = SQLSetEnvAttr(hEnv_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    ExitOnOdbcError(sr, hEnv_, SQL_HANDLE_ENV, hr, "Failed setting SQL ODBC version");

    sr = SQLAllocHandle(SQL_HANDLE_DBC, hEnv_, &hDbc_);
    ExitOnOdbcError(sr, hEnv_, SQL_HANDLE_ENV, hr, "Failed allocating SQL DB handle");

LExit:
	return;
}

CSqlConnection::~CSqlConnection()
{
    if (hDbc_)
    {
        if (bConnected_)
        {
            SQLDisconnect(hDbc_);
        }
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc_);
    }

    if (hEnv_)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv_);
    }
}

// https://docs.microsoft.com/en-us/sql/relational-databases/native-client/applications/using-connection-string-keywords-with-sql-server-native-client
HRESULT CSqlConnection::Connect(LPCWSTR szServer, LPCWSTR szInstance, USHORT nPort, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, bool bEncrypted, LPWSTR* pszError)
{
    CWixString szConnString;
    CWixString szServerTmp;
    HRESULT hr = S_OK;

    ExitOnNull((szServer && *szServer), hr, E_FAIL, "Server cannot be null");
    
    hr = szServerTmp.Copy(szServer);
    ExitOnFailure(hr, "Failed alloacting string");

    if (szInstance && *szInstance)
    {
        hr = szServerTmp.AppnedFormat(L"\\%s", szInstance);
        ExitOnFailure(hr, "Failed appending string");
    }
    else if (nPort > 0)
    {
        hr = szServerTmp.AppnedFormat(L",%u", nPort);
        ExitOnFailure(hr, "Failed appending string");
    }

    // Server\Instance,port
    hr = szConnString.Format(L"Driver={SQL Server}; Server={%s};", (LPCWSTR)szServerTmp);
    ExitOnFailure(hr, "Failed alloacting string");

    if (szDatabase && *szDatabase)
    {
        hr = szConnString.AppnedFormat(L" Database={%s};", szDatabase);
        ExitOnFailure(hr, "Failed appending string");
    }

    if (bEncrypted)
    {
        hr = szConnString.AppnedFormat(L" Encrypt=yes;");
        ExitOnFailure(hr, "Failed appending string");
    }
    else
    {
        hr = szConnString.AppnedFormat(L" Encrypt=no;");
        ExitOnFailure(hr, "Failed appending string");
    }

    // User/password last to log obfuscated connection string
    if (szUser && *szUser)
    {
        hr = szConnString.AppnedFormat(L" Trusted_Connection=no; UID={%s};", szUser);
        ExitOnFailure(hr, "Failed appending string");

        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Connection string '%ls PWD={******};'", (LPCWSTR)szConnString);
        if (szPassword && *szPassword)
        {
            hr = szConnString.AppnedFormat(L" PWD={%s};", szPassword);
            ExitOnFailure(hr, "Failed appending string");
        }
    }
    else
    {
        hr = szConnString.AppnedFormat(L" Trusted_Connection=yes;");
        ExitOnFailure(hr, "Failed appending string");
  
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Connection string '%ls'", (LPCWSTR)szConnString);
    }

    hr = Connect(szConnString, pszError);
    ExitOnFailure(hr, "Failed connecting to SQL server");

LExit:
    return hr;
}


HRESULT CSqlConnection::Connect(LPWSTR szConnectionString, LPWSTR* pszError)
{
    HRESULT hr = S_OK;
    SQLRETURN sr = SQL_SUCCESS;

    ExitOnNull(hDbc_, hr, E_FAIL, "SQL DB handle is invalid");

    if (bConnected_)
    {
        sr = SQLDisconnect(hDbc_);
        ExitOnOdbcErrorWithText(sr, hDbc_, SQL_HANDLE_DBC, hr, pszError, "Failed disconnecting database");

        bConnected_ = false;
    }

    sr = SQLDriverConnect(hDbc_, nullptr, szConnectionString, SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    ExitOnOdbcErrorWithText(sr, hDbc_, SQL_HANDLE_DBC, hr, pszError, "Failed connecting to SQL DB");

    bConnected_ = true;
    szConnectionString_.Copy(szConnectionString);

LExit:
    return hr;
}

bool CSqlConnection::IsConnected() const
{
    return bConnected_;
}

LPCWSTR CSqlConnection::ConnectionString() const
{
    return (LPCWSTR)szConnectionString_;
}

const SQLHDBC CSqlConnection::DatabaseHandle() const
{
    return hDbc_;
}

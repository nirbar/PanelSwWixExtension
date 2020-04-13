#include "SqlQuery.h"
#include <memutil.h>
#include <strutil.h>

CSqlQuery::~CSqlQuery()
{
    if (hStmt_)
    {
        SQLFreeStmt(hStmt_, SQL_CLOSE);
    }
}

HRESULT CSqlQuery::ExecuteQuery(const CSqlConnection &sqlConn, LPCWSTR szQuery, LPWSTR* pszResult, LPWSTR* pszError)
{
    HRESULT hr = S_OK;
    SQLRETURN sr = SQL_SUCCESS;
    SQLSMALLINT nColumns = 0;
    SQLINTEGER nRows = 0;
    LPWSTR szQueryTemp = nullptr;
    SQLLEN nDataSize = 0;
    CWixString szResult;
    CWixString szChunk;

    if (pszResult)
    {
        *pszResult = nullptr;
    }
    ExitOnNull(sqlConn.DatabaseHandle(), hr, E_FAIL, "SQL database handle is invalid");

    hr = StrAllocString(&szQueryTemp, szQuery, 0);
    ExitOnFailure(hr, "Failed copying query");

    if (!hStmt_)
    {
        sr = SQLAllocHandle(SQL_HANDLE_STMT, sqlConn.DatabaseHandle(), &hStmt_);
        ExitOnOdbcError(sr, sqlConn.DatabaseHandle(), SQL_HANDLE_DBC, hr, "Failed getting data size");
    }

    sr = SQLExecDirect(hStmt_, szQueryTemp, SQL_NTS);
    ExitOnOdbcErrorWithText(sr, hStmt_, SQL_HANDLE_STMT, hr, pszError, "Failed executing query");

    if (SQL_SUCCEEDED(SQLRowCount(hStmt_, &nRows)) && (nRows >= 0))
    {
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "%i rows were affected by query", nRows);
    }

    sr = SQLNumResultCols(hStmt_, &nColumns);
    ExitOnOdbcErrorWithText(sr, hStmt_, SQL_HANDLE_STMT, hr, pszError, "Failed getting result column count");

    if (nColumns <= 0)
    {
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Query yielded no results");
        ExitFunction();
    }
    if (nColumns > 1)
    {
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Query yielded %i columns. Returning first result only", nColumns);
    }

    sr = SQLFetch(hStmt_);
    ExitOnOdbcErrorWithText(sr, hStmt_, SQL_HANDLE_STMT, hr, pszError, "Failed fetching result");

    // Allocate chunk
    hr = szChunk.Allocate(DATA_CHUNK_SIZE / sizeof(WCHAR));
    ExitOnFailure(hr, "Failed allocting memory");

    // Get data in chunks
    do
    {
        nDataSize = 0;
        ((LPWSTR)szChunk)[0] = NULL;

        sr = SQLGetData(hStmt_, 1, SQL_C_WCHAR, (LPWSTR)szChunk, DATA_CHUNK_SIZE, &nDataSize);
        ExitOnOdbcErrorWithText(sr, hStmt_, SQL_HANDLE_STMT, hr, pszError, "Failed reading result");
        
        if ((nDataSize == SQL_NULL_DATA) || (nDataSize == 0) || (sr == SQL_NO_DATA))
        {
            break;
        }

        hr = szResult.AppnedFormat(L"%s", szChunk); // Wary of results containing '%' specifiers
        ExitOnFailure(hr, "Failed allocting memory");

        if ((nDataSize != SQL_NO_TOTAL) && (nDataSize <= DATA_CHUNK_SIZE))
        {
            break;
        }
    } while (true);

    hr = StrAllocString(pszResult, (LPCWSTR)szResult, 0);
    ExitOnFailure(hr, "Failed allocting memory");

LExit:
    ReleaseStr(szQueryTemp);

    return hr;
}

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

HRESULT CSqlQuery::ExecuteQuery(CSqlConnection& sqlConn, LPCWSTR szQuery, LPWSTR* pszResult, LPWSTR* pszError)
{
    HRESULT hr = S_OK;
    SQLRETURN sr = SQL_SUCCESS;
    BINDING* pBinding = nullptr;
    SQLSMALLINT nColumns = 0;
    SQLINTEGER nRows = 0;
    LPWSTR szQueryTemp = nullptr;

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

    sr = SQLNumResultCols(hStmt_, &nColumns);
    ExitOnOdbcErrorWithText(sr, hStmt_, SQL_HANDLE_STMT, hr, pszError, "Failed getting result column count");

    sr = SQLRowCount(hStmt_, &nRows);
    ExitOnOdbcErrorWithText(sr, hStmt_, SQL_HANDLE_STMT, hr, pszError, "Failed getting result row count");

    if ((nRows <= 0) || (nColumns <= 0))
    {
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Query yielded no results");
        ExitFunction();
    }
    if ((nRows > 1) || (nColumns > 1))
    {
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Query yielded %i columns and %i rows. Returning first result only", nColumns, nRows);
    }

    hr = AllocateBinding(&pBinding);
    ExitOnFailure(hr, "Failed binding result");

    sr = SQLFetch(hStmt_);
    ExitOnOdbcErrorWithText(sr, hStmt_, SQL_HANDLE_STMT, hr, pszError, "Failed fetching result");

    // Not logging result here as it may be secret
    if (pszResult && (pBinding->indPtr != SQL_NULL_DATA) && !pBinding->wszBuffer)
    {
        *pszResult = pBinding->wszBuffer;
        pBinding->wszBuffer = nullptr;
    }

LExit:
    if (pBinding)
    {
        ReleaseStr(pBinding->wszBuffer);
        ReleaseMem(pBinding);
    }
    ReleaseStr(szQueryTemp);

    return hr;
}

HRESULT CSqlQuery::AllocateBinding(BINDING **ppBinding)
{
    BINDING* pBindingTmp = nullptr; 
    HRESULT hr = S_OK;
    SQLRETURN sr = SQL_SUCCESS;
    SQLLEN nDataSize = 0;

    *ppBinding = nullptr;
    ExitOnNull(hStmt_, hr, E_FAIL, "SQL statement handle is invalid");

    pBindingTmp = (BINDING*)MemAlloc(sizeof(BINDING), true);
    ExitOnNull(pBindingTmp, hr, E_FAIL, "Failed allocating memory")

    sr = SQLColAttribute(hStmt_, 1, SQL_DESC_DISPLAY_SIZE, nullptr, 0, nullptr, &nDataSize);
    ExitOnOdbcError(sr, hStmt_, SQL_HANDLE_STMT, hr, "Failed getting data size");

    hr = StrAlloc(&pBindingTmp->wszBuffer, nDataSize + 1);
    ExitOnFailure(hr, "Failed allocating memory");

    sr = SQLBindCol(hStmt_, 1, SQL_C_TCHAR, (SQLPOINTER)pBindingTmp->wszBuffer, (nDataSize + 1) * sizeof(WCHAR), &pBindingTmp->indPtr);
    ExitOnOdbcError(sr, hStmt_, SQL_HANDLE_STMT, hr, "Failed binding column data");

    *ppBinding = pBindingTmp;
    pBindingTmp = nullptr;

LExit:
    if (pBindingTmp)
    {
        ReleaseStr(pBindingTmp->wszBuffer);
        ReleaseMem(pBindingTmp);
    }

    return hr;
}
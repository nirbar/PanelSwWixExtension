#pragma once
#include "SqlClientBase.h"
#include "SqlConnection.h"

class CSqlQuery : public CSqlClientBase
{
public:
    virtual ~CSqlQuery() noexcept;

    HRESULT ExecuteQuery(const CSqlConnection &sqlConn, LPCWSTR szQuery, LPWSTR* pszResult, LPWSTR *pszError = nullptr) noexcept;

private:

    HSTMT hStmt_ = NULL;
    const int DATA_CHUNK_SIZE = 2000;
};


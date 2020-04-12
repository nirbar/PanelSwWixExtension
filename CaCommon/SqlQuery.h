#pragma once
#include "SqlClientBase.h"
#include "SqlConnection.h"

class CSqlQuery : public CSqlClientBase
{
public:
    ~CSqlQuery();

    HRESULT ExecuteQuery(CSqlConnection& sqlConn, LPCWSTR szQuery, LPWSTR* pszResult, LPWSTR *pszError = nullptr);

private:
    typedef struct {
        LPWSTR wszBuffer;
        SQLLEN indPtr;
    } BINDING;

    HRESULT AllocateBinding(BINDING **ppBinding);

    HSTMT hStmt_ = NULL;
};


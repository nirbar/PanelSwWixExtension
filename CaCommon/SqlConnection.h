#pragma once

#include "stdafx.h"
#include "SqlClientBase.h"
#include <sql.h>
#include "WixString.h"

class CSqlConnection : CSqlClientBase
{
public:

    CSqlConnection();
    ~CSqlConnection();

    HRESULT Connect(LPCWSTR szServer, LPCWSTR szInstance, USHORT nPort, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, bool bEncrypted, LPWSTR* pszError = nullptr);

    HRESULT Connect(LPWSTR szConnectionString, LPWSTR *pszError = nullptr);

    bool IsConnected() const;

    LPCWSTR ConnectionString() const;

    const SQLHDBC DatabaseHandle() const;

private:
    SQLHENV hEnv_ = NULL;
    SQLHDBC hDbc_ = NULL;
    bool bConnected_ = false;
    CWixString szConnectionString_;
};


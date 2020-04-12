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

    HRESULT Connect(LPCWSTR szServer, LPCWSTR szInstance, USHORT nPort, LPCWSTR szDatabase = nullptr, LPCWSTR szUser = nullptr, LPCWSTR szPassword = nullptr, bool bEncrypted = false);

    HRESULT Connect(LPWSTR szConnectionString);

    bool IsConnected() const;

    LPCWSTR ConnectionString() const;

    const SQLHDBC DatabaseHandle() const;

private:
    SQLHENV hEnv_ = NULL;
    SQLHDBC hDbc_ = NULL;
    bool bConnected_ = false;
    CWixString szConnectionString_;
};


#pragma once

#include "stdafx.h"
#include "SqlClientBase.h"
#include <sql.h>
#include "WixString.h"

class CSqlConnection : CSqlClientBase
{
public:

    CSqlConnection() noexcept;
    virtual ~CSqlConnection() noexcept;

    HRESULT Connect(LPCWSTR szServer, LPCWSTR szInstance, USHORT nPort, LPCWSTR szDatabase, LPCWSTR szUser, LPCWSTR szPassword, bool bEncrypted, LPWSTR* pszError = nullptr) noexcept;

    HRESULT Connect(LPCWSTR szConnectionString, LPWSTR *pszError = nullptr) noexcept;

    bool IsConnected() const noexcept;

    LPCWSTR ConnectionString() const noexcept;

    const SQLHDBC DatabaseHandle() const noexcept;

private:
    SQLHENV hEnv_ = NULL;
    SQLHDBC hDbc_ = NULL;
    bool bConnected_ = false;
    CWixString szConnectionString_;
};


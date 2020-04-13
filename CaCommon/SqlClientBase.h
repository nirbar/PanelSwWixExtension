#pragma once

#include "stdafx.h"
#include <sql.h>
#include <sqlext.h>

#define ExitOnSqlError(sqlCode, x, f, ...)                                          ExitOnNull(SQL_SUCCEEDED(sqlCode), x, HRESULT_FROM_WIN32(sqlCode), f, __VA_ARGS__)
#define ExitOnOdbcError(sqlCode, sqlHndl, sqlHndlType, x, f, ...)                   if (sqlCode != SQL_SUCCESS) LogDiagnosticRecord(sqlHndl, sqlHndlType, sqlCode); ExitOnSqlError(sqlCode, x, f, __VA_ARGS__)
#define ExitOnOdbcErrorWithText(sqlCode, sqlHndl, sqlHndlType, x, pText, f, ...)    if (sqlCode != SQL_SUCCESS) LogDiagnosticRecord(sqlHndl, sqlHndlType, sqlCode, pText); ExitOnSqlError(sqlCode, x, f, __VA_ARGS__)


class CSqlClientBase
{
protected:
    HRESULT LogDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, SQLRETURN nRetCode, LPWSTR *pszText = nullptr);
};


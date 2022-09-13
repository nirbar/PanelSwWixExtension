#include "SqlClientBase.h"
#include <wcautil.h>
#include <strutil.h>

HRESULT CSqlClientBase::LogDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, SQLRETURN nRetCode, LPWSTR* pszText)
{
    SQLSMALLINT iRec = 1;
    SQLINTEGER iError = 0;
    LPWSTR wzMessage = nullptr;
    SQLSMALLINT nMessageSize = 0;
    SQLSMALLINT nRequiredMessageSize = 0;
    WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
    HRESULT hr = S_OK;
    SQLRETURN sr = SQL_SUCCESS;

    ExitOnNull((nRetCode != SQL_INVALID_HANDLE), hr, E_FAIL, "Invalid SQL handle");

    do
    {
        sr = SQLGetDiagRec(hType, hHandle, iRec, wszState, &iError, wzMessage, nMessageSize, &nRequiredMessageSize);
        if (sr == SQL_NO_DATA)
        {
            break;
        }
        ExitOnNull(SQL_SUCCEEDED(sr), hr, sr, "Failed getting diagnostic data");

        if (nRequiredMessageSize >= nMessageSize)
        {
            hr = StrAlloc(&wzMessage, ++nRequiredMessageSize);
            ExitOnFailure(hr, "Failed allocating memory");

            nMessageSize = nRequiredMessageSize;
            continue;
        }

        // Skip "Data truncated" messages
        if (::wcscmp(wszState, L"01004") == 0)
        {
            ++iRec;
            continue;
        }

        if (pszText)
        {
            if (*pszText)
            {
                StrAllocConcatFormatted(pszText, L"\n%ls", wzMessage);
            }
            else
            {
                StrAllocConcat(pszText, wzMessage, 0);
            }
        }
        WcaLog(LOGLEVEL::LOGMSG_STANDARD, "%ls: %ls (%d)", wszState, wzMessage, iError);
        ++iRec;
    } while (true);

LExit:
    ReleaseStr(wzMessage);

    return hr;
}

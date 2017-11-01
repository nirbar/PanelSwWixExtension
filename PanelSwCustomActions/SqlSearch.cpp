
#include "stdafx.h"
#include "../CaCommon/WixString.h"
#include <sqlutil.h>
#include <memutil.h>
#include <atlbase.h>

#define SqlSearchQuery L"SELECT `Property_`, `Server`, `Instance`, `Database`, `Username`, `Password`, `Query` FROM `PSW_SqlSearch`"
enum eSqlSearchQueryQuery { Property_ = 1, Server, Instance, Database, Username, Password, Query };

struct DBCOLUMNDATA
{
	DBLENGTH dwLength;
	DBSTATUS dwStatus;
	BYTE bData[1];
};

extern "C" __declspec( dllexport ) UINT SqlSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;
	BYTE *pRowData = NULL;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	hr = WcaTableExists(L"PSW_SqlSearch");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_SqlSearch'. Have you authored 'PanelSw:SqlSearch' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(SqlSearchQuery, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query on 'PSW_SqlSearch'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");
		if (pRowData)
		{
			MemFree(pRowData);
			pRowData = NULL;
		}

		// Get fields
		CWixString szProperty;
		CWixString szServer;
		CWixString szInstance;
		CWixString szDatabase;
		CWixString szUsername;
		CWixString szPassword;
		CWixString szQuery;
		IDBCreateSession *pDbSession = NULL;
		DBROWCOUNT nRows = 0;
		DBCOUNTITEM unRows = 0;
		HROW phRow[1];
		CComBSTR szError;
		CComPtr<IRowset> pRowset;
		CComPtr<IAccessor> pAccessor;
		CComPtr<IColumnsInfo> pColumnsInfo;
		DBCOLUMNINFO pColInfo[1];
		DBORDINAL nColCount = 0;
		CComBSTR szColNames;
		DBBINDING pDbBinding[1];
		DBCOLUMNDATA sColData;
		DBCOLUMNDATA *pColumn = NULL;
		HACCESSOR hAccessor = NULL;

		hr = WcaGetRecordString(hRecord, eSqlSearchQueryQuery::Property_, (LPWSTR*)szProperty);
		BreakExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, eSqlSearchQueryQuery::Server, (LPWSTR*)szServer);
		BreakExitOnFailure(hr, "Failed to get Server.");
		hr = WcaGetRecordFormattedString(hRecord, eSqlSearchQueryQuery::Instance, (LPWSTR*)szInstance);
		BreakExitOnFailure(hr, "Failed to get Instance.");
		hr = WcaGetRecordFormattedString(hRecord, eSqlSearchQueryQuery::Database, (LPWSTR*)szDatabase);
		BreakExitOnFailure(hr, "Failed to get Database.");
		hr = WcaGetRecordFormattedString(hRecord, eSqlSearchQueryQuery::Username, (LPWSTR*)szUsername);
		BreakExitOnFailure(hr, "Failed to get Username.");
		hr = WcaGetRecordFormattedString(hRecord, eSqlSearchQueryQuery::Password, (LPWSTR*)szPassword);
		BreakExitOnFailure(hr, "Failed to get Password.");
		hr = WcaGetRecordFormattedString(hRecord, eSqlSearchQueryQuery::Query, (LPWSTR*)szQuery);
		BreakExitOnFailure(hr, "Failed to get Query.");

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Executing SQL query '%ls'. Server='%ls', Instance='%ls', User='%ls'. Will place results in property '%ls'", (LPCWSTR)szQuery, (LPCWSTR)szServer, (LPCWSTR)szInstance, (LPCWSTR)szUsername, (LPCWSTR)szProperty);

		hr = SqlConnectDatabase((LPCWSTR)szServer, (LPCWSTR)szInstance, (LPCWSTR)szDatabase, szUsername.IsNullOrEmpty(), (LPCWSTR)szUsername, (LPCWSTR)szPassword, &pDbSession);
		BreakExitOnFailure(hr, "Failed connecting to database");
		BreakExitOnNull(pDbSession, hr, E_FAIL, "Failed connecting to database (NULL)");

		hr = SqlSessionExecuteQuery(pDbSession, (LPCWSTR)szQuery, &pRowset, &nRows, &szError);
		BreakExitOnFailure1(hr, "Failed executing query. %ls", (LPCWSTR)(LPWSTR)szError);
		BreakExitOnNull(pRowset, hr, E_FAIL, "Failed executing query (NULL)");
		BreakExitOnNull1((nRows <= 1), hr, E_INVALIDARG, "Query returned %i rows. Can only handle scalar queries", nRows);

		hr = pRowset->GetNextRows(DB_NULL_HCHAPTER, 0, 1, &unRows, (HROW**)&phRow);
		BreakExitOnFailure(hr, "Failed to get result row");

		// Empty result ==> clear property
		if ((hr == DB_S_ENDOFROWSET) || (unRows == 0))
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Query returned no results. Clearing property");

			hr = WcaSetProperty((LPCWSTR)szProperty, L"");
			BreakExitOnFailure1(hr, "Failed clearing property '%ls'", (LPCWSTR)szProperty);
			continue;
		}

		// Get column info
		hr = pRowset->QueryInterface<IColumnsInfo>(&pColumnsInfo);
		BreakExitOnFailure(hr, "Failed to get result column info");

		hr = pColumnsInfo->GetColumnInfo(&nColCount, (DBCOLUMNINFO**)&pColInfo, &szColNames);
		BreakExitOnFailure(hr, "Failed to get result column info");
		BreakExitOnNull1((nColCount <= 1), hr, E_INVALIDARG, "Query returned %i columns. Can only handle scalar queries", nColCount);

		if (nColCount == 0)
		{
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Query returned no results. Clearing property");

			hr = WcaSetProperty((LPCWSTR)szProperty, L"");
			BreakExitOnFailure1(hr, "Failed clearing property '%ls'", (LPCWSTR)szProperty);
			continue;
		}
		
		pDbBinding[0].dwPart = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
		pDbBinding[0].eParamIO = DBPARAMIO_NOTPARAM;
		pDbBinding[0].iOrdinal = pColInfo[0].iOrdinal;
		pDbBinding[0].wType = DBTYPE_WSTR;
		pDbBinding[0].pTypeInfo = NULL;
		pDbBinding[0].obValue = offsetof(DBCOLUMNDATA, bData);
		pDbBinding[0].obLength = offsetof(DBCOLUMNDATA, dwLength);
		pDbBinding[0].obStatus = offsetof(DBCOLUMNDATA, dwStatus);
		pDbBinding[0].cbMaxLen = pColInfo[0].ulColumnSize;
		pDbBinding[0].pObject = NULL;
		pDbBinding[0].pBindExt = NULL;
		pDbBinding[0].dwFlags = 0;
		pDbBinding[0].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		pDbBinding[0].bPrecision = 0;
		pDbBinding[0].bScale = 0;

		// Create accessor
		hr = pRowset->QueryInterface<IAccessor>(&pAccessor);
		BreakExitOnFailure(hr, "Failed to get result row accessor");

		hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, pDbBinding, 0, &hAccessor, NULL);
		BreakExitOnFailure(hr, "Failed to create column data accessor");

		pRowData = (BYTE*)MemAlloc(pDbBinding[0].cbMaxLen + sizeof(DBCOLUMNDATA), TRUE);
		BreakExitOnNull(pRowData, hr, E_FAIL, "Failed to allocate memory");

		hr = pRowset->GetData(phRow[0], hAccessor, pRowData);
		BreakExitOnFailure(hr, "Failed to get row data");

		pColumn = (DBCOLUMNDATA*)(pRowData + pDbBinding[0].obLength);
		switch (pColumn->dwStatus)
		{
		case DBSTATUS_S_ISNULL:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Query result is null. Clearing property");
			hr = WcaSetProperty((LPCWSTR)szProperty, L"");
			BreakExitOnFailure1(hr, "Failed clearing property '%ls'", (LPCWSTR)szProperty);
			break;

		case DBSTATUS_S_OK:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Query returned '%ls'", pColumn->bData ? (LPCWSTR)pColumn->bData : L"<null>");
			hr = WcaSetProperty((LPCWSTR)szProperty, pColumn->bData ? (LPCWSTR)pColumn->bData : L"");
			BreakExitOnFailure1(hr, "Failed setting property '%ls'", (LPCWSTR)szProperty);
			break;

		case DBSTATUS_S_TRUNCATED:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Query returned truncated data '%ls'", pColumn->bData ? (LPCWSTR)pColumn->bData : L"<null>");
			hr = WcaSetProperty((LPCWSTR)szProperty, pColumn->bData ? (LPCWSTR)pColumn->bData : L"");
			BreakExitOnFailure1(hr, "Failed setting property '%ls'", (LPCWSTR)szProperty);
			break;

		case DBSTATUS_E_BADACCESSOR:
		case DBSTATUS_E_CANTCONVERTVALUE:
		case DBSTATUS_E_SIGNMISMATCH:
		case DBSTATUS_E_DATAOVERFLOW:
		case DBSTATUS_E_CANTCREATE:
		case DBSTATUS_E_UNAVAILABLE:
		case DBSTATUS_E_PERMISSIONDENIED:
		case DBSTATUS_E_INTEGRITYVIOLATION:
		case DBSTATUS_E_SCHEMAVIOLATION:
		case DBSTATUS_E_BADSTATUS:
		case DBSTATUS_S_DEFAULT: // Unexpected when getting data
			hr = -::abs((long)pColumn->dwStatus);
			BreakExitOnFailure(hr, "Failed getting column data");
			break;
		}	}

	hr = ERROR_SUCCESS;
	WcaLog(LOGMSG_STANDARD, "Done.");

LExit:
	if (pRowData)
	{
		MemFree(pRowData);
		pRowData = NULL;
	}

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

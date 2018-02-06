
#include "stdafx.h"
#include <errno.h>

class CPWCHAR
{
	WCHAR* m_p;
public:
	CPWCHAR() :m_p(0){}
	CPWCHAR(WCHAR* p) :m_p(p){}
	~CPWCHAR(){ if (m_p != 0) delete[] m_p; }
	void operator =(PWCHAR p) { if (m_p) delete[] m_p; m_p = p; }
	operator WCHAR*() { return m_p; }
	WCHAR** operator &() { if (m_p) delete[] m_p; m_p = 0; return &m_p; }
};

#define READINIVALUES_QUERY L"SELECT `Id`, `FilePath`, `Section`, `Key`, `DestProperty`, `Attributes`, `Condition` FROM `PSW_ReadIniValues`"
enum ReadIniValuesAttributes
{
	NONE = 0,
	IGNORE_ERRORS = 1
};

extern "C" __declspec( dllexport ) UINT ReadIniValues(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;

	hr = WcaInitialize(hInstall, "ReadIniValues");
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_ReadIniValues exists.
	hr = WcaTableExists(L"PSW_ReadIniValues");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_ReadIniValues'. Have you authored 'PanelSw:ReadIniValues' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(READINIVALUES_QUERY, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query on 'ReadIniValues'.");
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		LPWSTR Id = nullptr;
		LPWSTR FilePath = nullptr;
		LPWSTR Section = nullptr;
		LPWSTR Key = nullptr;
		LPWSTR DestProperty = nullptr;
		LPWSTR Condition = nullptr;
		int Attributes;
		hr = WcaGetRecordString(hRecord, 1, &Id);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, 2, &FilePath);
		BreakExitOnFailure(hr, "Failed to get FilePath.");
		hr = WcaGetRecordFormattedString(hRecord, 3, &Section);
		BreakExitOnFailure(hr, "Failed to get Section.");
		hr = WcaGetRecordFormattedString(hRecord, 4, &Key);
		BreakExitOnFailure(hr, "Failed to get Key.");
		hr = WcaGetRecordString(hRecord, 5, &DestProperty);
		BreakExitOnFailure(hr, "Failed to get DestProperty.");
		hr = WcaGetRecordInteger(hRecord, 6, &Attributes);
		BreakExitOnFailure(hr, "Failed to get Attributes.");
		hr = WcaGetRecordString(hRecord, 7, &Condition);
		BreakExitOnFailure(hr, "Failed to get Condition.");
		WcaLog(LOGMSG_STANDARD, "Id=%S; FilePath=%S; Section=%S; Key=%S; DestProperty=%S; Attributes=%i; Condition=%S"
			, Id
			, FilePath
			, Section
			, Key
			, DestProperty
			, Attributes
			, Condition
			);
		bIgnoreErrors = ((Attributes & ReadIniValuesAttributes::IGNORE_ERRORS) == ReadIniValuesAttributes::IGNORE_ERRORS);

		// Test condition
		MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, Condition);
		switch (condRes)
		{
		case MSICONDITION::MSICONDITION_NONE:
		case MSICONDITION::MSICONDITION_TRUE:
			WcaLog(LOGMSG_STANDARD, "Condition evaluated to true / none.");
			break;

		case MSICONDITION::MSICONDITION_FALSE:
			WcaLog(LOGMSG_STANDARD, "Skipping. Condition evaluated to false");
			continue;

		case MSICONDITION::MSICONDITION_ERROR:
			hr = E_FAIL;
			BreakExitOnFailure(hr, "Bad Condition field");
		}

		// Get the value.
		DWORD dwSize = 1024;
		CPWCHAR Value(new WCHAR[dwSize]);
		DWORD dwRes;
		while ((dwRes = ::GetPrivateProfileStringW(Section, Key, nullptr, Value, 1024, FilePath)) == (dwSize - 1))
		{
			dwSize += 1024;
			Value = new WCHAR[dwSize];
		}
		WcaLog(LOGMSG_STANDARD, "GetPrivateProfileStringW=%u;errno=%i;GetLastError=%u;", dwRes, errno, ::GetLastError());

		// Error?
		if (dwRes == 0)
		{
			dwRes = ::GetLastError();
			if (dwRes != ERROR_SUCCESS)
			{
				hr = HRESULT_FROM_WIN32(dwRes);
				WcaLogError(hr, "Failed reading ini file value");
				if (bIgnoreErrors)
				{
					hr = S_OK;
				}
				BreakExitOnFailure(hr, "File not found.");
			}
			continue;
		}

		hr = WcaSetProperty(DestProperty, Value);
		if (!bIgnoreErrors)
		{
			BreakExitOnFailure(hr, "Failed to set property.");
		}
	}

	hr = ERROR_SUCCESS;
	WcaLog(LOGMSG_STANDARD, "Done.");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

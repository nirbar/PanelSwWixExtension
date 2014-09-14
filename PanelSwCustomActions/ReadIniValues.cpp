
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

UINT __stdcall ReadIniValues(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;
	bool bIgnoreErrors = false;

	hr = WcaInitialize(hInstall, "ReadIniValues");
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized.");

	// Ensure table PSW_ReadIniValues exists.
	hr = WcaTableExists(L"PSW_ReadIniValues");
	ExitOnFailure(hr, "Table does not exist 'PSW_ReadIniValues'. Have you authored 'PanelSw:ReadIniValues' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(READINIVALUES_QUERY, &hView);
	ExitOnFailure(hr, "Failed to execute SQL query on 'ReadIniValues'.");
	WcaLog(LOGMSG_STANDARD, "Executed query.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		ExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		WCHAR *Id = NULL;
		WCHAR *FilePath = NULL;
		WCHAR *Section = NULL;
		WCHAR *Key = NULL;
		WCHAR *DestProperty = NULL;
		WCHAR *Condition = NULL;
		int Attributes;
		hr = WcaGetRecordString(hRecord, 1, &Id);
		ExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordFormattedString(hRecord, 2, &FilePath);
		ExitOnFailure(hr, "Failed to get FilePath.");
		hr = WcaGetRecordFormattedString(hRecord, 3, &Section);
		ExitOnFailure(hr, "Failed to get Section.");
		hr = WcaGetRecordFormattedString(hRecord, 4, &Key);
		ExitOnFailure(hr, "Failed to get Key.");
		hr = WcaGetRecordString(hRecord, 5, &DestProperty);
		ExitOnFailure(hr, "Failed to get DestProperty.");
		hr = WcaGetRecordInteger(hRecord, 6, &Attributes);
		ExitOnFailure(hr, "Failed to get Attributes.");
		hr = WcaGetRecordString(hRecord, 7, &Condition);
		ExitOnFailure(hr, "Failed to get Condition.");
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
			ExitOnFailure(hr, "Bad Condition field");
		}

		// Get the value.
		DWORD dwSize = 1024;
		CPWCHAR Value(new WCHAR[dwSize]);
		DWORD dwRes;
		while ((dwRes = ::GetPrivateProfileStringW(Section, Key, NULL, Value, 1024, FilePath)) == (dwSize - 1))
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
				ExitOnFailure(hr, "File not found.");
			}
			continue;
		}

		hr = WcaSetProperty(DestProperty, Value);
		if (!bIgnoreErrors)
		{
			ExitOnFailure(hr, "Failed to set property.");
		}
	}

	hr = ERROR_SUCCESS;
	WcaLog(LOGMSG_STANDARD, "Done.");

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

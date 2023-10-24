#pragma once
#include "stdafx.h"
#include "WixString.h"
#include "errorHandling.pb.h"

#define E_RETRY					__HRESULT_FROM_WIN32(ERROR_RETRY)
#define E_INSTALL_USEREXIT		__HRESULT_FROM_WIN32(ERROR_INSTALL_USEREXIT)

// Must match with ..\PanelSwCustomActions\pch.h
enum PSW_MSI_MESSAGES
{
	PSW_MSI_MESSAGES_TOPSHELF_ERROR = 27000,
	PSW_MSI_MESSAGES_EXEC_ON_ERROR = 27001,
	PSW_MSI_MESSAGES_SERVICE_CONFIG_ERROR = 27002,
	PSW_MSI_MESSAGES_DISM_PACKAGE_ERROR = 27003,
	PSW_MSI_MESSAGES_DISM_FEATURE_ERROR = 27004,
	PSW_MSI_MESSAGES_SQL_SCRIPT_EROR = 27005,
	PSW_MSI_MESSAGES_EXEC_ON_CONSOLE_ERROR = 27006,
	PSW_MSI_MESSAGES_WEBSITE_CONFIG_ERROR = 27007,
	PSW_MSI_MESSAGES_SQL_SEARCH_ERROR = 27008,
	PSW_MSI_MESSAGES_JSON_JPATH_ERROR = 27009,
	PSW_MSI_MESSAGES_DISM_UNWANTED_FEATURE_ERROR = 27010,
	PSW_MSI_MESSAGES_EXEC_ON_PROMPT_ALWAYS = 27011,
	PSW_MSI_MESSAGES_FILE_DOWNGRADE = 27012,
	PSW_MSI_MESSAGES_ZIP_FILE_ERROR = 27013,
	PSW_MSI_MESSAGES_ZIP_ARCHIVE_ERROR = 27014,
	PSW_MSI_MESSAGES_UNZIP_ARCHIVE_ERROR = 27015,
	PSW_MSI_MESSAGES_UNZIP_FILE_ERROR = 27016,
};

class CErrorPrompter
{
public:
	CErrorPrompter(DWORD dwErrorId
		, INSTALLMESSAGE flags = (INSTALLMESSAGE)(INSTALLMESSAGE::INSTALLMESSAGE_ERROR | MB_ABORTRETRYIGNORE | MB_DEFBUTTON1 | MB_ICONERROR)
		, HRESULT defaultHr = E_FAIL
		, com::panelsw::ca::ErrorHandling errorHandling = com::panelsw::ca::ErrorHandling::fail
	)
		: _dwErrorId(dwErrorId)
		, _errorHandling(errorHandling)
		, _flags(flags)
		, _defaultHr(defaultHr)
	{
	}

	void SetErrorHandling(com::panelsw::ca::ErrorHandling errorHandling)
	{
		_errorHandling = errorHandling;
	}

	template<typename... ARGS>
	HRESULT Prompt(ARGS ...args)
	{
		HRESULT hr = S_OK;
		PMSIHANDLE hRec = ::MsiCreateRecord(1 + sizeof...(ARGS));
		ExitOnNull(hRec, hr, E_FAIL, "Failed to create record");

		hr = WcaSetRecordInteger(hRec, 1, _dwErrorId);
		ExitOnFailure(hr, "Failed to set record parameter #1");

		hr = PromptRecord(hRec, 2, args...);
	LExit:
		return hr;
	}

private:

	template<typename T1, typename... REST>
	HRESULT PromptRecord(MSIHANDLE hRecord, DWORD dwField, T1 val1, REST ...args)
	{
		HRESULT hr = S_OK;

		hr = SetRecordField(hRecord, dwField, val1);
		ExitOnFailure(hr, "Failed to set record parameter #%u", dwField);

		hr = PromptRecord(hRecord, ++dwField, args...);
	LExit:
		return hr;
	}

	HRESULT PromptRecord(MSIHANDLE hRecord, DWORD dwField)
	{
		if (_errorHandling == com::panelsw::ca::ErrorHandling::fail)
		{
			return E_FAIL;
		}
		if (_errorHandling == com::panelsw::ca::ErrorHandling::ignore)
		{
			return S_FALSE;
		}

		UINT promptResult = IDABORT;
		promptResult = WcaProcessMessage(_flags, hRecord);
		switch (promptResult)
		{
		case IDABORT:
		case IDCANCEL:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User aborted");
			return E_INSTALL_USEREXIT;

		case IDTRYAGAIN:
		case IDRETRY:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User chose to retry");
			return E_RETRY;

		case IDOK:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User approved");
			return S_OK;

		case IDIGNORE:
		case IDCONTINUE:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User ignored");
			return S_FALSE;

		case 0: // Default, in silent mode
			return _defaultHr;

		default:
			WcaLog(LOGLEVEL::LOGMSG_STANDARD, "User chose unknown button %u", promptResult);
			return __HRESULT_FROM_WIN32(promptResult);
		}
	}

	template<typename T>
	HRESULT SetRecordField(MSIHANDLE hRecord, DWORD dwField, T value)
	{
		WcaLogError(E_INVALIDARG, "CErrorPrompter::Prompt called with unsupported template type. Only LPSTR, LPCSTR, LPWSTR, LPCWSTR, CWixString, and DWORD are supported");
		return E_INVALIDARG;
	}

	template<>
	HRESULT SetRecordField<LPWSTR>(MSIHANDLE hRecord, DWORD dwField, LPWSTR szValue)
	{
		return WcaSetRecordString(hRecord, dwField, szValue);
	}

	template<>
	HRESULT SetRecordField<LPCWSTR>(MSIHANDLE hRecord, DWORD dwField, LPCWSTR szValue)
	{
		if (szValue)
		{
			return WcaSetRecordString(hRecord, dwField, szValue);
		}
		return S_OK;
	}

	template<>
	HRESULT SetRecordField<LPSTR>(MSIHANDLE hRecord, DWORD dwField, LPSTR szValue)
	{
		if (szValue)
		{
			return __HRESULT_FROM_WIN32(MsiRecordSetStringA(hRecord, dwField, szValue));
		}
		return S_OK;
	}

	template<>
	HRESULT SetRecordField<LPCSTR>(MSIHANDLE hRecord, DWORD dwField, LPCSTR szValue)
	{
		if (szValue)
		{
			return __HRESULT_FROM_WIN32(MsiRecordSetStringA(hRecord, dwField, szValue));
		}
		return S_OK;
	}

	template<>
	HRESULT SetRecordField<DWORD>(MSIHANDLE hRecord, DWORD dwField, DWORD dwValue)
	{
		return WcaSetRecordInteger(hRecord, dwField, dwValue);
	}

	template<>
	HRESULT SetRecordField<const CWixString&>(MSIHANDLE hRecord, DWORD dwField, const CWixString& szValue)
	{
		if (!szValue.IsNullOrEmpty())
		{
			return WcaSetRecordString(hRecord, dwField, (LPCWSTR)szValue);
		}
		return S_OK;
	}

	DWORD _dwErrorId;
	com::panelsw::ca::ErrorHandling _errorHandling;
	INSTALLMESSAGE _flags;
	HRESULT _defaultHr;
};

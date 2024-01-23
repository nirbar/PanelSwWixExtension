#pragma once
class CSummaryStream
{
public:

	static HRESULT IsPackageX64();
	static HRESULT IsUserContext();

private:

	enum SummaryStreamProperties
	{
		PID_CODEPAGE = 1,
		PID_TITLE = 2,
		PID_SUBJECT = 3,
		PID_AUTHOR = 4,
		PID_KEYWORDS = 5,
		PID_COMMENTS = 6,
		PID_TEMPLATE = 7,
		PID_LASTAUTHOR = 8,
		PID_REVNUMBER = 9,
		PID_LASTPRINTED = 11,
		PID_CREATE_DTM = 12,
		PID_LASTSAVE_DTM = 13,
		PID_PAGECOUNT = 14,
		PID_WORDCOUNT = 15,
		PID_CHARCOUNT = 16,
		PID_APPNAME = 18,
		PID_SECURITY = 19
	};

	// See https://docs.microsoft.com/en-us/windows/win32/msi/word-count-summary
	enum SummaryStreamWordCount
	{
		WORD_COUNT_SHORT_NAMES = 1,
		WORD_COUNT_COMPRESSED = 2,
		WORD_COUNT_ADMIN_IMAGE = 4,
		WORD_COUNT_UAC_COMPLIANT = 8,
	};

	static HRESULT GetSummaryStreamProperty(SummaryStreamProperties iProperty, LPWSTR* pszValue);
	static HRESULT GetSummaryStreamProperty(SummaryStreamProperties iProperty, INT* piValue);
};
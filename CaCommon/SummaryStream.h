#pragma once
class CSummaryStream
{
public:

	// Singleton
	static CSummaryStream* GetInstance() noexcept;

	HRESULT IsPackageX64( bool *pIsX64) noexcept;


private:

	enum SummaryStreamProperties
	{
		PID_CODEPAGE=1,
		PID_TITLE=2,
		PID_SUBJECT=3,
		PID_AUTHOR=4,
		PID_KEYWORDS=5,
		PID_COMMENTS=6,
		PID_TEMPLATE=7,
		PID_LASTAUTHOR=8,
		PID_REVNUMBER=9,
		PID_LASTPRINTED=11,
		PID_CREATE_DTM=12,
		PID_LASTSAVE_DTM=13,
		PID_PAGECOUNT=14,
		PID_WORDCOUNT=15,
		PID_CHARCOUNT=16,
		PID_APPNAME=18,
		PID_SECURITY=19
	};

	// Singleton
	static CSummaryStream _sInst;
	CSummaryStream() noexcept;

	HRESULT GetProperty( SummaryStreamProperties eProp, LPWSTR* ppProp) noexcept;


	WCHAR* _pTemplate;
};


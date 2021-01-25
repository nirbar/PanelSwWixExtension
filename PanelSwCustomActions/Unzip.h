#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "..\poco\Zip\include\Poco\Zip\ZipLocalFileHeader.h"
#include "unzipDetails.pb.h"
#include <string>

class CUnzip :
	public CDeferredActionBase
{
public:

	CUnzip() : CDeferredActionBase("Unzip") { }
	
	HRESULT AddUnzip(LPCWSTR zipFile, LPCWSTR targetFolder, ::com::panelsw::ca::UnzipDetails_UnzipFlags flags);

protected:
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT ShouldOverwriteFile(LPCWSTR szFile, ::com::panelsw::ca::UnzipDetails_UnzipFlags flags);

	HRESULT SetFileTimes(LPCSTR szFilePath, const std::string &extradField);

#pragma pack(push, 1)
	struct ExtraDataHeader
	{
		short tag = 0;
		short size = 0;
	};
#pragma pack(pop)

#pragma pack(push, 1)
	// Tag 0x5455, size depends on flags
	struct ExtraDataUnixTime
	{
		/*
		+ *Flags         Byte        info bits
		+ *(ModTime)Long        time of last modification(UTC / GMT)
		+ *(AcTime)Long        time of last access(UTC / GMT)
		+ *(CrTime)Long        time of original creation(UTC / GMT)
		*/

		char flags = 0;
		unsigned int times[3]; // 0-3 times
	};
#pragma pack(pop)

#pragma pack(push, 1)
	// Tag 0x0A, size 32
	struct ExtraDataWindowsTime
	{
		/*
		+ *0x000A        Short       tag for this extra block type("UT")
		+ *TSize         Short       total data size for this block
		+ *(ModTime)Long        time of last modification(UTC / GMT)
		+ *(AcTime)Long        time of last access(UTC / GMT)
		+ *(CrTime)Long        time of original creation(UTC / GMT)
		*/

		int reserved;
		short tag = 0; // Expected 1
		short size = 0; // Expected 24
		FILETIME modificationTime;
		FILETIME accessTime;
		FILETIME creationTime;
	};
#pragma pack(pop)

	HRESULT FindTimeInEntry(const std::string &extradField, FILETIME *createTime, FILETIME *accessTime, FILETIME *modifyTime);
};
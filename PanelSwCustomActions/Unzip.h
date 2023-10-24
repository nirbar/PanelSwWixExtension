#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "../CaCommon/ErrorPrompter.h"
#include "..\poco\Zip\include\Poco\Zip\ZipLocalFileHeader.h"
#include "unzipDetails.pb.h"
#include "zipDetails.pb.h"
#include "..\poco\Zip\include\Poco\Zip\ZipArchive.h"
#include <string>

class CUnzip :
	public CDeferredActionBase
{
public:

	CUnzip(bool bZip);

	HRESULT AddUnzip(LPCWSTR zipFile, LPCWSTR targetFolder, ::com::panelsw::ca::UnzipDetails_UnzipFlags flags, com::panelsw::ca::ErrorHandling errorHandling);

	HRESULT AddZip(LPCWSTR zipFile, LPCWSTR sourceFolder, LPCWSTR szPattern, bool bRecursive, com::panelsw::ca::ErrorHandling errorHandling);

protected:
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	bool isZip_ = true;

	HRESULT ExecuteOneUnzip(::com::panelsw::ca::UnzipDetails *pDetails);
	HRESULT ExecuteOneZip(::com::panelsw::ca::ZipDetails *pDetails);

	HRESULT ShouldOverwriteFile(LPCWSTR szFile, ::com::panelsw::ca::UnzipDetails_UnzipFlags flags);

	HRESULT UnzipOneFile(std::istream* zipFileStream, const ::Poco::Zip::ZipLocalFileHeader& fileHeader, ::com::panelsw::ca::UnzipDetails::UnzipFlags flags, Poco::Path targetFolder);
	HRESULT SetFileTimes(LPCSTR szFilePath, const std::string& extradField);

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

	HRESULT FindTimeInEntry(const std::string& extradField, FILETIME* createTime, FILETIME* accessTime, FILETIME* modifyTime);

	CErrorPrompter _unzipPrompter;
	CErrorPrompter _unzipOneFilePrompter;
	CErrorPrompter _zipPrompter;
	CErrorPrompter _zipOneFilePrompter;
};

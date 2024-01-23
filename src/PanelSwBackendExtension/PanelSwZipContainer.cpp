#include "pch.h"
#include "PanelSwZipContainer.h"
#include "..\poco\Zip\include\Poco\Zip\ZipStream.h"
#include "..\poco\Zip\include\Poco\Zip\Compress.h"
#include "..\poco\Foundation\include\Poco\UnicodeConverter.h"
#include "..\poco\Foundation\include\Poco\Delegate.h"
#include "..\poco\Foundation\include\Poco\StreamCopier.h"
#include <fstream>
#include <shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")
#pragma comment (lib, "Iphlpapi.lib")
#pragma comment (lib, "PocoZipmt.lib")
using namespace Poco::Zip;
using namespace std;

CPanelSwZipContainer::~CPanelSwZipContainer()
{
	Reset();
}

HRESULT CPanelSwZipContainer::Reset()
{
	HRESULT hr = S_OK;
	try
	{
		_pArchive.reset();
		_zipStream.reset();
		_currFile.clear();
	}
	catch (Poco::Exception ex)
	{
		hr = (ex.code() == 0) ? E_FAIL : __HRESULT_FROM_WIN32(ex.code());
		BextExitOnFailure(hr, "Failed to close zip. %hs", ex.displayText().c_str());
	}
	catch (std::exception ex)
	{
		hr = E_FAIL;
		BextExitOnFailure(hr, "Failed to close zip. %hs", ex.what());
	}
	catch (...)
	{
		hr = E_FAIL;
		BextExitOnFailure(hr, "Failed to open archive");
	}

LExit:
	return hr;
}

HRESULT CPanelSwZipContainer::ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath)
{
	HRESULT hr = S_OK;
	std::string zipFileA;

	try 
	{
		Poco::UnicodeConverter::toUTF8(wzFilePath, zipFileA);

		_zipStream.reset(new std::ifstream(zipFileA, std::ios::binary));
		BextExitOnNull(_zipStream.get(), hr, E_FAIL, "Failed to open stream for ZIP file '%ls'", wzFilePath);

		_pArchive.reset(new ZipArchive(*_zipStream));
		BextExitOnNull(_pArchive.get(), hr, E_FAIL, "Failed to open ZIP file '%ls'", wzFilePath);
	}
	catch (Poco::Exception ex)
	{
		hr = (ex.code() == 0) ? E_FAIL : __HRESULT_FROM_WIN32(ex.code());
		BextExitOnFailure(hr, "Failed to open zip '%hs'. %hs", zipFileA.c_str(), ex.displayText().c_str());
	}
	catch (std::exception ex)
	{
		hr = E_FAIL;
		BextExitOnFailure(hr, "Failed to open zip '%hs'. %hs", zipFileA.c_str(), ex.what());
	}
	catch (...)
	{
		hr = E_FAIL;
		BextExitOnFailure(hr, "Failed to open archive");
	}
	BextLog(BUNDLE_EXTENSION_LOG_LEVEL_STANDARD, "Openned ZIP container '%ls'", wzFilePath);

LExit:
	return hr;
}

HRESULT CPanelSwZipContainer::ContainerNextStream(BSTR* psczStreamName)
{
	HRESULT hr = S_OK;
	ZipArchive::FileHeaders::const_iterator it;
	ZipArchive::FileHeaders::const_iterator endIt;
	LPWSTR szCurrFile = nullptr;

	if (psczStreamName && *psczStreamName)
	{
		::SysFreeString(*psczStreamName);
		*psczStreamName = nullptr;
	}

	try
	{
		if (_currFile.empty())
		{
			it = _pArchive->headerBegin();
		}
		else
		{
			it = _pArchive->findHeader(_currFile);
			++it;
		}

		if (it == _pArchive->headerEnd())
		{
			hr = E_NOMOREITEMS;
			ExitFunction();
		}

		_currFile = it->first;
	}
	catch (Poco::Exception ex)
	{
		hr = (ex.code() == 0) ? E_FAIL : __HRESULT_FROM_WIN32(ex.code());
		BextExitOnFailure(hr, "Failed to get next stream. %hs", ex.displayText().c_str());
	}
	catch (std::exception ex)
	{
		hr = E_FAIL;
		BextExitOnFailure(hr, "Failed to get next stream. %hs", ex.what());
	}
	catch (...)
	{
		hr = E_FAIL;
		BextExitOnFailure(hr, "Failed to get next stream");
	}

	hr = StrAllocStringAnsi(&szCurrFile, _currFile.c_str(), 0, CP_UTF8);
	BextExitOnFailure(hr, "Failed to convert string to wide-string");

	if (psczStreamName)
	{
		*psczStreamName = ::SysAllocString(szCurrFile);
		BextExitOnNull(*psczStreamName, hr, E_FAIL, "Failed to allocate sys string");
	}

LExit:
	ReleaseStr(szCurrFile);
	return hr;
}

HRESULT CPanelSwZipContainer::ContainerStreamToFile(LPCWSTR wzFileName)
{
	HRESULT hr = S_OK;
	std::string targetFileA;
	ZipArchive::FileHeaders::const_iterator it;

	Poco::UnicodeConverter::toUTF8(wzFileName, targetFileA);

	try
	{
		it = _pArchive->findHeader(_currFile);
		ZipInputStream zipin(*_zipStream, it->second);
		std::ofstream out(targetFileA.c_str(), std::ios::binary);

		std::streamsize bytes = Poco::StreamCopier::copyStream(zipin, out);
		BextExitOnNull((bytes == it->second.getUncompressedSize()), hr, E_FAIL, "Error extracting file '%hs': %I64i / %I64i bytes written", targetFileA.c_str(), bytes, it->second.getUncompressedSize());
	}
	catch (Poco::Exception ex)
	{
		hr = (ex.code() == 0) ? E_FAIL : __HRESULT_FROM_WIN32(ex.code());
		BextExitOnFailure(hr, "Failed to unzip '%hs'. %hs", targetFileA.c_str(), ex.displayText().c_str());
	}
	catch (std::exception ex)
	{
		hr = E_FAIL;
		BextExitOnFailure(hr, "Failed to unzip '%hs'. %hs", targetFileA.c_str(), ex.what());
	}
	catch (...)
	{
		hr = E_FAIL;
		BextExitOnFailure(hr, "Failed to unzip file '%hs'", targetFileA.c_str());
	}

LExit:
	return hr;
}

HRESULT CPanelSwZipContainer::ContainerStreamToBuffer(BYTE** ppbBuffer, SIZE_T* pcbBuffer)
{
	return E_NOTIMPL;
}

HRESULT CPanelSwZipContainer::ContainerSkipStream()
{
	return S_OK;
}

HRESULT CPanelSwZipContainer::ContainerClose()
{
	return Reset();
}

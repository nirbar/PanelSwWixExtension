#include "pch.h"
#include "PanelSwBackendExtension.h"
#include <BextBaseBundleExtensionProc.h>
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

//TODO: Support multiple concurrent containers

CPanelSwBundleExtension::CPanelSwBundleExtension(IBundleExtensionEngine* pEngine)
	: CBextBaseBundleExtension(pEngine)
{
}

CPanelSwBundleExtension::~CPanelSwBundleExtension()
{
	Reset();
}

HRESULT CPanelSwBundleExtension::Reset()
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

STDMETHODIMP CPanelSwBundleExtension::ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath, LPVOID* pContext)
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

STDMETHODIMP CPanelSwBundleExtension::ContainerNextStream(LPVOID pContext, BSTR* psczStreamName)
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

STDMETHODIMP CPanelSwBundleExtension::ContainerStreamToFile(LPVOID pContext, LPCWSTR wzFileName)
{
	HRESULT hr = S_OK;
	std::string targetFileA;
	ZipArchive::FileHeaders::const_iterator it;

	Poco::UnicodeConverter::toUTF8(wzFileName, targetFileA);
	BextLog(BUNDLE_EXTENSION_LOG_LEVEL_STANDARD, "Extracting '%ls'", wzFileName);

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

// Not really needed because it is only used to read the manifest by the engine, and that is always a cab.
STDMETHODIMP CPanelSwBundleExtension::ContainerStreamToBuffer(LPVOID pContext, BYTE** ppbBuffer, SIZE_T* pcbBuffer)
{
	return E_NOTIMPL;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerSkipStream(LPVOID pContext)
{
	return S_OK;
}

//TODO Release everything in the context
STDMETHODIMP CPanelSwBundleExtension::ContainerClose(LPVOID pContext)
{
	return Reset();
}

extern "C" HRESULT WINAPI BundleExtensionCreate(
	__in const BUNDLE_EXTENSION_CREATE_ARGS * pArgs,
	__inout BUNDLE_EXTENSION_CREATE_RESULTS * pResults
)
{
	HRESULT hr = S_OK;
	IBundleExtensionEngine* pEngine = nullptr;
	CPanelSwBundleExtension* pExtension = nullptr;

	hr = BextInitializeFromCreateArgs(pArgs, &pEngine);
	ExitOnFailure(hr, "Failed to initialize bext");

	pExtension = new CPanelSwBundleExtension(pEngine);
	BextExitOnNull(pExtension, hr, E_OUTOFMEMORY, "Failed to create new CPanelSwBundleExtension.");

	hr = pExtension->Initialize(pArgs);
	BextExitOnFailure(hr, "CPanelSwBundleExtension initialization failed.");

	pResults->pfnBundleExtensionProc = BextBaseBundleExtensionProc;
	pResults->pvBundleExtensionProcContext = pExtension;

LExit:
	ReleaseObject(pEngine);

	return hr;
}

extern "C" void WINAPI BundleExtensionDestroy()
{
	BextUninitialize();
}

#pragma once
#include "pch.h"
#include "lzma-sdk/CPP/Common/Common.h"
#include "lzma-sdk/CPP/7zip/IDecl.h"
#include "lzma-sdk/CPP/7zip/Archive/IArchive.h"
#include "lzma-sdk/CPP/Common/MyString.h"
#include "lzma-sdk/CPP/Common/MyCom.h"
#include <memory>

class CPanelSwLzmaExtractCallback Z7_final :
	public IArchiveExtractCallback,
	public IArchiveExtractCallbackMessage2,
	public CMyUnknownImp
{
	Z7_COM_UNKNOWN_IMP_1(
		/* IArchiveExtractCallback, */
		IArchiveExtractCallbackMessage2)

		Z7_IFACE_COM7_IMP(IProgress)
		Z7_IFACE_COM7_IMP(IArchiveExtractCallback)
		Z7_IFACE_COM7_IMP(IArchiveExtractCallbackMessage2)

public:
	HRESULT Init(IInArchive* archive, UInt32 extractCount, UInt32* extractIndices, FString* extractPaths);

	BOOL HasErrors() const;

private:
	HRESULT OperationResultToString(Int32 opRes, LPWSTR *psz) const;

	IInArchive* _archive = nullptr;
	std::unique_ptr<UInt32[]> _extractIndices;
	std::unique_ptr<FString[]> _extractPaths;
	UInt32 _extractCount = 0;
	DWORD _dwErrCount = 0;
};

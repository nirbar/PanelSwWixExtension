#pragma once
#include "pch.h"
#include "lzma-sdk/CPP/Common/Common.h"
#include "lzma-sdk/CPP/7zip/IDecl.h"
#include "lzma-sdk/CPP/7zip/IStream.h"
#include "lzma-sdk/CPP/Common/MyCom.h"

Z7_class_final(CPanelSwLzmaInStream) :
	public IInStream,
	public IStreamGetSize,
	public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_2(
	  IInStream,
	  IStreamGetSize)

  Z7_IFACE_COM7_IMP(ISequentialInStream)
  Z7_IFACE_COM7_IMP(IInStream)
public:
  Z7_IFACE_COM7_IMP(IStreamGetSize)

public:
  virtual ~CPanelSwLzmaInStream();

  HRESULT InitContainer(HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize);
  
  static const DWORD64 INFINITE_CONTAINER_SIZE = (~(DWORD64)0);

private:
  void ReleaseContainer();

  static unsigned const MAX_RETRIES = 10;

  HANDLE _hBundle = INVALID_HANDLE_VALUE;
  DWORD64 _qwContainerStartPos = 0;
  DWORD64 _qwContainerSize = 0;
  DWORD64 _qwBundleSize = 0;
};

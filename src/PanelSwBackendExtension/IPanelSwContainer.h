#pragma once

#include "pch.h"

class IPanelSwContainer
{
public:
	virtual HRESULT ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath) = 0;

	virtual HRESULT ContainerNextStream(BSTR* psczStreamName) = 0;

	virtual HRESULT ContainerStreamToFile(LPCWSTR wzFileName) = 0;

	virtual HRESULT ContainerStreamToBuffer(BYTE** ppbBuffer, SIZE_T* pcbBuffer) = 0;

	virtual HRESULT ContainerSkipStream() = 0;

	virtual HRESULT ContainerClose() = 0;
};

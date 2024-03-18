#pragma once

#include "pch.h"

class IPanelSwContainer
{
public:
	virtual HRESULT ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath) = 0;

	virtual HRESULT ContainerOpenAttached(LPCWSTR wzContainerId, HANDLE hBundle, DWORD64 qwContainerStartPos, DWORD64 qwContainerSize) = 0;

	virtual HRESULT ContainerExtractFiles(DWORD cFiles, LPCWSTR* psczEmbeddedIds, LPCWSTR* psczTargetPaths) = 0;

	virtual HRESULT ContainerClose() = 0;
};

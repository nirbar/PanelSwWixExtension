#pragma once
#include "IFileFilter.h"

class CFileSpecFilter
	: public IFileFilter
{
public:
	HRESULT Initialize(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive) override;

	HRESULT IsMatch(LPCWSTR szFilePath) const override;

	void Release() override;

private:
	LPWSTR _szBaseFolder = nullptr;
	LPWSTR _szFilter = nullptr;
	bool _bRecursive = false;
};

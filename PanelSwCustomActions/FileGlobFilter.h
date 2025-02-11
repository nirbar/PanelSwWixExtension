#pragma once
#include "IFileFilter.h"
#include <regex>

class CFileGlobFilter
	: public IFileFilter
{
public:
	HRESULT Initialize(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive) override;
	
	HRESULT IsMatch(LPCWSTR szFilePath) const override;

	void Release() override;

private:
	std::wregex _rxPattern;
	LPWSTR _szBaseFolder = nullptr;
};

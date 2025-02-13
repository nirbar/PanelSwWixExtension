#pragma once
#include "IFileFilter.h"
#include <regex>

class CFileGlobFilter
	: public IFileFilter
{
public:
	virtual ~CFileGlobFilter();

	/* Supported glob format as in https://code.visualstudio.com/docs/editor/glob-patterns
		- / to separate path segments
		- * to match zero or more characters in a path segment
		- ? to match on one character in a path segment
		- ** to match any number of path segments, including none
		- {} to group conditions (for example {**\*.html, **\*.txt} matches all HTML and text files)
		- [] to declare a range of characters to match (example.[0-9] to match on example.0, example.1, …)
		- [!...] to negate a range of characters to match (example.[!0-9] to match on example.a, example.b, but not example.0)
	*/
	HRESULT Initialize(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive) override;
	
	HRESULT IsMatch(LPCWSTR szFilePath) const override;

	void Release() override;

private:
	std::wregex _rxPattern;
	LPWSTR _szBaseFolder = nullptr;
};

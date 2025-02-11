#pragma once

class IFileFilter
{
public:
	
	static HRESULT InferFilter(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive, IFileFilter** ppFilter);
	
	virtual HRESULT Initialize(LPCWSTR szBaseFolder, LPCWSTR szFilter, bool bRecursive) = 0;
	
	virtual HRESULT IsMatch(LPCWSTR szFilePath) const = 0;

	virtual void Release() = 0;
};

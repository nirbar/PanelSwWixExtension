#pragma once

template <class T, class D> class CAutoRelease
{
public:

	CAutoRelease(T obj)
		: _obj(obj)
	{
	}

	~CAutoRelease()
	{
		Release();
	}

	void Release()
	{
		D(_obj);
	}

	T& operator=(T& other)
	{
		Release();
		_obj = other;
		return _obj;
	}

	operator T()
	{
		return _obj;
	}

	T* operator &()
	{
		Release();
		return &_obj;
	}

private:
	T _obj;
};

typedef CAutoRelease<HKEY, decltype(::RegCloseKey)> CHKEY;
typedef CAutoRelease<HANDLE, decltype(::CloseHandle)> CHANDLE;
typedef CAutoRelease<SC_HANDLE, decltype(::CloseServiceHandle)> CSC_HANDLE;

#include "StdAfx.h"
//#include "AirInstallerDlg.h"

#pragma once

enum
{
	UF_BINDSTATUS_FIRST_ = BINDSTATUS_FINDINGRESOURCE,
	UF_BINDSTATUS_LAST_ = BINDSTATUS_ACCEPTRANGES
};


class DownloadCallback : public IBindStatusCallback
{
public:
	DownloadCallback(HWND hWnd, HANDLE hEventStop, CString &name);

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IBindStatusCallback methods
	STDMETHOD(OnStartBinding)(DWORD, IBinding *);
	STDMETHOD(GetPriority)(LONG *);
	STDMETHOD(OnLowResource)(DWORD);
	STDMETHOD(OnProgress)(ULONG ulProgress,
						  ULONG ulProgressMax,
						  ULONG ulStatusCode,
						  LPCWSTR szStatusText);
	STDMETHOD(OnStopBinding)(HRESULT, LPCWSTR);
	STDMETHOD(GetBindInfo)(DWORD *, BINDINFO *);
	STDMETHOD(OnDataAvailable)(DWORD, DWORD, FORMATETC *, STGMEDIUM *);
	STDMETHOD(OnObjectAvailable)(REFIID, IUnknown *);

	bool Done();

	struct DOWNLOADSTATUS
	{
		ULONG ulProgress;
		ULONG ulProgressMax;
		ULONG ulStatusCode;
		LPCWSTR szStatusText;
		CString name;
	};

	//CAirInstallerDlg * m_airInstaller;

protected:
	ULONG m_ulObjRefCount;

private:
	HWND m_hWnd;
	HANDLE m_hEventStop;
	CString m_name;
	bool m_done;
};
#include "StdAfx.h"
#include "DownloadCallback.h"

#define    WM_USER_DISPLAYSTATUS				(WM_USER + 2)

// Callback

DownloadCallback::DownloadCallback(HWND hWnd, HANDLE hEventStop, CString &name)
{
	m_hWnd = hWnd;  // the window handle to display status

	m_hEventStop = hEventStop;  // the event object to signal to stop

	m_ulObjRefCount = 1;

	m_name = name; 

	m_done = false;

	m_airInstaller = NULL;
}
//-----------------------------------------------------------------------------
// IUnknown
STDMETHODIMP DownloadCallback::QueryInterface(REFIID riid, void **ppvObject)
{
	TRACE(_T("IUnknown::QueryInterface\n"));

	*ppvObject = NULL;
	
	// IUnknown
	if (::IsEqualIID(riid, __uuidof(IUnknown)))
	{
		TRACE(_T("IUnknown::QueryInterface(IUnknown)\n"));

		*ppvObject = this;
	}
	// IBindStatusCallback
	else if (::IsEqualIID(riid, __uuidof(IBindStatusCallback)))
	{
		TRACE(_T("IUnknown::QueryInterface(IBindStatusCallback)\n"));

		*ppvObject = static_cast<IBindStatusCallback *>(this);
	}

	if (*ppvObject)
	{
		(*reinterpret_cast<LPUNKNOWN *>(ppvObject))->AddRef();

		return S_OK;
	}
	
	return E_NOINTERFACE;
}                                             
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) DownloadCallback::AddRef()
{
	TRACE(_T("IUnknown::AddRef\n"));

	return ++m_ulObjRefCount;
}
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) DownloadCallback::Release()
{
	TRACE(_T("IUnknown::Release\n"));

	return --m_ulObjRefCount;
}
//-----------------------------------------------------------------------------
// IBindStatusCallback
STDMETHODIMP DownloadCallback::OnStartBinding(DWORD, IBinding *)
{
	TRACE(_T("DownloadCallback::OnStartBinding\n"));

	return S_OK;
}
//-----------------------------------------------------------------------------
STDMETHODIMP DownloadCallback::GetPriority(LONG *)
{
	TRACE(_T("DownloadCallback::GetPriority\n"));

	return E_NOTIMPL;
}
//-----------------------------------------------------------------------------
STDMETHODIMP DownloadCallback::OnLowResource(DWORD)
{
	TRACE(_T("IBindStatusCallback::OnLowResource\n"));

	return S_OK;
}
//-----------------------------------------------------------------------------
STDMETHODIMP DownloadCallback::OnProgress(ULONG ulProgress,
										 ULONG ulProgressMax,
										 ULONG ulStatusCode,
										 LPCWSTR szStatusText)
{
#ifdef _DEBUG
	static const LPCTSTR plpszStatus[] = 
	{
		_T("BINDSTATUS_FINDINGRESOURCE"),  // 1
		_T("BINDSTATUS_CONNECTING"),
		_T("BINDSTATUS_REDIRECTING"),
		_T("BINDSTATUS_BEGINDOWNLOADDATA"),
		_T("BINDSTATUS_DOWNLOADINGDATA"),
		_T("BINDSTATUS_ENDDOWNLOADDATA"),
		_T("BINDSTATUS_BEGINDOWNLOADCOMPONENTS"),
		_T("BINDSTATUS_INSTALLINGCOMPONENTS"),
		_T("BINDSTATUS_ENDDOWNLOADCOMPONENTS"),
		_T("BINDSTATUS_USINGCACHEDCOPY"),
		_T("BINDSTATUS_SENDINGREQUEST"),
		_T("BINDSTATUS_CLASSIDAVAILABLE"),
		_T("BINDSTATUS_MIMETYPEAVAILABLE"),
		_T("BINDSTATUS_CACHEFILENAMEAVAILABLE"),
		_T("BINDSTATUS_BEGINSYNCOPERATION"),
		_T("BINDSTATUS_ENDSYNCOPERATION"),
		_T("BINDSTATUS_BEGINUPLOADDATA"),
		_T("BINDSTATUS_UPLOADINGDATA"),
		_T("BINDSTATUS_ENDUPLOADINGDATA"),
		_T("BINDSTATUS_PROTOCOLCLASSID"),
		_T("BINDSTATUS_ENCODING"),
		_T("BINDSTATUS_VERFIEDMIMETYPEAVAILABLE"),
		_T("BINDSTATUS_CLASSINSTALLLOCATION"),
		_T("BINDSTATUS_DECODING"),
		_T("BINDSTATUS_LOADINGMIMEHANDLER"),
		_T("BINDSTATUS_CONTENTDISPOSITIONATTACH"),
		_T("BINDSTATUS_FILTERREPORTMIMETYPE"),
		_T("BINDSTATUS_CLSIDCANINSTANTIATE"),
		_T("BINDSTATUS_IUNKNOWNAVAILABLE"),
		_T("BINDSTATUS_DIRECTBIND"),
		_T("BINDSTATUS_RAWMIMETYPE"),
		_T("BINDSTATUS_PROXYDETECTING"),
		_T("BINDSTATUS_ACCEPTRANGES"),
		_T("???")  // unknown
	};
#endif

	TRACE(_T("_IBindStatusCallback::OnProgress\n"));

	TRACE(_T("_ulProgress: %lu, ulProgressMax: %lu\n"),
		  ulProgress, ulProgressMax);
	
	TRACE(_T("_ulStatusCode: %lu "), ulStatusCode);

	if (ulStatusCode < UF_BINDSTATUS_FIRST_ ||
		ulStatusCode > UF_BINDSTATUS_LAST_)
	{
		ulStatusCode = UF_BINDSTATUS_LAST_ + 1;
	}
	
#ifdef _DEBUG
	TRACE(_T("(%s), szStatusText: %ls\n"),
		  plpszStatus[ulStatusCode - UF_BINDSTATUS_FIRST_],
		  szStatusText);
#endif

	
	if( ulProgress == ulProgressMax && ulProgress != 0 ){
		m_done = true;
	}

	if (m_hWnd != NULL)
	{
		// inform the dialog box to display current status,
		// don't use PostMessage
		CUpgraderDlg::DOWNLOADSTATUS downloadStatus = { ulProgress, ulProgressMax, ulStatusCode, szStatusText };
		::SendMessage(m_hWnd, WM_USER_DISPLAYSTATUS, 0, reinterpret_cast<LPARAM>(&downloadStatus));
		TRACE0(" send status message \n ");
	}

	

	if (m_hEventStop != NULL)
	{
		if (::WaitForSingleObject(m_hEventStop, 0) == WAIT_OBJECT_0)
		{
			TRACE(_T("Canceled.\n"));
			//::SendMessage(m_hWnd, WM_USER_DISPLAYSTATUS, 0, NULL);
			return E_ABORT;  // canceled by the user
		}
	}

	return S_OK;
}
//-----------------------------------------------------------------------------
STDMETHODIMP DownloadCallback::OnStopBinding(HRESULT, LPCWSTR)
{
	TRACE(_T("IBindStatusCallback::OnStopBinding\n"));

	return S_OK;
}
//-----------------------------------------------------------------------------
STDMETHODIMP DownloadCallback::GetBindInfo(DWORD *, BINDINFO *)
{
	TRACE(_T("IBindStatusCallback::GetBindInfo\n"));

	return S_OK;
}
//-----------------------------------------------------------------------------
STDMETHODIMP DownloadCallback::OnDataAvailable(DWORD, DWORD,
											  FORMATETC *, STGMEDIUM *)
{
	TRACE(_T("IBindStatusCallback::OnDataAvailable\n"));

	return S_OK;
}
//-----------------------------------------------------------------------------
STDMETHODIMP DownloadCallback::OnObjectAvailable(REFIID, IUnknown *)
{
	TRACE(_T("IBindStatusCallback::OnObjectAvailable\n"));

	return S_OK;
}


bool DownloadCallback::Done()
{
	return m_done;
}
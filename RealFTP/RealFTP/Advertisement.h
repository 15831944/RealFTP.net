#pragma once
//#include "MyMenu.h"


class CAdvertisement 
{
public:
	CAdvertisement(void);
	~CAdvertisement(void);

	//CMyMenuBar * m_menuBar;
	HWND m_menuCallback;
	bool m_callbackSet;
	bool ad_visible;

	CString m_adTitle;
	CString m_adLink;

	struct THREADPARAM
	{
		HWND hWnd;
		CAdvertisement * m;
	};

	THREADPARAM m_threadParam;

	CWinThread *m_pAdvertisementThread;
	static UINT AdvertisementThread( LPVOID pParam );
	void Start();

};
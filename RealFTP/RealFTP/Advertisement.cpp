#include "StdAfx.h"
#include "Advertisement.h"
#include <windows.h>
#include "WebGrab.h"
#include "Xml.h"
#include "resource.h"
#include "Registry.h"

CAdvertisement::CAdvertisement(void)
{
	m_callbackSet = false;
	m_adTitle = CString(_T(""));
	m_adLink = CString(_T(""));
	ad_visible = true;
}

CAdvertisement::~CAdvertisement(void)
{
}

void CAdvertisement::Start()
{
	CRegistry reg;
	if(reg.IsPurchased())
	{
		return;
	}


	CAdvertisement::THREADPARAM * _param = new CAdvertisement::THREADPARAM;
	_param->m = this;
	m_pAdvertisementThread = AfxBeginThread(CAdvertisement::AdvertisementThread, &_param, THREAD_PRIORITY_LOWEST); 
	if (m_pAdvertisementThread != NULL)
	{
		TRACE(_T("AfxBeginThread: 0x%08lX\n"), m_pAdvertisementThread->m_nThreadID);
	}
}


/**
* FtpSyncWorker
*
*/
UINT CAdvertisement::AdvertisementThread( LPVOID pParam )
{
	CAdvertisement::THREADPARAM *const pThreadParam = static_cast<CAdvertisement::THREADPARAM *>(pParam);
	
	bool running = true;

	while(running){
		//TRACE0("Advertisement \n");
		//pThreadParam->m->m_ftpManager.SyncServerConnections();

		bool visible = true;
		
		HWND app_hwnd = AfxGetMainWnd()->GetSafeHwnd();
		HWND hwnd = GetForegroundWindow();
		if(hwnd == NULL)
		{
			TRACE0(" -- No app has focus \n");
			//AfxMessageBox( _T(" No app has focus  ") );
		}
		else if(hwnd == app_hwnd)
		{
			TRACE0(" -- has focus \n");
			//AfxMessageBox( _T(" has focus  ") );
		}
		else 
		{
			visible = false;
			TRACE0(" -- app not focused \n");
			//AfxMessageBox( _T(" app not focused  ") );
		}
		
#ifdef DEBUG
		visible = true;
#endif

		bool window_visible = IsWindowVisible(app_hwnd);
		bool is_iconic = IsIconic(app_hwnd);
		

		if(window_visible && !is_iconic) // Visible = not tool tray. Iconic = minimized
		{
			CWebGrab grab;
			CString webUrl(_T("http://realftp.net/app/advertisement.php"));
			CString strBuffer;
			if (grab.GetFile(webUrl, strBuffer, _T("RealFTP"), NULL)) {
			
				CXml xml;
				CString title(_T(""));
				CString link(_T(""));
				CString ad_id(_T(""));
				xml.GetTag(strBuffer, CString(_T("title")), title);
				xml.GetTag(strBuffer, CString(_T("link")), link);
				xml.GetTag(strBuffer, CString(_T("id")), ad_id);

				link = CString(_T("http://realftp.net/app/click.php?id=") + ad_id);

				//pThreadParam->m->m_adTitle = CString(_T("a")); //title;

				// Send blank ad. (prvent people from clicking on different ad from displayed)
				CAdvertisement * blankAd = new CAdvertisement();
				blankAd->m_adTitle = CString(_T(""));
				blankAd->m_adLink = CString(_T(""));
				PostMessage(AfxGetMainWnd()->GetSafeHwnd(), ID_AD_CHANGED, 0, (LPARAM)blankAd);

				Sleep(750);

				// Send ad event to be displayed. (goes to main -> menu)
				CAdvertisement * ad = new CAdvertisement();
				ad->m_adTitle = title;
				ad->m_adLink = link;
				PostMessage(AfxGetMainWnd()->GetSafeHwnd(), ID_AD_CHANGED, 0, (LPARAM)ad);
		
			}
		} 
		else  // Not visible
		{
			CAdvertisement * blankAd = new CAdvertisement();
			blankAd->m_adTitle = CString(_T(""));
			blankAd->m_adLink = CString(_T(""));
			PostMessage(AfxGetMainWnd()->GetSafeHwnd(), ID_AD_CHANGED, 0, (LPARAM)blankAd);

			Sleep(750);

			// Send ad event to be displayed. (goes to main -> menu)
			//CAdvertisement * ad = new CAdvertisement();
			//ad->m_adTitle = title;
			//ad->m_adLink = link;
			//PostMessage(AfxGetMainWnd()->GetSafeHwnd(), ID_AD_CHANGED, 0, (LPARAM)ad);
		}

		
		/*
		for(){
			bool window_visible = IsWindowVisible(app_hwnd);
			bool is_iconic = IsIconic(app_hwnd);
			if(window_visible && !is_iconic)
			{

			}	
		}
		*/

		Sleep(1000 * 60);

		CRegistry reg;
		running = !reg.IsPurchased();

	}
	return 0;
}


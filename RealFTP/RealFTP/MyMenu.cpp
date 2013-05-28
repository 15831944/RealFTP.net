#include "stdafx.h"
#include "MyMenu.h"
#include "DefaultWebBrowser.h"
#include "Registry.h"
#include "UpgradeDialog.h"


BEGIN_MESSAGE_MAP(CMyMenuBar, CMFCMenuBar)
	ON_MESSAGE(ID_AD_CHANGED, &CMyMenuBar::OnAdChanged)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


CMyMenuBar::CMyMenuBar(void)
{
	m_adInitalized = false;
	m_adWidth = 500;
	m_adVisible = true;
	CRegistry reg;
	if(reg.IsPurchased())
	{
		m_adVisible = false;
	}
	//m_ad = new CAdvertisement();
	//m_ad.m_menuBar = this;
	//m_ad.Start();
}


CMyMenuBar::~CMyMenuBar(void)
{	
}


/**
* OnFillBackground
*
*/
void CMyMenuBar::OnFillBackground(CDC* pDC)
{
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(rc,RGB(236,236,236));

	if(!m_adInitalized){
		m_adInitalized = true;
		HWND hWnd = GetSafeHwnd();
		m_ad.m_menuCallback = hWnd;
		m_ad.m_callbackSet = true;
		m_ad.Start();
	}
	
	if(m_adInitalized && m_adVisible)
	{
		CRect rect;
		GetClientRect(&rect);
		if(rect.right > 250)
		{
			m_ad.ad_visible = true;

			CString adText(m_ad.m_adTitle);
			m_adWidth = (adText.GetLength() * 8) + 50;
			if(m_adWidth == 0) // ad is 0.
			{
				m_adWidth = 500;
			}
			if(m_adWidth > rect.right - 200) // Ad is too long for window - shrink it.
			{
				m_adWidth = rect.right - 200;
				adText = adText.Mid(0, ((m_adWidth - 50)/8) );
			}

			int height = 18;
			int progress_x = rect.right - m_adWidth;
			int progress_y = 1;

			
			
			// background
			CPoint background_fill_points[4];
			background_fill_points[0] = CPoint(progress_x, progress_y);
			background_fill_points[1] = CPoint(progress_x, progress_y + height);
			background_fill_points[2] = CPoint(progress_x+(rect.right - progress_x)-20, progress_y + height);
			background_fill_points[3] = CPoint(progress_x+(rect.right - progress_x)-20, progress_y);
			CBrush backBrush(RGB(230, 230, 230));
			CBrush *oldBackBrush = pDC->SelectObject(&backBrush);
			CPen penBack(PS_NULL,0,RGB(0,0,0));
			pDC->SelectObject(&penBack);
			pDC->Polygon(background_fill_points, 4);
			
			// border
			CPoint prorgess_border_points[5];
			prorgess_border_points[0] = CPoint(progress_x, progress_y);
			prorgess_border_points[1] = CPoint(progress_x, progress_y + height);
			prorgess_border_points[2] = CPoint(progress_x+(rect.right - progress_x)-20, progress_y + height);
			prorgess_border_points[3] = CPoint(progress_x+(rect.right - progress_x)-20, progress_y);
			prorgess_border_points[4] = CPoint(progress_x, progress_y);
			CPen penOutline(PS_SOLID, 1, RGB(222, 222, 222));
			pDC->SelectObject(&penOutline);
			pDC->Polyline(prorgess_border_points, 5);

			
			//adText = adText + CString(_T(" . test bla bla bla, aha test"));

			CFont * fntTitle1 = new CFont;
			fntTitle1->CreateFont(16 /*height*/, 8 /*width*/, 0, 0,
						  FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
						OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
						ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
						_T("Arial"));
			//pDC->SetTextAlign( TA_BASELINE | TA_LEFT );
			COLORREF oldTextColor = pDC->SetTextColor(RGB(43, 93, 200)); // 100, 100, 110
			
			if(adText.GetLength() * 8 > (m_adWidth - 20))
			{
				//adText = adText.Mid(0, ((m_adWidth - 20)/8) );
				//adText = adText + CString(_T("..."));
			}

			CFont *fntOld = pDC->SelectObject(fntTitle1);
			pDC->SetBkMode(TRANSPARENT);
			pDC->TextOut( progress_x + 5, 2, adText.AllocSysString(), adText.GetLength() );
			

			// Seperator
			CPoint seperator_points[2];
			seperator_points[0] = CPoint(progress_x+(rect.right - progress_x)-20 - 20, progress_y + 0);
			seperator_points[1] = CPoint(progress_x+(rect.right - progress_x)-20 - 20, progress_y + height - 0);
			pDC->SelectObject(&penOutline);
			pDC->Polyline(seperator_points, 2);

			// Ex
			CPen penEx(PS_SOLID, 1, RGB(120, 120, 120));
			CPoint ex_one_points[2]; //  up to right
			ex_one_points[0] = CPoint(progress_x+(rect.right - progress_x)-20 - 5 - 1, progress_y + 4 + 1);
			ex_one_points[1] = CPoint(progress_x+(rect.right - progress_x)-20 - 16 + 1, progress_y + height - 3 - 1);
			pDC->SelectObject(&penEx);
			pDC->Polyline(ex_one_points, 2);
			CPoint ex_two_points[2]; // down to right
			ex_two_points[0] = CPoint(progress_x+(rect.right - progress_x)-20 - 15 + 1, progress_y + 4 + 1);
			ex_two_points[1] = CPoint(progress_x+(rect.right - progress_x)-20 - 4 - 1, progress_y + height - 3 - 1);
			pDC->SelectObject(&penEx);
			pDC->Polyline(ex_two_points, 2);


			// Set 
			pDC->SelectObject(fntOld);
			pDC->SetTextColor(oldTextColor);
		} else {
			m_ad.ad_visible = false;
		}
	}
}


/**
* OnAdChanged
*
*/
LRESULT CMyMenuBar::OnAdChanged(WPARAM wParam, LPARAM lParam)
{
	CAdvertisement * ad = (CAdvertisement*)lParam;
	if(ad)
	{
		m_ad.m_adTitle = ad->m_adTitle;
		m_ad.m_adLink = ad->m_adLink;
		
		delete ad;

		Invalidate();
	}
	return 1;
}


/**
* OnLButtonDown
*
*/
void CMyMenuBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	CMFCMenuBar::OnLButtonDown(nFlags, point);

	CRect rect;
	GetClientRect(&rect);
	if(rect.right > 700 && m_adVisible)
	{
		int height = 18;
		int progress_x = rect.right - m_adWidth;
		int progress_y = 1;

		{
			int left = progress_x;
			int right = progress_x+(rect.right - progress_x)-20 - 20;
			int top = progress_y;
			int bottom = progress_y + height;
			if(point.x > left && point.x < right && point.y > top && point.y < bottom)
			{
				TRACE1(" Ad Clicked: %s \n", m_ad.m_adTitle );

				DefaultWebBrowser b;
				b.launch(m_ad.m_adLink);
			}
		}

		{
			int left = progress_x+(rect.right - progress_x)-20 - 20;
			int right = progress_x+(rect.right - progress_x)-20;
			int top = progress_y;
			int bottom = progress_y + height;
			if(point.x > left && point.x < right && point.y > top && point.y < bottom)
			{
				TRACE0(" Ad Remove: %s \n" );

				CUpgradeDialog upgradeDlg;
				upgradeDlg.DoModal();
			}
		}
	}
}


/**
* HideAds
*
* Description: hide ads in toolbar. called by purchase.
*/
void CMyMenuBar::HideAds()
{
	m_adVisible = false;
	Invalidate();
}
#include "stdafx.h"
#include "UrlField.h"


BEGIN_MESSAGE_MAP(CUrlField, CEdit)
	ON_WM_PAINT()  
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	//ON_WM_CTLCOLOR()
	//ON_CBN_SETFOCUS(1234, &CUrlField::OnSetFocus)
	//ON_NOTIFY_REFLECT_EX(CBN_SELCHANGE, CUrlField::OnSelChange)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelChange)
END_MESSAGE_MAP()


CUrlField::CUrlField(void)
{
	m_selectedText = CString(_T(""));
}

CUrlField::~CUrlField(void)
{
}

void CUrlField::OnLButtonDown(UINT nFlags, CPoint point)
{
	//int i = 0;

	//CBitmapButton::OnLButtonDown(nFlags, point);
}

/*
BOOL CSearchField::PreTranslateMessage(MSG* pMsg)
{
   
	if(pMsg->message == WM_LBUTTONUP){
		int i = 0;
		//TRACE0(" lbutton ");
	}

	TRACE1(" bm: %d \n", pMsg->message);

    return FALSE; // all other cases still need default processing
}
*/

void CUrlField::OnButtonBitmap()
{
	int i = 0;
}

void CUrlField::OnPaint()
{
    CPaintDC dc (this);
	//CRealMemDC pDC(&dc);
	
	CRect rect;
	GetClientRect(&rect);
	

	//Manual BG repaint
	CBrush bg;
	bg.CreateStockObject(RGB(0,0,0));
	CBrush fillBrush(RGB(0, 0, 0));
	CBrush *oldFillBrush = dc.SelectObject(&fillBrush);
	rect = CRect( CPoint(rect.left, rect.top), CPoint(rect.right, rect.bottom - 2) );
	dc.FillRect(&rect, &bg);
	//pDC.FillRect(&rect, &bg);

	/// Icon - drop down arrow
	CBitmap bmp;
	if (bmp.LoadBitmap(IDB_DOWN_ARROW))
	{
      // Get the size of the bitmap
      BITMAP bmpInfo;
      bmp.GetBitmap(&bmpInfo);
      CDC dcMemory;
      dcMemory.CreateCompatibleDC(&dc);
      CBitmap* pOldBitmap = dcMemory.SelectObject(&bmp);
      CRect rect;
      GetClientRect(&rect);	  
      int nX = rect.right-rect.left - 14; // rect.left + (rect.Width() - bmpInfo.bmWidth) / 2;
      int nY = 9; // rect.top + (rect.Height() - bmpInfo.bmHeight) / 2;
      dc.BitBlt(nX, nY, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcMemory, 
         0, 0, SRCCOPY);
      dcMemory.SelectObject(pOldBitmap);
	}
   
	int width = rect.right-rect.left;
	int progress_x = 0;
	int progress_y = 0;
	CPoint prorgess_border_points[5];
	prorgess_border_points[0] = CPoint(progress_x, progress_y);
	prorgess_border_points[1] = CPoint(progress_x, progress_y + 21);
	prorgess_border_points[2] = CPoint(progress_x+(width)-1, progress_y + 21);
	prorgess_border_points[3] = CPoint(progress_x+(width)-1, progress_y);
	prorgess_border_points[4] = CPoint(progress_x, progress_y);
	CPen penOutline(PS_SOLID, 1, RGB(115, 115, 115));
	dc.SelectObject(&penOutline);
	dc.Polyline(prorgess_border_points, 5);

	// bottom line
	CPen penBot(PS_SOLID, 1, RGB(236, 236, 236));
	dc.SelectObject(&penBot);
	CPoint bot_points[2];
	bot_points[0] = CPoint(progress_x, progress_y + 22);
	bot_points[1] = CPoint(progress_x + width, progress_y + 22);
	dc.Polyline(bot_points, 2);


	//dc.SetTextColor(RGB(0, 255, 0));
	
}


void CUrlField::OnDraw(CDC* dc) // pDC
{
	CRealMemDC pDC(dc);
	CRect rect;
	GetClientRect(&rect);
	//pDC->DPtoLP(&rect);

	//Manual BG repaint
	CBrush bg;
	bg.CreateStockObject(RGB(0,0,0));
	CBrush fillBrush(RGB(0, 0, 0));
	CBrush *oldFillBrush = pDC->SelectObject(&fillBrush);
	rect = CRect( CPoint(rect.left, rect.top), CPoint(rect.right, rect.bottom - 2) );
	pDC->FillRect(&rect, &bg);

	/// Icon
	CBitmap bmp;
	if (bmp.LoadBitmap(IDB_DOWN_ARROW))
	{
      // Get the size of the bitmap
      BITMAP bmpInfo;
      bmp.GetBitmap(&bmpInfo);
      CDC dcMemory;
      dcMemory.CreateCompatibleDC( pDC );
      CBitmap* pOldBitmap = dcMemory.SelectObject(&bmp);
      CRect rect;
      GetClientRect(&rect);	  
      int nX = rect.right-rect.left - 14; // rect.left + (rect.Width() - bmpInfo.bmWidth) / 2;
      int nY = 9; // rect.top + (rect.Height() - bmpInfo.bmHeight) / 2;
      pDC->BitBlt(nX, nY, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcMemory, 
         0, 0, SRCCOPY);
      dcMemory.SelectObject(pOldBitmap);
	}
   
	int width = rect.right-rect.left;
	int progress_x = 0;
	int progress_y = 0;
	CPoint prorgess_border_points[5];
	prorgess_border_points[0] = CPoint(progress_x, progress_y);
	prorgess_border_points[1] = CPoint(progress_x, progress_y + 21);
	prorgess_border_points[2] = CPoint(progress_x+(width)-1, progress_y + 21);
	prorgess_border_points[3] = CPoint(progress_x+(width)-1, progress_y);
	prorgess_border_points[4] = CPoint(progress_x, progress_y);
	CPen penOutline(PS_SOLID, 1, RGB(115, 115, 115));
	pDC->SelectObject(&penOutline);
	pDC->Polyline(prorgess_border_points, 5);
}


BOOL CUrlField::OnEraseBkgnd(CDC* pDC)
{
	//pDC->SetTextColor(RGB(0, 255, 0));

	/*
	// TODO: Add your message handler code here and/or call default
	COLORREF   rgbBackGnd   =   RGB(236,236,236); 
	CBrush   brush; 
	brush.CreateSolidBrush(rgbBackGnd); 
	CBrush *pOldbrush = pDC->SelectObject(&brush);
	CRect   rect;     
	GetClientRect(&rect); 
	
	pDC->FillRect(rect,&brush);

	//return FALSE;
	return CComboBox::OnEraseBkgnd(pDC);
	*/
	return FALSE;
}



HBRUSH CUrlField::OnCtlColor(CDC* pDC, CWnd *pWnd, UINT nCtlColor)
{
	//return CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);
	CBrush m_hRBrush(RGB(255,0,255));

    switch ( nCtlColor )
	{
        case CTLCOLOR_LISTBOX:
             pDC->SetBkMode(TRANSPARENT);
             //pDC->SetTextColor(RGB(255,0,0));
             //pDC->SetBkColor(RGB(255,255,255)); 
             return m_hRBrush;
             break;
		case CTLCOLOR_STATIC:
			pDC->SetTextColor(RGB(255,0,0));
			pDC->SetBkMode(TRANSPARENT);
			return m_hRBrush;
			break;
        default:
             break;
   }
}

/**
* OnUrlSetFocus
*
* Description: Remove automatic selection of all text.
*/
void CUrlField::OnSetFocus()
{
	CString urlValue;
	this->GetWindowTextW(urlValue);
	this->SetEditSel(urlValue.GetLength(), urlValue.GetLength());
}

void CUrlField::OnSize(UINT nType, int cx, int cy)
{
	CComboBox::OnSize(nType, cx, cy);

	CString urlValue;
	this->GetWindowTextW(urlValue);
	this->SetEditSel(urlValue.GetLength(), urlValue.GetLength());
}

void CUrlField::OnSelChange() 
{
	int nIndex = GetCurSel() ;
	if(nIndex != LB_ERR) 
	{
		GetLBText( nIndex, m_selectedText );
		int i = 0;
	}
}


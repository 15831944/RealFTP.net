#include "stdafx.h"
#include "SearchField.h"


BEGIN_MESSAGE_MAP(CSearchField, CComboBox)
	ON_WM_PAINT ()  // JDT
	ON_WM_ERASEBKGND()
	//ON_WM_LBUTTONDOWN()
	//ON_EN_CHANGE(1236, &CMyButton::OnButtonBitmap) // yes
	//ON_BN_CLICKED(1236, &CMyButton::OnButtonBitmap)
	//ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// ON_CBN_SETFOCUS


CSearchField::CSearchField(void)
{
	//CSearchField::CSearchField();
}

CSearchField::~CSearchField(void)
{
}

void CSearchField::OnLButtonDown(UINT nFlags, CPoint point)
{
	int i = 0;

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

void CSearchField::OnButtonBitmap()
{
	int i = 0;
}

void CSearchField::OnPaint()
{
    CPaintDC dc (this);
    
	CRect rect;
	GetClientRect(&rect);
	//Manual BG repaint
	CBrush bg;
	bg.CreateStockObject(RGB(0,0,0));
	CBrush fillBrush(RGB(250, 0, 0));
	CBrush *oldFillBrush = dc.SelectObject(&fillBrush);
	rect = CRect( CPoint(rect.left, rect.top), CPoint(rect.right, rect.bottom - 2) );
	dc.FillRect(&rect, &bg);

	/// Icon
	CBitmap bmp;
	if (bmp.LoadBitmap(IDB_MAG))
	{
      // Get the size of the bitmap
      BITMAP bmpInfo;
      bmp.GetBitmap(&bmpInfo);
      CDC dcMemory;
      dcMemory.CreateCompatibleDC(&dc);
      CBitmap* pOldBitmap = dcMemory.SelectObject(&bmp);
      CRect rect;
      GetClientRect(&rect);	  
      int nX = rect.right-rect.left - 20; // rect.left + (rect.Width() - bmpInfo.bmWidth) / 2;
      int nY = 3; // rect.top + (rect.Height() - bmpInfo.bmHeight) / 2;
      dc.BitBlt(nX, nY, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcMemory, 
         0, 0, SRCCOPY);
      dcMemory.SelectObject(pOldBitmap);
   }


	int progress_x = 0;
	int progress_y = 0;
	CPoint prorgess_border_points[5];
	prorgess_border_points[0] = CPoint(progress_x, progress_y);
	prorgess_border_points[1] = CPoint(progress_x, progress_y + 21);
	prorgess_border_points[2] = CPoint(progress_x+(rect.right-rect.left)-1, progress_y + 21);
	prorgess_border_points[3] = CPoint(progress_x+(rect.right-rect.left)-1, progress_y);
	prorgess_border_points[4] = CPoint(progress_x, progress_y);
	CPen penOutline(PS_SOLID, 1, RGB(115, 115, 115));
	dc.SelectObject(&penOutline);
	dc.Polyline(prorgess_border_points, 5);

	//Invalidate();
}


BOOL CSearchField::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	COLORREF   rgbBackGnd   =   RGB(236,236,236); 
	CBrush   brush; 
	brush.CreateSolidBrush(rgbBackGnd); 
	CBrush *pOldbrush = pDC->SelectObject(&brush);
	CRect   rect;     
	GetClientRect(&rect); 
	// pDC->Rectangle(&rect);
	pDC->FillRect(rect,&brush);

	return CComboBox::OnEraseBkgnd(pDC);
}



HBRUSH CSearchField::OnCtlColor(CDC* pDC, CWnd *pWnd, UINT nCtlColor)
{
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
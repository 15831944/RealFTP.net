#include "stdafx.h"
#include "MyToolbar.h"

BEGIN_MESSAGE_MAP(CMyToolBar, CMFCToolBar)
	//ON_WM_PAINT ()  // JDT
END_MESSAGE_MAP()

void  CMyToolBar::OnFillBackground(CDC* pDC)
{
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(rc,RGB(236,236,236));
}

void CMyToolBar::OnPaint ()
{
	
    CPaintDC pDC(this);
    
    //CRect rect;
    //GetClientRect (&rect);

	int progress_x = 1;
	int progress_y = 4;
	CPoint prorgess_border_points[5];
	prorgess_border_points[0] = CPoint(progress_x, progress_y);
	prorgess_border_points[1] = CPoint(progress_x, progress_y + 5);
	prorgess_border_points[2] = CPoint(progress_x+75, progress_y + 5);
	prorgess_border_points[3] = CPoint(progress_x+75, progress_y);
	prorgess_border_points[4] = CPoint(progress_x, progress_y);
	CPen penOutline(PS_SOLID,1,RGB(204,204,204));
	pDC.SelectObject(&penOutline);
	pDC.Polyline(prorgess_border_points, 5);
}
#include "StdAfx.h"
#include "MyVisualManager.h"
#include "resource.h"

IMPLEMENT_DYNCREATE(CMyVisualManager, CMFCVisualManagerVS2008)

CMyVisualManager::CMyVisualManager(void)
{
	
}

CMyVisualManager::~CMyVisualManager(void)
{
}


void CMyVisualManager::OnFillBarBackground(
	   CDC* pDC,
	   CBasePane* pBar,
	   CRect rectClient,
	   CRect rectClip,
	   BOOL bNCArea )
{
	//CMFCVisualManagerVS2008::OnFillBarBackground(pDC, pBar, rectClient, rectClip, bNCArea);

	// Paint background color
	CBrush backBrush(RGB(236, 236, 236));
	CBrush* pOldBrush = pDC->SelectObject(&backBrush);
	CRect rect;
	pDC->GetClipBox(&rect);     // Erase the area needed
	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);

}

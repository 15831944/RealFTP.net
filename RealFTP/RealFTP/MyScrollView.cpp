#include "StdAfx.h"
#include "MyScrollView.h"


IMPLEMENT_DYNCREATE(CMyScrollView, CScrollView)

CMyScrollView::CMyScrollView(void)
{
	
}

CMyScrollView::~CMyScrollView(void){}

void CMyScrollView::OnDraw(CDC* pDC)
{
	//CSplitterWindowTutorialDoc* pDoc = GetDocument();
	int i = 0;
	//CScrollView::OnDraw(pDC);
}

void CMyScrollView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	CSize size;
	size.cx = size.cy = 1000;
	SetScrollSizes(MM_LOENGLISH, size); 
}


// SplitterWindowTutorialView.cpp : implementation of the CSplitterWindowTutorialView class
//

#include "stdafx.h"
#include "RealFTP.h"
#include "ConnectionListDoc.h"
#include "ConnectionListView.h"
#include "resource.h"
#include "MainFrm.h"
#include "MenuNavigator.h"

#define CONNECTION_SEPERATION 48

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView

IMPLEMENT_DYNCREATE(CConnectionListView, CScrollView) // CView

BEGIN_MESSAGE_MAP(CConnectionListView, CScrollView)
	//{{AFX_MSG_MAP(CConnectionListView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_WM_ERASEBKGND()
	//ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()    // CSCrollView
	ON_WM_VSCROLL() // CSCrollView
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView construction/destruction

CConnectionListView::CConnectionListView()
{
	m_vScrollPos = 0;
	m_clickDownY = 0;
	m_selectedItemIndex = -1;
	m_selectedItemDragY = 0;
	m_connectionManager.Load();
}

CConnectionListView::~CConnectionListView()
{
}

//void CConnectionListView::OnInitialUpdate()
//{
//	int i = 0;
//}

/*
BOOL CConnectionListView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}
*/

void CConnectionListView::OnSize(UINT nType, int cx, int cy)
{
	CRect rect;
	GetClientRect( &rect );

	int x = 0;
	int y = 0;
	
	CObArray * listOfConnections;
	listOfConnections = m_connectionManager.GetConnections();
	y = CONNECTION_SEPERATION * listOfConnections->GetSize();

	CSize size;
	size.cx = x + 10;
	size.cy = y + 10;
	SetScrollSizes(MM_TEXT, size); // fails from thread
}


void CConnectionListView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if(nSBCode == 0 || nSBCode == 2){
		m_vScrollPos -= 20;
		if(m_vScrollPos < 0){
				m_vScrollPos = 0;
		}
		CPoint pt( 0, m_vScrollPos );
		ScrollToPosition( pt );
	}
	// Scroll
	if(nSBCode == 5){
		CPoint pt( 0, nPos );
		ScrollToPosition( pt );
		m_vScrollPos = nPos;
	}
	if(nSBCode == 1 || nSBCode == 3){
		m_vScrollPos += 20;
		CRect rect;
		GetClientRect( &rect );
		CSize s = GetTotalSize(); // buggy  need range
		if(m_vScrollPos > (s.cy - rect.Height())){
			m_vScrollPos = (s.cy - rect.Height());
		}
		CPoint pt( 0, m_vScrollPos );
		ScrollToPosition( pt );
	}

	//Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView drawing

void CConnectionListView::OnPaint()
{
    CPaintDC dc (this);
    /*
	CRect rect;
	GetClientRect(&rect);
	//pDC->DPtoLP(&rect);

	//Manual BG repaint
	CBrush bg;
	bg.CreateStockObject(RGB(0,0,0));
	CBrush fillBrush(RGB(0, 0, 0));
	CBrush *oldFillBrush = dc.SelectObject(&fillBrush);
	dc.FillRect(&rect, &bg);

	int index = 0;

	// Draw connections
	CObArray * listOfConnections;
	listOfConnections = m_connectionManager.GetConnections();
	int i = 0;
	for(i = 0; i <  listOfConnections->GetSize(); i++){
		CConnection * con = (CConnection *)listOfConnections->GetAt(i);
		//TRACE1("  el: %s \n", con->name );
		if(con->m_path.GetLength() == 0){
			this->DrawServerConnection(&dc, index++, con->name, con->progress);
		} else {
			this->DrawBookmarkConnection(&dc, index++, con->name);
		}
	}

	// draw selected item last / on top when dragging
	if(m_selectedItemIndex >= 0 && m_selectedItemDragY > 0){
		CConnection * con = (CConnection *)listOfConnections->GetAt(m_selectedItemIndex);
		if(con->m_path.GetLength() == 0){
			this->DrawServerConnection(&dc, m_selectedItemIndex, con->name, con->progress);
		} else {
			this->DrawBookmarkConnection(&dc, m_selectedItemIndex, con->name);
		}
	}
	*/
}


/**
* OnDraw
*
* Description:
*/
void CConnectionListView::OnDraw(CDC* dc) // pDC
{
	CConnectionListDoc* pDoc = GetDocument();
	
	CRealMemDC pDC(dc);

	CRect rect;
	GetClientRect(&rect);
	pDC->DPtoLP(&rect);

	//Manual BG repaint
	CBrush bg;
	bg.CreateStockObject(RGB(0,0,0));
	CBrush fillBrush(RGB(0, 0, 0));
	CBrush *oldFillBrush = pDC->SelectObject(&fillBrush);
	pDC->FillRect(&rect, &bg);

	int index = 0;

	// Draw connections
	CObArray * listOfConnections;
	listOfConnections = m_connectionManager.GetConnections();
	int i = 0;
	for(i = 0; i <  listOfConnections->GetSize(); i++){
		CConnection * con = (CConnection *)listOfConnections->GetAt(i);
		//TRACE1("  el: %s \n", con->name );
		if(con->m_path.GetLength() == 0){
			this->DrawServerConnection(pDC, index++, con->name, con->progress);
		} else {
			this->DrawBookmarkConnection(pDC, index++, con->name);
		}
	}

	// draw selected item last / on top when dragging
	if(m_selectedItemIndex >= 0 && m_selectedItemDragY > 0){
		CConnection * con = (CConnection *)listOfConnections->GetAt(m_selectedItemIndex);
		if(con->m_path.GetLength() == 0){
			this->DrawServerConnection(pDC, m_selectedItemIndex, con->name, con->progress);
		} else {
			this->DrawBookmarkConnection(pDC, m_selectedItemIndex, con->name);
		}
	}

	// draw transfers
	for(i = 0; i < transfers.size(); i++ )
	{
		CFileTransfer * t;
		t = (CFileTransfer*)transfers[i];
		
		if(t->progress != 100)
		{
			this->DrawFileTransfer(pDC, index++, t->sending, t->fileName, t->progress);
			//TRACE1(" t %d  \n", index);
		}
		if(t->progress == 100)
		{
			//transfers.pop_back(); // is this correct?
			transfers.erase(transfers.begin()+i);

			// delete
			delete t;
		}
	}
}


/**
* DrawServerConnection
*
* Description: 
*/
void CConnectionListView::DrawServerConnection(CDC* pDC, int pos, CString name, int progress)
{
	int drawY = 0; if(m_selectedItemIndex == pos){ drawY = m_selectedItemDragY; }
	int progress_x = 56;
	int progress_y = (pos * CONNECTION_SEPERATION) + 40;
	CRect rect;
	GetClientRect(&rect);

	// Background
	CPoint background_fill_points[4];
	background_fill_points[0] = CPoint(0, progress_y + -30 + drawY);
	background_fill_points[1] = CPoint(0, progress_y + 20 + drawY);
	background_fill_points[2] = CPoint(rect.right, progress_y + 20 + drawY);
	background_fill_points[3] = CPoint(rect.right, progress_y + -30 + drawY);
	CBrush backBrush(RGB(255, 255, 255));
	CBrush *oldBackBrush = pDC->SelectObject(&backBrush);
	CPen penBack(PS_NULL,0,RGB(0,0,0));
	pDC->SelectObject(&penBack);
	pDC->Polygon(background_fill_points, 4);


	// Title
	CFont * fntTitle1 = new CFont;
	fntTitle1->CreateFont(16 /*height*/, 8 /*width*/, 0, 0,
			      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
				_T("Arial"));
    pDC->SetTextAlign( TA_BASELINE | TA_LEFT );
	COLORREF oldTextColor = pDC->SetTextColor(RGB(60, 60, 60));
	CFont *fntOld = pDC->SelectObject(fntTitle1);
	pDC->TextOut( 56, (pos * CONNECTION_SEPERATION) + 33 + drawY, name.AllocSysString(), name.GetLength() );

	
	// Progress Bar
	CPoint prorgess_border_points[5];
	prorgess_border_points[0] = CPoint(progress_x, progress_y + drawY);
	prorgess_border_points[1] = CPoint(progress_x, progress_y + 5 + drawY);
	prorgess_border_points[2] = CPoint(progress_x+75, progress_y + 5 + drawY);
	prorgess_border_points[3] = CPoint(progress_x+75, progress_y + drawY);
	prorgess_border_points[4] = CPoint(progress_x, progress_y + drawY);
	CPen penOutline(PS_SOLID,1,RGB(204,204,204));
	CPen lightPenOutline(PS_SOLID,1,RGB(214,214,214));
	if(progress == 100){
		pDC->SelectObject(&lightPenOutline);
	} else {
		pDC->SelectObject(&penOutline);
	}
	pDC->Polyline(prorgess_border_points, 5);
	
	CPoint prorgess_fill_points[4];
	prorgess_fill_points[0] = CPoint(progress_x+1, progress_y + 1 + drawY);
	prorgess_fill_points[1] = CPoint(progress_x+1, progress_y + 5 + drawY);
	prorgess_fill_points[2] = CPoint(progress_x+((int)((float)progress * 0.75)), progress_y + 5 + drawY);
	prorgess_fill_points[3] = CPoint(progress_x+((int)((float)progress * 0.75)), progress_y + 1 + drawY);
	CBrush fillBrush(RGB(227, 227, 227));
	CBrush lightfillBrush(RGB(237, 237, 237));
	if(progress == 100){
		pDC->SelectObject(&lightfillBrush);
	} else {
		pDC->SelectObject(&fillBrush);
	}
	CPen penFill(PS_NULL,0,RGB(0,0,0));
	pDC->SelectObject(&penFill);
	pDC->Polygon(prorgess_fill_points, 4);
	
	
	// Icon
	CBitmap bmp;
	if (bmp.LoadBitmap(IDB_DRIVE_MED))
	{
		// Get the size of the bitmap
		BITMAP bmpInfo;
		bmp.GetBitmap(&bmpInfo);
		CDC dcMemory;
		dcMemory.CreateCompatibleDC(pDC);
		CBitmap* pOldBitmap = dcMemory.SelectObject(&bmp);
		CRect rect;
		GetClientRect(&rect);
		int nX = 10; // rect.left + (rect.Width() - bmpInfo.bmWidth) / 2;
		int nY = (pos * CONNECTION_SEPERATION) + 16 + drawY; // rect.top + (rect.Height() - bmpInfo.bmHeight) / 2;
		pDC->BitBlt(nX, nY, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcMemory, 
			0, 0, SRCCOPY);
		dcMemory.SelectObject(pOldBitmap);
	}

	// Selected
	if(m_selectedItemIndex == pos)
	{
		CPoint prorgess_fill_points[3];
		prorgess_fill_points[0] = CPoint(rect.right - 10, progress_y + -5);
		prorgess_fill_points[1] = CPoint(rect.right - 0, progress_y + 5 );
		prorgess_fill_points[2] = CPoint(rect.right - 0, progress_y -15);
		CBrush fillBrush(RGB(237, 237, 237));
		CBrush *oldFillBrush = pDC->SelectObject(&fillBrush);
		CPen penFill(PS_NULL,0,RGB(0,0,0));
		pDC->SelectObject(&penFill);
		pDC->Polygon(prorgess_fill_points, 3);

		CPoint prorgess_border_points[4]; 
		prorgess_border_points[0] = CPoint(rect.right - 10, progress_y + -5);
		prorgess_border_points[1] = CPoint(rect.right - 0, progress_y + 5 );
		prorgess_border_points[2] = CPoint(rect.right - 0, progress_y -15);
		prorgess_border_points[3] = CPoint(rect.right - 10, progress_y + -5);
		CPen penOutline(PS_SOLID,1,RGB(214,214,214));
		pDC->SelectObject(&penOutline);
		pDC->Polyline(prorgess_border_points, 4);
	}
}


/**
* DrawBookmarkConnection
*
* Description: 
*/
void CConnectionListView::DrawBookmarkConnection(CDC* pDC, int pos, CString name)
{
	int drawY = 0; if(m_selectedItemIndex == pos){ drawY = m_selectedItemDragY; }
	CRect rect;
	GetClientRect(&rect);

	CFont * fntTitle1 = new CFont;
	fntTitle1->CreateFont(16 /*height*/, 8 /*width*/, 0, 0,
			      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
				_T("Arial"));
	
	int nameWidth = name.GetLength() * 8;
	int windowWidth = rect.Width();
	if(nameWidth > rect.Width() - 60 ){
		int charsOver = (int)((float)(nameWidth - (rect.Width() - 60)) / (float)8);
		//TRACE1(" too wide %d  \n", charsOver);
		if( name.GetLength() - charsOver > 10 ){
			name = name.Mid(charsOver + 2, name.GetLength() - charsOver - 2);
			name = CString(_T("..")) + name;
		}
	}

    pDC->SetTextAlign( TA_BASELINE | TA_LEFT );
	COLORREF oldTextColor = pDC->SetTextColor(RGB(60, 60, 60));
	CFont *fntOld = pDC->SelectObject(fntTitle1);
	pDC->TextOut(56, (pos * CONNECTION_SEPERATION) + 37 + drawY, name.AllocSysString(), name.GetLength());

	// Folder
	CBitmap fbmp;
	if (fbmp.LoadBitmap(IDB_FOLDER_MED))
	{
      // Get the size of the bitmap
      BITMAP bmpInfo;
      fbmp.GetBitmap(&bmpInfo);
      CDC dcMemory;
      dcMemory.CreateCompatibleDC(pDC);
      CBitmap* pOldBitmap = dcMemory.SelectObject(&fbmp);
      CRect rect;
      GetClientRect(&rect);
      int nX = 12; // rect.left + (rect.Width() - bmpInfo.bmWidth) / 2;
      int nY = (pos * CONNECTION_SEPERATION) + 16; // rect.top + (rect.Height() - bmpInfo.bmHeight) / 2;
      pDC->BitBlt(nX, nY + drawY, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcMemory, 0, 0, SRCCOPY);
      dcMemory.SelectObject(pOldBitmap);
   }
}



/**
* DrawFileTransfer
*
* Description: 
*/
void CConnectionListView::DrawFileTransfer(CDC* pDC, int pos, bool sending, CString name, int progress)
{
	int drawY = 0; if(m_selectedItemIndex == pos){ drawY = m_selectedItemDragY; }
	int progress_x = 56;
	int progress_y = (pos * CONNECTION_SEPERATION) + 40;
	CRect rect;
	GetClientRect(&rect);

	// Background
	CPoint background_fill_points[4];
	background_fill_points[0] = CPoint(0, progress_y + -30 + drawY);
	background_fill_points[1] = CPoint(0, progress_y + 20 + drawY);
	background_fill_points[2] = CPoint(rect.right, progress_y + 20 + drawY);
	background_fill_points[3] = CPoint(rect.right, progress_y + -30 + drawY);
	CBrush backBrush(RGB(255, 255, 255));
	CBrush *oldBackBrush = pDC->SelectObject(&backBrush);
	CPen penBack(PS_NULL,0,RGB(0,0,0));
	pDC->SelectObject(&penBack);
	pDC->Polygon(background_fill_points, 4);


	// Title
	CFont * fntTitle1 = new CFont;
	fntTitle1->CreateFont(16 /*height*/, 8 /*width*/, 0, 0,
			      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
				_T("Arial"));
    pDC->SetTextAlign( TA_BASELINE | TA_LEFT );
	COLORREF oldTextColor = pDC->SetTextColor(RGB(60, 60, 60));
	CFont *fntOld = pDC->SelectObject(fntTitle1);
	pDC->TextOut( 56, (pos * CONNECTION_SEPERATION) + 33 + drawY, name.AllocSysString(), name.GetLength() );

	
	// Progress Bar
	CPoint prorgess_border_points[5];
	prorgess_border_points[0] = CPoint(progress_x, progress_y + drawY);
	prorgess_border_points[1] = CPoint(progress_x, progress_y + 5 + drawY);
	prorgess_border_points[2] = CPoint(progress_x+100, progress_y + 5 + drawY);
	prorgess_border_points[3] = CPoint(progress_x+100, progress_y + drawY);
	prorgess_border_points[4] = CPoint(progress_x, progress_y + drawY);
	CPen penOutline(PS_SOLID,1,RGB(204,204,204));
	CPen lightPenOutline(PS_SOLID,1,RGB(214,214,214));
	if(progress == 100){
		pDC->SelectObject(&lightPenOutline);
	} else {
		pDC->SelectObject(&penOutline);
	}
	pDC->Polyline(prorgess_border_points, 5);
	
	CPoint prorgess_fill_points[4];
	prorgess_fill_points[0] = CPoint(progress_x+1, progress_y + 1 + drawY);
	prorgess_fill_points[1] = CPoint(progress_x+1, progress_y + 5 + drawY);
	prorgess_fill_points[2] = CPoint(progress_x+((int)((float)progress * 1.0)), progress_y + 5 + drawY);
	prorgess_fill_points[3] = CPoint(progress_x+((int)((float)progress * 1.0)), progress_y + 1 + drawY);
	CBrush fillBrush(RGB(227, 227, 227));
	CBrush lightfillBrush(RGB(237, 237, 237));
	if(progress == 100){
		pDC->SelectObject(&lightfillBrush);
	} else {
		pDC->SelectObject(&fillBrush);
	}
	CPen penFill(PS_NULL,0,RGB(0,0,0));
	pDC->SelectObject(&penFill);
	pDC->Polygon(prorgess_fill_points, 4);
	
	if(sending)
	{
		// Icon
		CBitmap bmp;
		if (bmp.LoadBitmap(IDB_UPLOAD))
		{
			// Get the size of the bitmap
			BITMAP bmpInfo;
			bmp.GetBitmap(&bmpInfo);
			CDC dcMemory;
			dcMemory.CreateCompatibleDC(pDC);
			CBitmap* pOldBitmap = dcMemory.SelectObject(&bmp);
			CRect rect;
			GetClientRect(&rect);
			int nX = 13; // rect.left + (rect.Width() - bmpInfo.bmWidth) / 2;
			int nY = (pos * CONNECTION_SEPERATION) + 18 + drawY; // rect.top + (rect.Height() - bmpInfo.bmHeight) / 2;
			pDC->BitBlt(nX, nY, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcMemory, 
				0, 0, SRCCOPY);
			dcMemory.SelectObject(pOldBitmap);
		}
	}
	else 
	{
		// Icon
		CBitmap bmp;
		if (bmp.LoadBitmap(IDB_DOWNLOAD))
		{
			// Get the size of the bitmap
			BITMAP bmpInfo;
			bmp.GetBitmap(&bmpInfo);
			CDC dcMemory;
			dcMemory.CreateCompatibleDC(pDC);
			CBitmap* pOldBitmap = dcMemory.SelectObject(&bmp);
			CRect rect;
			GetClientRect(&rect);
			int nX = 13; // rect.left + (rect.Width() - bmpInfo.bmWidth) / 2;
			int nY = (pos * CONNECTION_SEPERATION) + 18 + drawY; // rect.top + (rect.Height() - bmpInfo.bmHeight) / 2;
			pDC->BitBlt(nX, nY, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcMemory, 
				0, 0, SRCCOPY);
			dcMemory.SelectObject(pOldBitmap);
		}
	}
	
}



void CConnectionListView::UpdateConnections()
{
	m_connectionManager.Load();
	Invalidate();
}


void CConnectionListView::OnMouseMove(UINT nFlags, CPoint point) 
{
	CPoint oldPoint;
	oldPoint.x = point.x;
	oldPoint.y = point.y;
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);

	int offset = point.y - m_clickDownY;
	if(m_clickDownY == -1){
		offset = 0;
	}
	//TRACE3(" x : %d  - %d   - %d \n", m_selectedItemIndex, offset, m_clickDownY);
	if(m_selectedItemIndex >= 0 && offset != 0){
		if(m_selectedItemDragY != offset){
			m_selectedItemDragY = offset;
			Invalidate();
		}
	}
}


/**
* OnLButtonDown
*
*/
void CConnectionListView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CPoint oldPoint;
	oldPoint.x = point.x;
	oldPoint.y = point.y;
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point); 

    // event processing code here
	m_clickDownY = point.y;

	int itemIndex = (point.y - 15) / CONNECTION_SEPERATION;
	CObArray * listOfConnections;
	listOfConnections = m_connectionManager.GetConnections();
	if(itemIndex >= 0 && itemIndex < listOfConnections->GetSize()){ // Item
		
		if(m_selectedItemIndex != itemIndex){
			m_selectedItemIndex = itemIndex;
			//m_connection = con;
			Invalidate(); // Paint selection marker
			//m_clickDownY = point.y;
		}

		//  Load file view
		//CWnd * p = this->GetParent();
		//CWnd * p2 = p->GetParent();

		CMainFrame * mainFrame = (CMainFrame *)AfxGetMainWnd();
		//CMainFrame * mainFrame = (CMainFrame *)this->GetParent()->GetParent(); // bug if layout is changed
		mainFrame->LoadServerConnection(m_selectedItemIndex);

	} else {

		if(m_selectedItemIndex != -1){
			//m_selectedItemIndex = -1;  // Don't deselect server when clicking on empty space.
			//Invalidate(); // Paint selection marker
		}
	}

	//TRACE1(" itemIndex %d \n", itemIndex);

	/*
	CMenu contextMenu;
	CMenu *tracker; 
	contextMenu.LoadMenu(IDR_CONNECTIONS_MENU); 
	ClientToScreen(&point);
	tracker = contextMenu.GetSubMenu(0); 
	//show the context menu
	//tracker->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd()); 
	*/ 
}


void CConnectionListView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CPoint oldPoint;
	oldPoint.x = point.x;
	oldPoint.y = point.y;
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);

	// Check for server order re-arrange
	int offset = point.y - m_clickDownY;
	if(m_clickDownY == -1){
		offset = 0;
	}
	//TRACE1(" offset: %d \n", offset);
	
	
	int movePositions = 0;
	int off = offset;
	while(off > 35)
	{
		movePositions += 1;
		off = off - CONNECTION_SEPERATION;
	}
	while(off < -35)
	{
		movePositions -= 1;
		off = off + CONNECTION_SEPERATION;
	}

	if(offset > 35) // Move down
	{
		// If not allready bottom
		CObArray * listOfConnections;
		listOfConnections = m_connectionManager.GetConnections();
		if(m_selectedItemIndex < listOfConnections->GetSize() -1 )
		{
			//TRACE0("Swap down. \n");

			int replaceIndex = m_selectedItemIndex + off;
			if(replaceIndex >= listOfConnections->GetSize()){
				replaceIndex = listOfConnections->GetSize() -1;
			}

			m_connectionManager.Swap(m_selectedItemIndex, replaceIndex); // m_selectedItemIndex + 1
			m_connectionManager.Store();
			Invalidate(); 
		}
		offset = offset - CONNECTION_SEPERATION;
	}

	if(offset < -35 && m_selectedItemIndex > 0) // Move up if not allready top
	{
		//TRACE0("Swap up. \n");

		int replaceIndex = m_selectedItemIndex + off;
		if(replaceIndex < 0){
			replaceIndex = 0;
		}

		m_connectionManager.Swap(m_selectedItemIndex, replaceIndex); // m_selectedItemIndex - 1
		m_connectionManager.Store();
		Invalidate(); 
		offset = offset + CONNECTION_SEPERATION;
	}

	m_clickDownY = -1; // Unset click Y loc
	if(m_selectedItemDragY != 0){ // Remove any drag
		m_selectedItemDragY = 0;
		Invalidate();
	}
}


/**
* OnRButtonDown
*
* Description: 
*/
void CConnectionListView::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint oldPoint;
	oldPoint.x = point.x;
	oldPoint.y = point.y;
	ClientToScreen(&oldPoint);

	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);

	int itemIndex = (point.y - 15) / CONNECTION_SEPERATION;
	CObArray * listOfConnections;
	listOfConnections = m_connectionManager.GetConnections();
	if(itemIndex >= 0 && itemIndex < listOfConnections->GetSize()){
		CConnection * con = (CConnection *)listOfConnections->GetAt(itemIndex);

		m_selectedItemIndex = itemIndex;
		m_connection = con;

		CMenu contextMenu;
		CMenu *tracker; 
		contextMenu.LoadMenu(IDR_CONNECTION_MENU); 
		ClientToScreen(&point);
		tracker = contextMenu.GetSubMenu(0); 

		// get files for folder   (can be slow)
		std::vector<CFileContainer> subFolders;
		CMenu *subMenu = tracker->GetSubMenu(0);
		CMenuNavigator menuNav;
		menuNav.Reset();
		menuNav.BuildMenu(con, CString(_T("/")), subMenu, 2); 
		subMenu->DeleteMenu(ID_CON_FOLDERS_BACK, MF_BYCOMMAND);
		
		
		//show the context menu
		tracker->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, oldPoint.x, oldPoint.y, AfxGetMainWnd());
	} else { // no item clicked
		CMenu contextMenu;
		CMenu *tracker; 
		contextMenu.LoadMenu(IDR_CONNECTIONS_MENU); 
		ClientToScreen(&point);
		tracker = contextMenu.GetSubMenu(0); 
		//show the context menu
		tracker->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, oldPoint.x, oldPoint.y, AfxGetMainWnd()); 
	}

}



void CConnectionListView::SetProgress(int progress) // CConnection & con, 
{
	//con.progress = progress;
	//CObArray * listOfConnections;
	m_listOfConnections = m_connectionManager.GetConnections();
	int i = 0;
	for(i = 0; i <  m_listOfConnections->GetSize(); i++){
		CConnection * c = (CConnection *)m_listOfConnections->GetAt(i);

		if(c->progress != progress){
			c->progress = progress;
			Invalidate();
		}
	}
}


void CConnectionListView::SetProgress(CString & conName, int progress)  // CConnection * con
{
	m_listOfConnections = m_connectionManager.GetConnections();
	int i = 0;
	for(i = 0; i <  m_listOfConnections->GetSize(); i++){
		CConnection * c = (CConnection *)m_listOfConnections->GetAt(i);

		if( c->name.Compare(conName) != -1 ){

			if(c->progress != progress){
				c->progress = progress;
				Invalidate();
			}
		}
	}
}


/**
* UpdateProgress
*
* Description: Called by MainFrame.OnUpdateProgress().
* Param: CFileTransfer. To be deleted when done.
*/
void CConnectionListView::UpdateProgress(CFileTransfer * transfer) 
{
	// store transfer info
	bool contains = false;
	int i;
	for(i = 0; i < transfers.size(); i++ )
	{
		CFileTransfer * t;
		t = (CFileTransfer*)transfers[i];
		if(t->fileName.Compare(transfer->fileName) == 0)   
		{
			contains = true;
		}
	}
	if(!contains)
	{
		CFileTransfer * transferClone = new CFileTransfer(transfer);
		transfers.push_back(transferClone);
		//TRACE1(" connectionList add transfer: %s ", transfer->fileName );
	}


	// update connection bar progress indicator
	m_listOfConnections = m_connectionManager.GetConnections();
	//int i = 0;
	for(i = 0; i < m_listOfConnections->GetSize(); i++){
		CConnection * c = (CConnection *)m_listOfConnections->GetAt(i);
		if( c->name.Compare( transfer->connection->name ) == 0 ){
			if(c->progress != transfer->progress){
				c->progress = transfer->progress;
				//TRACE2(" ConnectionList Progress: %d  %s  \n" , c->progress, c->name);
				Invalidate();
			}
		}
	}

	// update transfers
	for(i = 0; i < transfers.size(); i++)
	{
		CFileTransfer * t;
		t = (CFileTransfer*)transfers[i];
		if( t->fileName.Compare( transfer->fileName ) == 0 ){
			if( t->progress != transfer->progress){
				t->progress = transfer->progress;
				//TRACE2(" ConnectionList transfer Progress: %d  %s  \n" , t->progress, t->fileName);
				Invalidate();
			}
		}
	}

	// Clean up
	delete transfer;
}


/**
* SelectConnection
*
* Description: select a connection by id. Called by main LoadUrl
*/
void CConnectionListView::SelectConnection(int id) 
{
	m_selectedItemIndex = id;
	m_clickDownY = -1;
	m_selectedItemDragY = -1;
	Invalidate();
}



/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView printing

BOOL CConnectionListView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CConnectionListView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CConnectionListView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}



BOOL CConnectionListView::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}




/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView diagnostics

#ifdef _DEBUG
void CConnectionListView::AssertValid() const
{
	CView::AssertValid();
}

void CConnectionListView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CConnectionListDoc* CConnectionListView::GetDocument() // non-debug version is inline
{
	//ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSplitterWindowTutorialDoc)));
	return (CConnectionListDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView message handlers

// SplitterWindowTutorialView.cpp : implementation of the CSplitterWindowTutorialView class
//

#include "stdafx.h"
#include "RealFTP.h"
#include "ConnectionListDoc.h"
#include "FileView.h"
#include "resource.h"
#include "ConnectionManager.h"
#include "FtpManager.h"
#include "MainFrm.h"
#include "MenuNavigator.h"
#include <afxdisp.h> // Drop target
#include <afxole.h> // Drop target

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView

IMPLEMENT_DYNCREATE(CFileView, CScrollView ) // CScrollView  CListView

BEGIN_MESSAGE_MAP(CFileView, CScrollView) //
	//{{AFX_MSG_MAP(CSplitterWindowTutorialView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SIZE()    // CSCrollView
	ON_WM_VSCROLL() // CSCrollView
	ON_MESSAGE(333, ResizeFileViewForContent)
	//ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBegindrag) // list view
END_MESSAGE_MAP()

// ResizeScrollViewForContent  add message

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView construction/destruction

CFileView::CFileView() : m_bUseDnDHelper(false), m_piDropHelper(NULL)
{
	// TODO: add construction code here
	//menu1.LoadMenu(IDR_MAINFRAME);

	// Create an instance of the shell DnD helper object.

    if ( SUCCEEDED( CoCreateInstance ( CLSID_DragDropHelper, NULL, 
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDropTargetHelper, 
                                       (void**) &m_piDropHelper ) ))
    {
        m_bUseDnDHelper = true;
    }

	//SetMenu(&menu1);
	m_initalized = false;
	m_dropTargetRegistered = false;
	m_viewSynced = false;
	m_connection = 0;
	m_selected = -1;
	m_path = CString(_T("/"));
	m_shiftDown = false;
	m_vScrollPos = 0;

	m_fontTitle = new CFont;
	m_fontTitle->CreateFont(16 /*height*/, 8 /*width*/, 0, 0,
			      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
				_T("Arial"));
	 m_fontMedium = new CFont;
	 m_fontMedium->CreateFont(12 /*height*/, 6 /*width*/, 0, 0,
			      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
				_T("Arial"));

	//TRACE1(" this: %s ", this->Get  );

	Initalize();
}

CFileView::~CFileView(void)
{
	// m_fontTitle
	// m_fontMedium
	if ( NULL != m_piDropHelper )
        m_piDropHelper->Release();
}

void CFileView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	//CListView::OnInitialUpdate();

	// Drop target
	if(!m_dropTargetRegistered){
		m_droptarget.Register ( this );
		m_dropTargetRegistered = true;
	}

	CSize size;
	size.cx = 1000;
	size.cy = 1200;
	//SetScrollSizes(MM_TEXT, size); 
	
}

void CFileView::OnUpdate(CView * senderView, LPARAM lHint, CObject * pHint)
{
	CScrollView::OnUpdate(senderView, lHint, pHint);
	//CListView::OnUpdate(senderView, lHint, pHint);

	int i = 0;
	CSize size;
	size.cx = size.cy = 1000;
	//SetScrollSizes(MM_LOENGLISH, size); 
	//SetScrollSizes(MM_TEXT, size); 
}

void CFileView::Initalize()
{
	m_initalized = true;

	VIEW_THREADPARAM * _param = new VIEW_THREADPARAM;
	_param->v = this;
	_param->x = 5;

	m_pViewSyncThread = AfxBeginThread(CFileView::ViewSyncWorker, _param); //, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (m_pViewSyncThread != NULL)
	{
		TRACE(_T("AfxBeginThread: 0x%08lX\n"), m_pViewSyncThread->m_nThreadID);
	}
}

UINT CFileView::ViewSyncWorker(LPVOID pParam)
{
	CFileView::VIEW_THREADPARAM *const pThreadParam = static_cast<CFileView::VIEW_THREADPARAM *>(pParam);

	while(true){
		
		// Sync with FTP...
		if(pThreadParam->v->m_viewSynced == false && pThreadParam->v->m_connection) 
		{
			CFtpManager ftp;
			// Read from ftp to disk
			ftp.GetFtpDirectory(pThreadParam->v->m_connection, pThreadParam->v->m_path);  

			// Load files into array from disk
			pThreadParam->v->m_files.clear();
			ftp.ReadLocalDirectory(pThreadParam->v->m_connection, pThreadParam->v->m_path, pThreadParam->v->m_files);

			// Bug 
			//pThreadParam->v->ResizeScrollViewForContent(); // resize scroll area for new content. 
			//PostMessage() 
			//::PostMessage(pThreadParam->v->GetSafeHwnd(), 333, 0, 0);
			//::PostMessage(NULL, 333, 0, 0);

			pThreadParam->v->m_viewSynced = true;
			pThreadParam->v->Invalidate(); // refresh view with synced marker
			TRACE0(" *** view synced with FTP *** \n");
		}
		
		Sleep(1000 * 1);
	}

	return 0;
}






BOOL CFileView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView drawing

void CFileView::OnDraw(CDC* pDC)
{
	CConnectionListDoc* pDoc = GetDocument();
	
	// TODO: add draw code for native data here
	CRect rect;
	GetClientRect( &rect );
	
	pDC->DPtoLP(&rect);

	
	// Draw files

	int seperator_x = 95;
	int seperator_y = 110;
	int indent_x = 65;
	int x = indent_x;
	int y = 55;
	int i = 0;
	for(i = 0; i < m_files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)m_files[i];
		m_files[i].view_x = x;
		m_files[i].view_y = y;

		// Selected
		if(i == m_selected){ 
			CPoint prorgess_fill_points[5];
			prorgess_fill_points[0] = CPoint(x-35, y - 35);
			prorgess_fill_points[1] = CPoint(x-35, y + 25);
			prorgess_fill_points[2] = CPoint(x+35, y + 25);
			prorgess_fill_points[3] = CPoint(x+35, y - 35);
			prorgess_fill_points[4] = CPoint(x-35, y - 35);
			CBrush fillBrush(RGB(237, 237, 237));
			pDC->SelectObject(&fillBrush);
			CPen penFill(PS_SOLID,1,RGB(210, 210, 210));
			pDC->SelectObject(&penFill);
			pDC->Polyline(prorgess_fill_points, 5);
		}

		// Draw file
		if(f.dir){
			this->DrawFolder(pDC, x, y, f.name);
		} else {
			this->DrawFile(pDC, x, y, f.name, 100);
		}

		x += seperator_x;
		if(x > rect.Width() - seperator_x){
			x = indent_x;
			y += seperator_y;
		}
	}

	// Set size
	CSize size;
	size.cx = 200;
	size.cy = y + 100;
	//SetScrollSizes(MM_LOENGLISH, size); 
	//SetScrollSizes(MM_TEXT, size); 

	if(m_connection && rect.Width() > 100){
		
		int bottom_center_x = rect.Width() / 2;
		int bottom_center_y = rect.Height();

		// Bottom background
		CPoint background_fill_points[4];
		background_fill_points[0] = CPoint(bottom_center_x - 35, bottom_center_y - 20);
		background_fill_points[1] = CPoint(bottom_center_x - 35, bottom_center_y + 0 );
		background_fill_points[2] = CPoint(bottom_center_x + 35, bottom_center_y + 0 );
		background_fill_points[3] = CPoint(bottom_center_x + 35, bottom_center_y - 20);
		CBrush backBrush(RGB(245, 245, 245));
		CBrush *oldBackBrush = pDC->SelectObject(&backBrush);
		CPen penBack(PS_NULL,0,RGB(0,0,0));
		pDC->SelectObject(&penBack);
		pDC->Polygon(background_fill_points, 4);

		// Text
		//CFont * fntTitle1 = new CFont;
		//fntTitle1->CreateFont(12 /*height*/, 6 /*width*/, 0, 0,
		//	      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
		//		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		//		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
		//		_T("Arial"));
		CFont *fntOld = pDC->SelectObject(m_fontMedium);
		pDC->SetTextAlign( TA_BASELINE | TA_CENTER );
		COLORREF oldTextColor = pDC->SetTextColor(RGB(80, 80, 80));
		pDC->SetBkColor(RGB(245, 245, 245));
		CString synced(_T("Synced"));
		if(!m_viewSynced){
			synced = CString(_T("Local"));
		}
		pDC->TextOut( rect.Width() / 2, rect.Height() - 6, synced.AllocSysString(), synced.GetLength());

		// Bottom Box Outline
		CPoint control_box_border_points[5];
		control_box_border_points[0] = CPoint(bottom_center_x - 35, bottom_center_y - 20);
		control_box_border_points[1] = CPoint(bottom_center_x - 35, bottom_center_y + 0 );
		control_box_border_points[2] = CPoint(bottom_center_x + 35, bottom_center_y + 0 );
		control_box_border_points[3] = CPoint(bottom_center_x + 35, bottom_center_y - 20);
		control_box_border_points[4] = CPoint(bottom_center_x - 35, bottom_center_y - 20);
		CPen penOutline(PS_SOLID,1,RGB(204,204,204));
		
		pDC->SelectObject(&penOutline);
		pDC->Polyline(control_box_border_points, 5);
	}
}


void CFileView::GetFileLocation(int index, int x, int y)
{
	CRect rect;
	GetClientRect( &rect );

	
}

void CFileView::DrawFile(CDC* pDC, int x, int y, CString name, int progress)
{
	//CFont * fntTitle1 = new CFont;
	//fntTitle1->CreateFont(16 /*height*/, 8 /*width*/, 0, 0,
	//		      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
	//			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	//			ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
	//			_T("Arial"));
	CRect rect;
    GetClientRect( &rect );
    pDC->SetTextAlign( TA_BASELINE | TA_CENTER );
	COLORREF oldTextColor = pDC->SetTextColor(RGB(60, 60, 60));
	//CString text(_T("Server Name"));
	CFont *fntOld = pDC->SelectObject(m_fontTitle);
	//pDC->TextOut( rect.right / 2, rect.bottom / 2, text.AllocSysString(), text.GetLength() );
	pDC->TextOut( x , y + 40, name.AllocSysString(), name.GetLength() );

	/*
	int progress_x = 56;
	int progress_y = (pos * 50) + 40;
	CPoint prorgess_border_points[5];
	prorgess_border_points[0] = CPoint(progress_x, progress_y);
	prorgess_border_points[1] = CPoint(progress_x, progress_y + 5);
	prorgess_border_points[2] = CPoint(progress_x+75, progress_y + 5);
	prorgess_border_points[3] = CPoint(progress_x+75, progress_y);
	prorgess_border_points[4] = CPoint(progress_x, progress_y);
	CPen penOutline(PS_SOLID,1,RGB(204,204,204));
	pDC->SelectObject(&penOutline);
	pDC->Polyline(prorgess_border_points, 5);
	
	CPoint prorgess_fill_points[4];
	prorgess_fill_points[0] = CPoint(progress_x+1, progress_y + 1);
	prorgess_fill_points[1] = CPoint(progress_x+1, progress_y + 5);
	prorgess_fill_points[2] = CPoint(progress_x+((int)((float)progress * 0.75))+1, progress_y + 5);
	prorgess_fill_points[3] = CPoint(progress_x+((int)((float)progress * 0.75))+1, progress_y + 1);
	CBrush fillBrush(RGB(227, 227, 227));
	CBrush *oldFillBrush = pDC->SelectObject(&fillBrush);
	CPen penFill(PS_NULL,0,RGB(0,0,0));
	pDC->SelectObject(&penFill);
	pDC->Polygon(prorgess_fill_points, 4);
	*/

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
	  int nX = x  - (bmpInfo.bmWidth/2); // rect.left + (rect.Width() - bmpInfo.bmWidth) / 2;
	  int nY = y  - (bmpInfo.bmHeight/2); // rect.top + (rect.Height() - bmpInfo.bmHeight) / 2;
      pDC->BitBlt(nX, nY, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcMemory, 
         0, 0, SRCCOPY);
      dcMemory.SelectObject(pOldBitmap);
   }
}

void CFileView::DrawFolder(CDC* pDC, int x, int y, CString name)
{
	// Folder
	CBitmap fbmp;
	if (fbmp.LoadBitmap(IDB_FOLDER_LARGE))
	{
		// Get the size of the bitmap
		BITMAP bmpInfo;
		fbmp.GetBitmap(&bmpInfo);
		CDC dcMemory;
		dcMemory.CreateCompatibleDC(pDC);
		CBitmap* pOldBitmap = dcMemory.SelectObject(&fbmp);
		CRect rect;
		GetClientRect(&rect);
		int nX = x - 30 ; // rect.left + (rect.Width() - bmpInfo.bmWidth) / 2;
		int nY = y - 30; // rect.top + (rect.Height() - bmpInfo.bmHeight) / 2;
		pDC->BitBlt(nX, nY, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcMemory, 
			0, 0, SRCCOPY);
		dcMemory.SelectObject(pOldBitmap);
	}

	// Label
	//CFont * fntTitle1 = new CFont;
	//fntTitle1->CreateFont(16 /*height*/, 8 /*width*/, 0, 0,
	//		      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
	//			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	//			ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
	//			_T("Arial"));
	CRect rect;
    GetClientRect( &rect );
    pDC->SetTextAlign( TA_BASELINE | TA_CENTER );
	COLORREF oldTextColor = pDC->SetTextColor(RGB(60, 60, 60));
	CFont *fntOld = pDC->SelectObject(m_fontTitle);
	CString line2(_T(""));
	if(name.GetLength() > 12){
		line2 = CString( name.Mid(12, name.GetLength() - 12) );
		name = name.Mid(0, 12);
	}
	pDC->TextOut( x + 0, y + 40, name.AllocSysString(), name.GetLength());
	// Line 2
	pDC->TextOut( x + 0, y + 60, line2.AllocSysString(), line2.GetLength());
}


void CFileView::OnMouseMove(UINT nFlags, CPoint point) 
{
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);
}

void CFileView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);

	//CScrollView::OnLButtonDown(nFlags, point);
}

void CFileView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);

	bool itemClicked = false;

	int i = 0;
	for(i = 0; i < m_files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)m_files[i];
		if( point.x > f.view_x -35 && point.x < f.view_x + 35 &&
			point.y > f.view_y -35 && point.y < f.view_y + 35)
		{
			m_selected = i;
			Invalidate();
			itemClicked = true;
			//TRACE0("clicked file \n");
		}
	}

	if(itemClicked == false) // deselect
	{
		m_selected = -1;
		Invalidate();
	}
	//CScrollView::OnLButtonUp(nFlags, point);
}

void CFileView::OnLButtonDblClk( UINT nFlags, CPoint point)
{
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);

	int i = 0;
	for(i = 0; i < m_files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)m_files[i];
		//TRACE2(" file : %d  %d \n", f.view_x, f.view_y);

		if( point.x > f.view_x -35 && point.x < f.view_x + 35 &&
			point.y > f.view_y -35 && point.y < f.view_y + 35)
		{
			m_selected = i;

			CFtpManager ftp;
			m_files.clear();

			m_path = m_path + f.name + CString(_T("/"));
			ftp.ReadLocalDirectory(m_connection, m_path, m_files);

			m_viewSynced = false; 
			m_selected = -1;

			OnSize(0, 0, 0); // resize scroll area for new content. 

			Invalidate();

			//CMainFrame * mainFrame = (CMainFrame *)this->GetParent()->GetParent();
			CMainFrame * mainFrame = (CMainFrame *)AfxGetMainWnd();
			mainFrame->DirectoryLoaded( CString( m_connection->host + _T("")  + m_path) );

			TRACE0("clicked file \n");
		}
	}
}

void CFileView::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint oldPoint;
	oldPoint.x = point.x;
	oldPoint.y = point.y;
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);

	bool folderClicked = false;
	bool fileClicked = false;
	int clickedIndex = -1;
	int i = 0;
	for(i = 0; i < m_files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)m_files[i];
		//TRACE2(" file : %d  %d \n", f.view_x, f.view_y);

		if( point.x > f.view_x -35 && point.x < f.view_x + 35 &&
			point.y > f.view_y -35 && point.y < f.view_y + 35)
		{
			clickedIndex = i;
			if(f.dir){
				folderClicked = true;
			} else {
				fileClicked = true;
			}
		}
	}

	if(folderClicked)
	{
		CMenu contextMenu;
		CMenu *tracker; 
		contextMenu.LoadMenu(IDR_FOLDER_MENU); 
		ClientToScreen(&oldPoint);
		tracker = contextMenu.GetSubMenu(0); 

		// get files for folder
		std::vector<CFileContainer> subFolders;

		//CFtpManager ftp;
		CFileContainer clickedFolder;
		clickedFolder = m_files[clickedIndex];

		//ftp.ReadLocalDirectory(m_connection, CString(m_path + clickedFolder.name + _T("/")) , subFolders);  
		// Add files
		CMenu *subMenu = tracker->GetSubMenu( 0 );
		CMenuNavigator menuNav;
		menuNav.BuildMenu(m_connection, CString(m_path + clickedFolder.name + _T("/")), subMenu, 5);
		subMenu->DeleteMenu( ID_FOLDERS_BACK, MF_BYCOMMAND );

		//show the context menu
		tracker->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, oldPoint.x, oldPoint.y, AfxGetMainWnd());
	}

	if(fileClicked)
	{
		CMenu contextMenu;
		CMenu *tracker; 
		contextMenu.LoadMenu(IDR_FILE_MENU); 
		ClientToScreen(&oldPoint);
		tracker = contextMenu.GetSubMenu(0); 

		//show the context menu
		tracker->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, oldPoint.x, oldPoint.y, AfxGetMainWnd());
	}

	// Nothing clicked
	if(!folderClicked && !fileClicked)
	{
		CMenu contextMenu;
		CMenu *tracker; 
		contextMenu.LoadMenu(IDR_FOLDERS_MENU); 
		ClientToScreen(&oldPoint);
		tracker = contextMenu.GetSubMenu(0); 

		//show the context menu
		tracker->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, oldPoint.x, oldPoint.y, AfxGetMainWnd());
	}

}


/**
*
*
*/
void CFileView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	//TRACE3("nPos: %d   %d   curr: %d  \n", nSBCode, nPos, m_vScrollPos);
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

    // TODO: Add your message handler code here and/or call default
    //CScrollView::OnVScroll(nSBCode, nPos, pScrollBar);
    //Invalidate();
}


void CFileView::OnSize(UINT nType, int cx, int cy)
{
	ResizeScrollViewForContent();
}

LRESULT CFileView::ResizeFileViewForContent(WPARAM wParam,LPARAM lParam)
{
	ResizeScrollViewForContent();
	return 0;
}

void CFileView::ResizeScrollViewForContent()
{
	CRect rect;
	GetClientRect( &rect );

	int seperator_x = 95;
	int seperator_y = 110;
	int indent_x = 65;
	int x = indent_x;
	int y = 55;
	int i = 0;
	for(i = 0; i < m_files.size(); i++)
	{
		x += seperator_x;
		if(x > rect.Width() - seperator_x)
		{
			x = indent_x;
			y += seperator_y;
		}
	}

	CSize size;
	size.cx = x + 10;
	size.cy = y + 75;
	SetScrollSizes(MM_TEXT, size); // fails from thread

	//CPoint pt( 0, m_vScrollPos );
	//ScrollToPosition( pt );
	//Invalidate();
}

BOOL CFileView::PreTranslateMessage(MSG* pMsg)
{
	
    if (pMsg->message == WM_KEYDOWN)
    {
        //pMsg->wParam == VK_RETURN
		if(pMsg->wParam == 16){
			m_shiftDown = true;
		}


		//TRACE1(" Key down  %d \n", pMsg->wParam);
	}
	if (pMsg->message == WM_KEYUP)
    {
		if(pMsg->wParam == 16){
			m_shiftDown = false;
		}

		if(pMsg->wParam == 37){ // left
			if(m_selected > 0){
				m_selected = m_selected - 1;
				Invalidate();
			}
		}
		if(pMsg->wParam == 39){ // right
			m_selected = m_selected + 1;
			Invalidate();
		}
		if(pMsg->wParam == 38){ // up
		
		}
		if(pMsg->wParam == 40){ // down
		
		}

		//TRACE1(" Key up  %d \n", pMsg->wParam);
	}
	return false;
}



void CFileView::LoadServerConnection(int index)
{
	CConnectionManager conManager;
	conManager.Load();
	CConnection * con = conManager.GetAt(index);
	m_connection = con;

	// temp (load from disk first)
	CFtpManager ftp;
	//ftp.LoadFilesForView(con, CString(_T("/"))); // should be done in thread 

	m_files.clear();
	ftp.ReadLocalDirectory(con, CString(_T("/")), m_files);
	int c = m_files.size();
	TRACE1(" files: %d \n", c);
	m_viewSynced = false; // only loaded from disk, not network
	m_selected = -1;

	m_path = CString(_T("/")); // ??? go back to root?

	ResizeScrollViewForContent(); // resize scroll area for new content.  

	//CSize size;
	//size.cx = 200;
	//size.cy = 1000;
	//SetScrollSizes(MM_LOENGLISH, size); 
	//SetScrollSizes(MM_TEXT, size); 

	Invalidate(); // display what we have so far
}

void CFileView::LoadDirectory(CString &dir)
{
	CFtpManager ftp;
	m_files.clear();
	ftp.ReadLocalDirectory(m_connection, dir, m_files);
	m_viewSynced = false; // only loaded from disk, not network
	m_selected = -1;
	m_path = CString(dir); 

	ResizeScrollViewForContent(); // resize scroll area for new content.  

	Invalidate(); // display what we have so far
}




void CFileView::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW*    pNMLV = (NMLISTVIEW*) pNMHDR;
	//CListCtrl&     list = GetListCtrl();
	COleDataSource datasrc;
	HGLOBAL        hgDrop;
	DROPFILES*     pDrop;
	CStringList    lsDraggedFiles;
	POSITION       pos;
	int            nSelItem;
	CString        sFile;
	UINT           uBuffSize = 0;
	TCHAR*         pszBuff;
	FORMATETC      etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    *pResult = 0;   // return value ignored

    // For every selected item in the list, put the filename into lsDraggedFiles.
	/*
    pos = list.GetFirstSelectedItemPosition();
    while ( NULL != pos )
        {
        nSelItem = list.GetNextSelectedItem ( pos );
        sFile = list.GetItemText ( nSelItem, 0 );

        lsDraggedFiles.AddTail ( sFile );

        // Calculate the # of chars required to hold this string.

        uBuffSize += lstrlen ( sFile ) + 1;
        }

    // Add 1 extra for the final null char, and the size of the DROPFILES struct.

    uBuffSize = sizeof(DROPFILES) + sizeof(TCHAR) * (uBuffSize + 1);

    // Allocate memory from the heap for the DROPFILES struct.

    hgDrop = GlobalAlloc ( GHND | GMEM_SHARE, uBuffSize );

    if ( NULL == hgDrop )
        return;

    pDrop = (DROPFILES*) GlobalLock ( hgDrop );

    if ( NULL == pDrop )
        {
        GlobalFree ( hgDrop );
        return;
        }

    // Fill in the DROPFILES struct.

    pDrop->pFiles = sizeof(DROPFILES);

#ifdef _UNICODE
    // If we're compiling for Unicode, set the Unicode flag in the struct to
    // indicate it contains Unicode strings.

    pDrop->fWide = TRUE;
#endif

    // Copy all the filenames into memory after the end of the DROPFILES struct.

    pos = lsDraggedFiles.GetHeadPosition();
    pszBuff = (TCHAR*) (LPBYTE(pDrop) + sizeof(DROPFILES));

    while ( NULL != pos )
        {
        lstrcpy ( pszBuff, (LPCTSTR) lsDraggedFiles.GetNext ( pos ) );
        pszBuff = 1 + _tcschr ( pszBuff, '\0' );
        }

    GlobalUnlock ( hgDrop );

    // Put the data in the data source.

    datasrc.CacheGlobalData ( CF_HDROP, hgDrop, &etc );

    // Add in our own custom data, so we know that the drag originated from our 
    // window.  OnDragEnter() checks for this custom format, and
    // doesn't allow the drop if it's present.  This is how we prevent the user
    // from dragging and then dropping in our own window.
    // The data will just be a dummy bool.
HGLOBAL hgBool;

    hgBool = GlobalAlloc ( GHND | GMEM_SHARE, sizeof(bool) );

    if ( NULL == hgBool )
        {
        GlobalFree ( hgDrop );
        return;
        }

    // Put the data in the data source.

    etc.cfFormat = g_uCustomClipbrdFormat;
    
    datasrc.CacheGlobalData ( g_uCustomClipbrdFormat, hgBool, &etc );


    // Start the drag 'n' drop!

DROPEFFECT dwEffect = datasrc.DoDragDrop ( DROPEFFECT_COPY | DROPEFFECT_MOVE );

    // If the DnD completed OK, we remove all of the dragged items from our
    // list.

    switch ( dwEffect )
        {
        case DROPEFFECT_COPY:
        case DROPEFFECT_MOVE:
            {
            // The files were copied or moved.
            // Note: Don't call GlobalFree() because the data will be freed by the drop target.

            for ( nSelItem = list.GetNextItem ( -1, LVNI_SELECTED );
                  nSelItem != -1;
                  nSelItem = list.GetNextItem ( nSelItem, LVNI_SELECTED ) )
                {
                list.DeleteItem ( nSelItem );
                nSelItem--;
                }

            // Resize the list columns.

            list.SetColumnWidth ( 0, LVSCW_AUTOSIZE_USEHEADER );
            list.SetColumnWidth ( 1, LVSCW_AUTOSIZE_USEHEADER );
            list.SetColumnWidth ( 2, LVSCW_AUTOSIZE_USEHEADER );
            }
        break;

        case DROPEFFECT_NONE:
            {
            // This needs special handling, because on NT, DROPEFFECT_NONE
            // is returned for move operations, instead of DROPEFFECT_MOVE.
            // See Q182219 for the details.
            // So if we're on NT, we check each selected item, and if the
            // file no longer exists, it was moved successfully and we can
            // remove it from the list.

            if ( g_bNT )
                {
                bool bDeletedAnything = false;

                for ( nSelItem = list.GetNextItem ( -1, LVNI_SELECTED );
                      nSelItem != -1;
                      nSelItem = list.GetNextItem ( nSelItem, LVNI_SELECTED ) )
                    {
                    CString sFilename = list.GetItemText ( nSelItem, 0 );

                    if ( 0xFFFFFFFF == GetFileAttributes ( sFile ) &&
                         GetLastError() == ERROR_FILE_NOT_FOUND )
                        {
                        // We couldn't read the file's attributes, and GetLastError()
                        // says the file doesn't exist, so remove the corresponding 
                        // item from the list.

                        list.DeleteItem ( nSelItem );
                    
                        nSelItem--;
                        bDeletedAnything = true;
                        }
                    }

                // Resize the list columns if we deleted any items.

                if ( bDeletedAnything )
                    {
                    list.SetColumnWidth ( 0, LVSCW_AUTOSIZE_USEHEADER );
                    list.SetColumnWidth ( 1, LVSCW_AUTOSIZE_USEHEADER );
                    list.SetColumnWidth ( 2, LVSCW_AUTOSIZE_USEHEADER );

                    // Note: Don't call GlobalFree() because the data belongs to 
                    // the caller.
                    }
                else
                    {
                    // The DnD operation wasn't accepted, or was canceled, so we 
                    // should call GlobalFree() to clean up.

                    GlobalFree ( hgDrop );
                    GlobalFree ( hgBool );
                    }
                }   // end if (NT)
            else
                {
                // We're on 9x, and a return of DROPEFFECT_NONE always means
                // that the DnD operation was aborted.  We need to free the
                // allocated memory.

                GlobalFree ( hgDrop );
                GlobalFree ( hgBool );
                }
            }
        break;  // end case DROPEFFECT_NONE
    }   // end switch
		
	*/
}


DROPEFFECT CFileView::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) 
{
	DROPEFFECT dwEffect = DROPEFFECT_NONE;

    // Check for our own custom clipboard format in the data object.  If it's
    // present, then the DnD was initiated from our own window, and we won't
    // accept the drop.
    // If it's not present, then we check for CF_HDROP data in the data object.

    if ( NULL == pDataObject->GetGlobalData ( g_uCustomClipbrdFormat ))
        {
        // Look for CF_HDROP data in the data object, and accept the drop if
        // it's there.

        if ( NULL != pDataObject->GetGlobalData ( CF_HDROP ) )
            dwEffect = DROPEFFECT_COPY;
        }

    // Call the DnD helper.

    if ( m_bUseDnDHelper )
        {
        // The DnD helper needs an IDataObject interface, so get one from
        // the COleDataObject.  Note that the FALSE param means that
        // GetIDataObject will not AddRef() the returned interface, so 
        // we do not Release() it.

        IDataObject* piDataObj = pDataObject->GetIDataObject ( FALSE ); 

        m_piDropHelper->DragEnter ( GetSafeHwnd(), piDataObj, &point, dwEffect );
	}

    return dwEffect;
}

DROPEFFECT CFileView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) 
{
	DROPEFFECT dwEffect = DROPEFFECT_NONE;

    // Check for our own custom clipboard format in the data object.  If it's
    // present, then the DnD was initiated from our own window, and we won't
    // accept the drop.
    // If it's not present, then we check for CF_HDROP data in the data object.

    if ( NULL == pDataObject->GetGlobalData ( g_uCustomClipbrdFormat ))
        {
        // Look for CF_HDROP data in the data object, and accept the drop if
        // it's there.

        if ( NULL != pDataObject->GetGlobalData ( CF_HDROP ) )
            dwEffect = DROPEFFECT_COPY;
        }

    // Call the DnD helper.

    if ( m_bUseDnDHelper )
        {
        m_piDropHelper->DragOver ( &point, dwEffect );
        }

    return dwEffect;
}

BOOL CFileView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) 
{
	BOOL bRet;

    // Read the CF_HDROP data and put the files in the main window's list.

    bRet = ReadHdropData ( pDataObject );

    // Call the DnD helper.

    if ( m_bUseDnDHelper )
    {
        // The DnD helper needs an IDataObject interface, so get one from
        // the COleDataObject.  Note that the FALSE param means that
        // GetIDataObject will not AddRef() the returned interface, so 
        // we do not Release() it.

        IDataObject* piDataObj = pDataObject->GetIDataObject ( FALSE ); 

        m_piDropHelper->Drop ( piDataObj, &point, dropEffect );
    }
    return bRet;
}

void CFileView::OnDragLeave() 
{
    if ( m_bUseDnDHelper )
    {
        m_piDropHelper->DragLeave();
    }
}

// ReadHdropData() reads CF_HDROP data from the passed-in data object, and 
// puts all dropped files/folders into the main window's list control.
BOOL CFileView::ReadHdropData ( COleDataObject* pDataObject )
{
	HGLOBAL     hg;
	HDROP       hdrop;
	UINT        uNumFiles;
	TCHAR       szNextFile [MAX_PATH];
	SHFILEINFO  rFileInfo;
	LVFINDINFO  rlvFind = { LVFI_STRING, szNextFile };
	LVITEM      rItem;
	//CListCtrl&  list = 0; //GetListCtrl();
	int         nIndex = 0; //list.GetItemCount();
	HANDLE      hFind;
	WIN32_FIND_DATA rFind;
	TCHAR       szFileLen [64];

    // Get the HDROP data from the data object.

    hg = pDataObject->GetGlobalData ( CF_HDROP );
    
    if ( NULL == hg )
        {
        return FALSE;
        }

    hdrop = (HDROP) GlobalLock ( hg );

    if ( NULL == hdrop )
        {
        GlobalUnlock ( hg );
        return FALSE;
        }

    // Get the # of files being dropped.

    uNumFiles = DragQueryFile ( hdrop, -1, NULL, 0 );

    for ( UINT uFile = 0; uFile < uNumFiles; uFile++ )
        {
        // Get the next filename from the HDROP info.

        if ( DragQueryFile ( hdrop, uFile, szNextFile, MAX_PATH ) > 0 )
            {
            // If the filename is already in the list, skip it.

           // if ( -1 != list.FindItem ( &rlvFind, -1 ))
           //     continue;

            // Get the index of the file's icon in the system image list and
            // it's type description.

            SHGetFileInfo ( szNextFile, 0, &rFileInfo, sizeof(rFileInfo),
                            SHGFI_SYSICONINDEX | SHGFI_ATTRIBUTES |
                              SHGFI_TYPENAME );

            // Set up an LVITEM for the file we're about to insert.

            ZeroMemory ( &rItem, sizeof(LVITEM) );

            rItem.mask    = LVIF_IMAGE | LVIF_TEXT;
            rItem.iItem   = 0; //nIndex;
            rItem.pszText = szNextFile;
            rItem.iImage  = rFileInfo.iIcon;

            // If the file has the hidden attribute set, its attributes in
            // the SHFILEINFO struct will include SFGAO_GHOSTED.  We'll mark
            // the item as "cut" in our list to give it the same ghosted look.

            if ( rFileInfo.dwAttributes & SFGAO_GHOSTED )
                {
                rItem.mask |= LVIF_STATE;
                rItem.state = rItem.stateMask = LVIS_CUT;
                }

            // Insert it into the list!

            //list.InsertItem ( &rItem );

            // Set column #1 to the file's type description.

            //list.SetItemText ( nIndex, 1, rFileInfo.szTypeName );

            // Get the file size, and put that in column #2.

            hFind = FindFirstFile ( szNextFile, &rFind );

            if ( INVALID_HANDLE_VALUE != hFind )
                {
                StrFormatByteSize ( rFind.nFileSizeLow, szFileLen, 64 );
                FindClose ( hFind );
                }

            //list.SetItemText ( nIndex, 2, szFileLen );

            nIndex++;
            }
        }

    // Resize columns so all text is visible.

    //list.SetColumnWidth ( 0, LVSCW_AUTOSIZE_USEHEADER );
    //list.SetColumnWidth ( 1, LVSCW_AUTOSIZE_USEHEADER );
    //list.SetColumnWidth ( 2, LVSCW_AUTOSIZE_USEHEADER );

    //list.EnsureVisible ( nIndex-1, FALSE );

    return TRUE;
}




/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView printing

BOOL CFileView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CFileView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CFileView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView diagnostics

#ifdef _DEBUG
void CFileView::AssertValid() const
{
	CView::AssertValid();
}

void CFileView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CConnectionListDoc* CFileView::GetDocument() // non-debug version is inline
{
	//ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSplitterWindowTutorialDoc)));
	return (CConnectionListDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView message handlers

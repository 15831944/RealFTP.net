// SplitterWindowTutorialView.cpp : implementation of the CSplitterWindowTutorialView class
//

#include "stdafx.h"
#include "RealFTP.h"
#include "ConnectionListDoc.h"
#include "FileListView.h"
#include "resource.h"
#include "ConnectionManager.h"
#include "FtpManager.h"
#include "MainFrm.h"
#include "MenuNavigator.h"
#include <afxdisp.h> // Drop target
#include <afxole.h> // Drop target
#include "RenameDialog.h"
#include "File.h"
#include "InputDialog.h"
#include "FilePermissions.h"
#include "LocalStore.h"

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView

IMPLEMENT_DYNCREATE(CFileListView, CListView) // CScrollView  CListView

BEGIN_MESSAGE_MAP(CFileListView, CListView) //
	//{{AFX_MSG_MAP(CSplitterWindowTutorialView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
//	ON_WM_LBUTTONDOWN()
//	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SIZE()    
	//ON_WM_VSCROLL() 
//	ON_MESSAGE(333, ResizeFileViewForContent)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBegindrag) 
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)    // experimental  
	ON_WM_DRAWITEM_REFLECT()
END_MESSAGE_MAP()


  


// ResizeScrollViewForContent  add message

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView construction/destruction

CFileListView::CFileListView() : m_bUseDnDHelper(false), m_piDropHelper(NULL)
{
	
	// Create an instance of the shell DnD helper object.
    if ( SUCCEEDED( CoCreateInstance ( CLSID_DragDropHelper, NULL, 
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDropTargetHelper, 
                                       (void**) &m_piDropHelper ) ))
    {
        m_bUseDnDHelper = true;
    }

	//SetMenu(&menu1);
	m_hover_item = CString(_T(""));
	//m_tooltip = 0;
	m_initalized = false;
	m_dropTargetRegistered = false;
	m_viewSynced = false;
	m_connection = 0;
	m_selected = -1;
	m_path = CString(_T("/"));
	m_shiftDown = false;
	m_vScrollPos = 0;
	m_viewMode = 0;

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
	m_historyDlg = NULL;

	Initalize();
}

CFileListView::~CFileListView(void)
{
	// m_fontTitle
	// m_fontMedium
	if ( NULL != m_piDropHelper )
        m_piDropHelper->Release();
}

void CFileListView::OnInitialUpdate()
{
	if(!m_dropTargetRegistered){

		CListView::OnInitialUpdate();

		CListCtrl& list = GetListCtrl();
		list.EnableToolTips(false);
		
		m_tooltip.Create( this );   

		//CToolTip tt;// = new CToolTip();

		// Drop target
		m_droptarget.Register ( this );
		m_dropTargetRegistered = true;
	

		//CSize size;
		//size.cx = 1000;
		//size.cy = 1200;
		//SetScrollSizes(MM_TEXT, size); 
	
		// LVBKIMAGE

		SHFILEINFO rInfo;

		m_ImageListMed = new CImageList();
		m_ImageListMed->Create(32,32,ILC_COLOR32,0,0);

		m_ImageListLarge = new CImageList();
		m_ImageListLarge->Create(48,48,ILC_COLOR32,0,0);
		//HICON hIcon=AfxGetApp()->LoadIcon(IDB_FOLDER_MED);
		//m_ImageList->Add(hIcon);
		
		//HIMAGELIST hImglist;
		//hImglist = (HIMAGELIST) SHGetFileInfo ( _T(""), 0, &rInfo, sizeof(SHFILEINFO), 
		//										SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX );
		//m_imglist.Attach ( hImglist );
		//HIMAGELIST hImglistNormal;
		//hImglistNormal = (HIMAGELIST) SHGetFileInfo ( _T(""), 0, &rInfo, sizeof(SHFILEINFO), 
		//										SHGFI_ICON  | SHGFI_LARGEICON | SHGFI_SYSICONINDEX );
		//m_imglistLarge.Attach ( hImglistNormal );
		// Make sure the list control has the share image list style, so it won't
		// destroy the system image list when the dialog closes.
		// LVS_EX_SUBITEMIMAGES
		
		//list.ModifyStyle ( LVS_TYPEMASK, LVS_REPORT | LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS );
		list.ModifyStyle ( LVS_TYPEMASK, LVS_ICON | LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS );
		
		//list.SetImageList ( &m_imglist, LVSIL_SMALL );
		//list.SetImageList ( &m_imglistLarge, LVSIL_NORMAL );
		list.SetImageList ( m_ImageListLarge, LVSIL_SMALL );
		list.SetImageList ( m_ImageListLarge, LVSIL_NORMAL );

		
		
		// Enable tooltips for items that aren't completely visible.
		list.SetExtendedStyle ( LVS_EX_INFOTIP       | LVS_EX_SUBITEMIMAGES);

		// Set up the list control columns.
		list.InsertColumn ( 0, _T("File Name"), LVCFMT_LEFT, 0, 0 );
		list.InsertColumn ( 1, _T("Type"), LVCFMT_LEFT, 0, 1 );
		list.InsertColumn ( 2, _T("Size"), LVCFMT_LEFT, 0, 2 );
		list.InsertColumn ( 3, _T("Permissions"), LVCFMT_LEFT, 0, 3 );

		list.SetColumnWidth ( 0, LVSCW_AUTOSIZE_USEHEADER );
		list.SetColumnWidth ( 1, LVSCW_AUTOSIZE_USEHEADER );
		list.SetColumnWidth ( 2, LVSCW_AUTOSIZE_USEHEADER );
		list.SetColumnWidth ( 3, LVSCW_AUTOSIZE_USEHEADER );


		{
			CBitmap bmp;
			bmp.LoadBitmap(IDB_FOLDER_LARGE);
			m_folderLargeIndex = m_ImageListLarge->Add(&bmp, RGB(0, 0, 0));
		}

		{
			CBitmap bmp;
			bmp.LoadBitmap(IDB_FILE_LARGE);
			m_fileLargeIndex = m_ImageListLarge->Add(&bmp, RGB(0, 0, 0));
		}

		{
			CBitmap bmp;
			bmp.LoadBitmap(IDB_FOLDER_MED);
			m_folderMedIndex = m_ImageListMed->Add(&bmp, RGB(0, 0, 0));
		}

		{
			CBitmap bmp;
			bmp.LoadBitmap(IDB_FILE_MED);
			m_fileMedIndex = m_ImageListMed->Add(&bmp, RGB(0, 0, 0));
		}
		
		// EXE
		{
			CBitmap bmp;
			bmp.LoadBitmap(IDB_EXE_LARGE);
			m_exeLargeIndex = m_ImageListLarge->Add(&bmp, RGB(0, 0, 0));
		}
		{
			CBitmap bmp;
			bmp.LoadBitmap(IDB_EXE_MED);
			m_exeMedIndex = m_ImageListMed->Add(&bmp, RGB(0, 0, 0));
		}
		
	}
}

/*
void CFileListView::OnUpdate(CView * senderView, LPARAM lHint, CObject * pHint)
{
	CListView::OnUpdate(senderView, lHint, pHint);

	int i = 0;
	CSize size;
	size.cx = size.cy = 1000;
	//SetScrollSizes(MM_LOENGLISH, size); 
	//SetScrollSizes(MM_TEXT, size); 
}
*/


void CFileListView::Initalize()
{
	m_initalized = true;
	
	VIEW_THREADPARAM * _param = new VIEW_THREADPARAM;
	_param->v = this;
	_param->x = 5;

	m_pViewSyncThread = AfxBeginThread(CFileListView::ViewSyncWorker, _param); //, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (m_pViewSyncThread != NULL)
	{
		TRACE(_T("AfxBeginThread: 0x%08lX\n"), m_pViewSyncThread->m_nThreadID);
	}
}


/**
* ViewSyncWorker
*
* Description: Thread checks current cached view to see if different from remote ftp server.
*/
UINT CFileListView::ViewSyncWorker(LPVOID pParam)
{
	CFileListView::VIEW_THREADPARAM *const pThreadParam = static_cast<CFileListView::VIEW_THREADPARAM *>(pParam);

	while(true){
		
		// Sync with FTP...
		if(pThreadParam->v->m_viewSynced == false && pThreadParam->v->m_connection) 
		{
			CFtpManager ftp;
			
			int cachedFileCount = pThreadParam->v->m_files.size();
			
			//TRACE0(" about to sync \n");

			// Read from ftp to disk
			bool r = ftp.GetFtpDirectory(pThreadParam->v->m_connection, pThreadParam->v->m_path);  

			// Load files into array from disk
			pThreadParam->v->m_files.clear();
			ftp.ReadLocalDirectory(pThreadParam->v->m_connection, pThreadParam->v->m_path, pThreadParam->v->m_files); 
			if(cachedFileCount != pThreadParam->v->m_files.size() || ftp.DirectoryUpdated()){ //  count alone not enough 
				pThreadParam->v->AddFilesToList(pThreadParam->v->m_files);

				// Bug 
				//pThreadParam->v->ResizeScrollViewForContent(); // resize scroll area for new content. 
				//::PostMessage(pThreadParam->v->GetSafeHwnd(), 333, 0, 0);
				//::PostMessage(NULL, 333, 0, 0);
				TRACE0(" *** view synced with FTP *** \n");

				pThreadParam->v->Invalidate(); // refresh view with synced marker
			} else {
				TRACE0(" *** view synced with FTP (allready up to date.) *** \n");
			}
			pThreadParam->v->m_viewSynced = true;
			//pThreadParam->v->Invalidate(); // refresh view with synced marker

			// mark ui to show synced
			// todo....

		}
		Sleep(500 * 1);

		// recheck if synced after x minutes.
		// TODO

	}
	return 0;
}





/*
// interfers currently
BOOL CFileListView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	

	return CView::PreCreateWindow(cs);
}
*/


/**
* OnCustomDraw
*
*/
void CFileListView::OnCustomDraw(NMHDR * pNMHDR, LRESULT * pResult) 
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)pNMHDR; 
	int iRow = lplvcd->nmcd.dwItemSpec; 

	CListCtrl&  list = GetListCtrl();
	CFileContainer f;
	CLocalStore stored;
		
	//lplvcd->
	//lplvcd->nmcd.hdc

	CListCtrl& theListCtrl = GetListCtrl(); 
	COLORREF crBkColor = ::GetSysColor(COLOR_3DFACE); 

	CRect rect;
	GetClientRect( &rect );

	switch(lplvcd->nmcd.dwDrawStage) 
	{ 
		case CDDS_PREPAINT: 
			*pResult = CDRF_NOTIFYSUBITEMDRAW; // CMT: ask for subitem notifications.

			// way too slow
			/* 
			// Sync Status
			if(m_connection && rect.Width() > 100){
				int bottom_center_x = rect.Width() / 2;
				int bottom_center_y = rect.Height();
				
				// Bottom background
				CPoint background_fill_points[4];
				background_fill_points[0] = CPoint(10, 10);
				background_fill_points[1] = CPoint(10, 25 );
				background_fill_points[2] = CPoint(25, 25 );
				background_fill_points[3] = CPoint(25, 10);
				CBrush backBrush(RGB(245, 245, 245));
				//CBrush *oldBackBrush = pDC->SelectObject(&backBrush);
				CPen penBack(PS_NULL,0,RGB(0,0,0));
				//pDC->SelectObject(&penBack);
				//pDC->Polygon(background_fill_points, 4);
				Polyline(lplvcd->nmcd.hdc, background_fill_points, 4);
				
				// Text
				//CFont * fntTitle1 = new CFont;
				//fntTitle1->CreateFont(12 , 6 , 0, 0,
				//	      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
				//		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				//		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
				//		_T("Arial"));
				//CFont *fntOld =  SelectObject(m_fontMedium);
				//pDC->SetTextAlign( TA_BASELINE | TA_CENTER );
				//COLORREF oldTextColor = pDC->SetTextColor(RGB(80, 80, 80));
				//pDC->SetBkColor(RGB(245, 245, 245));
				CString synced(_T("Synced"));
				if(!m_viewSynced){
					synced = CString(_T("Local"));
				}
				//pDC->TextOut( rect.Width() / 2, rect.Height() - 6, synced.AllocSysString(), synced.GetLength());
				//TextOut(lplvcd->nmcd.hdc, rect.Width() / 2, rect.Height() - 26, synced.AllocSysString(), synced.GetLength() );
			}
			*/

			break; 
		case CDDS_ITEMPREPAINT: 
			
			if( iRow >= 0 && iRow < m_files.size()  )
			{
				f = (CFileContainer)m_files[iRow]; 
				if(m_viewMode == 0 && f.dir && stored.IsFolderStored(f)  ) // icon view
				{
					CRect r = CRect();
					list.GetItemRect(iRow, r, 0);

					CPoint check_points[3]; 
					check_points[0] = CPoint(r.right - 20, r.bottom -22);
					check_points[1] = CPoint(r.right - 17, r.bottom -19 );
					check_points[2] = CPoint(r.right - 10, r.bottom -29);
					
					CPen penOutline(PS_SOLID,3,RGB(90,210,90));
					//lplvcd->nmcd.hdc.SelectObject(&penOutline);
					SelectObject(lplvcd->nmcd.hdc, penOutline);
					
					Polyline(lplvcd->nmcd.hdc, check_points, 3); // setAntiAliasHint(Polyline.ANTIALIAS_NICE ST);
				}
			}

			/*
			// Sync Status
			if(m_connection && rect.Width() > 100){
				int bottom_center_x = rect.Width() / 2;
				int bottom_center_y = rect.Height();

				// Text
				//CFont * fntTitle1 = new CFont;
				//fntTitle1->CreateFont(12 , 6 , 0, 0,
				//	      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
				//		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				//		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
				//		_T("Arial"));
				//CFont *fntOld =  SelectObject(m_fontMedium);
				//pDC->SetTextAlign( TA_BASELINE | TA_CENTER );
				//COLORREF oldTextColor = pDC->SetTextColor(RGB(80, 80, 80));
				//pDC->SetBkColor(RGB(245, 245, 245));
				CString synced(_T("Synced"));
				if(!m_viewSynced){
					synced = CString(_T("Local"));
				}
				//pDC->TextOut( rect.Width() / 2, rect.Height() - 6, synced.AllocSysString(), synced.GetLength());
				TextOut(lplvcd->nmcd.hdc, rect.Width() / 2, rect.Height() - 26, synced.AllocSysString(), synced.GetLength() );

			}
			*/

			break; 
		default: // CMT: it wasn't a notification that was interesting to us. 
			*pResult = CDRF_DODEFAULT; 
	} 
}


void CFileListView::DrawItem ( LPDRAWITEMSTRUCT lpDrawItemStruct )
{
	int i = 0;
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView drawing
// Not called
void CFileListView::OnDraw(CDC* pDC)
{
	CConnectionListDoc* pDoc = GetDocument();
	
	// TODO: add draw code for native data here
	CRect rect;
	GetClientRect( &rect );
	
	pDC->DPtoLP(&rect);

	
	// Draw files
	/*
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
	*/
	

	// Set size
	//CSize size;
	//size.cx = 200;
	//size.cy = 55 + 100;
	//SetScrollSizes(MM_LOENGLISH, size); 
	//SetScrollSizes(MM_TEXT, size); 
	/*
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
		//fntTitle1->CreateFont(12 , 6 , 0, 0,
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
	*/
}


void CFileListView::GetFileLocation(int index, int x, int y)
{
	CRect rect;
	GetClientRect( &rect );
}

void CFileListView::DrawFile(CDC* pDC, int x, int y, CString name, int progress)
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

void CFileListView::DrawFolder(CDC* pDC, int x, int y, CString name)
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


/**
* OnMouseMove
*
* Description: handle mouse movements. 
*	Load file details to display in tooltip. Can't display tooltip - putting on hold.
*   tooltip depricated. not handled here.
*/
void CFileListView::OnMouseMove(UINT nFlags, CPoint point) 
{
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	//dc.DPtoLP(&point);
	CListCtrl&  list = GetListCtrl();
	//list.ScreenToClient(&point);

	int i = 0;
	if((i = list.HitTest(point)) != -1)
	{
		CFileContainer f;
		if( i < m_files.size()  )
		{
			f = (CFileContainer)m_files[i];  // breaks

			CString file_tooltip(f.name + _T(" ") + f.remoteDate + _T(" ") + f.permissions);

			if(m_hover_item.Compare(file_tooltip) != 0){
				//TRACE1(" Hover: %s \n", file_tooltip);
				m_hover_item = file_tooltip;

				//CToolTip tt;// = new CToolTip();
				//m_tt.ShowWindow(SW_NORMAL);
			
				//m_tooltip.ShowWindow(SW_SHOW);

				//CMainFrame * mainFrame = (CMainFrame *)this->GetParent()->GetParent();
				// AfxGetMainWnd()
				//CRect  rect;
				//m_tooltip = new CToolTipCtrl(); 
				//m_tooltip -> Create ( this ); 
				//m_tooltip -> AddTool (this); 
				//m_tooltip -> AddTool( this , _T("Good"), CRect(0,0,100,100),1);
				//m_tooltip -> Activate (true); 
				//m_tooltip -> UpdateTipText (_T("hj"), this ); 
				//m_tooltip.Activate(true);
				//m_tooltip.UpdateTipText(_T("haza"), GetSafeHwnd(), 0 );
			}
		}
	} else {
		if(m_hover_item.GetLength() != 0)
		{
			// notify item hover removed event
		}
		m_hover_item = CString(_T(""));	

		//m_tooltip.Activate(false);
		//if(m_tooltip){
		//	m_tooltip ->Activate (false); 
		//	delete m_tooltip; 
		//	m_tooltip = 0; 
		//}

		//m_tt.ShowWindow(SW_HIDE);

	}
	CListView::OnMouseMove(nFlags, point);
}


void CFileListView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);

	CListView::OnLButtonDown(nFlags, point);
}


void CFileListView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);
	/*
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
	*/
	//CListView::OnLButtonUp(nFlags, point);
}


void CFileListView::OnLButtonDblClk( UINT nFlags, CPoint point)
{
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	//dc.DPtoLP(&point);
	CListCtrl&  list = GetListCtrl();
	//list.ScreenToClient(&point);

	int i = 0;
	if((i = list.HitTest(point)) != -1)
	{
		//TRACE1(" i %d \n", i);

		CFileContainer f;
		f = (CFileContainer)m_files[i];

		m_selected = i;

		CFtpManager ftp;
		m_files.clear();

		m_path = m_path + f.name + CString(_T("/"));
		ftp.ReadLocalDirectory(m_connection, m_path, m_files);

		AddFilesToList(m_files);

		m_viewSynced = false; 
		m_selected = -1;

		OnSize(0, 0, 0); // resize scroll area for new content. 

		//CMainFrame * mainFrame = (CMainFrame *)this->GetParent()->GetParent();
		CMainFrame * mainFrame = (CMainFrame *)AfxGetMainWnd();
		mainFrame->DirectoryLoaded( CString( m_connection->host + _T("")  + m_path) );

		TRACE0("clicked file \n");
	}
}


/**
* OnRButtonDown
*
*/
void CFileListView::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint oldPoint;
	oldPoint.x = point.x;
	oldPoint.y = point.y;
	// Translate click point for scroll position
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);

	CListCtrl&  list = GetListCtrl();

	bool folderClicked = false;
	bool fileClicked = false;
	int clickedIndex = -1;
	CFileContainer f;
	int i = 0;
	if((i = list.HitTest(point)) != -1)
	{
		f = (CFileContainer)m_files[i];
		m_selected = i;

		clickedIndex = i;
		if(f.dir){
			folderClicked = true;
		} else {
			fileClicked = true;
		}
	}

	/*
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
	*/

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
		menuNav.Reset();
		menuNav.BuildMenu(m_connection, CString(m_path + clickedFolder.name + _T("/")), subMenu, 5);
		subMenu->DeleteMenu( ID_FOLDERS_BACK, MF_BYCOMMAND );

		// Check items
		CLocalStore stored;
		if(stored.IsFolderStored(f))
		{
			tracker->CheckMenuItem(ID_FOLDER_STORELOCALLY, MF_CHECKED | MF_BYCOMMAND);
		}

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
void CFileListView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	//TRACE3("nPos: %d   %d   curr: %d  \n", nSBCode, nPos, m_vScrollPos);
	if(nSBCode == 0 || nSBCode == 2){
		m_vScrollPos -= 20;
		if(m_vScrollPos < 0){
				m_vScrollPos = 0;
		}
		CPoint pt( 0, m_vScrollPos );
	//	ScrollToPosition( pt );
	}
	// Scroll
	if(nSBCode == 5){
		CPoint pt( 0, nPos );
	//	ScrollToPosition( pt );
		m_vScrollPos = nPos;
	}
	if(nSBCode == 1 || nSBCode == 3){
		m_vScrollPos += 20;
		CRect rect;
		GetClientRect( &rect );
	//	CSize s = GetTotalSize(); // buggy  need range
	//	if(m_vScrollPos > (s.cy - rect.Height())){
	//		m_vScrollPos = (s.cy - rect.Height());
	//	}
	//	CPoint pt( 0, m_vScrollPos );
	//	ScrollToPosition( pt );
	}

    // TODO: Add your message handler code here and/or call default
    //CScrollView::OnVScroll(nSBCode, nPos, pScrollBar);
    //Invalidate();
}


void CFileListView::OnSize(UINT nType, int cx, int cy)
{
	ResizeScrollViewForContent();
}

LRESULT CFileListView::ResizeFileViewForContent(WPARAM wParam,LPARAM lParam)
{
	ResizeScrollViewForContent();
	return 0;
}

void CFileListView::ResizeScrollViewForContent()
{
	CListCtrl&  list = GetListCtrl();
	list.Arrange(LVA_ALIGNLEFT);

	/*
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
	//SetScrollSizes(MM_TEXT, size); // fails from thread

	//CPoint pt( 0, m_vScrollPos );
	//ScrollToPosition( pt );
	//Invalidate();
	*/
}

void CFileListView::SetViewMode(int mode)
{
	CListCtrl&  list = GetListCtrl();
	if(mode == 0){ // Icon
		//
		list.ModifyStyle ( LVS_TYPEMASK, LVS_ICON | LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS      );
		list.SetExtendedStyle ( LVS_EX_INFOTIP       | LVS_EX_SUBITEMIMAGES);

		list.SetImageList ( m_ImageListLarge, LVSIL_SMALL );
		list.SetImageList ( m_ImageListLarge, LVSIL_NORMAL );

		OnSize(0, 0, 0); // resize scroll area for new content. 
	} else if(mode == 1) { // Detail
		list.ModifyStyle ( LVS_TYPEMASK, LVS_REPORT | LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS );
		list.SetExtendedStyle ( LVS_EX_INFOTIP   );
		
		list.SetImageList ( m_ImageListMed, LVSIL_SMALL );
		list.SetImageList ( m_ImageListMed, LVSIL_NORMAL );
		
		list.SetColumnWidth ( 0, LVSCW_AUTOSIZE_USEHEADER );
		list.SetColumnWidth ( 1, LVSCW_AUTOSIZE_USEHEADER );
		list.SetColumnWidth ( 2, LVSCW_AUTOSIZE_USEHEADER );
		list.SetColumnWidth ( 3, LVSCW_AUTOSIZE_USEHEADER );
		
		OnSize(0, 0, 0); // resize scroll area for new content. 
	}
}

//

bool CFileListView::GetSelectedFile(CFileContainer & file)
{
	if(m_selected >= 0 && m_selected < m_files.size()){
		file = (CFileContainer)m_files[m_selected];
		return true;
	} 
	return false;
}


BOOL CFileListView::PreTranslateMessage(MSG* pMsg)
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

	if(m_tooltip){
		m_tooltip.RelayEvent(pMsg);
	}

	return CListView::PreTranslateMessage(pMsg);
	//return false;
}


/**
* LoadServerConnection
*
*/
void CFileListView::LoadServerConnection(int index)
{
	CConnectionManager conManager;
	conManager.Load();
	CConnection * con = conManager.GetAt(index);
	m_connection = con;

	if(m_connection->host.GetLength() == 0){
		m_files.clear();
		AddFilesToList(m_files);
		m_viewSynced = true; // don't bother loading from network
		m_selected = -1;
		Invalidate(); // remove old content
		return;
	}

	// temp (load from disk first)
	CFtpManager ftp;
	//ftp.LoadFilesForView(con, CString(_T("/"))); // should be done in thread 
	
	// connection / bookmark
	if(m_connection->m_path.GetLength() == 0){
		m_path = CString(_T("/")); 
	} else {
		m_path = CString(m_connection->m_path); 
	}

	m_files.clear();
	ftp.ReadLocalDirectory(con, m_path, m_files);
	int c = m_files.size();
	TRACE1(" files: %d \n", c);

	// add to list
	AddFilesToList(m_files);

	m_viewSynced = false; // only loaded from disk, not network
	m_selected = -1;

	ResizeScrollViewForContent(); // resize scroll area for new content.  

	//CSize size;
	//size.cx = 200;
	//size.cy = 1000;
	//SetScrollSizes(MM_LOENGLISH, size); 
	//SetScrollSizes(MM_TEXT, size); 

	Invalidate(); // display what we have so far
}


/**
* LoadDirectory
*
*/
void CFileListView::LoadDirectory(CString &dir)
{
	if(m_connection->host.GetLength() == 0){
		return;
	}

	CFtpManager ftp;
	m_files.clear();
	ftp.ReadLocalDirectory(m_connection, dir, m_files);

	AddFilesToList(m_files);

	m_viewSynced = false; // only loaded from disk, not network
	m_selected = -1;
	m_path = CString(dir); 

	ResizeScrollViewForContent(); // resize scroll area for new content.  

	Invalidate(); // display what we have so far
}


/**
* SelectFile
*
* Description: Load a dir and select a given file.
*/
void CFileListView::SelectFile( CFileContainer * file )
{
	// Find file index in current view
	int i = 0;
	for(i = 0; i < m_files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)m_files[i];
		if( f.path.Compare(file->path) == 0 && f.name.Compare(file->name) == 0 )
		{
			m_selected = i; // set selected item
		}
	}
	
	// Select item
	CListCtrl& list = GetListCtrl();
	list.SetItemState(m_selected, LVIS_SELECTED, LVIS_SELECTED);
	list.SetSelectionMark(m_selected);
	list.EnsureVisible(m_selected, FALSE);
	list.SetFocus();

	//Invalidate(); // display what we have so far
}


/**
* RenameFile
*
*/
void CFileListView::RenameFile()
{
	if(m_selected > -1){
		CListCtrl& list = GetListCtrl();
		//list.EditLabel(m_selected);
		CFileContainer f;
		GetSelectedFile(f);
		CRenameDialog renameDlg;
		
		renameDlg.SetName(f.name);
		int r = renameDlg.DoModal();
		if(r == IDOK){
			CString name;
			renameDlg.GetName(name);
			TRACE1("Rename to: %s \n", name);

			CFtpManager ftp;
			bool renamed = ftp.RenameFile(m_connection, f.path, f.name, name);
			if(renamed){
				// refresh view
				LoadDirectory( f.path );
			} else {
				// Display error message
				TRACE0("Error renaming file.\n");
			}
		}
	}
}


/**
* DeleteFile
*
*/
void CFileListView::DeleteFile()
{
	if(m_selected > -1){
		CListCtrl& list = GetListCtrl();
		CFileContainer f;
		GetSelectedFile(f);

		CFtpManager ftp;
		bool deleted = ftp.DeleteFile(m_connection, f);
		if(deleted){
			// refresh view
			LoadDirectory( f.path );
		} else {
			// Display error message
			TRACE0("Error deleting file.\n");
		}
	}
}


/**
* ChangeFilePermissions
*
* Description: Change file or folder permissions of selected set.
*/
void CFileListView::ChangeFilePermissions()
{
	CFilePermissions dialog;

	CFileContainer f;
	GetSelectedFile(f);
	dialog.SetPermissions(f.permissions);

	//CInputDialog dialog;
	//dialog.SetWindowTextW(_T("bla")) ; // = TEXT("Folder name:");
	if (dialog.DoModal() == IDOK){
		// Do something
		CString input(_T(""));
		dialog.GetPermissions(input);
		CFtpManager ftp;
		CWaitDialog waitDialog;

		// file mouse clicked on
		bool changed = ftp.ChangeFilePermissions(m_connection, f, input, waitDialog);
		
		// selection set
		POSITION       pos;
		int            nSelItem;
		CListCtrl&  list = GetListCtrl();
		pos = list.GetFirstSelectedItemPosition();
		while ( NULL != pos )
		{
			nSelItem = list.GetNextSelectedItem ( pos );
			CFileContainer file;
			file = (CFileContainer)m_files[nSelItem];
			ftp.ChangeFilePermissions(m_connection, file, input, waitDialog);
			TRACE1(" selected file: %s \n", file.name);
		}

		if(!changed)
		{
			AfxMessageBox(_T("An error occured changing file permissions."));
		}
	}
}


/**
* UploadSelection
*
* Description: Upload selected and right clicked files
*/
void CFileListView::UploadSelection()
{
	CFtpManager ftp;
	CWaitDialog waitDialog;
	//if(m_historyDlg)
	//{
	//	ftp.m_historyDlg = m_historyDlg;
	//}

	// Mouse over while clicked
	CFileContainer f;
	GetSelectedFile(f); 
	if(f.dir){
		ftp.SendFolder(m_connection, f, waitDialog);
	} else {
		ftp.SendFile(m_connection, f, waitDialog);
	}

	// Selection set
	POSITION       pos;
	int            nSelItem;
	CListCtrl&  list = GetListCtrl();
	pos = list.GetFirstSelectedItemPosition();
	while ( NULL != pos )
	{
		nSelItem = list.GetNextSelectedItem ( pos );
		CFileContainer file;
		file = (CFileContainer)m_files[nSelItem];
		//TRACE1(" selected file: %s \n", file.name);
		if(f.name.Compare(file.name) != 0) // Not allready uploaded
		{
			if(file.dir){
				ftp.SendFolder(m_connection, file, waitDialog);
			} else {
				ftp.SendFile(m_connection, file, waitDialog);
			}
		}
	}
}


/**
* DownloadSelection
*
* Description: 
*/
void CFileListView::DownloadSelection()
{
	CFtpManager ftp;
	CWaitDialog waitDialog;
	//if(m_historyDlg)
	//{
	//	ftp.m_historyDlg = m_historyDlg;
	//}

	// Mouse over while clicked
	CFileContainer f;
	GetSelectedFile(f);  
	if(f.dir){
		ftp.ReceiveFolder(m_connection, f, waitDialog);
	} else {
		ftp.ReceiveFile(m_connection, f, waitDialog);
	}

	// Selection set
	POSITION       pos;
	int            nSelItem;
	CListCtrl&  list = GetListCtrl();
	pos = list.GetFirstSelectedItemPosition();
	while ( NULL != pos )
	{
		nSelItem = list.GetNextSelectedItem ( pos );
		CFileContainer file;
		file = (CFileContainer)m_files[nSelItem];
		//TRACE1(" selected file: %s \n", file.name);
		if(f.name.Compare(file.name) != 0) // Not allready uploaded
		{
			if(file.dir){
				ftp.ReceiveFolder(m_connection, file, waitDialog);
			} else {
				ftp.ReceiveFile(m_connection, file, waitDialog);
			}
		}
	}
}


/**
* CreateFolder
*
*/
void CFileListView::CreateFolder(CString & folder)
{
	CFtpManager ftp;
	bool deleted = ftp.CreateFolder(m_connection, m_path, folder);
	if(deleted){
		// refresh view
		LoadDirectory( m_path );
	} else {
		// Display error message
		TRACE0("Error creating folder.\n");
	}
}


void CFileListView::DeleteFolder()
{
	CFileContainer file;
	GetSelectedFile(file);

	CFtpManager ftp;
	bool deleted = ftp.DeleteFolder(m_connection, file);
	if(deleted){
		// refresh view
		LoadDirectory( m_path );
	} else {
		// Display error message
		TRACE0("Error creating folder.\n");
		AfxMessageBox(_T("Error: Unable to delete this folder."));
	}
}


/**
* GetSelectedFolder
*
*/
bool CFileListView::GetSelectedFolder(CFileContainer & folder)
{
	if(m_selected >= 0 && m_selected < m_files.size())
	{
		CFileContainer f;
		f = (CFileContainer)m_files[m_selected];
		if(f.dir)
		{
			folder = f;
			return true;
		}
	}
	return false;
}


void CFileListView::AddFilesToList(std::vector<CFileContainer> &files)
{
	GetListCtrl().DeleteAllItems();
	CListCtrl&  list = GetListCtrl();
	int i = 0;
	for(i = 0; i < files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)files[i];

		LVITEM      rItem;
		ZeroMemory ( &rItem, sizeof(LVITEM) );

        rItem.mask    = LVIF_IMAGE | LVIF_TEXT;
        rItem.iItem   = i; // nIndex;
		rItem.pszText = CString(f.name).AllocSysString(); // szNextFile;
        //rItem.iImage  = i; // rFileInfo.iIcon;

		//CBitmap bmp;
		//if (bmp.LoadBitmap(IDB_DRIVE_MED)){
			//HICON icon=LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
			//rItem. = icon;
			// m_imglist
		//}
		//IBBkImg.pszImage="res://C:\\myprog.exe/#2/#101";

		//LVBKIMAGE IBBkImg;
		//ZeroMemory(&IBBkImg,sizeof(LVBKIMAGE));
		//IBBkImg.ulFlags=LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
		//OleInitialize(NULL);
		//SendMessage (l_hwnd,LVM_SETBKIMAGE,0,(LPARAM)(LPLVBKIMAGE) &IBBkImg);

		
		int itemIndex = list.InsertItem (  &rItem );

		CRect rect;
		list.GetItemRect(itemIndex, &rect, LVIR_LABEL);

		CString toolTipText(_T(""));
		if(f.dir == 0){
			CString size(_T(""));
			size.Format(_T("%ld"),f.size);
			toolTipText = toolTipText + _T("size: ") + size + CString(_T("  "));
		}
		toolTipText = toolTipText + _T("permissions: ") + f.permissions;
		m_tooltip.AddTool(&list, toolTipText, &rect, itemIndex+1);

		if(f.dir){
			list.SetItemText ( i, 1, CString(_T("Folder")).AllocSysString() );
		//	rItem.iImage  = 0;
			GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_folderMedIndex , 0, 0, 0);
		} else {
			// File name
			list.SetItemText ( i, 1, CString(_T("File")).AllocSysString() );

			// File size
			CString fileSize(_T(""));
			fileSize.Format(_T("%ld"), f.size); 
			list.SetItemText ( i, 2, fileSize.AllocSysString() );

			// Permissions
			CString filePermissions(f.permissions);
			list.SetItemText ( i, 3, filePermissions.AllocSysString() );

			//rItem.iImage  = 1;
			if(m_viewMode == 0 ){ // Large
				if(f.name.Find(_T(".exe")) != -1){
					GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_exeLargeIndex, 0, 0, 0);
				} else { 
					// default file
					GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_fileLargeIndex, 0, 0, 0);
				}
			} else { // Medium
				if(f.name.Find(_T(".exe")) != -1){
					GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_exeMedIndex, 0, 0, 0);
				} else {
					GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_fileMedIndex, 0, 0, 0);
				}
			}
		}

		//CBitmap bmp;
		//bmp.LoadBitmap(IDB_FOLDER_MED);
		// add the thumbnail to the image list
		//int imgPos = m_imglist.Add(&bmp, RGB(0, 0, 0));
		//GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, imgPos , 0, 0, 0); // m_folderMedIndex

		//int imgPosNormal = m_imglistLarge.Add(&bmp, RGB(0, 0, 0));
		//GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, imgPosNormal , 0, 0, 0);
		//int imgPos2 = m_ImageList->Add(&bmp, RGB(0, 0, 0));
		//GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, imgPos2 , 0, 0, 0);

		//TRACE2("m_folderMedIndex %d %d \n" , imgPos2, m_folderMedIndex);

		//GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_folderMedIndex , 0, 0, 0);
	}


	// Select item
	if(m_selected > -1)
	{
		CListCtrl& l = GetListCtrl();
		l.SetItemState(m_selected, LVIS_SELECTED, LVIS_SELECTED);
		l.SetSelectionMark(m_selected);
		l.EnsureVisible(m_selected, FALSE);
		l.SetFocus();
	}
	{
		/*
		CListCtrl& l = GetListCtrl();
		GetListCtrl().SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		GetListCtrl().SetSelectionMark(0);
		GetListCtrl().EnsureVisible(0, FALSE);
		GetListCtrl().RedrawItems(0, 0);
		//this->SetRedraw(TRUE);
		//Invalidate();
		//this->RedrawWindow();
		//l.SetCheck(0, TRUE);
		l.SetFocus();
		//this->SetFocus();
		*/
	}
}




//
// Drag drop
//


/**
* OnBegindrag - OUT
* 
* Description: download, copy from app to system
*/
void CFileListView::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
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

	CString localStorePath;
	CFtpManager ftp;
	ftp.GetLocalStorePath(m_connection, localStorePath);

	CWaitDialog waitDialog; // For downloading files not local.

	bool files_ready = true;

    // For every selected item in the list, put the filename into lsDraggedFiles.
	CListCtrl&  list = GetListCtrl();
    pos = list.GetFirstSelectedItemPosition();
    while ( NULL != pos )
	{
        nSelItem = list.GetNextSelectedItem ( pos );
        //sFile = list.GetItemText ( nSelItem, 0 );  // Nope, this is name only not full path

		CFileContainer file;
		file = (CFileContainer)m_files[nSelItem];
		sFile = CString(localStorePath + file.path + file.name);
		sFile.Replace('/', '\\');

		// Is file or folder

		if( IsDirectory(sFile) )
		{

			// the folder must exist
			bool r = ftp.IsFolderLocal(m_connection, file);
			if(r == false)
			{
				AfxMessageBox(_T("Sorry, these files are not local, they are being downloaded now. Please drag again once the transfer is complete."));

				// download folder
				ftp.ReceiveFolder(m_connection, file, waitDialog);
			}
			else 
			{
				// Don't include file list '.files.rftp'
				//if( sFile.Find(_T(".files.rftp")) == -1 )
				//{
					lsDraggedFiles.AddTail ( sFile );
					//TRACE1(" file: ",  sFile);
				//}
			}
		}
		else  // File
		{
			// The file must exist
			if ( 0xFFFFFFFF == GetFileAttributes ( sFile ) && GetLastError() == ERROR_FILE_NOT_FOUND )
			{
				TRACE1(" File not found %s \n", sFile);
				// Or file out of date.
			
			
				// Start download...
				ftp.ReceiveFile(m_connection, file, waitDialog);
				if(files_ready){ // Only domodal once, first file.
					// Alert user.
					AfxMessageBox(_T("Sorry, these files are not local, they are being downloaded now. Please drag again once the transfer is complete."));

					waitDialog.DoModal();
				}
				files_ready = false;

			} else {

				// Don't include file list '.files.rftp'
				//if( sFile.Find(_T(".files.rftp")) != -1 )
				//{
					lsDraggedFiles.AddTail ( sFile );
				//} 
			}
		} // is file

        // Calculate the # of chars required to hold this string.
        uBuffSize += lstrlen ( sFile ) + 1;
	}

	// Abort if files are not ready.
	if(!files_ready)
	{
		// Cancel drag/drop
		return;
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
	DROPEFFECT dwEffect = datasrc.DoDragDrop ( DROPEFFECT_COPY  ); // | DROPEFFECT_MOVE

	
    // If the DnD completed OK, we remove all of the dragged items from our list.
    switch ( dwEffect )
	{
        case DROPEFFECT_COPY:
		{
			
		}
        case DROPEFFECT_MOVE:
		{
            // The files were copied or moved.
            // Note: Don't call GlobalFree() because the data will be freed by the drop target.
            for ( nSelItem = list.GetNextItem ( -1, LVNI_SELECTED );
                  nSelItem != -1;
                  nSelItem = list.GetNextItem ( nSelItem, LVNI_SELECTED ) )
			{
                //list.DeleteItem ( nSelItem );
                //nSelItem--;
			}

            // Resize the list columns.
            //list.SetColumnWidth ( 0, LVSCW_AUTOSIZE_USEHEADER );
            //list.SetColumnWidth ( 1, LVSCW_AUTOSIZE_USEHEADER );
            //list.SetColumnWidth ( 2, LVSCW_AUTOSIZE_USEHEADER );
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



					/*
                    if ( 0xFFFFFFFF == GetFileAttributes ( sFile ) &&
                         GetLastError() == ERROR_FILE_NOT_FOUND )
					{
                        // We couldn't read the file's attributes, and GetLastError()
                        // says the file doesn't exist, so remove the corresponding 
                        // item from the list.
                        //list.DeleteItem ( nSelItem );
                    
                        nSelItem--;
                        bDeletedAnything = true;
					}
					*/
				}

                // Resize the list columns if we deleted any items.
                if ( bDeletedAnything )
				{
                    //list.SetColumnWidth ( 0, LVSCW_AUTOSIZE_USEHEADER );
                    //list.SetColumnWidth ( 1, LVSCW_AUTOSIZE_USEHEADER );
                    //list.SetColumnWidth ( 2, LVSCW_AUTOSIZE_USEHEADER );

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
		
}


/**
* In
*/
DROPEFFECT CFileListView::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) 
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

DROPEFFECT CFileListView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) 
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


BOOL CFileListView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) 
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

void CFileListView::OnDragLeave() 
{
    if ( m_bUseDnDHelper )
    {
        m_piDropHelper->DragLeave();
    }
}

/**
* Called by drop target. 
*    1) Displays wait dialog.
*    2) Copies droped files into local store.
*    3) Starts FTP transfer of files to server in thread and continues.
*    4) Updates wait dialog progress with file transfer, app is usable otherwise.
*
* ReadHdropData() reads CF_HDROP data from the passed-in data object, and 
* puts all dropped files/folders into the main window's list control.
*/
BOOL CFileListView::ReadHdropData ( COleDataObject* pDataObject )
{
	HGLOBAL     hg;
	HDROP       hdrop;
	UINT        uNumFiles;
	TCHAR       szNextFile [MAX_PATH];
	SHFILEINFO  rFileInfo;
	LVFINDINFO  rlvFind = { LVFI_STRING, szNextFile };
	LVITEM      rItem;
	CListCtrl&  list = GetListCtrl();
	int         nIndex = list.GetItemCount();
	HANDLE      hFind;
	WIN32_FIND_DATA rFind;
	TCHAR       szFileLen [64];
	CFtpManager ftp;

	CWaitDialog waitDialog; // wait dialog (Don't display)

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

	bool sent = false;

    // Get the # of files being dropped.
    uNumFiles = DragQueryFile ( hdrop, -1, NULL, 0 );
    for ( UINT uFile = 0; uFile < uNumFiles; uFile++ )
	{
        // Get the next filename from the HDROP info.
        if ( DragQueryFile ( hdrop, uFile, szNextFile, MAX_PATH ) > 0 )
        {
			CString localStorePath;
			ftp.GetLocalStorePath(m_connection, localStorePath);
			CString localPath(localStorePath + m_path);
			localPath.Replace('/', '\\');

			CString fileName(szNextFile);
			int start = fileName.ReverseFind('\\');
			if(start != -1){
				fileName = fileName.Mid(start + 1, fileName.GetLength() - start - 1);
			}
			localPath = CString(localPath + fileName);

			TRACE2(" Drop file:   from: %s  to: %s  \n", szNextFile, localPath);

			
			// if source a folder?
			if(IsDirectory(szNextFile))  // Directory
			{
				TRACE0(" DIR \n");

				SHFILEOPSTRUCT s = { 0 };   // copy directory
				s.hwnd = m_hWnd;
				s.wFunc = FO_COPY;
				CString from(szNextFile); 
				from +=  _T("\\*");
				from += '\0';
				s.pFrom = from.AllocSysString();
				CString to(localPath); 
				to += _T("\\");
				to += '\0';
				s.pTo = to.AllocSysString();
				s.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION;
				int r = SHFileOperation( &s );
				if(r == 0)
				{
					CFileContainer file;
					file.path = m_path; // + fileName;
					file.name = fileName;
					file.dir = true;
					ftp.SendFolder(m_connection, file, waitDialog);
				}

			} else {				//  file

				// Copy file to local store
				bool copied = CopyFile((LPCTSTR)CString(szNextFile), (LPCTSTR)localPath, false);
				if(copied == false){
					DWORD e = GetLastError();
					TRACE1(" Error: %d  \n", e);
				}
				if(copied){
					// FTP Send file to server
					//bool sent = ftp.SendFile(m_connection, m_path, fileName); // depricate
					CFileContainer file;
					file.path = m_path;
					file.name = fileName;
					bool s = ftp.SendFile(m_connection, file, waitDialog);
					if(!sent){
						//TRACE0("Error sending file...\n");
					}
					sent = true;
				}
			}

			/*
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
            
            // Set column #1 to the file's type description.
            
            // Get the file size, and put that in column #2.
            hFind = FindFirstFile ( szNextFile, &rFind );
            if ( INVALID_HANDLE_VALUE != hFind )
			{
                StrFormatByteSize ( rFind.nFileSizeLow, szFileLen, 64 );
                FindClose ( hFind );
			}
			*/

            nIndex++;
		}
	} // files

	// Refresh view vith new content
	if(sent){
		// Reload view
		LoadDirectory(m_path);
	}
    return TRUE;
}


//not completely tested but after some debug I'm sure it'll work
bool CFileListView::IsDirectory(LPCTSTR sDirName)
{
    //First define special structure defined in windows
    WIN32_FIND_DATA findFileData; ZeroMemory(&findFileData, sizeof(WIN32_FIND_DATA));
    //after that call WinAPI function finding file\directory
    //(don't forget to close handle after all!)
    HANDLE hf = ::FindFirstFile(sDirName, &findFileData);
    if (hf  ==  INVALID_HANDLE_VALUE) //also predefined value - 0xFFFFFFFF
    return false;
    //closing handle!
    ::FindClose(hf);
    // true if directory flag in on
    return (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}


CString CFileListView::GetPath()
{
	return m_path;
}


/*
INT_PTR CFileListView::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
    CPoint pt(GetMessagePos());
    ScreenToClient(&pt);
	

    //if (!ShowToolTip(pt))
     //   return -1;

    int nRow, nCol;
    //CellHitTest(pt, nRow, nCol);

    //Get the client (area occupied by this control)
    RECT rcClient;
    GetClientRect( &rcClient );

    //Fill in the TOOLINFO structure
    pTI->hwnd = m_hWnd;
    pTI->uId = (UINT) (nRow * 1000 + nCol);
    // Send TTN_NEEDTEXT when tooltip should be shown
    pTI->lpszText = LPSTR_TEXTCALLBACK;
    pTI->rect = rcClient;

    return pTI->uId;
    // Must return a unique value for each cell (Marks a new tooltip)
}

BOOL CFileListView::OnToolNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
    CPoint pt(GetMessagePos());
    ScreenToClient(&pt);

    int nRow, nCol;
    //CellHitTest(pt, nRow, nCol);

    CString tooltip = GetToolTipText(nRow, nCol);
    //if (tooltip.IsEmpty())
        return FALSE;

    // Non-unicode applications can receive requests for tooltip-text in unicode
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
#ifndef _UNICODE
    if (pNMHDR->code == TTN_NEEDTEXTA)
        lstrcpyn(pTTTA->szText, static_cast<LPCTSTR>(tooltip), sizeof(pTTTA->szText));
    else
        _mbstowcsz(pTTTW->szText, static_cast<LPCTSTR>(tooltip), 
				sizeof(pTTTW->szText)/sizeof(WCHAR));
#else
    if (pNMHDR->code == TTN_NEEDTEXTA)
        _wcstombsz(pTTTA->szText, static_cast<LPCTSTR>(tooltip), sizeof(pTTTA->szText));
    else
        lstrcpyn(pTTTW->szText, static_cast<LPCTSTR>(tooltip), 
				sizeof(pTTTW->szText)/sizeof(WCHAR));
#endif
    // If wanting to display a tooltip which is longer than 80 characters,
    // one must allocate the needed text-buffer instead of using szText,
    // and point the TOOLTIPTEXT::lpszText to this text-buffer.
    // When doing this, one is required to release this text-buffer again
    return TRUE;
}
*/


/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView printing

BOOL CFileListView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CFileListView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CFileListView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView diagnostics

#ifdef _DEBUG
void CFileListView::AssertValid() const
{
	CView::AssertValid();
}

void CFileListView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CConnectionListDoc* CFileListView::GetDocument() // non-debug version is inline
{
	//ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSplitterWindowTutorialDoc)));
	return (CConnectionListDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialView message handlers

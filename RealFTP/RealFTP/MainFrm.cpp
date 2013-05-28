
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "RealFTP.h"
#include "MainFrm.h"
#include "MyVisualManager.h"
#include "ConnectionDialog.h"
#include "HistoryDialog.h"
#include "MenuNavigator.h"
#include "InputDialog.h"
#include "TermsDialog.h"
#include "LocalStore.h"
#include "ExitDialog.h"
#include "Upgrade.h"
#include "DefaultWebBrowser.h"
#include "Registry.h"
#include "UpgradeDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_PAINT ()  // JDT
	ON_WM_SIZE()
	//ON_WM_NCHITTEST() // JDT
	ON_WM_SETFOCUS()
	ON_WM_ERASEBKGND()  // JDT
	ON_WM_CLOSE()
	//ON_WM_SYSCOMMAND()  // Adding menu items. Locks window in place
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)
	ON_COMMAND(ID_NEW_CONNECTION_MENU_ITEM, &CMainFrame::OnMenuNewConnection)
	ON_BN_CLICKED(IDC_BACK_BUTTON_ID, &CMainFrame::OnBackButton)
	ON_BN_CLICKED(IDC_NEXT_BUTTON_ID, &CMainFrame::OnNextButton)
	ON_BN_CLICKED(IDC_QUEUE_BUTTON_ID, &CMainFrame::OnQueueButton)
	//ON_CONTROL(EDIT_COMPLETE, IDC_SEARCH, &CMainFrame::OnQueueButton)
	ON_COMMAND(ID_NEWCONNECTION_NEWCONNECTION, &CMainFrame::OnNewConnectionMenu)
	ON_COMMAND(ID_CONNECTION_EDIT, &CMainFrame::OnConnectionEdit)
	ON_COMMAND(ID_CONNECTION_DELETE, &CMainFrame::OnConnectionDelete)
	ON_COMMAND(ID_FOLDER_RENAME, &CMainFrame::OnFolderRename)
	ON_COMMAND(ID_VIEW_ICON, &CMainFrame::OnViewIcon)
	ON_COMMAND(ID_VIEW_DETAIL, &CMainFrame::OnViewDetail)
	ON_COMMAND(ID_FILE_OPEN, &CMainFrame::OnFileOpen)
	ON_COMMAND(ID_FILE_EDIT, &CMainFrame::OnFileEdit)
	ON_COMMAND(ID_FILE_RENAME, &CMainFrame::OnFileRename)
	ON_COMMAND(ID_FILE_DELETE, &CMainFrame::OnFileDelete) 
	ON_COMMAND(ID_FOLDERS_NEWFOLDER, &CMainFrame::OnNewFolder) 
	ON_COMMAND(ID_FOLDER_DELETE, &CMainFrame::OnFolderDelete)
	ON_COMMAND(ID_FOLDERS_BOOKMARK, &CMainFrame::OnFoldersBookmark)
	ON_COMMAND(ID_FOLDER_BOOKMARK, &CMainFrame::OnBookmarkSelectedFolder)
	ON_COMMAND(ID_FOLDER_STORELOCALLY, &CMainFrame::OnToggleFolderStoreLocally)
	ON_COMMAND(ID_FOLDER_DOWNLOAD, &CMainFrame::OnFileDownload)
	ON_COMMAND(ID_FOLDER_UPLOAD, &CMainFrame::OnFileUpload) 
	ON_COMMAND(ID_FILE_PERMISSIONS, &CMainFrame::OnFilePermissions)
	ON_COMMAND(ID_FOLDER_PERMISSIONS, &CMainFrame::OnFilePermissions)
	ON_COMMAND(ID_FILE_DOWNLOAD, &CMainFrame::OnFileDownload) 
	ON_COMMAND(ID_FILE_UPLOAD, &CMainFrame::OnFileUpload) 
	ON_COMMAND(ID_UPGRADE_APP, &CMainFrame::OnUpgrade) 
	ON_COMMAND(ID_HELP_MANUAL, &CMainFrame::OnHelpManual)
	ON_COMMAND(ID_HELP_BUYUPGRADE, &CMainFrame::OnBuyUpgrade)
	ON_COMMAND(ID_HELP_SUPPORT, &CMainFrame::OnSupport)
	ON_COMMAND_RANGE(5000, 6000, &CMainFrame::OnMenuNavigateItem)
	ON_MESSAGE(333, &CMainFrame::ResizeFileViewForContent)
	ON_MESSAGE(ID_UPDATE_PROGRESS, &CMainFrame::OnUpdateProgress)
	ON_MESSAGE(ID_UPGRADE_AVAILABLE, &CMainFrame::OnUpgradeAvailable)
	ON_MESSAGE(ID_PENDING_TRANSFERS, &CMainFrame::OnPendingTransfers)
	ON_MESSAGE(ID_LOAD_URL, &CMainFrame::OnLoadUrl)
	ON_MESSAGE(ID_LOAD_DIR, &CMainFrame::OnLoadDir)
	ON_MESSAGE(ID_SELECT_FILE, &CMainFrame::OnSelectFile)
	ON_MESSAGE(ID_EXIT, &CMainFrame::OnExit)
	ON_MESSAGE(ID_AD_CHANGED, &CMainFrame::OnAdChanged)
	ON_MESSAGE(ID_UPGRADE_COMPLETE, &CMainFrame::OnUpgradeComplete)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2008);

	// flag to check if the splitter has been created
	m_bInitSplitter = false;
	m_exit = false;
	
	// Terms
	CTermsDialog termsDialog;
	termsDialog.DiscloseTerms();

}

CMainFrame::~CMainFrame()
{
	delete m_historyDlg;
}


/**
* OnClose
*
* Description: 
*/
void CMainFrame::OnClose()
{
	if(m_exit)
	{
		CFrameWndEx::OnClose();
		HideTrayIcon();
	}
	CExitDialog exitDialog;
	int dlgRsp = exitDialog.DoModal();
	if(dlgRsp == IDCANCEL)
	{
		CFrameWndEx::OnClose();
		HideTrayIcon();
	}
	else 
	{
		ShowWindow(SW_HIDE);
	}
}


/**
* OnCreate
*
* Description: App crete event. Initalize threads and objects.
*/
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	HICON icon=LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
	SetIcon(icon, FALSE);

	BOOL bNameValid;
	// set the visual manager and style based on persisted value
	//OnApplicationLook(theApp.m_nAppLook);
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMyVisualManager)); // CMyVisualManager CMFCVisualManagerVS2008
	CDockingManager::SetDockingMode(DT_SMART);
	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}
	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	// Replaced with split view
	// create a view to occupy the client area of the frame 
	/*
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 10, 10), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}
	*/

	
	/*
	//theApp.m_bHiColorIcons = 0;
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) 
			||
		!m_wndToolBar.LoadToolBar( IDR_MAINFRAME )) // theApp.m_bHiColorIcons ? IDR_MAINFRAME_256 : IDR_MAINFRAME
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	
	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	//m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	// Allow user-defined toolbars operations:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);
	*/
	
	if (!m_headerDoc.Create(_T("test"), 
							this, 
							CSize(400, 40), 
							false, // gripper
							4,   // nID child win id (kinda works with 0)
							WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP | CBRS_FLOAT_MULTI,  // WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_HIDE_INPLACE
							AFX_CBRS_REGULAR_TABS, 
							AFX_DEFAULT_DOCKING_PANE_STYLE) )
	{
		TRACE0("Failed to create Header Bar \n");
		return -1;      // fail to create
	}
//	m_headerDoc.EnableDocking(CBRS_ALIGN_ANY); 
	//m_headerDoc.SetResizeMode(false);
	m_headerDoc.SetMinSize(CSize(200, 40));
	m_headerDoc.OnInitialUpdate();
	//m_headerDoc.ShowPane(TRUE, FALSE, TRUE);
	m_headerDoc.ShowWindow(SW_SHOW);
	

	// FooterPane
	CRect r = CRect(400, 15, 10, 15);
	m_footerPane.Create(_T("CFooterPane"),  WS_VISIBLE, r, this, 5, AFX_DEFAULT_DOCKING_PANE_STYLE, 0);
	/*
	if (!m_footerPane.Create(_T("footer"), 
							this, 
							CSize(400, 15), 
							false, // gripper
							4,   // nID child win id (kinda works with 0)
							WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI,  // WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_HIDE_INPLACE
							AFX_CBRS_REGULAR_TABS, 
							AFX_DEFAULT_DOCKING_PANE_STYLE) )
	{
		TRACE0("Failed to create Header Bar \n");
		return -1;      // fail to create
	}
	*/
	m_footerPane.SetMinSize(CSize(200, 15));
	m_footerPane.OnInitialUpdate();
	//m_footerPane.ShowWindow(SW_SHOW);


	//CDockBar *pTop = (CDockBar *)::AfxGetMainWnd()->GetControlBar(AFX_IDW_DOCKBAR_TOP);
	
	//DWORD x = m_headerDoc.GetEnabledAlignment();
	//DWORD y = CBRS_ALIGN_ANY;

	
	//m_wndStatusBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, AFX_IDW_STATUS_BAR);
	//if (!m_wndStatusBar.Create(this))
	//{
	//	TRACE0("Failed to create status bar\n");
	//	return -1;      // fail to create
	//}
	//m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));
	

	// TODO: Delete these five lines if you don't want the toolbar and menubar to be dockable
	//m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	//m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);   // required or won't show toolbar
	DockPane(&m_wndMenuBar);
	//DockPane(&m_wndToolBar);
	DockPane(&m_headerDoc);

	// Bottom pane
	//m_footerPane.EnableDocking(CBRS_ALIGN_BOTTOM);
	//DockPane(&m_footerPane, AFX_IDW_DOCKBAR_BOTTOM);

	//m_wndToolBar.ShowPane(SW_SHOW,0,0);
	//m_wndToolBar.SetWi

	// enable Visual Studio 2005 style docking window behavior
	//CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// Enable toolbar and docking window menu replacement
	//EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	//CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == NULL)
	{
		// load user-defined toolbar images
		if (m_UserImages.Load(_T(".\\UserImages.bmp")))
		{
			CMFCToolBar::SetUserImages(&m_UserImages);
		}
	}

	// enable menu personalization (most-recently used commands)
	// TODO: define your own basic commands, ensuring that each pulldown menu has at least one basic command.
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	//CMFCToolBar::SetBasicCommands(lstBasicCommands);


	// Display New Connection Dialog if no connections exist
	m_connectionManager.Load();
	CObArray * listOfConnections;
	listOfConnections = m_connectionManager.GetConnections();
	int connectionCount = listOfConnections->GetSize();
	if(connectionCount == 0)
	{
		PostMessage( WM_COMMAND, ID_NEW_CONNECTION_MENU_ITEM, 0 );
	}

	// Start sync thread
	//
	//m_ftpManager.SyncServerConnections();
	THREADPARAM * _param = new THREADPARAM;
	_param->m = this;
	// THREAD_PRIORITY_IDLE   THREAD_PRIORITY_LOWEST
	m_pFtpSyncThread = AfxBeginThread(CMainFrame::FtpSyncWorker, &_param, THREAD_PRIORITY_LOWEST); //, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (m_pFtpSyncThread != NULL)
	{
		//TRACE(_T("AfxBeginThread: 0x%08lX\n"), m_pFtpSyncThread->m_nThreadID);
	}


	// Start Ad manager
	//m_wndMenuBar.m_ad = m_ad;
	//m_ad.Start();
	

	// Create tray icon.
	LoadTrayIcon();

	// Check for an update
	CUpgrade update;
	update.CheckForUpgrade();

	// Hostory
	m_historyDlg = new CHistoryDialog(this);
	m_historyDlg->Create(CHistoryDialog::IDD, GetDesktopWindow());
	
	// Registry - GUID
	//CRegistry reg;
	//bool p = reg.IsPurchased();
	//reg.Purchase();
	//p = reg.IsPurchased();
	//CString guid(_T(""));
	//reg.GetGUID(guid);

	return 0;
}


/**
* FtpSyncWorker
*
*/
UINT CMainFrame::FtpSyncWorker( LPVOID pParam )
{
	CMainFrame::THREADPARAM *const pThreadParam = static_cast<CMainFrame::THREADPARAM *>(pParam);
	while(true){
		TRACE0("FTP Server Sync\n");
		pThreadParam->m->m_ftpManager.SyncServerConnections();
		Sleep(1000 * 60 * 60); // hourly
	}
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// *********
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	//calculate client size
	CRect cr;
	GetWindowRect( &cr );
	
	
	// Create the main splitter with 1 row and 2 columns
	if ( !m_mainSplitter.CreateStatic( this, 1, 2 ) )
	{
		//MessageBox( _T("Error setting up m_mainSplitter"), _T("ERROR"), MB_OK | MB_ICONERROR );
		return FALSE;
	}
	// The views for each pane must be created 
	if ( !m_mainSplitter.CreateView( 0, 0, RUNTIME_CLASS(CConnectionListView), CSize(cr.Width()/4, cr.Height()), pContext ) )
	{
		MessageBox( _T("Error setting up splitter view"), _T("ERROR"), MB_OK | MB_ICONERROR );
		return FALSE;
	}
	if ( !m_mainSplitter.CreateView( 0, 1, RUNTIME_CLASS(CFileListView), CSize(cr.Width()/4 * 3 , cr.Height()), pContext ) )
	{
		MessageBox( _T("Error setting up splitter view"), _T("ERROR"), MB_OK | MB_ICONERROR );
		return FALSE;
	}
	// Set 
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
	v->m_historyDlg = m_historyDlg;


	/*
	if ( !m_mainSplitter.CreateView( 1, 0, RUNTIME_CLASS(CFooterPane), CSize(cr.Width() , 15), pContext ) )
	{
		return FALSE;
	}
	if ( !m_mainSplitter.CreateView( 1, 1, RUNTIME_CLASS(CFooterPane), CSize(cr.Width() , 15), pContext ) )
	{
		return FALSE;
	}
	*/




	/*
	CFrameWndEx * f = this;
	// 2 rows 1 column
	if(!m_vertical_splitter.CreateStatic(this, 2, 1, WS_CHILD | WS_VISIBLE, AFX_IDW_PANE_FIRST))
	{
		return FALSE;
	}
	// first row is main spliter
	if (!m_mainSplitter.CreateStatic(
		&m_vertical_splitter, // our parent window is the first splitter
		1, 2, // the new splitter is 2 rows, 1 column
		WS_CHILD | WS_VISIBLE | WS_BORDER, // style, WS_BORDER is needed
		m_vertical_splitter.IdFromRowCol(0, 0)))	// new splitter is in the first row, 2nd column 
	{
		TRACE0("Failed to create nested splitter\n");
		return FALSE;
	}
	// The views for each pane must be created 
	if ( !m_mainSplitter.CreateView( 0, 0, RUNTIME_CLASS(CConnectionListView), CSize(cr.Width()/4, cr.Height()), pContext ) )
	{
		MessageBox( _T("Error setting up splitter view"), _T("ERROR"), MB_OK | MB_ICONERROR );
		return FALSE;
	}
	if ( !m_mainSplitter.CreateView( 0, 1, RUNTIME_CLASS(CFileListView), CSize(cr.Width()/4 * 3 , cr.Height()), pContext ) )
	{
		MessageBox( _T("Error setting up splitter view"), _T("ERROR"), MB_OK | MB_ICONERROR );
		return FALSE;
	}
	
	// WS_EX_WINDOWEDGE
	// WS_EX_CLIENTEDGE
	//dwExStyle &= ~WS_EX_CLIENTEDGE;
	//m_vertical_splitter.ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
	if ( !m_vertical_splitter.CreateView( 1, 0, RUNTIME_CLASS(CFooterPane), CSize(cr.Width(), 15), pContext ) )
	{
		return FALSE;
	}
	//m_vertical_splitter.GetPane(0, 1)->SetS
	*/
	

	//int a = m_mainSplitter.GetColumnCount();
	//int b = m_vertical_splitter.GetColumnCount();

	// CDockablePane
	// CFrameWndEx
	//CFrameWndEx * fx = new CFrameWndEx();
	if(AfxIsExtendedFrameClass(this))
	{
		int ok = 1;
		//m_wndStatusBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, AFX_IDW_STATUS_BAR);
	}

	/*
	bool c = m_statusBar.Create(this); // , WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, AFX_IDW_STATUS_BAR);
	m_statusBar.SetIndicators(indicators, 4); //Set the number of panes 
	//m_statusBar.m_nMinHeight = 20;
	CRect rect;
	GetClientRect(&rect);
	//Size the two panes
	m_statusBar.SetPaneInfo(0, AFX_IDS_APP_TITLE, SBPS_NORMAL, rect.Width()-100);      
	//m_statusBar.SetPaneInfo(1, ID_INDICATOR_TIME, SBPS_STRETCH, 0);
	m_statusBar.SetPaneText(0, _T("test") );
	//This is where we actually draw it on the screen
	//RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST, ID_INDICATOR_TIME);
	m_statusBar.GetStatusBarCtrl().SetBkColor(RGB(255,0,0));
	//m_statusBar.ShowWindow(
	*/
	
	//if (!m_wndStatusBar.Create(this))
	//{
	//	TRACE0("Failed to create status bar\n");
	//	return -1;      // fail to create
	//}
	//m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));


	
	//change flag to show splitter created
	m_bInitSplitter = true;

	//return TRUE instead of the parent method since that would
	//not show our window
	return TRUE;
}



// CMainFrame message handlers

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// forward focus to the view window
	//m_wndView.SetFocus();
	m_mainSplitter.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	//if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	//	return TRUE;

	if (m_mainSplitter.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;


	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;


	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMyVisualManager)); // CMyVisualManager CMFCVisualManagerVS2008
	CDockingManager::SetDockingMode(DT_SMART);

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	// base class does the real work

	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}


	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i ++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}

	return TRUE;
}


/**
* OnSize
*
*/
void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
	
	CRect cr;
	GetWindowRect(&cr);
	
	if (m_bInitSplitter && nType != SIZE_MINIMIZED )
	{
		/*
		m_mainSplitter.SetRowInfo( 0, cy, 0 );
		m_mainSplitter.SetColumnInfo( 0, cr.Width() / 2, 50);
		m_mainSplitter.SetColumnInfo( 1, cr.Width() / 2, 50);
		m_mainSplitter.RecalcLayout();
		*/

		if(cy > 150 && false){

			if(m_headerDoc){
				CSize header_size = CSize(400, 40);
				m_headerDoc.AdjustLayout();
				m_headerDoc.AdjustSizeImmediate(true);
				//m_headerDoc.ShowPane(true, false, false);
				//m_headerDoc.CalcWindowRect(

				//m_headerDoc.SetMinSize(header_size);
				//m_headerDoc.UpdateVirtualRect(header_size); 
			}
			int body_height = 0;
			int body_min = 0;
			int footer_height = 0;
			int footer_min = 0;
			m_vertical_splitter.GetRowInfo(0, body_height, body_min);
			m_vertical_splitter.GetRowInfo(1, footer_height, footer_min);

			TRACE3("  footer_height: %d   cy: %d   body:  %d ", footer_height  , cy, body_height);

			m_vertical_splitter.SetRowInfo(0, cy - 100, cy - 100);
			m_vertical_splitter.SetRowInfo(1, 15,  15);
		} else {
		
		}
		//m_vertical_splitter.SetColumnInfo( 0, cr.Width(), 15);
		//m_vertical_splitter.RecalcLayout();

		//TRACE1("  cy %d \n", cy);
	}
}



// 
LRESULT CMainFrame::OnNcHitTest(CPoint point)
{	
	LRESULT res = CFrameWndEx::OnNcHitTest(point);
	//TRACE1("Hit %d \n", res);
	if( res == HTLEFT || res == HTRIGHT )
	//	res = HTBORDER;
	if(res == HTTOP || res==HTBOTTOM) {
		//res = HTBORDER;
	}
	if(res == HTBORDER){
		int i = 0;
	}
	return res;
}

// 
BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	// Set brush to desired background color
	//CBrush backBrush(RGB(236, 236, 236));
	  
	// Save old brush
	//CBrush* pOldBrush = pDC->SelectObject(&backBrush);
	//CRect rect;
	//pDC->GetClipBox(&rect);     // Erase the area needed
//	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);
	//pDC->SelectObject(pOldBrush);
      
	return FALSE;
}

void CMainFrame::OnPaint ()
{
    CPaintDC dc (this);
    
    CRect rect;
    GetClientRect (&rect);

	
	//CBrush backBrush(RGB(255, 0, 0));
	//CBrush* pOldBrush = dc.SelectObject(&backBrush);
	
	/*
	CRect rect;
      dc.GetClipBox(&rect);     // Erase the area needed
      dc.PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),
          PATCOPY);
	*/
}

void CMainFrame::OnMenuNewConnection()
{
	// TODO: Add your command handler code here
	CConnectionDialog dlg;
	dlg.DoModal();
	
	// Update view with updated connections
	CConnectionListView * v = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	v->UpdateConnections();
}


void CMainFrame::OnBackButton()
{
	m_headerDoc.OnBackButton();
}

void CMainFrame::OnNextButton()
{
	//TRACE0("Main: Next Button");
	m_headerDoc.OnNextButton();
}


/**
* OnQueueButton
*
* Description: Display history dialog window
*/
void CMainFrame::OnQueueButton()
{
	//TRACE0(" Main: Queue Button");
	m_historyDlg->ShowWindow(SW_SHOW);
	//CHistoryDialog dlg;
	//dlg.DoModal();
}

void CMainFrame::OnNewConnectionMenu()
{
	// TODO: Add your command handler code here
	OnMenuNewConnection();
}


void CMainFrame::OnConnectionEdit()
{
	CConnectionListView * v = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	CConnectionDialog dlg;
	CConnectionManager conManager;
	conManager.Load();
	CConnection * con = conManager.GetAt(v->m_selectedItemIndex);
	dlg.SetConnectionIndex(v->m_selectedItemIndex);
	dlg.DoModal();
	v->UpdateConnections(); // Reload view
}


void CMainFrame::OnConnectionDelete()
{
	CConnectionListView * v = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	CConnectionManager conManager;
	conManager.Load();
	conManager.RemoveAt(v->m_selectedItemIndex);
	conManager.Store();
	v->UpdateConnections();
}


void CMainFrame::OnFolderRename()
{
	TRACE0("OnFolderRename: \n");
}

void CMainFrame::OnNewFile()
{
}


void CMainFrame::OnViewIcon()
{
	TRACE0("OnViewIcon: \n");
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
	v->SetViewMode(0);
}

void CMainFrame::OnViewDetail()
{
	TRACE0("OnViewDetail: \n");
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
	v->SetViewMode(1);
}


/**
* OnFileOpen
*  
*/
void CMainFrame::OnFileOpen()
{
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);

	THREADPARAM * _param = new THREADPARAM;
	_param->m = this;
	_param->filelistView = v;

	//CWaitDialog * waitDialog = new CWaitDialog();
	//waitDialog->Create(IDD_WAIT_DIALOG, this);
	//_param->waitDialog = waitDialog;
	
	
	m_pFileOpenThread = AfxBeginThread(CMainFrame::OnFileOpenThread, _param); 
	if (m_pFileOpenThread != NULL)
	{
		//TRACE(_T("AfxBeginThread: 0x%08lX\n"), m_pFileOpenThread->m_nThreadID);
	}
}

// 
UINT CMainFrame::OnFileOpenThread(LPVOID pParam)
{
	CMainFrame::THREADPARAM *const pThreadParam = static_cast<CMainFrame::THREADPARAM *>(pParam);
	//TRACE0("OnFileOpen: \n", );
	//CFileListView * v = (CFileListView *)pThreadParam->m->m_mainSplitter.GetPane(0,1);

	// Get the file 
	CFileContainer file;
	pThreadParam->filelistView->GetSelectedFile(file);
	CString filePath(file.localPath + file.name);

	// Check if it is local
	// Check local file is up to date
	if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(filePath) && GetLastError()==ERROR_FILE_NOT_FOUND)
	{
		//File not found

		CWaitDialog waitDialog;
		//CWaitDialog * waitDialog = new CWaitDialog();
		
		bool r = pThreadParam->m->m_ftpManager.ReceiveFile(pThreadParam->filelistView->m_connection, file, waitDialog);
		//bool r = pThreadParam->m->m_ftpManager.ReceiveFile(pThreadParam->filelistView->m_connection, file, *waitDialog);
		
		// display wait dialog, and progress.
		
		//waitDialog->Create(IDD_WAIT_DIALOG, GetDesktopWindow());
		//waitDialog->ShowWindow(SW_SHOW);

		int waitDlgRsp = waitDialog.DoModal();
		if(waitDlgRsp == IDCANCEL){
			// cancel 

		}
		
	
		// file received, wait dialog dismissed
	}

	// Open 
	ShellExecute( // WinExec is obsolete.
         pThreadParam->m->m_hWnd,		// hwnd
         _T("open"),					// operation/verb.
         filePath,						// executable.
         _T(""),						// parameters.
         NULL,							// directory
         SW_SHOW );						// how to be displayed 

	return 1;
}

/**
* OnFileOpen
*
* Description:
*/
void CMainFrame::OnFileEdit()
{
	TRACE0("OnFileOpen: \n");
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);

	// Get the file 
	CFileContainer file;
	v->GetSelectedFile(file);
	CString filePath(file.localPath + file.name);

	// Check if it is local
	if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(filePath) && GetLastError()==ERROR_FILE_NOT_FOUND)
	{
		//File not found
		CWaitDialog waitDialog;
		bool r = m_ftpManager.ReceiveFile(v->m_connection, file, waitDialog);
		int i = 0;
		
	}

	// Check local file is up to date

	// Open 
	ShellExecute( // WinExec is obsolete.
         this->m_hWnd,    // hwnd
         _T("edit"),          // operation/verb.
         filePath,  // executable.
         _T(""),        // parameters.
         NULL,            // directory
         SW_SHOW );       // how to be displayed 
}


/**
* OnFileRename
*
*/
void CMainFrame::OnFileRename()
{
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
	v->RenameFile();
}


/**
* OnFileDelete
*
*/
void CMainFrame::OnFileDelete()
{
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
	v->DeleteFile();
}



/**
* OnNewFolder
*
*/
void CMainFrame::OnNewFolder()
{
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
	CInputDialog dialog;
	//dialog.SetWindowTextW(_T("bla")) ; // = TEXT("Folder name:");
	if (dialog.DoModal() == IDOK){
		// Do something
		CString input(dialog.m_inputText);
		v->CreateFolder(input);
		TRACE1("a %s \n", input);
	}
}



void CMainFrame::OnFolderDelete()
{
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
	v->DeleteFolder();
}


/**
* OnBookmarkSelectedFolder
*
* Description: Bookmark the selected folder 
*/
void CMainFrame::OnBookmarkSelectedFolder()
{
	CConnectionListView * connectionView = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	CFileListView * fileView = (CFileListView *)m_mainSplitter.GetPane(0,1);
	CConnectionManager connectionManager;
	connectionManager.Load();

	CConnection bookmark;
	CFileContainer folder;
	bool hasFolder = fileView->GetSelectedFolder(folder);
	if(hasFolder){
		bookmark.m_path = CString(folder.path + folder.name + _T("/"));
		bookmark.name = CString(folder.path + folder.name + _T("/"));
		bookmark.host = CString(fileView->m_connection->host);
		bookmark.user = CString(fileView->m_connection->user);
		bookmark.password = CString(fileView->m_connection->password);
	
		connectionManager.Add(bookmark);
		connectionManager.Store();
	
		// Update connection view 
		connectionView->UpdateConnections();
	} else {
		// Error
	}
}


/**
* OnFolderBookmark
*
* Description: Bookmark the folder being viewed
*/
void CMainFrame::OnFoldersBookmark()
{
	CConnectionListView * connectionView = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	CFileListView * fileView = (CFileListView *)m_mainSplitter.GetPane(0,1);
	//v->BookmarkFolder();

	CConnectionManager connectionManager;
	connectionManager.Load();

	CConnection bookmark;
	bookmark.name = CString(fileView->m_path);
	bookmark.m_path = CString(fileView->m_path);
	bookmark.host = CString(fileView->m_connection->host);
	bookmark.user = CString(fileView->m_connection->user);
	bookmark.password = CString(fileView->m_connection->password);
	
	connectionManager.Add(bookmark);
	connectionManager.Store();
	
	// Update connection view 
	connectionView->UpdateConnections();
}


/**
* OnFilePermissions
*
* Description: 
*/
void CMainFrame::OnFilePermissions()
{
	CFileListView * fileView = (CFileListView *)m_mainSplitter.GetPane(0,1);
	fileView->ChangeFilePermissions();
}


/**
* OnFileDownload
*
* Description: Tell the view view to download the selected files.
*/
void CMainFrame::OnFileDownload()
{
	CFileListView * fileView = (CFileListView *)m_mainSplitter.GetPane(0,1);
	fileView->DownloadSelection();
}


/**
* OnFileUpload
*
* Description: Tell the view view to upload the selected files.
*/
void CMainFrame::OnFileUpload()
{
	CFileListView * fileView = (CFileListView *)m_mainSplitter.GetPane(0,1);
	fileView->UploadSelection();
}


/**
* OnToggleStoreFolderLocally
*
* Description: 
*/
void CMainFrame::OnToggleFolderStoreLocally()
{
	CLocalStore stored;
	CFileListView * fileView = (CFileListView *)m_mainSplitter.GetPane(0,1);
	CFileContainer file;
	if(fileView->GetSelectedFolder(file))
	{
		stored.ToggleStore(file);
	}
}


/**
* OnMenuNavigateItem
*
* Description: navigate menu item pressed.
*/
void CMainFrame::OnMenuNavigateItem(UINT nID)
{
	CMenuNavigator menuNav;
	CString clickPath;
	menuNav.GetPathForID(nID, clickPath);
	
	TRACE1(" clicked: %s \n", clickPath);

	CConnectionListView * connectionListView = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	CFileListView * fileListView = (CFileListView *)m_mainSplitter.GetPane(0,1);
	CConnection * con;
	con = connectionListView->m_connection;
	if(con == NULL){
		con = fileListView->m_connection; // bad - when called from con list menu, view.con is null
	}
	if(con == NULL){ 
		return;
	}

	fileListView->m_connection = con;  
	fileListView->LoadDirectory(clickPath);

	// header address bar
	if(m_headerDoc){
		m_headerDoc.SetUrl( CString(con->host + clickPath));
		m_headerDoc.m_connection = connectionListView->m_connection;
		m_headerDoc.m_path = CString(_T("/"));


		CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
		m_headerDoc.m_path = CString(v->GetPath());
	}
}


/**
* LoadServerConnection
*
* Description: pass through function call from connection list to file view.
*/
void CMainFrame::LoadServerConnection(int index)
{
	int i = m_mainSplitter.GetColumnCount();
	int r = m_vertical_splitter.GetRowCount();

	CFrameWndEx * f = this;

	bool inited = m_bInitSplitter;

	if(m_mainSplitter.GetColumnCount() != 2){
		TRACE0("ERROR: MainFrame.m_mainSplitter is null.\n");

		//CSplitterWnd m_vertical_splitter;
		//CSplitterWnd * m = (CSplitterWnd*)m_vertical_splitter.GetPane(0, 0);
		//int ii = m->GetColumnCount();

		return;
	}

	
	// file list view
	CWnd * w = m_mainSplitter.GetPane(0, 1);
	//CFileView * v = (CFileView *)m_mainSplitter.GetPane(0,1); // old
	CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
	v->LoadServerConnection(index);
	

	// header address bar
	if(m_headerDoc){
		CConnectionManager conManager;
		conManager.Load();
		CConnection * con = conManager.GetAt(index);
		m_headerDoc.SetUrl(con->host + con->m_path);
		m_headerDoc.SetConnectionID(index);
		m_headerDoc.m_path = CString(_T("/"));

		CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
		m_headerDoc.m_path = CString(v->GetPath());
	}
}


/**
* DirectoryLoaded
*
* Description: called by fileView when changing directories.
*/
void CMainFrame::DirectoryLoaded(CString &url)
{
	if(m_headerDoc){
		//CConnectionManager conManager;
		//conManager.Load();
		//CConnection * con = conManager.GetAt(index);
		m_headerDoc.SetUrl(url);
	}
}

/**
* OnUpdateProgress
*
* Description: Updates transfer progress in connection view. 
*    Progress updates in the wait dialog are sent directly, not through this class.
*/
LRESULT CMainFrame::OnUpdateProgress(WPARAM wParam, LPARAM lParam)
{
	CConnectionListView * connectionView = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	//std::auto_ptr<CConnection> c(reinterpret_cast<CConnection*>(wParam));
	//CConnection * cn = (CConnection*)wParam;
	//CString name(_T(""));
	//if(cn){
	//	name = CString(cn->name);
	//} else {
	//	return 1;
	//}

	CFileTransfer * transfer = (CFileTransfer*)lParam;
	if(transfer)
	{
		{
			CFileTransfer * transferClone = new CFileTransfer(transfer);
			connectionView->UpdateProgress(transferClone);
		}
		if(m_historyDlg)
		{
			CFileTransfer * transferClone = new CFileTransfer(transfer);
			m_historyDlg->UpdateFileTransfer(transferClone);
		}
	}

	// Delete transfer
	delete transfer;



	//std::auto_ptr<CConnection>  con(reinterpret_cast<CConnection*>(wParam));
	//TRACE1(" con %s \n", con->host);
	
	//v->SetProgress((int)lParam);
	//v->SetProgress(name, (int)lParam);
	//connectionView->SetProgress(name, transfer->progress);
	
	//SetProgress( &con, (int)lParam);
	// Waiting dialog
	//m_waitDialog.SetProgress((int)lParam);

	return 1;
}

// depricate
void CMainFrame::SetProgress(CConnection & con, int progress)
{
	CConnectionListView * v = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	//v->SetProgress(con, progress);
}


// number of pending transactions
LRESULT CMainFrame::OnPendingTransfers(WPARAM wParam,LPARAM lParam)
{
	m_waitDialog.SetPendingTransfers((int)lParam);
	return 1;
}


/**
* OnUpgradeAvailable
*
* Description: called by ID_UPGRADE_AVAILABLE message when update is detected. 
*   Adds an update menu item.
*/
LRESULT CMainFrame::OnUpgradeAvailable(WPARAM wParam, LPARAM lParam)
{
	CConnectionListView * connectionView = (CConnectionListView *)m_mainSplitter.GetPane(0,0);

	// update menu
	CMenu contextMenu;
	CMenu *tracker; 
	contextMenu.LoadMenu(IDR_MAINFRAME);  

	contextMenu.AppendMenuW(MF_STRING, ID_UPGRADE_APP, _T("Upgrade Now!"));

	// Recreate menu with new items
	m_wndMenuBar.CreateFromMenu( contextMenu, false, true);

	/*
	int c = contextMenu.GetMenuItemCount();
	for(int i = 0; i < c; i++){
		CString label(_T(""));
		contextMenu.GetMenuStringW(i, label, MF_BYPOSITION);
		if(label.Compare(_T("&Help")) == 0){
			tracker = contextMenu.GetSubMenu(i);
			//int c2 = tracker->GetMenuItemCount();
			//tracker->AppendMenuW(MF_STRING, ID_UPGRADE_APP, _T("Upgrade Now!"));
			contextMenu.AppendMenuW(MF_STRING, ID_UPGRADE_APP, _T("Upgrade Now!"));
			// Recreate menu with new items
			m_wndMenuBar.CreateFromMenu( contextMenu, false, true);
		}
	}
	*/
	return 1;
}


void CMainFrame::OnUpgrade()
{
	TRACE0("Update \n");
	//DefaultWebBrowser b;
	//b.launch(CString(_T("http://realftp.net")));

	CUpgrade upgrade;
	upgrade.Upgrade();
}


void CMainFrame::OnHelpManual()
{
	TRACE0("Manual \n");
	DefaultWebBrowser b;
	b.launch(CString(_T("http://realftp.net/doc/documentation.html")));
}


/**
* OnBuyUpgrade
*
* Description: 
*/
void CMainFrame::OnBuyUpgrade()
{
	/*
	CRegistry reg;
	CString guid(_T(""));
	reg.GetGUID(guid);
	DefaultWebBrowser b;
	b.launch(CString(_T("http://realftp.net/upgrade/index.php?guid=")) + guid);
	*/
	CUpgradeDialog upgradeDlg;
	upgradeDlg.DoModal();
}


/**
* OnSupport
*
*/
void CMainFrame::OnSupport()
{ 
	DefaultWebBrowser b;
	b.launch(CString(_T("http://realftp.net/support/")));
}


LRESULT CMainFrame::ResizeFileViewForContent(WPARAM wParam,LPARAM lParam)
{
	//ResizeScrollViewForContent();
	int i = 0;
	return 0;
}


/**
* OnLoadUrl
*
* Description: Load a url into the view
*
* Parameters:	wparam: - 0 = no callback, 1 = yes callback
*				lparam: CString url path
*/
LRESULT CMainFrame::OnLoadUrl(WPARAM wParam, LPARAM lParam)
{
	CConnectionListView * connectionView = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	CFileListView * fileListView = (CFileListView *)m_mainSplitter.GetPane(0,1);

	CString * urlArg = (CString*)lParam;
	if(urlArg){
		TRACE1(" Load url: %s \n", urlArg->AllocSysString());
		CString url(urlArg->AllocSysString());
		delete urlArg;

		CString host(_T(""));
		CString dir(_T(""));
		// Parse
		if(url.Find(_T("ftp://")) != -1){
			url = url.Mid(url.Find(_T("ftp://")) + 6, url.GetLength() - url.Find(_T("ftp://")));
		}
		int pos = url.Find('/');
		if(pos != -1){
			host = url.Mid(0, pos);
			dir = url.Mid(pos, url.GetLength() - pos);
		} else {
			host = CString(url);
			dir = CString(_T("/"));
		}

		// select connection
		int connection_id = -1;
		CObArray * listOfConnections;
		listOfConnections = m_connectionManager.GetConnections();
		int i = 0;
		for(i = 0; i < listOfConnections->GetSize(); i++){
			CConnection * con = (CConnection *)listOfConnections->GetAt(i);
			if(con->host.Compare(host) == 0){
				connection_id = i;
				connectionView->SelectConnection(connection_id);
			}
		}

		// Prompt for new connection information
		if(connection_id < 0){
			CConnectionDialog dlg;
			dlg.SetHost(host); // prefill in host
			dlg.DoModal();
	
			// Update view with updated connections
			connectionView->UpdateConnections();

			// Re check host in connections
			m_connectionManager.Load();
			listOfConnections = m_connectionManager.GetConnections();
			int i = 0;
			for(i = 0; i < listOfConnections->GetSize(); i++){
				CConnection * con = (CConnection *)listOfConnections->GetAt(i);
				if(con->host.Compare(host) == 0){
					connection_id = i;
					connectionView->SelectConnection(connection_id);
				}
			}
			if(connection_id < 0){
				return 1; // No connection entered, abort
			}
		}

		// Load file list view
		fileListView->LoadServerConnection(connection_id);
		fileListView->LoadDirectory(dir);

		// Load url in header history.
		if(m_headerDoc && ((int)wParam) == 1){
			m_headerDoc.SetUrl( CString(host + dir) );
			m_headerDoc.SetConnectionID(connection_id); 
			m_headerDoc.m_path = CString(dir);          // This needs work
			CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
			m_headerDoc.m_path = CString(v->GetPath());
		}

		// Cleanup
		delete urlArg;
	}
	return 1;
}


/**
* OnLoadDir
*
* Description: 
*/
LRESULT CMainFrame::OnLoadDir(WPARAM wParam, LPARAM lParam)
{
	CConnectionListView * connectionView = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	CFileListView * fileListView = (CFileListView *)m_mainSplitter.GetPane(0,1);

	CString * dirArg = (CString*)lParam;
	if(dirArg){
		TRACE1(" Load dir: %s \n", dirArg->AllocSysString());
		CString dir(dirArg->AllocSysString());

		// Load file list view
		//fileListView->LoadServerConnection(connection_id);
		fileListView->LoadDirectory(dir);

		// Load url in header history.
		if(m_headerDoc && ((int)wParam) == 1){
			m_headerDoc.SetUrl( CString( fileListView->m_connection->host + dir) );
			//m_headerDoc.SetConnectionID(connection_id); 
			m_headerDoc.m_path = CString(dir);          // This needs work
			CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
			m_headerDoc.m_path = CString(v->GetPath());
		}

		// Cleanup
		delete dirArg;
	}
	return 1;
}


/**
* OnSelectFile
*
* Description: Load file view to dir and select file given.
*  wParam: Not used
*  lParam: CFileContainer *
*/
LRESULT CMainFrame::OnSelectFile(WPARAM wParam, LPARAM lParam)
{
	CConnectionListView * connectionView = (CConnectionListView *)m_mainSplitter.GetPane(0,0);
	CFileListView * fileListView = (CFileListView *)m_mainSplitter.GetPane(0,1);
	CFileContainer * file = (CFileContainer*)lParam;
	if(file){
		//TRACE1(" select File: %s \n", file->path );
		CString dir(file->path);

		// Load file list view
		//fileListView->LoadServerConnection(connection_id);
		fileListView->LoadDirectory( dir );
		fileListView->SelectFile( file );

		// Load url in header history.
		if(m_headerDoc && ((int)wParam) == 1){
			m_headerDoc.SetUrl( CString( fileListView->m_connection->host + dir) );
			//m_headerDoc.SetConnectionID(connection_id); 
			m_headerDoc.m_path = CString(dir);          // This needs work
			CFileListView * v = (CFileListView *)m_mainSplitter.GetPane(0,1);
			m_headerDoc.m_path = CString(v->GetPath());
		}

		// Cleanup
		delete file;
	}
	return 1;
}



LRESULT CMainFrame::OnUpgradeComplete(WPARAM wParam, LPARAM lParam)
{
	if(m_wndMenuBar){
		m_wndMenuBar.HideAds();
		//m_ad.
	}
	return 1;
}


LRESULT CMainFrame::OnExit(WPARAM wParam, LPARAM lParam)
{
	m_exit = true;
	AfxGetMainWnd()->SendMessage(WM_CLOSE);
	return 1;
}


/**
* OnEditPaste
*
* Description: Ctrl-V paste.
*	Send command to header url bar.
*	Send command to header search bar.
*/
void CMainFrame::OnEditPaste()
{
	if(m_headerDoc){
		m_headerDoc.OnEditPaste();
	}
}

void CMainFrame::OnEditCopy()
{
	if(m_headerDoc){
		m_headerDoc.OnEditCopy();
	}
}


/**
* TrayIcon
*
* Description: 
*/
void CMainFrame::LoadTrayIcon()
{
	HICON hIconKeys = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_trayIcon.CreateIcon(hIconKeys, 1, "Keys" );
	HWND hWnd = GetSafeHwnd();
	m_trayIcon.SetInstaller(hWnd);
	m_trayIcon.ShowIcon(1);
}

/**
* LoadTrayBaloon
*
* Description: Load a tray baloon. 
*    Called by DelayLoadTrayBaloonThread.
*/
void CMainFrame::LoadTrayBaloon()
{
	/*
    CString tooltip(_T("Installing "));
    tooltip = tooltip + m_XmlFeed.installerAttributes.szSoftware_name;
    m_trayIcon.ChangeIconTip(1, tooltip); 
    m_trayIcon.ShowBalloon(1, 
        tooltip.AllocSysString(), 
        _T("You can continue working while your software downloads. To view detailed progress, click the icon below."), 
        NIIF_INFO);
		*/
}


/**
* HideTrayIcon
*
*/
void CMainFrame::HideTrayIcon()
{
	m_trayIcon.HideIcon(1);
	m_trayIcon.DeleteIcon(1); // error
}


void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
    bool bRecognized = false;
    // We have to pass all system commands to the base handler. In <BR>    // ClinicBoard1, we needed to override to use the following switch <BR>    // condtion:
    //
    //        switch (wParam & 0xFFF0)
    //
    // Starting with VS2005, they're passing the actual message ID instead of 
    // a WPARAM, so we don't have to do that anymore. jms - 25JAN2007
    switch (nID)
    {
        case SC_CLOSE:
        case SC_HOTKEY:
        case SC_HSCROLL:
        case SC_KEYMENU:
        case SC_MAXIMIZE:
        case SC_MINIMIZE:
        case SC_MOUSEMENU:
        case SC_MOVE:
        case SC_NEXTWINDOW:
        case SC_PREVWINDOW:
        case SC_RESTORE:
        case SC_SCREENSAVE:
        case SC_SIZE:
        case SC_TASKLIST:
        case SC_VSCROLL:
        case 61587: // show the system menu
        case 61443: // allow window resizing by click/dragging the main window <BR>                    // borders
/*
        // no longer supported in VS2005/MFC8?
        case SC_ARRANGE:
    #if(WINVER >= 0x0400)
        case SC_DEFAULT:
        case SC_MONITORPOWER:
        case SC_CONTEXTHELP:
        case SC_SEPARATOR:
    #endif
*/
            CFrameWndEx::OnSysCommand(nID, lParam);
            return;
    }

    switch (nID)
    {
        //case ID_SHOW_MESSAGE1 :
        case ID_UPGRADE_APP :
            PostMessage(WM_COMMAND, nID, 0);
            break;
    }
}


/**
* OnAdChanged
* 
*/
LRESULT CMainFrame::OnAdChanged(WPARAM wParam, LPARAM lParam)
{
	CAdvertisement * ad = (CAdvertisement*)lParam;
	if(ad)
	{
		if(m_wndMenuBar){
			m_wndMenuBar.OnAdChanged(0, (LPARAM)lParam);
		}	
	}
	return 1;
}


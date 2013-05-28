
// RealFTP.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "RealFTP.h"
#include "MainFrm.h"

#include "ConnectionListDoc.h"
#include "ConnectionListView.h"
#include "afxwin.h"
#include "Version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRealFTPApp

BEGIN_MESSAGE_MAP(CRealFTPApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CRealFTPApp::OnAppAbout)
END_MESSAGE_MAP()


// CRealFTPApp construction

CRealFTPApp::CRealFTPApp()
{
	m_bHiColorIcons = TRUE;

	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("RealFTP.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CRealFTPApp object

CRealFTPApp theApp;

UINT g_uCustomClipbrdFormat = RegisterClipboardFormat ( _T("MultiFiler_3BCFE9D1_6D61_4cb6_9D0B_3BB3F643CA82") );
bool g_bNT = (0 == (GetVersion() & 0x80000000) );

// CRealFTPApp initialization

BOOL CRealFTPApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;
	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);






	// The one and only window has been initialized, so show and update it
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}

int CRealFTPApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	AfxOleTerm(FALSE);

	return CWinAppEx::ExitInstance();
}

// CRealFTPApp message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };
	
	virtual  BOOL OnInitDialog( );
	afx_msg BOOL OnEraseBkgnd(CDC * pDC);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()

	CBitmap Background;
	CSize bitmapSize;
public:
	//afx_msg void OnConnectionDelete();
	CStatic m_version;
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VERSION, m_version);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	//ON_COMMAND(ID_CONNECTION_DELETE, &CAboutDlg::OnConnectionDelete)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


BOOL CAboutDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    //SetIcon(m_hIcon, TRUE);
    //SetIcon(m_hIcon, FALSE);
    ShowWindow(SW_SHOWNORMAL);
    //Load bitmap From Resource, Get bitmap Size
	Background.LoadBitmapW(IDB_ABOUT_BITMAP); //Load bitmap
    BITMAP bm; //Create bitmap Handle to get dimensions
	Background.GetBitmap(&bm); //Load bitmap into handle
    bitmapSize = CSize(bm.bmWidth, bm.bmHeight); // Get bitmap Sizes;

	
	CString cs(_T(""));
	//cs.Format("Version: %s", CGlobalFunctions::GetFileVersionX());
	CGlobalFunctions::GetFileVersionX(cs);
	cs = CString(_T("Version ")) + cs;

	//CGlobalFunctions::GetVersionInfo(NULL, "FileVersion", cs);
	//cs = cs + CString();

	m_version.SetWindowTextW(cs);

	CenterWindow();

    Invalidate(1); 
    //Force the dialog to repaint itself;
    //End bitmap Information
    return TRUE; // return TRUE unless you set the focus to a control
}

BOOL CAboutDlg::OnEraseBkgnd(CDC* pDC)
{
    CDC dcMemory;
    dcMemory.CreateCompatibleDC(pDC);
    CBitmap* pOldbitmap = dcMemory.SelectObject(&Background);
    CRect rcClient;
    GetClientRect(&rcClient);
    const CSize& sbitmap = bitmapSize;
    pDC->BitBlt(0,0,sbitmap.cx,sbitmap.cy,&dcMemory,0,0,SRCCOPY);
    dcMemory.SelectObject(pOldbitmap);
    return TRUE;
    //return CDialog::OnEraseBkgnd(pDC); Remove and return TRUE
}

/*
void CAboutDlg::OnDestroy()
{
    CDialog::OnDestroy();
    Background.DeleteObject(); // Delete Background bitmap
    //BrushHol.DeleteObject(); 
    // Delete Brush
}
*/

// App command to run the dialog
void CRealFTPApp::OnAppAbout()
{
	CAboutDlg aboutDlg;

	aboutDlg.DoModal();
}

// CRealFTPApp customization load/save methods

void CRealFTPApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CRealFTPApp::LoadCustomState()
{
}

void CRealFTPApp::SaveCustomState()
{
}

// CRealFTPApp message handlers



// ConnectionDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RealFTP.h"
#include "ConnectionDialog.h"
#include "afxdialogex.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "resource.h"

// CConnectionDialog dialog

IMPLEMENT_DYNAMIC(CConnectionDialog, CDialogEx)

CConnectionDialog::CConnectionDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CConnectionDialog::IDD, pParent)
{
	m_connectionIndex = -1;
	m_initalHost = CString(_T(""));
}

CConnectionDialog::~CConnectionDialog()
{
}

void CConnectionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TITLE_EDIT, m_connectionName);
	DDX_Control(pDX, IDC_HOST_EDIT, m_host);
	DDX_Control(pDX, IDC_USER_NAME_EDIT, m_userName);
	DDX_Control(pDX, IDC_PASSWORD_EDIT, m_password);
}


BEGIN_MESSAGE_MAP(CConnectionDialog, CDialogEx)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDOK, &CConnectionDialog::OnBnClickedOk)
	ON_STN_CLICKED(IDC_CON_STATUS, &CConnectionDialog::OnStnClickedConStatus)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CConnectionDialog message handlers


BOOL CConnectionDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Load values if editing existing connection
	if(m_connectionIndex != -1){
		CConnectionManager connections;
		connections.Load();
		CConnection * con = connections.GetAt(m_connectionIndex);
		//TRACE1(" name %s", con->name);
		m_connectionName.SetWindowText(con->name);
		m_host.SetWindowText(con->host);
		m_userName.SetWindowText(con->user);
		m_password.SetWindowText(con->password);
	}

	if(m_initalHost.GetLength() > 0){
		m_host.SetWindowText(m_initalHost); 
		m_connectionName.SetWindowText(m_initalHost);
	}

	// IDC_CONNECTION_TITLE
	//SetBkColor(COLORREF crColor)
	
	//CStatic * title = (CStatic*)GetDlgItem(IDC_CONNECTION_TITLE);
	//title->SetW SetBkColor(RGB(255,255,255))

	return true;
}


BOOL CConnectionDialog::OnEraseBkgnd(CDC* pDC)
{
    CDC dcMemory;
    dcMemory.CreateCompatibleDC(pDC);
    
	CRect rect;
	GetClientRect( &rect );

	// back
	CBrush bg;
	bg.CreateStockObject(RGB(240,240,240));
	CBrush fillBrush(RGB(240, 240, 240));
	CBrush *oldFillBrush = pDC->SelectObject(&fillBrush);
	pDC->FillRect(&rect, &bg);
	
	// Top Bar
	CPoint background_fill_points[4];
	background_fill_points[0] = CPoint(0, 0);
	background_fill_points[1] = CPoint(0, 50);
	background_fill_points[2] = CPoint(rect.right, 50 );
	background_fill_points[3] = CPoint(rect.right, 0);
	CBrush backBrush(RGB(255, 255, 255));
	CBrush *oldBackBrush = pDC->SelectObject(&backBrush);
	CPen penBack(PS_NULL,0,RGB(250,0,0));
	pDC->SelectObject(&penBack);
	pDC->Polygon(background_fill_points, 4);


	// Title
	CString title(_T("FTP Connection Information"));
	CFont * fntTitle1 = new CFont;
	fntTitle1->CreateFont(16 /*height*/, 8 /*width*/, 0, 0,
			      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN,
				_T("Arial"));
    //pDC->SetTextAlign( TA_BASELINE | TA_LEFT );
	COLORREF oldTextColor = pDC->SetTextColor(RGB(60, 60, 60));
	CFont *fntOld = pDC->SelectObject(fntTitle1);
	pDC->TextOut( 26, 17, title.AllocSysString(), title.GetLength() );

	


	// Botom Bar
	CPoint bottom_fill_points[4];
	bottom_fill_points[0] = CPoint(-1, rect.bottom );
	bottom_fill_points[1] = CPoint(-1, rect.bottom - 50);
	bottom_fill_points[2] = CPoint(rect.right, rect.bottom - 50 );
	bottom_fill_points[3] = CPoint(rect.right, rect.bottom );
	CBrush bottomBrush(RGB(230, 230, 230));
	pDC->SelectObject(&bottomBrush);
	CPen bottomPenBack(PS_NULL,0,RGB(230,230,230));
	pDC->SelectObject(&bottomPenBack);
	pDC->Polygon(bottom_fill_points, 4);

	pDC->SelectObject(&oldBackBrush);
	
	return TRUE;
    //return CDialog::OnEraseBkgnd(pDC); //Remove and return TRUE
}

// Depricate this
int CConnectionDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}


void CConnectionDialog::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here

	// Save the values
	CString title;
	m_connectionName.GetWindowText(title);
	
	CString host;
	m_host.GetWindowText(host);

	CString user;
	m_userName.GetWindowText(user);

	CString password;
	m_password.GetWindowText(password);

	CConnection con;
	con.name = CString(title);
	con.host = CString(host);
	con.user = CString(user);
	con.password = CString(password);
	
	
	if(m_connectionIndex == -1){
		// New Connection
		CConnectionManager connections;
		connections.Load();
		connections.Add(con);
		connections.Store();
	} else {
		// Edit connection
		CConnectionManager connections;
		connections.Load();
		connections.Update(m_connectionIndex, con);
		connections.Store();
	}

	CDialogEx::OnOK();
}


// Depricated - replaced with SetConnectionIndex()
void CConnectionDialog::SetConnection(CConnection &con)
{
	//m_connection = con;
}

void CConnectionDialog::SetConnectionIndex(int i)
{
	m_connectionIndex = i;
}




void CConnectionDialog::OnStnClickedConStatus()
{
	// TODO: Add your control notification handler code here
}


void CConnectionDialog::SetHost(CString & host)
{
	m_initalHost = CString(host);
}
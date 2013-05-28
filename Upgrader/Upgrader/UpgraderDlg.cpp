
// UpgraderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Upgrader.h"
#include "UpgraderDlg.h"
#include "afxdialogex.h"
#include "DownloadCallback.h"
#include "DownloadManager.h"
#include "RunCommand.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CUpgraderDlg dialog

#define    WM_USER_ENDDOWNLOAD           (WM_USER + 1)
#define    WM_USER_DISPLAYSTATUS		 (WM_USER + 2)

CUpgraderDlg::CUpgraderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CUpgraderDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUpgraderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress_bar);
}

BEGIN_MESSAGE_MAP(CUpgraderDlg, CDialogEx)
	ON_MESSAGE(WM_USER_ENDDOWNLOAD, OnEndDownload)
	ON_MESSAGE(WM_USER_DISPLAYSTATUS, OnDisplayStatus)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// CUpgraderDlg message handlers

BOOL CUpgraderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here


	m_downloadParam.hWnd = this->GetSafeHwnd();
	m_eventStop.ResetEvent();  // nonsignaled
	m_downloadParam.hEventStop = m_eventStop;
	m_downloadParam.strFileName.Empty();
	m_downloadParam._this = this;
	//m_downloadParam.s

	m_pDownloadThread = AfxBeginThread(RunDownloadThread, &m_downloadParam);
	if (m_pDownloadThread == NULL)
	{	
		//l.LogEvent(m_launcher_id, /*type*/CString(_T("109")), /*offer_id*/CString(_T(" ")), /*data*/CString(_T(" ")), /*ret*/CString(_T("")), /*level*/l.DONT_CHECK);
	}


	return TRUE;  // return TRUE  unless you set the focus to a control
}


UINT CUpgraderDlg::RunDownloadThread(LPVOID pParam)
{
	THREADPARAM* ts = (THREADPARAM*)pParam;

	DownloadCallback callback(ts->hWnd, ts->hEventStop, CString(_T("")));
	DownloadManager dm;
	dm.m_dnldCallback = &callback;



	bool downloadedOk = false;
	int downloadTries = 1;
	while(!downloadedOk && downloadTries > 0){
		
		// Download with WinINet
		if(dm.DownloadFile(CString(_T("http://realftp.net/RealFTP.exe")), ts->strFileName, CString(_T("")))){ // Temp
			downloadedOk = true;
		}
		downloadTries--;
	}

	VERIFY(::PostMessage(ts->hWnd, WM_USER_ENDDOWNLOAD, 0, 0));

	return 1;
}

LRESULT CUpgraderDlg::OnDisplayStatus(WPARAM, LPARAM lParam)
{
	const DOWNLOADSTATUS *const pDownloadStatus = reinterpret_cast<DOWNLOADSTATUS *>(lParam);
	
	
	TRACE1(" download status  %d \n", pDownloadStatus->ulProgress);
	
	m_progress_bar.SetPos(pDownloadStatus->ulProgress);

	return 1;
}

LRESULT CUpgraderDlg::OnEndDownload(WPARAM, LPARAM)
{
	//ASSERT_POINTER(m_pDownloadThread, CWinThread);
	//ASSERT_KINDOF(CWinThread, m_pDownloadThread);
	
	TRACE(_T("OnEndDownload()\n"));


	// RUN IT

	TCHAR programPath[_MAX_PATH];
	CString path(_T(""));
	if(::GetModuleFileName(NULL, programPath, _MAX_PATH))
	{
		path = CString(programPath);
		if(path.ReverseFind('\\') != -1)
		{
			
			path = path.Mid(0, path.ReverseFind('\\') + 1);
			path = path + CString(_T("RealFTP.exe"));
			

			RunCommand cmd;
			DWORD retCode;
			CString error(_T(""));
			if(cmd.runAndForget( path )){
			
				//
				PostMessageW(WM_CLOSE, 0, 0);

			}
		}
	}


	return 1;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CUpgraderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CUpgraderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


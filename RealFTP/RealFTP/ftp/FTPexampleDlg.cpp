#include "stdafx.h"
#include "FTPexample.h"
#include "FTPexampleDlg.h"
#include "FTPBrowseForFileAndFolder.h"
#include "FTPLogonInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CFTPexampleDlg::CFTPexampleDlg(CWnd* pParent /*=NULL*/)
   : CDialog(CFTPexampleDlg::IDD, pParent), m_FTPClient(nsSocket::CreateDefaultBlockingSocketInstance(), 30)
{
   m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CFTPexampleDlg::~CFTPexampleDlg()
{
}

void CFTPexampleDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_RE_PROTOCOL, m_ProtocolOutput);
}

BEGIN_MESSAGE_MAP(CFTPexampleDlg, CDialog)
   ON_WM_PAINT()
   ON_WM_QUERYDRAGICON()
   ON_BN_CLICKED(IDC_BTN_BROWSE, OnBnClickedBtnBrowse)
   ON_BN_CLICKED(IDC_BTN_LOGON_INFO, OnBnClickedBtnLogonInfo)
END_MESSAGE_MAP()

BOOL CFTPexampleDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   SetIcon(m_hIcon, TRUE);
   SetIcon(m_hIcon, FALSE);

   return TRUE;
}

void CFTPexampleDlg::OnPaint() 
{
   if (IsIconic())
   {
      CPaintDC dc(this);

      SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

      int cxIcon = GetSystemMetrics(SM_CXICON);
      int cyIcon = GetSystemMetrics(SM_CYICON);
      CRect rect;
      GetClientRect(&rect);
      int x = (rect.Width() - cxIcon + 1) / 2;
      int y = (rect.Height() - cyIcon + 1) / 2;

      dc.DrawIcon(x, y, m_hIcon);
   }
   else
   {
      CDialog::OnPaint();
   }
}

HCURSOR CFTPexampleDlg::OnQueryDragIcon()
{
   return static_cast<HCURSOR>(m_hIcon);
}

void CFTPexampleDlg::OnBnClickedBtnBrowse()
{
   nsFTP::nsView::CFTPBrowseForFileAndFolderDlg ftpDlg;
   m_FTPClient.AttachObserver(&m_ProtocolOutput);

   m_FTPClient.Login(m_LogonInfo);

   if( ftpDlg.DoModal(&m_FTPClient, true)==IDOK )
   {
      CFileDialog dlgLocalFile(FALSE, NULL, NULL, OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT);
      if( dlgLocalFile.DoModal()==IDOK )
      {
         nsFTP::nsView::CFTPProgressDlg dlg(this);
         dlg.Create(IDD_FTP_DLG_PROGRESS);
         dlg.ShowWindow(SW_SHOW);
         dlg.AttachObserver(this);

         m_FTPClient.SetResumeMode(false);
         m_FTPClient.AttachObserver(&dlg);
         m_FTPClient.DownloadFile(tstring(ftpDlg.GetFullPath()), tstring(dlgLocalFile.GetPathName()));
         m_FTPClient.DetachObserver(&dlg);
      }
   }

   nsFTP::nsView::CFTPProgressDlg dlg(this);
   dlg.Create(IDD_FTP_DLG_PROGRESS);
   dlg.ShowWindow(SW_SHOW);
   dlg.AttachObserver(this);

   //nsFTP::TSpFTPFileStatusVector list;
   //m_FTPClient.List("/", list);

   //for( nsFTP::TSpFTPFileStatusVector::iterator it=list.begin(); it!=list.end(); ++it )
   //   TRACE("\n%s", (*it)->Name().c_str());
   //m_FTPClient.Logout();

   m_FTPClient.Logout();
}

void CFTPexampleDlg::OnBnClickedBtnLogonInfo()
{
   nsFTP::nsView::CFTPLogonInfoDlg FTPLogonInfoDlg;
   FTPLogonInfoDlg.SetLogonInfo(m_LogonInfo);
   if( FTPLogonInfoDlg.DoModal()==IDOK )
      FTPLogonInfoDlg.GetLogonInfo(m_LogonInfo);
}

void CFTPexampleDlg::OnAbort()
{
   m_FTPClient.Abort();
}

void CFTPexampleDlg::OnCancel()
{
   m_FTPClient.Logout();
   CDialog::OnCancel();
}

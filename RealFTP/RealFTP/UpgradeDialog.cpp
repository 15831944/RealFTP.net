// UpgradeDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RealFTP.h"
#include "UpgradeDialog.h"
#include "afxdialogex.h"
#include "Registry.h"
#include "WebGrab.h"
#include "DefaultWebBrowser.h"

// CUpgradeDialog dialog

IMPLEMENT_DYNAMIC(CUpgradeDialog, CDialogEx)

CUpgradeDialog::CUpgradeDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CUpgradeDialog::IDD, pParent)
{
	m_running = true;
	m_activationCode = CString(_T(""));
}

CUpgradeDialog::~CUpgradeDialog()
{
}

void CUpgradeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_EDIT1, m_ActivationCode);
	DDX_Control(pDX, IDC_ACTIVATION_CODE, m_activationCodeEdit);
}


BEGIN_MESSAGE_MAP(CUpgradeDialog, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &CUpgradeDialog::OnBnClickedCancel)
	ON_MESSAGE(ID_UPGRADE_COMPLETE, &CUpgradeDialog::OnUpgradeComplete)
	ON_BN_CLICKED(IDC_OK_BUTTON, &CUpgradeDialog::OnBnClickedOkButton)
	ON_EN_CHANGE(IDC_ACTIVATION_CODE, &CUpgradeDialog::OnEnChangeActivationCode)
END_MESSAGE_MAP()


// CUpgradeDialog message handlers

BOOL CUpgradeDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CRect rect;
	GetClientRect( &rect );

	// Set Ok to active button
	static_cast<CButton*>(GetDlgItem(IDC_OK_BUTTON))->SetButtonStyle(BS_DEFPUSHBUTTON);


	CRegistry reg;
	CString guid(_T(""));
	reg.GetGUID(guid);
	if(!reg.IsPurchased()){
		DefaultWebBrowser b;
		b.launch(CString(_T("http://realftp.net/upgrade/index.php?guid=")) + guid);
	
		//THREADPARAM * _param = new THREADPARAM;
		//_param->m = this;
		AfxBeginThread(CUpgradeDialog::CheckWorker, this, THREAD_PRIORITY_ABOVE_NORMAL );
		//m_checkThread = AfxBeginThread(CUpgradeDialog::CheckWorker, &_param, THREAD_PRIORITY_NORMAL); 
		//if (m_checkThread != NULL)
		//{
			//TRACE(_T("AfxBeginThread: 0x%08lX\n"), m_pFtpSyncThread->m_nThreadID);
		//}

	} else {
	
		OnOK();

		AfxMessageBox(_T("You are upgraded. Thanks!"));
	}

	return TRUE;
}


/**
* CheckWorker
*
* Description: CheckWorker
*/
UINT CUpgradeDialog::CheckWorker( LPVOID pParam )
{
	if(pParam == NULL)
        AfxEndThread(1, TRUE);

	//CUpgradeDialog::THREADPARAM *const pThreadParam = static_cast<CUpgradeDialog::THREADPARAM *>(pParam);
	//CUpgradeDialog::THREADPARAM * pThreadParam = (CUpgradeDialog::THREADPARAM * )pParam;

	CUpgradeDialog* m = (CUpgradeDialog *)pParam;

	bool running = true;
	CWebGrab grab;
	CRegistry registry;
	CString guid(_T(""));
	registry.GetGUID(guid);

	while(running){
		TRACE1("   Activation check %s \n", guid);
		//pThreadParam->m->m_ftpManager.SyncServerConnections();
		running = m->m_running;
		
		CString webUrl(_T("http://realftp.net/upgrade/getcode.php?guid=") + guid);
		CString strBuffer;
		if (grab.GetFile(webUrl, strBuffer, _T("RealFTP"), NULL)) {
			if(strBuffer.GetLength() > 0){
				registry.SetUpgradeKey(strBuffer);
				registry.Purchase();
				::PostMessage(m->GetSafeHwnd(), ID_UPGRADE_COMPLETE, 0, (LPARAM)0); // input dialog
				::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), ID_UPGRADE_COMPLETE, 0, (LPARAM)0); // main window
				running = false;
			}
		}

		
		//running = pThreadParam->m->m_running;
		//GetParent();

		Sleep(1000 * 5); // 
	}
	return 0;
}


void CUpgradeDialog::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();

	//m_checkThread->SuspendThread();
	//m_checkThread->ExitInstance();

	m_running = false;
}


LRESULT CUpgradeDialog::OnUpgradeComplete(WPARAM wParam, LPARAM lParam)
{
	m_running = false;
	OnCancel();
	AfxMessageBox(_T("You are now upgraded. Thanks!"));
	return 1;
}

void CUpgradeDialog::OnBnClickedOkButton()
{
	// TODO: Add your control notification handler code here
	m_running = false;

	TRACE0("OK \n");
	
	// Check with the server to see if this key is present.
	// If it is set paid.
	CWebGrab grab;
	CString activation_code(_T(""));
	CRegistry registry;
	CString guid(_T(""));
	registry.GetGUID(guid);
	//static_cast<CEdit*>(GetDlgItem(IDC_ACTIVATION_CODE))->

	bool validCode = false;
	
	CString webUrl(_T("http://realftp.net/upgrade/check.php?code=") + m_activationCode + _T("&guid=") + guid);
	CString strBuffer;
	if (grab.GetFile(webUrl, strBuffer, _T("RealFTP"), NULL)) {
		if(strBuffer.GetLength() > 0 && strBuffer.Compare(_T("true")) == 0){
			validCode = true;
		}
	}
	
	if(validCode){
		this->OnOK();
		AfxMessageBox(_T("You are now upgraded. Thanks!"));
		registry.Purchase();
		registry.SetUpgradeKey(m_activationCode);
	} else {
		AfxMessageBox(_T("Sorry the code you entered was not valid."));
	}
}


void CUpgradeDialog::OnEnChangeActivationCode()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	m_activationCodeEdit.GetWindowText(m_activationCode);
}

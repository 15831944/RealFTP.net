// TermsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RealFTP.h"
#include "TermsDialog.h"
#include "afxdialogex.h"


// CTermsDialog dialog

IMPLEMENT_DYNAMIC(CTermsDialog, CDialogEx)

CTermsDialog::CTermsDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTermsDialog::IDD, pParent)
{
	// IDC_TERMS_EDIT
	m_agreed = false;
}

CTermsDialog::~CTermsDialog()
{
}

void CTermsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TERMS_EDIT, m_terms);
	DDX_Control(pDX, IDC_AGREE_CHECK, m_agree);
}


BEGIN_MESSAGE_MAP(CTermsDialog, CDialogEx)
	ON_BN_CLICKED(IDC_AGREE_CHECK, &CTermsDialog::OnBnClickedAgreeCheck)
END_MESSAGE_MAP()


// CTermsDialog message handlers
BOOL CTermsDialog::OnInitDialog() 
{
   CDialog::OnInitDialog();
   
   char * terms = "Terms and Conditions of RealFTP Software \r\n\r\n" 
				"You must agree and accepts these terms and conditions before you may use this software.\r\n\r\n"
				
	"END-USER LICENSE AGREEMENT FOR RealFTP IMPORTANT PLEASE READ THE TERMS AND CONDITIONS OF THIS LICENSE AGREEMENT CAREFULLY BEFORE CONTINUING WITH THIS PROGRAM INSTALL: Subject Reality Software End-User License Agreement (\"EULA\") is a legal agreement between you (either an individual or a single entity) and Subject Reality Software. for the Subject Reality Software RealFTP software application(s) identified above which may include associated software components, media, printed materials, and \"online\" or electronic documentation (\"RealFTP software application\"). By installing, copying, or otherwise using the RealFTP software application, you agree to be bound by the terms of this EULA. This license agreement represents the entire agreement concerning the program between you and Subject Reality Software, (referred to as \"licenser\"), and it supersedes any prior proposal, representation, or understanding between the parties. If you do not agree to the terms of this EULA, do not install or use the RealFTP software application.\r\n"
				
	"The RealFTP software application is protected by copyright laws and international copyright treaties, as well as other intellectual property laws and treaties. The RealFTP software application is licensed, not sold.\r\n\r\n"
				
	"1. GRANT OF LICENSE. \r\n"
	"The RealFTP software application is licensed as follows: \r\n"
	"(a) Installation and Use.\r\n"
	"Subject Reality Software grants you the right to install and use copies of the RealFTP software application on your computer running a validly licensed copy of the operating system for which the RealFTP software application was designed [e.g., Microsoft Windows.]. \r\n"
	"(b) Backup Copies.\r\n"
	"You may also make copies of the RealFTP software application as may be necessary for backup and archival purposes.\r\n"
	"\r\n"
	"2. DESCRIPTION OF OTHER RIGHTS AND LIMITATIONS.\r\n"
	"(a) Maintenance of Copyright Notices.\r\n"
	"You must not remove or alter any copyright notices on any and all copies of the RealFTP software application.\r\n"
	"(b) Distribution.\r\n"
	"You may not distribute registered copies of the RealFTP software application to third parties. Evaluation versions available for download from Subject Reality Software's websites may be freely distributed.\r\n"
	"(c) Prohibition on Reverse Engineering, Decompilation, and Disassembly.\r\n"
	"You may not reverse engineer, decompile, or disassemble the RealFTP software application, except and only to the extent that such activity is expressly permitted by applicable law notwithstanding this limitation. \r\n"
	"(d) Rental.\r\n"
	"You may not rent, lease, or lend the RealFTP software application.\r\n"
	"(e) Support Services.\r\n"
	"Subject Reality Software may provide you with support services related to the RealFTP software application (\"Support Services\"). Any supplemental software code provided to you as part of the Support Services shall be considered part of the RealFTP software application and subject to the terms and conditions of this EULA. \r\n"
	"(f) Compliance with Applicable Laws.\r\n"
	"You must comply with all applicable laws regarding use of the RealFTP software application.\r\n"
	"\r\n"
	"3. TERMINATION \r\n"
	"Without prejudice to any other rights, Subject Reality Software may terminate this EULA if you fail to comply with the terms and conditions of this EULA. In such event, you must destroy all copies of the RealFTP software application in your possession.\r\n"
	"\r\n"
	"4. COPYRIGHT\r\n"
	"All title, including but not limited to copyrights, in and to the RealFTP software application and any copies thereof are owned by Subject Reality Software or its suppliers. All title and intellectual property rights in and to the content which may be accessed through use of the RealFTP software application is the property of the respective content owner and may be protected by applicable copyright or other intellectual property laws and treaties. This EULA grants you no rights to use such content. All rights not expressly granted are reserved by Subject Reality Software.\r\n"
	"\r\n"
	"5. NO WARRANTIES\r\n"
	"Subject Reality Software expressly disclaims any warranty for the RealFTP software application. The RealFTP software application is provided 'As Is' without any express or implied warranty of any kind, including but not limited to any warranties of merchantability, noninfringement, or fitness of a particular purpose. Subject Reality Software does not warrant or assume responsibility for the accuracy or completeness of any information, text, graphics, links or other items contained within the RealFTP software application. Subject Reality Software makes no warranties respecting any harm that may be caused by the transmission of a computer virus, worm, time bomb, logic bomb, or other such computer program. Subject Reality Software further expressly disclaims any warranty or representation to Authorized Users or to any third party.\r\n"
	"\r\n"
	"6. LIMITATION OF LIABILITY\r\n"
	"In no event shall Subject Reality Software be liable for any damages (including, without limitation, lost profits, business interruption, or lost information) rising out of 'Authorized Users' use of or inability to use the RealFTP software application, even if Subject Reality Software has been advised of the possibility of such damages. In no event will Subject Reality Software be liable for loss of data or for indirect, special, incidental, consequential (including lost profit), or other damages based in contract, tort or otherwise. Subject Reality Software shall have no liability with respect to the content of the RealFTP software application or any part thereof, including but not limited to errors or omissions contained therein, libel, infringements of rights of publicity, privacy, trademark rights, business interruption, personal injury, loss of privacy, moral rights or the disclosure of confidential information.\r\n";
  
   m_terms.SetWindowText(CString(terms));
  
   return TRUE;   // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


/**
* DiscloseTerms
*
* Description: Display terms and conditions.
*
*/
void CTermsDialog::DiscloseTerms()
{
	CTermsDialog termsDialog;
	// Read file
	CFile fleCars;
    CFileFind fndFile;
	CString fileName(_T(".acceptedrealftpterms"));
	BOOL exists = fndFile.FindFile(fileName);
    if( exists == FALSE )
    {
		int r = termsDialog.DoModal();
		if(r == IDOK) 
		{ 
			if(!termsDialog.m_agreed){
				AfxMessageBox(_T("You must agree to the terms before you may use this software."));
				DiscloseTerms();
				return;
			}

			// Wite file
			CFile fleCars;
			fleCars.Open(fileName, CFile::modeCreate | CFile::modeWrite);
			fleCars.Write(_T("1"), 1);
			fleCars.Close();
		}
		if(r == IDCANCEL) 
		{ 
			// Exit
			//ASSERT(AfxGetMainWnd() != NULL);
			AfxGetMainWnd()->SendMessage(WM_CLOSE);
		}
	}
}

void CTermsDialog::OnBnClickedAgreeCheck()
{
	m_agreed = !m_agreed;
}

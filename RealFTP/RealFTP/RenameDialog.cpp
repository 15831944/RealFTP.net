// Rename.cpp : implementation file
//

#include "stdafx.h"
#include "RealFTP.h"
#include "RenameDialog.h"
#include "afxdialogex.h"


// CRenameDialog dialog

IMPLEMENT_DYNAMIC(CRenameDialog, CDialogEx)

CRenameDialog::CRenameDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRenameDialog::IDD, pParent)
{
	//CDialog::OnInitDialog();

	m_name = CString();
}

CRenameDialog::~CRenameDialog()
{
}

void CRenameDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RENAME_EDIT, m_renameEdit);
}


BEGIN_MESSAGE_MAP(CRenameDialog, CDialogEx)
	ON_EN_CHANGE(IDC_RENAME_EDIT, &CRenameDialog::OnChangeEdit)
END_MESSAGE_MAP()


BOOL CRenameDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if(m_renameEdit){
		m_renameEdit.SetWindowTextW(m_name);
	}
	//CRenameDialog x = this;

	return true;
}

// CRename message handlers
void CRenameDialog::SetName(CString & name)
{
	m_name = CString(name);

	/*CWnd* e = ::GetDlgItem(IDC_RENAME_EDIT);
	if(e){
	//CWnd * e = (CWnd*)GetDlgItem(IDC_RENAME_EDIT); 
	e->SetWindowTextW(m_name);
	}*/

	if(m_renameEdit){
		m_renameEdit.SetWindowTextW(m_name);
	}

}
	
void CRenameDialog::GetName(CString & name)
{
	//CRenameDialog x = this;
	
	//CWnd* e = GetDlgItem(IDC_RENAME_EDIT);
	//e->GetWindowText(name);

	if(m_renameEdit){
		m_renameEdit.GetWindowText(m_name);
	}

	name = CString(m_name);
}


void CRenameDialog::OnChangeEdit()
{
	m_renameEdit.GetWindowText(m_name);
}
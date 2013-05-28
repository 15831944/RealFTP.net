// InputDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RealFTP.h"
#include "InputDialog.h"
#include "afxdialogex.h"


// CInputDialog dialog

IMPLEMENT_DYNAMIC(CInputDialog, CDialogEx)

CInputDialog::CInputDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CInputDialog::IDD, pParent)
{
	m_inputText = CString(_T("New Folder"));
}

CInputDialog::~CInputDialog()
{
}

void CInputDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_INPUT_EDIT, m_input);
}


BEGIN_MESSAGE_MAP(CInputDialog, CDialogEx)
	ON_EN_CHANGE(IDC_INPUT_EDIT, &CInputDialog::OnEnChangeEdit1)
END_MESSAGE_MAP()


// CInputDialog message handlers


void CInputDialog::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here

	// IDC_INPUT_EDIT
	m_input.GetWindowText(m_inputText);

		
}

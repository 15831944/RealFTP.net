// ExitDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RealFTP.h"
#include "ExitDialog.h"
#include "afxdialogex.h"


// CExitDialog dialog

IMPLEMENT_DYNAMIC(CExitDialog, CDialogEx)

CExitDialog::CExitDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CExitDialog::IDD, pParent)
{

}

CExitDialog::~CExitDialog()
{
}

void CExitDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CExitDialog, CDialogEx)
END_MESSAGE_MAP()


// CExitDialog message handlers

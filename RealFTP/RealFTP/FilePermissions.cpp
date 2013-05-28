// FilePermissions.cpp : implementation file
//

#include "stdafx.h"
#include "RealFTP.h"
#include "FilePermissions.h"
#include "afxdialogex.h"


// CFilePermissions dialog

IMPLEMENT_DYNAMIC(CFilePermissions, CDialogEx)

CFilePermissions::CFilePermissions(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFilePermissions::IDD, pParent)
{
	m_owner_read = false;
	m_owner_write = false;
	m_owner_execute = false;
	m_group_read = false;
	m_group_write = false;
	m_group_execute = false;
	m_public_read = false;
	m_public_write = false;
	m_public_execute = false;
}

CFilePermissions::~CFilePermissions()
{
}

void CFilePermissions::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OWNER_READ_CHECK, m_owner_read_check);
	DDX_Control(pDX, IDC_OWNER_WRITE_CHECK, m_owner_write_check);
	DDX_Control(pDX, IDC_OWNER_EXECUTE_CHECK, m_owner_execute_check);
	DDX_Control(pDX, IDC_GROUP_READ_CHECK, m_group_read_check);
	DDX_Control(pDX, IDC_GROUP_WRITE_CHECK, m_group_write_check);
	DDX_Control(pDX, IDC_GROUP_EXECUTE_CHECK, m_group_execute_check);
	DDX_Control(pDX, IDC_PUBLIC_READ_CHECK, m_public_read_check);
	DDX_Control(pDX, IDC_PUBLIC_WRITE_CHECK, m_public_write_check);
	DDX_Control(pDX, IDC_PUBLIC_EXECUTE_CHECK, m_public_execute_check);
}


BEGIN_MESSAGE_MAP(CFilePermissions, CDialogEx)
	ON_BN_CLICKED(IDC_OWNER_READ_CHECK, &CFilePermissions::OnOwnerRead)
	ON_BN_CLICKED(IDC_OWNER_WRITE_CHECK, &CFilePermissions::OnOwnerWrite)
	ON_BN_CLICKED(IDC_OWNER_EXECUTE_CHECK, &CFilePermissions::OnOwnerExecute)
	ON_BN_CLICKED(IDC_GROUP_READ_CHECK, &CFilePermissions::OnGroupRead)
	ON_BN_CLICKED(IDC_GROUP_WRITE_CHECK, &CFilePermissions::OnGroupWrite)
	ON_BN_CLICKED(IDC_GROUP_EXECUTE_CHECK, &CFilePermissions::OnGroupExecute)
	ON_BN_CLICKED(IDC_PUBLIC_READ_CHECK, &CFilePermissions::OnPublicRead)
	ON_BN_CLICKED(IDC_PUBLIC_WRITE_CHECK, &CFilePermissions::OnPublicWrite)
	ON_BN_CLICKED(IDC_PUBLIC_EXECUTE_CHECK, &CFilePermissions::OnPublicExecute)
END_MESSAGE_MAP()


BOOL CFilePermissions::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if(m_owner_read){
		m_owner_read_check.SetCheck(1);
	}
	if(m_owner_write){
		m_owner_write_check.SetCheck(1);
	}
	if(m_owner_execute){
		m_owner_execute_check.SetCheck(1);
	}

	if(m_group_read){
		m_group_read_check.SetCheck(1);
	}
	if(m_group_write){
		m_group_write_check.SetCheck(1);
	}
	if(m_group_execute){
		m_group_execute_check.SetCheck(1);
	}

	if(m_public_read){
		m_public_read_check.SetCheck(1);
	}
	if(m_public_write){
		m_public_write_check.SetCheck(1);
	}
	if(m_public_execute){
		m_public_execute_check.SetCheck(1);
	}

	return true;
}

// CFilePermissions message handlers
void CFilePermissions::OnOwnerRead()
{
	m_owner_read = !m_owner_read;
}

void CFilePermissions::OnOwnerWrite()
{
	m_owner_write = !m_owner_write;
}

void CFilePermissions::OnOwnerExecute()
{
	m_owner_execute = !m_owner_execute;
}

void CFilePermissions::OnGroupRead()
{
	m_group_read = !m_group_read;
}

void CFilePermissions::OnGroupWrite()
{
	m_group_write = !m_group_write;
}

void CFilePermissions::OnGroupExecute()
{
	m_group_execute = !m_group_execute;
}

void CFilePermissions::OnPublicRead()
{
	m_public_read = !m_public_read;
}

void CFilePermissions::OnPublicWrite()
{
	m_public_write = !m_public_write;
}

void CFilePermissions::OnPublicExecute()
{
	m_public_execute = !m_public_execute;
}


/**
* GetPermissions
*
*/
void CFilePermissions::GetPermissions(CString & permissions)
{
	int value = 0;

	if(m_public_execute){ value += 1; }
	if(m_public_write){ value += 2; }
	if(m_public_read){ value += 4; }
	if(m_group_execute){ value += 10; }
	if(m_group_write){ value += 20; }
	if(m_group_read){ value += 40; }
	if(m_owner_execute){ value += 100; }
	if(m_owner_write){ value += 200; }
	if(m_owner_read){ value += 400; }

	permissions = CString(_T(""));
	permissions.Format(_T("%d"), value);
	
}


/**
* SetPermissions
*
*/
void CFilePermissions::SetPermissions(CString & permissions)
{
	int value = 0;

	if(permissions.GetLength() != 3)
	{
		return;
	}

	if(permissions.GetAt(0) == '1'){m_owner_execute = true;}
	if(permissions.GetAt(0) == '2'){m_owner_write = true;}
	if(permissions.GetAt(0) == '4'){m_owner_read = true;}
	if(permissions.GetAt(0) == '5'){m_owner_execute = true; m_owner_read = true;}
	if(permissions.GetAt(0) == '6'){m_owner_write = true; m_owner_read = true;}
	if(permissions.GetAt(0) == '7'){m_owner_write = true; m_owner_read = true; m_owner_execute = true;}

	if(permissions.GetAt(1) == '1'){m_group_execute = true;}
	if(permissions.GetAt(1) == '2'){m_group_write = true;}
	if(permissions.GetAt(1) == '4'){m_group_read = true;}
	if(permissions.GetAt(1) == '5'){m_group_execute = true; m_group_read = true;}
	if(permissions.GetAt(1) == '6'){m_group_write = true; m_group_read = true;}
	if(permissions.GetAt(1) == '7'){m_group_write = true; m_group_read = true; m_group_execute = true;}

	if(permissions.GetAt(2) == '1'){m_public_execute = true;}
	if(permissions.GetAt(2) == '2'){m_public_write = true;}
	if(permissions.GetAt(1) == '4'){m_public_read = true;}
	if(permissions.GetAt(1) == '5'){m_public_execute = true; m_public_read = true;}
	if(permissions.GetAt(1) == '6'){m_public_write = true; m_public_read = true;}
	if(permissions.GetAt(1) == '7'){m_public_write = true; m_public_read = true; m_public_execute = true;}
}
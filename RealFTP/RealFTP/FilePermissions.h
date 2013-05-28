#pragma once
#include "afxwin.h"


// CFilePermissions dialog

class CFilePermissions : public CDialogEx
{
	DECLARE_DYNAMIC(CFilePermissions)

public:
	CFilePermissions(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFilePermissions();
	BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_PERMISSIONS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	

	DECLARE_MESSAGE_MAP()
public:
	CButton m_owner_read_check;
	CButton m_owner_write_check;
	CButton m_owner_execute_check;
	CButton m_group_read_check;
	CButton m_group_write_check;
	CButton m_group_execute_check;
	CButton m_public_read_check;
	CButton m_public_write_check;
	CButton m_public_execute_check;

	bool m_owner_read;
	bool m_owner_write;
	bool m_owner_execute;
	bool m_group_read;
	bool m_group_write;
	bool m_group_execute;
	bool m_public_read;
	bool m_public_write;
	bool m_public_execute;

	void OnOwnerRead();
	void OnOwnerWrite();
	void OnOwnerExecute();
	void OnGroupRead();
	void OnGroupWrite();
	void OnGroupExecute();
	void OnPublicRead();
	void OnPublicWrite();
	void OnPublicExecute();

	void GetPermissions(CString & permissions);
	void SetPermissions(CString & permissions);
};

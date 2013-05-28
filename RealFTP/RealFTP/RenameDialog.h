#pragma once
#include "afxwin.h"


// CRenameDialog dialog

class CRenameDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CRenameDialog)

public:
	CRenameDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRenameDialog();
	BOOL OnInitDialog();

	void SetName(CString & name);
	void GetName(CString & name);

// Dialog Data
	enum { IDD = IDD_RENAME_DIALOG };

protected:
	CString m_name;
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_renameEdit;
	afx_msg void OnChangeEdit();
};

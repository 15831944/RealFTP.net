#pragma once
#include "afxwin.h"


// CInputDialog dialog

class CInputDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CInputDialog)

public:
	CInputDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInputDialog();

// Dialog Data
	enum { IDD = IDD_INPUT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_input;
	afx_msg void OnEnChangeEdit1();
	CString m_inputText;
};

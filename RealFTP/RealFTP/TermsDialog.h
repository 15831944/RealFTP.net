#pragma once
#include "afxwin.h"


// CTermsDialog dialog

class CTermsDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CTermsDialog)

public:
	CTermsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTermsDialog();
	virtual  BOOL OnInitDialog( );
	void DiscloseTerms();

// Dialog Data
	enum { IDD = IDD_TERMS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_terms;
	CButton m_agree;
	bool m_agreed;
	afx_msg void OnBnClickedAgreeCheck();
};

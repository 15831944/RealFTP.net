#pragma once
//#include "afxwin.h"
#include "resource.h"
#include "afxwin.h"


// CUpgradeDialog dialog

class CUpgradeDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CUpgradeDialog)

public:
	CUpgradeDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CUpgradeDialog();

	struct THREADPARAM
	{
		HWND hWnd;
		CUpgradeDialog * m;
		int i;
	};
	THREADPARAM m_threadParam;
	CWinThread *m_checkThread;
	static UINT CheckWorker( LPVOID pParam );
	bool m_running;

	CString m_activationCode;

	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_BUY_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnUpgradeComplete(WPARAM wParam,LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_ActivationCode;
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOkButton();
	afx_msg void OnEnChangeActivationCode();
	CEdit m_activationCodeEdit;
};

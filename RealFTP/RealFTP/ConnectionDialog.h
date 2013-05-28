#pragma once
#include "afxwin.h"
#include "Connection.h"

// CConnectionDialog dialog

class CConnectionDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CConnectionDialog)

public:
	CConnectionDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConnectionDialog();
	//void OnInitialUpdate();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_CONNECTION_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct); // no
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_connectionName;
	//afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedOk();
	CEdit m_host;
	CEdit m_userName;
	CEdit m_password;

	CConnection m_connection;
	int m_connectionIndex;

	void SetConnection(CConnection &con);
	void SetConnectionIndex(int i);

	CString m_initalHost;
	void SetHost(CString & host);

	afx_msg void OnStnClickedConStatus();
	afx_msg BOOL OnEraseBkgnd(CDC * pDC);
};

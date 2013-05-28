#pragma once
#include "afxcmn.h"
#include "resource.h"
#include "FileTransfer.h"
#include <vector>

// CWaitDialog dialog

class CWaitDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CWaitDialog)

public:
	CWaitDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CWaitDialog();

	std::vector<CFileTransfer*> transfers;

	void SetProgress(int p);
	void SetPendingTransfers(int p);

// Dialog Data
	enum { IDD = IDD_WAIT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg LRESULT OnUpdateProgress(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnPendingTransfers(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnTransferDone(WPARAM wParam,LPARAM lParam);
	
	int m_transferCount;
	int progress;
	bool m_cancel;

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl m_progress;
};


// UpgraderDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "resource.h"

// CUpgraderDlg dialog
class CUpgraderDlg : public CDialogEx
{
// Construction
public:
	CUpgraderDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_UPGRADER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public:
	struct DOWNLOADSTATUS
	{
		ULONG ulProgress;
		ULONG ulProgressMax;
		ULONG ulStatusCode;
		LPCWSTR szStatusText;
	};

	typedef struct THREADPARAM
	{
		CUpgraderDlg*    _this;
		//IHTMLElement* pElement;
		CString sInstallerURL;
		CString strFileName;
		CString sFeedURL;
		CString sDownloadURL;
		CString sSoftwareName;
		HWND hWnd;
		HANDLE hEventStop;
		//XMLFeed * xmlFeed;
	} THREADSTRUCT;

	CWinThread *m_pDownloadThread;
	THREADPARAM m_downloadParam;
	static UINT RunDownloadThread(LPVOID pParam);
	CEvent m_eventStop;

// Implementation
protected:
	HICON m_hIcon;

	afx_msg LRESULT OnEndDownload(WPARAM, LPARAM);
	afx_msg LRESULT OnDisplayStatus(WPARAM, LPARAM lParam);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl m_progress_bar;
};

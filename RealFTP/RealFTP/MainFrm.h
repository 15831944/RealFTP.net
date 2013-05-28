
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
//#include "ChildView.h"
#include "MyMenu.h"
//#include "MyToolbar.h"
#include "HeaderDock.h"
#include "FooterPane.h"
#include "ConnectionListView.h"
#include "ConnectionManager.h"
#include "FtpManager.h"
#include "FileView.h"
#include "FileListView.h"
#include "WaitDialog.h"
#include "TrayIcon.h"
#include "RealStatusBar.h"
#include "Advertisement.h"
#include "HistoryDialog.h"

class CMainFrame : public CFrameWndEx
{
	
public:
	CMainFrame();
	CConnectionManager m_connectionManager;
	CFtpManager m_ftpManager;

	CHistoryDialog * m_historyDlg;
	//CHistoryDialog m_historyDlg2;

	afx_msg void OnEditPaste();
	afx_msg void OnEditCopy();

	struct THREADPARAM
	{
		HWND hWnd;
		CMainFrame * m;
		CFileListView * filelistView;
		CWaitDialog * waitDialog;
	};

	THREADPARAM m_threadParam;

	CWinThread *m_pFtpSyncThread;
	static UINT FtpSyncWorker( LPVOID pParam );

	CWinThread *m_pFileOpenThread;
	static UINT OnFileOpenThread(LPVOID pParam);
	
	//CFileView m_fileView;
	CTrayIcon m_trayIcon;

	CAdvertisement m_ad;

	afx_msg LRESULT OnAdChanged(WPARAM wParam, LPARAM lParam);

protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:
	//virtual void  OnFillBackground(CDC* /*pDC*/);
	void OnBackButton();
	void OnNextButton();
	void OnQueueButton();

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

	
protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

// Implementation
public:
	
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CMyMenuBar       m_wndMenuBar;  //  CMFCMenuBar
	//CMyToolBar       m_wndToolBar;  //  CMFCToolBar
	CMFCStatusBar     m_wndStatusBar;
	CMFCToolBarImages m_UserImages;
	//CChildView		m_wndView; // depricated

	//CStatusBar m_statusBar;
	CRealStatusBar m_statusBar;

	CHeaderDockPane m_headerDoc;
	CFooterPane m_footerPane;

	//  splitter object
	CSplitterWnd m_mainSplitter;
	CSplitterWnd m_vertical_splitter;
	bool m_bInitSplitter;

	CWaitDialog m_waitDialog;

	bool m_exit;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint ();

	afx_msg LRESULT OnNcHitTest(CPoint point);

	afx_msg LRESULT ResizeFileViewForContent(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnUpdateProgress(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnPendingTransfers(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnUpgradeAvailable(WPARAM wParam,LPARAM lParam);
	
	afx_msg LRESULT OnUpgradeComplete(WPARAM wParam,LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnMenuNewConnection();
	afx_msg void OnNewConnectionMenu();
	afx_msg void OnConnectionEdit();
	afx_msg void OnConnectionDelete();
	afx_msg void OnMenuNavigateItem(UINT nID);
	afx_msg void OnClose();

	afx_msg void OnFolderRename();
	afx_msg void OnFolderDelete();
	afx_msg void OnNewFile();
	afx_msg void OnBookmarkSelectedFolder();
	afx_msg void OnToggleFolderStoreLocally();
	afx_msg void OnFoldersBookmark();
	afx_msg void OnViewIcon();
	afx_msg void OnViewDetail();
	afx_msg void OnFileOpen();
	afx_msg void OnFileEdit();
	afx_msg void OnFileRename();
	afx_msg void OnFileDelete();
	afx_msg void OnNewFolder();
	afx_msg void OnFilePermissions();
	afx_msg void OnFileDownload();
	afx_msg void OnFileUpload();
	afx_msg void OnUpgrade();

	afx_msg void OnHelpManual();
	afx_msg void OnBuyUpgrade();
	afx_msg void OnSupport();

	afx_msg LRESULT OnLoadUrl(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLoadDir(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSelectFile(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnExit(WPARAM wParam, LPARAM lParam);

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	//afx_msg void OnDropFiles(HDROP hDropInfo);

	void LoadServerConnection(int index);
	void DirectoryLoaded(CString &url);
	void SetProgress(CConnection & con, int progress);
	void LoadTrayIcon();
	void LoadTrayBaloon();
	void HideTrayIcon();
};



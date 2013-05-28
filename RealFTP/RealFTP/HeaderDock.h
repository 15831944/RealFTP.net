#pragma once
#include "afxdockablepane.h"
#include "windows.h"
#include "MyButton.h"
#include "SearchField.h"
#include "UrlField.h"
#include "Connection.h"
#include <vector>
#include "File.h"

class CHeaderDockPane : public CDockablePane
{
public:
	CHeaderDockPane(void);
	~CHeaderDockPane(void);
	DECLARE_MESSAGE_MAP()

	struct SEARCH_THREADPARAM
	{
		HWND hWnd;
		CHeaderDockPane * _this;
		CConnection * con;
		//CRealFtpClient * ftpClient;
		CString path;
		CString search;
		//CWaitDialog * waitDialog;
	};

	SEARCH_THREADPARAM m_threadParam;
	CWinThread *m_pSearchThread;
	static UINT SearchWorker(LPVOID pParam);

	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	//virtual BOOL IsResizable() const;
	afx_msg BOOL OnEraseBkgnd(CDC * pDC);
	void DrawAd(CPaintDC & dc);

	void urlChanged();
	void urlKeyDown();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL CanFloat() const;
	void OnUrlSetFocus();
	void OnAdChanged();
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	//afx_msg LRESULT OnAdChanged(WPARAM wParam, LPARAM lParam);
public:
	void OnInitialUpdate();
	void SetUrl(CString &addr);
	void OnEditPaste();
	void OnEditCopy();
	void SetConnectionID(int id);

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd *pWnd, UINT nCtlColor);
	
	void OnBnClicked();
	void OnKeydownList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void memberFxn( NMHDR * pNotifyStruct, LRESULT * result );
	virtual void DoDataExchange(CDataExchange* pDX);   // DDX/DDV support

	void GetBackDir(CString &dir);

	BOOL RecreateComboBox(CComboBox* pCombo, LPVOID lpParam/*=0*/);

	CBitmapButton m_backBButton; // CBitmapButton CMyButton
	CBitmapButton m_nextBButton;
	CBitmapButton m_queueBButton;
	// dep
	CButton m_backButton;
	CButton m_nextButton;
	CButton m_queueButton;

	CConnection * m_connection;
	CString m_path;

private:
	CUrlField url; // CUrlField is a custom CComboBox
	CSearchField search;

	std::vector<CFileContainer> m_searchResults;

	int m_backIndex;  // index into history controlled by back next

	CString m_adTitle;
	CString m_adLink;

public:
	void OnBackButton();
	void OnNextButton();

protected:
	
};
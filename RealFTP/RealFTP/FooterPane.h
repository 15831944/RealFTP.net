#pragma once
#include "afxdockablepane.h"
#include "windows.h"
#include "Connection.h"

class CFooterPane : public CPane // CPane // CDockablePane
{
public:
	CFooterPane(void);
	~CFooterPane(void);
	DECLARE_MESSAGE_MAP()

	
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	//virtual BOOL IsResizable() const;
	afx_msg BOOL OnEraseBkgnd(CDC * pDC);

	void urlChanged();
	void urlKeyDown();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL CanFloat() const;
	void OnUrlSetFocus();
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view

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

protected:
	DECLARE_DYNCREATE(CFooterPane)

};
// SplitterWindowTutorialView.h : interface of the CConnectionListView class
//
/////////////////////////////////////////////////////////////////////////////
#include "ConnectionManager.h"
#include "FtpManager.h"
#include "FileTransfer.h"
#include <vector>

#if !defined(AFX_CONNECTIONLISTVIEW_H__A4706335_62EE_40D5_9F0A_3D6398BE7AD5__INCLUDED_)
#define AFX_CONNECTIONLISTVIEW_H__A4706335_62EE_40D5_9F0A_3D6398BE7AD5__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#include "ConnectionListDoc.h"
#endif // _MSC_VER > 1000

// CSplitterWindowTutorialView  -> CConnectionWindowView
class CConnectionListView : public CScrollView // CView // CListView CTreeCtrl  // CView
{
public:
	CConnectionListView();
	virtual ~CConnectionListView();

protected: // create from serialization only
	
	DECLARE_DYNCREATE(CConnectionListView)

	//virtual void OnInitialUpdate();

	void DrawServerConnection(CDC* pDC, int pos, CString name, int progress);
	void DrawBookmarkConnection(CDC* pDC, int pos, CString name);
	void DrawFileTransfer(CDC* pDC, int pos, bool sending, CString name, int progress);
	
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint ();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

// Attributes
public:
	CConnectionManager m_connectionManager;
	CObArray * m_listOfConnections;
	CFtpManager m_ftpManager;
	CMenu menu1;
	CConnectionListDoc* GetDocument();
	CConnectionListDoc* m_pDocument; // JDT
	int m_vScrollPos;

	std::vector<CFileTransfer*> transfers;
	void SetProgress( int progress); // CConnection & con,
	void SetProgress(CString & conName, int progress); // CConnection * con
	void UpdateProgress(CFileTransfer * transfer);
	void SelectConnection(int id);

	void UpdateConnections();
	int m_selectedItemIndex;
	int m_clickDownY;
	int m_selectedItemDragY;

	CConnection * m_connection;

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectionListView)
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	//virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	

// Generated message map functions
protected:
	//{{AFX_MSG(CConnectionListView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in SplitterWindowTutorialView.cpp
inline CConnectionListDoc* CConnectionListView::GetDocument()
   { return (CConnectionListDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPLITTERWINDOWTUTORIALVIEW_H__A4706335_62EE_40D5_9F0A_3D6398BE7AD5__INCLUDED_)

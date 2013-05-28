// SplitterWindowTutorialView.h : interface of the CSplitterWindowTutorialView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEVIEW_H__A4706335_62EE_40D5_9F0A_3D6398BE7AD5__INCLUDED_)
#define AFX_FILEVIEW_H__A4706335_62EE_40D5_9F0A_3D6398BE7AD5__INCLUDED_

#pragma once

#include "Connection.h"
#include <vector>
#include "File.h"
#include "ConnectionListDoc.h"
#include <afxcview.h>  // required for CListView

class CFileView : public CScrollView // CListView // CScrollView // CView
{
public:
	CFileView();
	virtual ~CFileView();

	struct VIEW_THREADPARAM
	{
		HWND hWnd;
		CFileView * v;
		int x;
	};

	CWinThread *m_pViewSyncThread;
	VIEW_THREADPARAM m_threadParam;
	static UINT ViewSyncWorker(LPVOID pParam);

	CListCtrl	c_FileList;

protected: // create from serialization only
	
	COleDropTarget     m_droptarget;
	IDropTargetHelper* m_piDropHelper;
    bool               m_bUseDnDHelper;
	BOOL ReadHdropData ( COleDataObject* pDataObject );


	
	
	DECLARE_DYNCREATE(CFileView)

	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView * senderView, LPARAM lHint, CObject * pHint);

	void DrawFile(CDC* pDC, int x, int y, CString name, int progress);
	void DrawFolder(CDC* pDC, int x, int y, CString name);
	void GetFileLocation(int index, int x, int y);

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	
	afx_msg LRESULT ResizeFileViewForContent(WPARAM wParam,LPARAM lParam);

	//afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);

	std::vector<CFileContainer> m_files;

// Attributes
public:
	void Initalize();
	CMenu menu1;
	CConnectionListDoc* GetDocument();
	CConnectionListDoc* m_pDocument; // JDT
	void LoadServerConnection(int index);
	void LoadDirectory(CString &dir);
	void ResizeScrollViewForContent();

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	CConnection * m_connection;
	
	bool m_viewSynced;
	bool m_initalized;
	bool m_dropTargetRegistered;

	CString m_path;
	int m_selected;

	bool m_shiftDown;
	int m_vScrollPos;

	CFont * m_fontTitle;
	CFont * m_fontMedium;

	
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitterWindowTutorialView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);


	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual void OnDragLeave();

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
	//{{AFX_MSG(CSplitterWindowTutorialView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in SplitterWindowTutorialView.cpp
inline CConnectionListDoc* CFileView::GetDocument()
   { return (CConnectionListDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPLITTERWINDOWTUTORIALVIEW_H__A4706335_62EE_40D5_9F0A_3D6398BE7AD5__INCLUDED_)

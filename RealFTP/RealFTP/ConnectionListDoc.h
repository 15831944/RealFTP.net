// SplitterWindowTutorialDoc.h : interface of the CSplitterWindowTutorialDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONNECTIONLISTDOC_H__461E3741_08D6_4309_9EE9_B33BB479259B__INCLUDED_)
#define AFX_CONNECTIONLISTDOC_H__461E3741_08D6_4309_9EE9_B33BB479259B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CConnectionListDoc : public CDocument
{
protected: // create from serialization only
	CConnectionListDoc();
	DECLARE_DYNCREATE(CConnectionListDoc)

// Attributes
public:



// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitterWindowTutorialDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CConnectionListDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CSplitterWindowTutorialDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPLITTERWINDOWTUTORIALDOC_H__461E3741_08D6_4309_9EE9_B33BB479259B__INCLUDED_)

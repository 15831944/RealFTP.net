// SplitterWindowTutorialDoc.cpp : implementation of the CSplitterWindowTutorialDoc class
//

#include "stdafx.h"
//#include "SplitterWindowTutorial.h"
#include "RealFTP.h"

#include "ConnectionListDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialDoc

IMPLEMENT_DYNCREATE(CConnectionListDoc, CDocument)

BEGIN_MESSAGE_MAP(CConnectionListDoc, CDocument)
	//{{AFX_MSG_MAP(CSplitterWindowTutorialDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialDoc construction/destruction

CConnectionListDoc::CConnectionListDoc()
{
	// TODO: add one-time construction code here

}

CConnectionListDoc::~CConnectionListDoc()
{
}

BOOL CConnectionListDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialDoc serialization

void CConnectionListDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialDoc diagnostics

#ifdef _DEBUG
void CConnectionListDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CConnectionListDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSplitterWindowTutorialDoc commands

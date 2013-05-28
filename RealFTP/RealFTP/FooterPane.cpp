#include "StdAfx.h"
#include "FooterPane.h"
#include "resource.h"
#include "ConnectionManager.h"

IMPLEMENT_DYNCREATE(CFooterPane, CPane ) // CPane CScrollView

CFooterPane::CFooterPane(void)
{
}

CFooterPane::~CFooterPane(void)
{
}

BEGIN_MESSAGE_MAP(CFooterPane, CPane ) // CPane CDockablePane
	ON_WM_PAINT()
//	ON_WM_SIZE()
	//ON_WM_NCHITTEST()  // blocks 
	//ON_NOTIFY( NM_CLICK, 1235, memberFxn ) // no
	//ON_NOTIFY(NM_CLICK,  2, OnBnClicked)
	//ON_NOTIFY( NM_CLICK, 1111, &CHeaderDockPane::OnKeydownList1 )
	//ON_BN_CLICKED(IDC_BACK_BUTTON_ID, &CHeaderDockPane::OnBackButton) // works but requires mainfrm
	//ON_BN_CLICKED(1235, &CHeaderDockPane::OnButtonBitmap)
	//ON_COMMAND(IDC_BACK_BUTTON_ID, &CHeaderDockPane::OnButtonBitmap)
	//ON_UPDATE_COMMAND_UI(99923, OnButtonBitmap)
	//ON_BN_CLICKED(1236, &CHeaderDockPane::OnBackButton)
	ON_WM_ERASEBKGND()
	//ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// WM_WINDOWPOSCHANGING  // capture resize
// WM_NCHITTEST
// WM_SIZING

void CFooterPane::OnInitialUpdate() 
{
	CFont * fntTitle1 = new CFont;
	fntTitle1->CreateFont(15 /*height*/, 8 /*width*/, 0, 0,
			      FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY, DEFAULT_PITCH, // | FF_ROMAN,
				_T("Arial"));    // Arial

	// dynamically create an edit control on the view
	CRect urlRect(145, 12, 500 /*end x*/, 33);
	//url.Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, urlRect, this, 1234); // CEdit
	
	// doesnt respond to WM_CUT, WM_COPY

	CRect searchRect(520, 12, 700 /*end x*/, 33);
	//search.Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, searchRect, this, IDC_SEARCH); // CEdit
	//search.Create( CBS_DROPDOWN  | WS_VSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER , searchRect, this, IDC_SEARCH);
	// CBS_DROPDOWNLIST CBS_SIMPLE
	//search.SetCurSel(0);
	
	//TRACE1("", m_backBButton.En);
}

void CFooterPane::OnBnClicked()
{
}

void CFooterPane::OnKeydownList1(NMHDR* pNMHDR, LRESULT* pResult)
{
   LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
   // TODO: Add your control notification handler
   //       code here
   
   *pResult = 0;
}

 void CFooterPane::memberFxn( NMHDR * pNotifyStruct, LRESULT * result )
 {
	int i = 0;
 }

void CFooterPane::DoDataExchange(CDataExchange* pDX)
{
	//CDockablePane::DoDataExchange(pDX);
	//DDX_Control(pDX, 1234, url);
    //DDX_Control(pDX, IDC_NUMERIC_BOX, m_NumericBox);
    //DDX_Text(pDX, IDC_NUMERIC_BOX, m_value);
    //DDV_MinMaxInt(pDX, m_value, 1, 20); // if the value in m_value is outside the range 1-20, MFC will pop up an error dialog
}

void CFooterPane::OnPaint()
{
	CPaintDC dc(this); 
	CDC pDC;
	pDC.CreateCompatibleDC(&dc);
	// device context for painting
	// TODO: Add your message handler code here
	// Do not call CDockablePane::OnPaint() for painting messages
	
	CRect rect;
	GetClientRect(&rect);
	
	//Manual BG repaint
	CBrush bg;
	bg.CreateStockObject(RGB(236,236,236));
	//CPen penFill(PS_NULL,0,RGB(250,0,0));
	//dc.SelectObject(&penFill);
	CBrush fillBrush(RGB(236, 236, 236));
	CBrush *oldFillBrush = dc.SelectObject(&fillBrush);
	dc.FillRect(&rect, &bg);
	//dc.Ellipse(&rect);
	
	//Invalidate();
}


void CFooterPane::OnDraw(CDC* dc) // pDC
{
	CRealMemDC pDC(dc);
	CRect rect;
	GetClientRect(&rect);
	//pDC->DPtoLP(&rect);

}

BOOL CFooterPane::OnEraseBkgnd(CDC * pDC)
{
	return FALSE;
}

/*
BOOL CHeaderDockPane::IsResizable() const
{
	return false;
}
*/

void CFooterPane::OnSize(UINT nType, int cx, int cy)
{
	//CPane::OnSize(nType, cx, cy);
	//CDockablePane::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	Invalidate();

	//TRACE2(" x y %d %d \n", cx, cy);

	//CRect urlRect(145, 12, cx - 220 /*end x*/, 21);
	//url.SetRect(urlRect);
	
	

	if(cy != 40){
		// resize
		CRect pos;
		GetWindowRect(pos);
		ScreenToClient(&pos);
		UINT flags = SWP_NOSENDCHANGING | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE;
		//SetWindowPos(NULL, 0, 0, pos.Width(), 45, flags);

		{
		// get the rect for the parent
		CRect rectClient;
		GetClientRect(rectClient);
		// flags to ensure SetWindowPos only moves and resizes
		UINT flags = SWP_NOSENDCHANGING |SWP_NOACTIVATE | SWP_NOZORDER;
		//SetWindowPos(NULL, 0, 0, rectClient.Width(), 45, flags);
		}
	}

	//Invalidate();
}

LRESULT CFooterPane::OnNcHitTest(CPoint point)
{	
	
	//LRESULT res = CDockablePane::OnNcHitTest(point);
	//LRESULT res = CPane::OnNcHitTest(point);
	LRESULT res = 0;

	//TRACE1("Hit %d \n", res);
	if( res == HTLEFT || res == HTRIGHT )
		res = HTBORDER;
	if(res == HTTOP || res==HTBOTTOM) {
		res = HTBORDER;
	}
	if(res == HTBORDER){
		int i = 0;
	}
	
	res = 0;
	return res;
}


void CFooterPane::urlChanged()
{
	//CString urlValue;
	//url.GetWindowTextW(urlValue);
	//TRACE1("Url: %s ", urlValue);
}


void CFooterPane::urlKeyDown()
{
	
}


void CFooterPane::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	int i = 0;
}


BOOL CFooterPane::PreTranslateMessage(MSG* pMsg)
{

	//
	// URL Enter Key
	//
    if (pMsg->message == WM_KEYDOWN &&
        pMsg->wParam == VK_RETURN 
		  )
    {
        
        return TRUE; // this doesn't need processing anymore
    }

	
	if(pMsg->message == WM_LBUTTONUP){
		//TRACE0(" lbutton ");
	}

	//TRACE1(" m: %d \n", pMsg->message);

    return FALSE; // all other cases still need default processing
}






void CFooterPane::SetConnectionID(int id)
{
	if(id > -1)
	{
		CConnectionManager conManager;
		conManager.Load();
		//if(id < conManager.)
		//{
			CConnection * con = conManager.GetAt(id);
			m_connection = con;
		//}
	}	
}


void CFooterPane::OnEditPaste()
{
	
}

void CFooterPane::OnEditCopy()
{
	
}


/**
* OnUrlSetFocus
*
* Description: Remove automatic selection of all text.
*/
void CFooterPane::OnUrlSetFocus()
{
	CString urlValue;
	
}

BOOL CFooterPane::CanFloat() const
{
	return FALSE;
}

HBRUSH CFooterPane::OnCtlColor(CDC* pDC, CWnd *pWnd, UINT nCtlColor)
{
	CBrush m_hRBrush(RGB(255,0,255));

    switch ( nCtlColor )
	{
        case CTLCOLOR_LISTBOX:
             //pDC->SetBkMode(TRANSPARENT);
             pDC->SetTextColor(RGB(255,0,0));
             pDC->SetBkColor(RGB(255,255,255)); 
             return m_hRBrush;
             break;
		case CTLCOLOR_STATIC:
			pDC->SetTextColor(RGB(255,0,0));
			pDC->SetBkMode(TRANSPARENT);
			return m_hRBrush;
			break;
        default:
             break;
   }
}
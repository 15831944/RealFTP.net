#include "StdAfx.h"
#include "HeaderDock.h"
#include "resource.h"
#include <vector>
#include "FtpManager.h"
#include "ConnectionManager.h"


CHeaderDockPane::CHeaderDockPane(void)
{
	m_backIndex = 0;
	m_path = CString(_T("/"));
}

CHeaderDockPane::~CHeaderDockPane(void)
{
	// m_backBButton delete();
}

BEGIN_MESSAGE_MAP(CHeaderDockPane, CDockablePane)
	ON_WM_PAINT()
	ON_WM_SIZE()
	//ON_WM_NCHITTEST()  // blocks 
	//ON_NOTIFY( NM_CLICK, 1235, memberFxn ) // no
	//ON_NOTIFY(NM_CLICK,  2, OnBnClicked)
	//ON_NOTIFY( NM_CLICK, 1111, &CHeaderDockPane::OnKeydownList1 )
	ON_EN_CHANGE(1234, &CHeaderDockPane::urlChanged) // yes
	//ON_BN_CLICKED(IDC_BACK_BUTTON_ID, &CHeaderDockPane::OnBackButton) // works but requires mainfrm
	//ON_BN_CLICKED(1235, &CHeaderDockPane::OnButtonBitmap)
	//ON_COMMAND(IDC_BACK_BUTTON_ID, &CHeaderDockPane::OnButtonBitmap)
	//ON_UPDATE_COMMAND_UI(99923, OnButtonBitmap)
	//ON_BN_CLICKED(1236, &CHeaderDockPane::OnBackButton)
	ON_WM_ERASEBKGND()
	//ON_WM_CTLCOLOR()
	ON_CBN_SETFOCUS(1234, &CHeaderDockPane::OnUrlSetFocus)
	//ON_MESSAGE(ID_AD_CHANGED, &CHeaderDockPane::OnAdChanged)
END_MESSAGE_MAP()

// WM_WINDOWPOSCHANGING  // capture resize
// WM_NCHITTEST
// WM_SIZING

void CHeaderDockPane::OnInitialUpdate() 
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
	url.Create( CBS_DROPDOWN  | WS_VSCROLL | WS_CHILD | WS_VISIBLE  , urlRect, this, 1234);
	url.SetFont(fntTitle1);
	url.ModifyStyle( 0, CBS_SORT );
	//RecreateComboBox(&url, 0);
	url.SetWindowText(CString(_T("ftp://")));
	// doesnt respond to WM_CUT, WM_COPY

	CRect searchRect(520, 12, 700 /*end x*/, 33);
	//search.Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, searchRect, this, IDC_SEARCH); // CEdit
	search.Create( CBS_DROPDOWN  | WS_VSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER , searchRect, this, IDC_SEARCH);
	// CBS_DROPDOWNLIST CBS_SIMPLE
	//search.SetCurSel(0);
	search.SetDroppedWidth(300);
	search.SetFont(fntTitle1);
	
	m_backBButton.Create(NULL, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CRect(14,5,32 + 14,32 + 5), this, IDC_BACK_BUTTON_ID);
	m_backBButton.EnableWindow(TRUE);
	m_backBButton.LoadBitmaps(IDB_BACK, IDB_BACK /*sel*/, IDB_BACK /*foc*/, IDB_BACK /*dis*/);
	//m_backBButton.SizeToContent();

	m_nextBButton.Create(NULL, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CRect(14 + 41,5,32 + 14 + 41,32 + 5), this, IDC_NEXT_BUTTON_ID);
	m_nextBButton.EnableWindow(TRUE);
	m_nextBButton.LoadBitmaps(IDB_NEXT, IDB_NEXT /*sel*/, IDB_NEXT /*foc*/, IDB_NEXT /*dis*/);
	//m_backBButton.SizeToContent();

	m_queueBButton.Create(NULL, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CRect(14 + (41*2),5,32 + 14 + (41*2),32 + 5), this, IDC_QUEUE_BUTTON_ID);
	m_queueBButton.EnableWindow(TRUE);
	m_queueBButton.LoadBitmaps(IDB_QUEUE, IDB_QUEUE /*sel*/, IDB_QUEUE /*foc*/, IDB_QUEUE /*dis*/);
	//m_queueBButton.SizeToContent();

	//TRACE1("", m_backBButton.En);
}

void CHeaderDockPane::OnBnClicked()
{
}

void CHeaderDockPane::OnKeydownList1(NMHDR* pNMHDR, LRESULT* pResult)
{
   LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
   // TODO: Add your control notification handler
   //       code here
   
   *pResult = 0;
}

 void CHeaderDockPane::memberFxn( NMHDR * pNotifyStruct, LRESULT * result )
 {
	int i = 0;
 }

void CHeaderDockPane::DoDataExchange(CDataExchange* pDX)
{
	int x = 0;

	CDockablePane::DoDataExchange(pDX);
	DDX_Control(pDX, 1234, url);

    //DDX_Control(pDX, IDC_NUMERIC_BOX, m_NumericBox);
    //DDX_Text(pDX, IDC_NUMERIC_BOX, m_value);
    //DDV_MinMaxInt(pDX, m_value, 1, 20); // if the value in m_value is outside the range 1-20, MFC will pop up an error dialog
}

void CHeaderDockPane::OnPaint()
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
	
	//DrawAd(dc);

	//Invalidate();
}


void CHeaderDockPane::OnDraw(CDC* dc) // pDC
{
	CRealMemDC pDC(dc);
	CRect rect;
	GetClientRect(&rect);
	//pDC->DPtoLP(&rect);

}


void CHeaderDockPane::DrawAd(CPaintDC & dc)
{
	//CPaintDC dc(this); 
	//CDC pDC;
	//pDC.CreateCompatibleDC(&dc);

	CRect rect;
	GetClientRect(&rect);

	if(rect.right > 500)
	{
		CBrush bg;
		bg.CreateStockObject(RGB(0,0,0));
		CBrush fillBrush(RGB(250, 0, 0));
		CBrush *oldFillBrush = dc.SelectObject(&fillBrush);
		//rect = CRect( CPoint(rect.left, rect.top), CPoint(rect.right, rect.bottom - 2) );
		//rect = CRect( CPoint(200, 5), CPoint(400, 25) );
		//dc.FillRect(&rect, &bg);


		int height = 22;
		int progress_x = rect.right - 300 - 1;
		int progress_y = -22;
		CPoint prorgess_border_points[5];
		prorgess_border_points[0] = CPoint(progress_x, progress_y);
		prorgess_border_points[1] = CPoint(progress_x, progress_y + height);
		prorgess_border_points[2] = CPoint(progress_x+(rect.right - progress_x)-20 - 1, progress_y + height);
		prorgess_border_points[3] = CPoint(progress_x+(rect.right - progress_x)-20 - 1, progress_y);
		prorgess_border_points[4] = CPoint(progress_x, progress_y);
		CPen penOutline(PS_SOLID, 1, RGB(175, 175, 175));
		dc.SelectObject(&penOutline);
		dc.Polyline(prorgess_border_points, 5);

	}
}

BOOL CHeaderDockPane::OnEraseBkgnd(CDC * pDC)
{
	return FALSE;
}

/*
BOOL CHeaderDockPane::IsResizable() const
{
	return false;
}
*/

void CHeaderDockPane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	Invalidate();

	//TRACE2(" x y %d %d \n", cx, cy);

	//CRect urlRect(145, 12, cx - 220 /*end x*/, 21);
	//url.SetRect(urlRect);
	
	if(url){
		//url.SetEditSel(0, 0);
	}

	if(url && cx > 500){
		url.MoveWindow(145, 11, cx - 360, 21);
	}
	if(search && cx > 500){
		search.MoveWindow(cx - 195, 11, 175, 21);
	}

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

LRESULT CHeaderDockPane::OnNcHitTest(CPoint point)
{	
	LRESULT res = CDockablePane::OnNcHitTest(point);
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


void CHeaderDockPane::urlChanged()
{
	//CString urlValue;
	//url.GetWindowTextW(urlValue);
	//TRACE1("Url: %s ", urlValue);
}


void CHeaderDockPane::urlKeyDown()
{
	CString urlValue;
	url.GetWindowTextW(urlValue);
}


void CHeaderDockPane::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	int i = 0;
}


/**
* PreTranslateMessage
*
*/
BOOL CHeaderDockPane::PreTranslateMessage(MSG* pMsg)
{
	//CDockablePane::PreTranslateMessage(pMsg);

	//
	// Enter new URL and hit Enter Key
	//
    if (pMsg->message == WM_KEYDOWN &&
        pMsg->wParam == VK_RETURN &&
		url.HasFocus()  )
    {
        // handle return pressed in edit control
		CString windowText(_T(""));
		url.GetWindowTextW(windowText);		// Get window text
		if(url.m_selectedText.GetLength() != 0)
		{
			int i = 0;
			//windowText = url.m_selectedText;
		}
		int idx = url.GetCurSel();			// Get selection text
		if( idx > -1 ) 
		{
		//	url.GetLBText( idx, windowText );
		}
		TRACE1(" windowText %s \n ", windowText);

		CString * urlStr = new CString(windowText);
		AfxGetMainWnd()->PostMessage(ID_LOAD_URL, /*Callback SetUrl*/1, (LPARAM)urlStr);
		
		// If the ID_LOAD_URL is successfull, SetUrl(url) will be called to add this item to the history.

		
		/*
		std::vector<CString> urlHistory; 
		//TRACE0(" \n\n");
		CString str, str2;
		int n;
		int count = url.GetCount();
		for (int i=0;i < count; i++) 
		{
			n = url.GetLBTextLen( i );
			url.GetLBText( i, str.GetBuffer(n) );
			str.ReleaseBuffer();
			urlHistory.push_back(str);
			//url.DeleteString(i);
			//TRACE1(" read: %s \n", str);
		}
		for (int i= count - 1;i >= 0 ; i--) 
		{
			url.DeleteString(i);
		}
		
		// Add url to history. 
		urlHistory.insert(urlHistory.begin(), windowText.AllocSysString());
		for (int i=0; i < urlHistory.size(); i++)
		{
			CString entry(urlHistory[i]);
			//TRACE1(" url entry: %s \n", entry);
			int pos = url.AddString(entry.AllocSysString());
			url.SetItemData(pos, i);
		}
		*/

		//int pos = url.AddString(CString(_T("me")).AllocSysString());
		//url.SetItemData(pos, 0);

		// Load view
		// PostMessage( .... );

        return TRUE; // this doesn't need processing anymore
    }

	if (//pMsg->message == CBN_SELCHANGE &&
        //pMsg->wParam == VK_RETURN &&
		url.HasFocus()  )
    {
		//TRACE1("m %d    %d  \n" , pMsg->message, CBN_SELCHANGE);
	}


	
	//TRACE2(" message: %d   %d \n", pMsg->message, CBN_SELCHANGE);

	

	/*
	if (pMsg->message == WM_KEYDOWN &&
        pMsg->wParam == VK_RETURN &&
		GetFocus() == &search  )
    {
		search.SetWindowText(CString(_T("searching...")));
		
        // handle return pressed in edit control
        return TRUE; // this doesn't need processing anymore
    }
	*/

	//
	// Click on a url history drop down item.
	// url is a CComboBox 
	//
	if(url.HasFocus() && pMsg->message == CBN_SELENDOK && pMsg->message == CBN_SELCHANGE) 
	{
		TRACE0(" url history select  \n");
		int index = url.GetCurSel();
		int size = url.GetCount();
		if(index > -1 && index < size)
		{
			CString str, str2;
			int n;
			n = url.GetLBTextLen( index );
			url.GetLBText( index, str.GetBuffer(n) );
			str.ReleaseBuffer();
			str2.Format(_T("  %s\r\n"), index, str.GetBuffer(0));
			//TRACE1("item: %s \n", str2);
			CString * urlStr = new CString(str);
			//AfxGetMainWnd()->PostMessage(ID_LOAD_URL, 0, (LPARAM)urlStr);
		}
	}
	

	//
	// Search for input string Enter key
	//
	if(pMsg->wParam == VK_RETURN && search.HasFocus()){
		CString str, str2;
		
		m_searchResults.empty(); // remove old results.

		
		int n;
		for (int i = 0; i < search.GetCount(); i++)
		{
			int c = search.GetCount();

			//n = search.GetLBTextLen( i );
			//search.GetLBText( i, str.GetBuffer(n) );
			//str.ReleaseBuffer();
			//str2.Format(_T("item %d: %s\r\n"), i, str.GetBuffer(0));
			//afxDump << str2;
			search.DeleteString(i);
			//search.DeleteItem(
		}
		
		CString searchString(_T(""));
		search.GetWindowTextW(searchString);

		// Start search thread.....
		SEARCH_THREADPARAM * _param = new SEARCH_THREADPARAM;
		_param->_this = this;
		_param->con = m_connection;
		_param->search = searchString;
		_param->path = m_path; 
		m_pSearchThread = AfxBeginThread(CHeaderDockPane::SearchWorker, _param); 
		if (m_pSearchThread != NULL)
		{
			TRACE(_T("AfxBeginThread: 0x%08lX\n"), m_pSearchThread->m_nThreadID);
		}

		//int count = search.GetCount();
		//for (int i=0; i < count; i++) 
		//{
		//	search.DeleteString(i);
		//}

		search.AddString(CString(_T("searching...")));
		search.ShowDropDown(true);

		//return FALSE;
	}


	//
	// Select Search result to view
	//
	if(search.HasFocus() && pMsg->message == 15) // CBN_SELCHANGE=1  
	{
		TRACE0(" CBN_SELCHANGE  \n");
		int index = search.GetCurSel();
		int size = search.GetCount();
		int rsize = m_searchResults.size();
		if(index > -1 && index < size && index < rsize)
		{
			CString str, str2;
			int n;
			n = search.GetLBTextLen( index );
			search.GetLBText( index, str.GetBuffer(n) );
			str.ReleaseBuffer();
			str2.Format(_T("item %d: %s\r\n"), index, str.GetBuffer(0));
			//TRACE1("item: %s \n", str2);

			CFileContainer f;
			f = (CFileContainer)m_searchResults.at(index);
			TRACE1(" Load search result -> file %s \n", f.name);

			CString * dirStr = new CString(f.path);
			//AfxGetMainWnd()->PostMessage(ID_LOAD_DIR, /*Callback SetUrl*/0, (LPARAM)dirStr);

			CFileContainer * argFile = new CFileContainer(f);
			AfxGetMainWnd()->PostMessage(ID_SELECT_FILE, /* */0, (LPARAM)argFile);
		
			//search.SetWindowTextW(_T(""));
			search.SetCurSel(-1); // Deselect item
		}
		//search.Get
	}


	if(pMsg->message == WM_LBUTTONUP){
		//TRACE0(" lbutton ");
	}

	//TRACE1(" m: %d \n", pMsg->message);

    return FALSE; // all other cases still need default processing
}


/**
* SearchWorker
*
* Description: search thread
*/
UINT CHeaderDockPane::SearchWorker(LPVOID pParam)
{
	CHeaderDockPane::SEARCH_THREADPARAM *const pThreadParam = static_cast<CHeaderDockPane::SEARCH_THREADPARAM *>(pParam);

	pThreadParam->_this->m_searchResults.clear();
	pThreadParam->_this->m_searchResults.empty();

	if(pThreadParam->con && pThreadParam->search.GetLength() > 0)
	{
		CFtpManager ftp;
		std::vector<CFileContainer> files;
		ftp.SearchLocalDirectory(pThreadParam->con, pThreadParam->path, pThreadParam->search, files);

		int i = 0;
		for(i = 0; i < files.size(); i++)
		{
			CFileContainer f;
			f = (CFileContainer)files[i];
			// add

			CString displayName( f.path + f.name );

			int pos = pThreadParam->_this->search.AddString( displayName.AllocSysString());

			pThreadParam->_this->m_searchResults.push_back(f);
		}
	}

	//
	//pThreadParam->_this->search.AddString(CString(_T("done")));
	pThreadParam->_this->search.ShowDropDown(true);

	return 1;
}


/**
* OnBackButton
*
* Description: Back
*/
void CHeaderDockPane::OnBackButton()
{
	//TRACE1(" Dock: Back Button %d \n", m_backIndex);
	m_backIndex++;

	// bounds check
	int history_count = url.GetCount();
	if(m_backIndex >= history_count){
		m_backIndex = history_count;
		return;
	}

	// Get the url
	CString backUrl(_T(""));
	int index = m_backIndex; // history_count - m_backIndex - 2;
	int n = url.GetLBTextLen( index );
	url.GetLBText( index , backUrl.GetBuffer(n) ); // history_count
	backUrl.ReleaseBuffer();

	// update url 
	url.SetWindowText(backUrl); // 

	// Load the url
	CString * urlStr = new CString(backUrl);
	AfxGetMainWnd()->PostMessage(ID_LOAD_URL, /*Callback SetUrl*/0, (LPARAM)urlStr);
}


/**
* OnNextButton
*
* Description: 
*/
void CHeaderDockPane::OnNextButton()
{
	//TRACE1(" Dock: Next Button %d \n", m_backIndex);
	m_backIndex--;
	int history_count = url.GetCount();
	if(m_backIndex < 0){
		m_backIndex = 0;
		return;
	}
	
	// Get the url
	CString backUrl(_T(""));
	int index = m_backIndex; // history_count
	int n = url.GetLBTextLen( index );
	url.GetLBText( index, backUrl.GetBuffer(n) );
	backUrl.ReleaseBuffer();

	// update url 
	url.SetWindowText(backUrl); // 

	// Load the url
	CString * urlStr = new CString(backUrl);
	AfxGetMainWnd()->PostMessage(ID_LOAD_URL, /*Callback SetUrl*/0, (LPARAM)urlStr);
}


// DEPRICATE
void CHeaderDockPane::GetBackDir(CString &dir)
{
	std::vector<CString> urlHistory; 
	CString str, str2;
	int n;
	for (int i=0;i < url.GetCount(); i++)
	{
		n = url.GetLBTextLen( i );
		url.GetLBText( i, str.GetBuffer(n) );
		str.ReleaseBuffer();
		urlHistory.push_back(str);
	}
	
	if(urlHistory.size() > 1){
		dir = CString(urlHistory[urlHistory.size()-2]);
	}

	// Strip out domain.
	int startPos = dir.Find(_T("/"));
	if(startPos != -1){
		dir = dir.Mid(startPos, (dir.GetLength() - startPos));
	}

	if(dir.GetLength() == 0){
		dir = CString(_T("/")); // failed
	}
}


/**
* SetUrl  
*
* Description: Set url address, called by connection list or file list view navigation.
* Ads url to history.
*/
void CHeaderDockPane::SetUrl(CString &addr)
{
	url.SetWindowText(addr);

	std::vector<CString> urlHistory; 
	CString str, str2;
	int n;
	int count = url.GetCount();
	for (int i=0;i < count; i++) 
	{
		n = url.GetLBTextLen( i );
		url.GetLBText( i, str.GetBuffer(n) );
		str.ReleaseBuffer();
		urlHistory.push_back(str);
		//url.DeleteString(i);
		//TRACE1(" read: %s \n", str);
	}
	for (int i = count - 1; i >= 0; i--) 
	{
		url.DeleteString(i);
	}	
	urlHistory.insert(urlHistory.begin(), addr.AllocSysString());	
	for (int i=0; i < urlHistory.size(); i++)
	{
		CString entry(urlHistory[i]);
		//TRACE1(" url entry: %s \n", entry);
		int pos = url.AddString(entry.AllocSysString());
		url.SetItemData(pos, i);
	}

	m_backIndex = 0;


	// Set Path
	//m_path
}


void CHeaderDockPane::SetConnectionID(int id)
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


void CHeaderDockPane::OnEditPaste()
{
	if(url.HasFocus())
	{
		url.Paste();
	}
	if(search.HasFocus())
	{
		search.Paste();
	}
}


void CHeaderDockPane::OnEditCopy()
{
	if(url.HasFocus())
	{
		url.Copy();
	}
	if(search.HasFocus())
	{
		search.Copy();
	}
}


/**
* OnUrlSetFocus
*
* Description: Remove automatic selection of all text.
*/
void CHeaderDockPane::OnUrlSetFocus()
{
	CString urlValue;
	url.GetWindowTextW(urlValue);
	url.SetEditSel( urlValue.GetLength(), urlValue.GetLength());
}


void CHeaderDockPane::OnAdChanged()
{
	//m_adTitle
}


BOOL CHeaderDockPane::CanFloat() const
{
	return FALSE;
}


HBRUSH CHeaderDockPane::OnCtlColor(CDC* pDC, CWnd *pWnd, UINT nCtlColor)
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



//LRESULT CHeaderDockPane::OnAdChanged(WPARAM wParam, LPARAM lParam)
//{
//	return 1;
//}

// recreate the combo box by copying styles etc, and list items
// and applying them to a newly created control
/*

*/
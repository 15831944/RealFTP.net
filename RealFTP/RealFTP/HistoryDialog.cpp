// HistoryDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RealFTP.h"
#include "HistoryDialog.h"
#include "afxdialogex.h"


// CHistoryDialog dialog

IMPLEMENT_DYNAMIC(CHistoryDialog, CDialogEx)

CHistoryDialog::CHistoryDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHistoryDialog::IDD, pParent)
{

}

CHistoryDialog::~CHistoryDialog()
{
}

void CHistoryDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST2, m_listCtrl);
}


BEGIN_MESSAGE_MAP(CHistoryDialog, CDialogEx)
	ON_WM_SIZE()    
END_MESSAGE_MAP()


// CHistoryDialog message handlers

BOOL CHistoryDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	CRect rect;
	GetClientRect( &rect );

	//m_listView.Create(_T("CListView"), _T("History"), WS_CHILD | WS_VISIBLE, rect, this, 1234555, NULL);

	//m_listView.SetParent(this);
    //m_listView.SetFocus();

	// Set up CListView
	//CListCtrl& list = m_listView.GetListCtrl();
	//list.Create(WS_CHILD | WS_VISIBLE, rect, this, 123444);



	SHFILEINFO rInfo;
	m_ImageListMed = new CImageList();
	m_ImageListMed->Create(32,32,ILC_COLOR32,0,0);
	//list.ModifyStyle ( LVS_TYPEMASK, LVS_ICON | LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS );

	m_listCtrl.ModifyStyle ( LVS_TYPEMASK, LVS_REPORT | LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS );
	m_listCtrl.SetExtendedStyle ( LVS_EX_INFOTIP   );

	m_listCtrl.SetImageList ( m_ImageListMed, LVSIL_SMALL );
	m_listCtrl.SetImageList ( m_ImageListMed, LVSIL_NORMAL );

	m_listCtrl.InsertColumn ( 0, _T("File Name"), LVCFMT_LEFT, 0, 0 );
	m_listCtrl.InsertColumn ( 1, _T("Type"), LVCFMT_LEFT, 0, 1 );
	m_listCtrl.InsertColumn ( 2, _T("Size"), LVCFMT_LEFT, 0, 2 );
	m_listCtrl.InsertColumn ( 3, _T("Permissions"), LVCFMT_LEFT, 0, 3 );

	m_listCtrl.SetColumnWidth ( 0, 200 );
	m_listCtrl.SetColumnWidth ( 1, 80 );
	m_listCtrl.SetColumnWidth ( 2, 80 );
	m_listCtrl.SetColumnWidth ( 3, LVSCW_AUTOSIZE_USEHEADER );
	{
		CBitmap bmp;
		bmp.LoadBitmap(IDB_FOLDER_MED);
		m_folderMedIndex = m_ImageListMed->Add(&bmp, RGB(0, 0, 0));
	}

	{
		CBitmap bmp;
		bmp.LoadBitmap(IDB_FILE_MED);
		m_fileMedIndex = m_ImageListMed->Add(&bmp, RGB(0, 0, 0));
	}
		
	// EXE
	{
		//CBitmap bmp;
		//bmp.LoadBitmap(IDB_EXE_MED);
		//m_exeMedIndex = m_ImageListMed->Add(&bmp, RGB(0, 0, 0));
	}


	CFileContainer f;
	f.dir = 1;
	f.name = CString(_T("Test")); 
	f.path = CString(_T("Test")); 
	f.localPath = CString(_T("Test")); 
	f.size = 123;
	f.remoteDate = CString(_T("Test")); 
	f.host = CString(_T("Test")); 
	//m_files.push_back(f);

    AddFilesToList(m_files);

    return FALSE;
}


/**
*
*
*/
void CHistoryDialog::PostNcDestroy() 
{	
    CDialogEx::PostNcDestroy();
    delete this;
}


/**
* UpdateFileTransfer
*
* Description: Called when a file transfer change occurs
*/
void CHistoryDialog::UpdateFileTransfer(CFileTransfer * t)
{
	bool found = false;

	int i = 0;
	for(i = 0; i < m_files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)m_files[i];

		if(f.name.Compare( t->fileName ) == 0)
		{
			found = true;
		}
	}

	if(!found)
	{
		CFileContainer f;
		f.dir = 1;
		f.name = CString(t->fileName); 
		f.path = CString(t->path); 
		f.localPath = CString(_T("Test")); 
		f.size = 123;
		f.remoteDate = CString(_T("Test")); 
		f.host = CString(_T("Test")); 
		m_files.push_back(f);

		AddFilesToList(m_files);


		CFileContainer transfer;
		transfer.name = CString(t->fileName);
		transfer.path = CString(t->path);
		//m_transfers.push_back(transfer);

		
	}

	// cleanup 
	delete t;
}

void CHistoryDialog::AddTransfer(CFileContainer & f)
{
	//if(m_files){
		//m_files.push_back(f);
		//AddFilesToList(m_files);
	//}
}



/**
* AddTransfersToList
*
* Description: Add transfer list to history dialog.
*/
void CHistoryDialog::AddTransfersToList(std::vector<CFileTransfer> &transfers)
{
	m_listCtrl.DeleteAllItems();

	int i = 0;
	for(i = 0; i < transfers.size(); i++)
	{
		CFileTransfer f;
		f = (CFileTransfer)transfers[i];

		LVITEM      rItem;
		ZeroMemory ( &rItem, sizeof(LVITEM) );

        rItem.mask    = LVIF_IMAGE | LVIF_TEXT;
        rItem.iItem   = i; // nIndex;
		rItem.pszText = CString(f.fileName).AllocSysString(); // szNextFile;
        //rItem.iImage  = i; // rFileInfo.iIcon;
	
		int itemIndex = m_listCtrl.InsertItem( &rItem );

		CRect rect;
		m_listCtrl.GetItemRect(itemIndex, &rect, LVIR_LABEL);

		CString toolTipText(_T(""));
		//if(f.  .dir == 0){
		//	CString size(_T(""));
		//	size.Format(_T("%ld"),f.size);
		//	toolTipText = toolTipText + _T("size: ") + size + CString(_T("  "));
		//}
		//toolTipText = toolTipText + _T("permissions: ") + f.permissions;
		//m_tooltip.AddTool(&list, toolTipText, &rect, itemIndex+1);

		if(f.isDir){
			m_listCtrl.SetItemText ( i, 1, CString(_T("Folder")).AllocSysString() );
		//	rItem.iImage  = 0;
			//m_listView.GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_folderMedIndex , 0, 0, 0);
			m_listCtrl.SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_folderMedIndex , 0, 0, 0);
		} else {
			// File name
			m_listCtrl.SetItemText ( i, 1, CString(_T("File")).AllocSysString() );

			// File size
			CString fileSize(_T(""));
			fileSize.Format(_T("%ld"), f.size); 
			m_listCtrl.SetItemText ( i, 2, fileSize.AllocSysString() );

			// Permissions
			CString filePermissions(_T("X")); // f.permissions
			m_listCtrl.SetItemText ( i, 3, filePermissions.AllocSysString() );

			//rItem.iImage  = 1;
			//if(m_viewMode == 0 ){ // Large
			//	if(f.name.Find(_T(".exe")) != -1){
			//		GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_exeLargeIndex, 0, 0, 0);
			//	} else { 
			//		// default file
			//		GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_fileLargeIndex, 0, 0, 0);
			//	}
			//} else { // Medium
				//if(f.name.Find(_T(".exe")) != -1){
				//	GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_exeMedIndex, 0, 0, 0);
				//} else {
					//m_listView.GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_fileMedIndex, 0, 0, 0);
					m_listCtrl.SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_fileMedIndex, 0, 0, 0);
				//}
			//}
		}
	}
}


/**
* AddFilesToList
*
* DEPRICATE
*/
void CHistoryDialog::AddFilesToList(std::vector<CFileContainer> &files)
{
	//m_listView.GetListCtrl().DeleteAllItems();
	m_listCtrl.DeleteAllItems();
	//CListCtrl&  list = m_listView.GetListCtrl();
	int i = 0;
	for(i = 0; i < files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)files[i];

		LVITEM      rItem;
		ZeroMemory ( &rItem, sizeof(LVITEM) );

        rItem.mask    = LVIF_IMAGE | LVIF_TEXT;
        rItem.iItem   = i; // nIndex;
		rItem.pszText = CString(f.name).AllocSysString(); // szNextFile;
        //rItem.iImage  = i; // rFileInfo.iIcon;
	
		int itemIndex = m_listCtrl.InsertItem (  &rItem );

		CRect rect;
		m_listCtrl.GetItemRect(itemIndex, &rect, LVIR_LABEL);

		CString toolTipText(_T(""));
		if(f.dir == 0){
			CString size(_T(""));
			size.Format(_T("%ld"),f.size);
			toolTipText = toolTipText + _T("size: ") + size + CString(_T("  "));
		}
		//toolTipText = toolTipText + _T("permissions: ") + f.permissions;
		//m_tooltip.AddTool(&list, toolTipText, &rect, itemIndex+1);

		if(f.dir){
			m_listCtrl.SetItemText ( i, 1, CString(_T("Folder")).AllocSysString() );
		//	rItem.iImage  = 0;
			//m_listView.GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_folderMedIndex , 0, 0, 0);
			m_listCtrl.SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_folderMedIndex , 0, 0, 0);
		} else {
			// File name
			m_listCtrl.SetItemText ( i, 1, CString(_T("File")).AllocSysString() );

			// File size
			CString fileSize(_T(""));
			fileSize.Format(_T("%ld"), f.size); 
			m_listCtrl.SetItemText ( i, 2, fileSize.AllocSysString() );

			// Permissions
			CString filePermissions(f.permissions);
			m_listCtrl.SetItemText ( i, 3, filePermissions.AllocSysString() );

			//rItem.iImage  = 1;
			//if(m_viewMode == 0 ){ // Large
			//	if(f.name.Find(_T(".exe")) != -1){
			//		GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_exeLargeIndex, 0, 0, 0);
			//	} else { 
			//		// default file
			//		GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_fileLargeIndex, 0, 0, 0);
			//	}
			//} else { // Medium
				//if(f.name.Find(_T(".exe")) != -1){
				//	GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_exeMedIndex, 0, 0, 0);
				//} else {
					//m_listView.GetListCtrl().SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_fileMedIndex, 0, 0, 0);
					m_listCtrl.SetItem(itemIndex, 0, LVIF_IMAGE, NULL, m_fileMedIndex, 0, 0, 0);
				//}
			//}
		}
	}
}


/**
* OnSize
*
* Description: Resize the dialog content.
*/
void CHistoryDialog::OnSize(UINT nType, int cx, int cy)
{
	if(m_listCtrl)
	{
		CRect rect;
		GetClientRect( &rect );
		//CListCtrl& list = m_listView.GetListCtrl();
		//list.MoveWindow(&rect, true); 
		m_listCtrl.MoveWindow(&rect, true);
	}
}
#pragma once
#include <afxcview.h>  // required for CListView
#include <vector>
#include "File.h"
#include "FileTransfer.h"
#include "afxwin.h"
#include "afxcmn.h"

// CHistoryDialog dialog

class CHistoryDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CHistoryDialog)

public:
	CHistoryDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CHistoryDialog();

	std::vector<CFileContainer> m_files;
	std::vector<CFileTransfer> m_transfers;
	CListView m_listView;
	CImageList  * m_ImageListMed;
	int m_folderMedIndex;
	int m_fileMedIndex;
	
	void UpdateFileTransfer(CFileTransfer * t);
	void AddTransfer(CFileContainer & f);
	void AddFilesToList(std::vector<CFileContainer> &files);
	void AddTransfersToList(std::vector<CFileTransfer> &transfers);
	void PostNcDestroy();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_HISTORY_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_listCtrl;
};

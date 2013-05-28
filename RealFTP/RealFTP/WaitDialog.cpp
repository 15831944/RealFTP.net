// WaitDialog.cpp : implementation file
//

#include "stdafx.h"
#include "RealFTP.h"
#include "WaitDialog.h"
#include "afxdialogex.h"


// CWaitDialog dialog

IMPLEMENT_DYNAMIC(CWaitDialog, CDialogEx)

CWaitDialog::CWaitDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CWaitDialog::IDD, pParent)
{
	m_transferCount = 0;
	m_cancel = false;
}

CWaitDialog::~CWaitDialog()
{
	// delete transfers in  transfers list
	// TODO
}

void CWaitDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
}


BEGIN_MESSAGE_MAP(CWaitDialog, CDialogEx)
	ON_MESSAGE(ID_UPDATE_PROGRESS, &CWaitDialog::OnUpdateProgress)
	ON_MESSAGE(ID_PENDING_TRANSFERS, &CWaitDialog::OnPendingTransfers)
	ON_MESSAGE(ID_TRANSFER_DONE, &CWaitDialog::OnTransferDone)
END_MESSAGE_MAP()


// CWaitDialog message handlers


void CWaitDialog::SetProgress(int p)
{
	// divide by number of transfers
	int total = 0;
	int i;
	for(i = 0; i < transfers.size(); i++ )
	{
		CFileTransfer * t;
		t = (CFileTransfer*)transfers[i];
		total = total + t->progress;
	}
	int percentage = (int)(((float)total / (float)transfers.size())) ;
	p = percentage;

	progress = p;
	if(m_progress){
		m_progress.SetPos(p);
	}
}

void CWaitDialog::SetPendingTransfers(int p)
{

}


/**
* OnUpdateProgress
*
* Description: Called by CRealFtpCLient event.
* Param: CFileTransfer allocated object to be deleted.
*/
LRESULT CWaitDialog::OnUpdateProgress(WPARAM wParam, LPARAM lParam)
{
	CFileTransfer * transfer = (CFileTransfer*)lParam;
	if(transfer)
	{
		bool contains = false;
		int i;
		for(i = 0; i < transfers.size(); i++ )
		{
			CFileTransfer * t;
			t = (CFileTransfer*)transfers[i];
			if(t->fileName.Compare(transfer->fileName) == 0)
			{
				contains = true;
			}
		}
		if(!contains)
		{
			CFileTransfer * transferClone = new CFileTransfer(transfer);
			transfers.push_back(transferClone);
		}
		
		TRACE1(" progress: %d \n ", transfer->progress);
		SetProgress((int)transfer->progress);

		// cleanup
		delete transfer;
	}
	return 1;
}


LRESULT CWaitDialog::OnPendingTransfers(WPARAM wParam,LPARAM lParam)
{
	SetPendingTransfers((int)lParam);
	return 1;
}


LRESULT CWaitDialog::OnTransferDone(WPARAM wParam,LPARAM lParam)
{
	// if no more transfers
	OnOK();
	return 1;
}
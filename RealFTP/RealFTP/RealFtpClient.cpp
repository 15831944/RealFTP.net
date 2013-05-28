#include "StdAfx.h"
#include "RealFtpClient.h"
#include "resource.h"
#include "FileTransfer.h"

CRealFtpClient::CRealFtpClient(void)
{
	fileSize = 0;
	m_last_sent_progress = -1;
	m_delete = false;
	m_transfer = NULL;
}

CRealFtpClient::~CRealFtpClient(void)
{
	if(m_transfer)
	{
		delete m_transfer;
	}
}


/**
* ReceiveFileStatus
*
* Description: Handler called with update to transfer.
*/
BOOL CRealFtpClient::ReceiveFileStatus(long bytesReceived){
    //TRACE1(" bytesReceived %d \n", bytesReceived);
	
	int progress = 0;
	if(bytesReceived > 0 && fileSize > 0){
		progress = (int)(((float)bytesReceived / (float)fileSize) * 100);
	}
	if(progress > 100){
		progress = 100;
		TRACE0(" receive file progress > 100 \n");
	}

	if(m_transfer){
		m_transfer->progress = progress;
	}

	//TRACE1(" receiving file: %d  \n", progress);
	
	if(m_last_sent_progress != progress) // Only send a message if it is new 
	{
		//PostMessage( AfxGetMainWnd()->GetSafeHwnd(), ID_UPDATE_PROGRESS, 0, progress); // main window
		CFileTransfer * postTransfer = new CFileTransfer(m_transfer);
		PostMessage( AfxGetMainWnd()->GetSafeHwnd(), ID_UPDATE_PROGRESS, 0, (LPARAM)postTransfer); // main window
	
		if(m_waitDialog){
			//PostMessage( m_waitDialog->GetSafeHwnd(), ID_UPDATE_PROGRESS, 0, progress); // wait dialog
			CFileTransfer * pTransfer = new CFileTransfer(m_transfer);
			PostMessage( m_waitDialog->GetSafeHwnd(), ID_UPDATE_PROGRESS, 0, (LPARAM)pTransfer); // wait dialog
			//Sleep(1);
		}
	}
	m_last_sent_progress = progress;

	BOOL r = CUT_FTPClient::ReceiveFileStatus(bytesReceived);
	//return ptrFTPClient->ReceiveFileStatus(bytesReceived);
	return r;
}


/**
* SendFileStatus - overided
*
* Description: 
*/
BOOL CRealFtpClient::SendFileStatus(long bytesSent){
    //TRACE1(" bytesSent %d \n", bytesSent);

	int progress = 0;
	if(bytesSent > 0 && fileSize > 0){
		progress = (int)(((float)bytesSent / (float)fileSize) * 100);
	}
	if(progress > 100)
	{
		progress = 100;
		TRACE0(" send file progress > 100 \n");
	}

	if(m_transfer){
		m_transfer->progress = progress;
	}

	con->progress = progress;
	//TRACE1(" sending con: %s  \n", con->name);
	//TRACE1(" sending file: %d  \n", progress);

	if(m_last_sent_progress != progress)
	{
		//PostMessage( AfxGetMainWnd()->GetSafeHwnd(), ID_UPDATE_PROGRESS, (WPARAM)con, progress); // main window

		CFileTransfer * postTransfer = new CFileTransfer(m_transfer);
		PostMessage( AfxGetMainWnd()->GetSafeHwnd(), ID_UPDATE_PROGRESS, (WPARAM)con, (LPARAM)postTransfer); // main window
	
		if(m_waitDialog){
			//PostMessage( m_waitDialog->GetSafeHwnd(), ID_UPDATE_PROGRESS, 0, progress); // wait dialog
			CFileTransfer * pTransfer = new CFileTransfer(m_transfer);
			PostMessage( m_waitDialog->GetSafeHwnd(), ID_UPDATE_PROGRESS, 0, (LPARAM)pTransfer); // wait dialog
			//Sleep(1);
		}
	}
	m_last_sent_progress = progress;

	BOOL r = CUT_FTPClient::SendFileStatus(bytesSent);

	return r;
}
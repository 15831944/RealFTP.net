#pragma once
#include "ftp_c.h"
#include "WaitDialog.h"
#include "Connection.h"
#include "FileTransfer.h"

class CRealFtpClient : public CUT_FTPClient
{
public:
	CRealFtpClient(void);
	~CRealFtpClient(void);

	long fileSize;
	CWaitDialog * m_waitDialog;
	CConnection * con;
	CFileTransfer * m_transfer;

	int m_last_sent_progress;
	bool m_delete;

	// CUT_WSClient::ReceiveFileStatus
	virtual BOOL ReceiveFileStatus(long bytesReceived);

	// CUT_WSClient::SendFileStatus
	virtual BOOL SendFileStatus(long bytesSent);

protected:


};
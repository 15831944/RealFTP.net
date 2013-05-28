#pragma once
#include "afx.h"
#include "Connection.h"
#include "File.h"
#include <vector>
#include "uh_ctrl.h"
#include "ut_clnt.h"
#include "ftp_c.h"
//#include "FTPClient.h"
//#include "MainFrm.h"
#include "RealFtpClient.h"
#include "WaitDialog.h"
#include "HistoryDialog.h"

class CFtpManager : public CObject
{
public:
	CFtpManager(void);
	~CFtpManager(void);

	bool m_directoryUpdated;
	bool DirectoryUpdated();
	
	struct FTP_THREADPARAM
	{
		HWND hWnd;
		CFtpManager * c;
		CConnection * con;
		//CUT_FTPClient * ftpClient;
		CRealFtpClient * ftpClient;
		CString localPath;
		CString remotePath;
		long fileSize;
		CWaitDialog * waitDialog;
	};

	CString name;
	CString host;
	CString user;
	CString password;
	CString key;
	
	//CHistoryDialog * m_historyDlg;

public:
	bool Connect(CConnection * con, CUT_FTPClient & ftpClient);
	bool TestConnection(CConnection * con);
	void GetLocalStorePath(CConnection * con, CString & path);

	bool ReadLocalDirectory(CConnection * con, CString &dir, std::vector<CFileContainer> &files);
	bool GetFtpDirectory(CConnection * con, CString &dir, CRealFtpClient * ftpClient=0);
	
	//bool SendFile(CConnection * con, CString & filePath, CString & fileName);
	bool SendFile(CConnection * con, CFileContainer & file, CWaitDialog & waitDialog);
	bool SendFolder(CConnection * con, CFileContainer & file, CWaitDialog & waitDialog);
	
	//bool ReceiveFile(CConnection * con, CString & filePath, CString & fileName); // depricate
	bool ReceiveFile(CConnection * con, CFileContainer & file, CWaitDialog & waitDialog);
	bool ReceiveFolder(CConnection * con, CFileContainer & file, CWaitDialog & waitDialog);
	
	bool IsFolderLocal(CConnection * con, CFileContainer & file);

	bool RenameFile(CConnection * con, CString & filePath, CString & fileName, CString & newName);
	bool ChangeFilePermissions(CConnection * con, CFileContainer & file, CString & permissions, CWaitDialog & waitDialog);

	bool DeleteFile(CConnection * con, CFileContainer & file);
	bool CreateFolder(CConnection * con, CString & path, CString & folder);
	bool DeleteFolder(CConnection * con, CFileContainer & file);

	bool SearchLocalDirectory(CConnection * con, CString &dir, CString & search, std::vector<CFileContainer> &files);

	void SyncServerConnections();
	void SyncServerConnection(CConnection * con);
	void SyncServerFolder(CConnection * con, CRealFtpClient * ftpClient, CString &folder);
	//void SyncServerFolder(CString & path);
	//void ConnectToServer(CConnection * con);
	_int64 Delta(SYSTEMTIME st1, SYSTEMTIME st2);

	FTP_THREADPARAM m_threadParam;
	int m_maxTransfers;
	int m_activeTransfers; // Global static
	//std::vector<CWinThread> & m_transferThreads;

	CWinThread *m_pSendFileThread;
	static UINT SendFileWorker(LPVOID pParam);

	CWinThread *m_pReceiveFileThread;
	static UINT ReceiveFileWorker(LPVOID pParam);
	
protected:
	CUT_FTPClient m_ftpClient;
	CUH_Control m_history;

private:

	//static CUT_FTPClient m_ftpClient;
	//static CUH_Control m_history;

};
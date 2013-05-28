#include "StdAfx.h"
#include "FtpManager.h"
#include "ConnectionManager.h"
#include "Xml.h"
#include "LocalStore.h"

#define HISTORY_ENTRY_SIZE MAX_PATH + 80


CFtpManager::CFtpManager(void)
{
	m_maxTransfers = 3;
	m_activeTransfers = 0;
	m_directoryUpdated = false;
}

CFtpManager::~CFtpManager(void)
{
}


/**
* GetLocalStorePath
*
*/
void CFtpManager::GetLocalStorePath(CConnection * con, CString & path)
{
	TCHAR programPath[_MAX_PATH];
	if(::GetModuleFileName(NULL, programPath, _MAX_PATH))
	{
		path = CString(programPath);
		if(path.ReverseFind('\\') != -1)
		{
			path = path.Mid(0, path.ReverseFind('\\') + 1);
		}

		path = path + CString(_T("files\\"));
		if(GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES){
			CreateDirectory(path, NULL);
		}
		path = path + CString(con->host);
		if(GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES){
			CreateDirectory(path, NULL);
		}
	}
}


/**
* Connect
*
* Description: Given a connection object, connect m_ftpClient.
*/
bool CFtpManager::Connect(CConnection * con, CUT_FTPClient & ftpClient)
{
	bool result = false;
	if(!ftpClient.IsConnected()) // Not connected
	{
		int retcode = 11;
		retcode = ftpClient.FTPConnect(con->host, con->user, con->password, _T(""));
		if(retcode == UTE_SUCCESS)
		{
			/*
			int index = 0;
			size_t size = 0;
			_TCHAR	pBuf[MAX_PATH+1];
			*pBuf = 0;
			while(m_ftpClient.GetMultiLineResponse(pBuf, MAX_PATH, index, &size) == UTE_SUCCESS) {
				index++;
				//history.AddLine(pBuf);
				//TRACE1(" data %s \n", pBuf);
			}
			*/
			result = true;
			ftpClient.SetFireWallMode(TRUE);
		}
		if(retcode == UTE_BAD_HOSTNAME){TRACE0("  UTE_BAD_HOSTNAME \n");}   
		if(retcode == UTE_CONNECT_FAILED){TRACE0("  UTE_CONNECT_FAILED \n");}  
		if(retcode == UTE_NO_RESPONSE){TRACE0("  UTE_NO_RESPONSE \n");}  
		if(retcode == UTE_INVALID_RESPONSE){TRACE0("  UTE_INVALID_RESPONSE \n");}  
		if(retcode == UTE_USER_NA){TRACE0("  UTE_USER_NA \n");}
		if(retcode == UTE_PASS_NA){TRACE0("  UTE_PASS_NA \n");}
		if(retcode == UTE_ACCT_NA){TRACE0("  UTE_ACCT_NA \n");}
	} else {
		result = true;
	}
	return result;
}


/**
* TestConnection
*
* Description: test ftp connection. Used by the conenction dialog.
*/
bool CFtpManager::TestConnection(CConnection * con)
{
	bool result = false;
	CUT_FTPClient ftpClient;
	int retcode = 11;
	retcode = ftpClient.FTPConnect(con->host, con->user, con->password, _T(""));
	if(retcode == UTE_SUCCESS)
	{
		result = true;
	}
	ftpClient.Close();
	return result;
}


/**
* ReadLocalDirectory
*
* Description: Read files folders from directory.
*
* Param: con - describes connection information
*        dir - path. 
*		 files - output list
*
* Return: bool - true. False if file does not exist, has not been read.
*/
bool CFtpManager::ReadLocalDirectory(CConnection * con, CString &dir, std::vector<CFileContainer> &files)
{
	bool result = true;
	if(con->host.GetLength() == 0)
	{
		return false;
	}

	if(dir.GetAt( dir.GetLength() - 1 ) != '/')
	{
		dir = dir + CString(_T("/"));
	}

	CString localStorePath;
	GetLocalStorePath(con, localStorePath);

	// Read files and folders in directory.
	localStorePath = localStorePath + CString(dir);
	localStorePath.Replace('/', '\\');
	CString localStoreSearchPath = CString(localStorePath + CString(_T("*.*")));
	/*
	CFileFind finder; 
	BOOL bWorking = finder.FindFile(localStoreSearchPath);
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if(finder.IsDirectory())
		{
			CFileContainer f;
			f.dir = 1;
			f.name = finder.GetFileName(); //  .GetFileTitle();
			f.path = CString(dir); //finder.GetFilePath();
			f.localPath = CString(localStorePath);
			if(f.name.Trim().Compare(_T(".")) != 0 && f.name.Trim().Compare(_T("..")) != 0){
				//files.push_back(f);
			}
		} 
		// else sync local file
	}
	finder.Close();
	*/

	// Read file 
	CString dirContentFile(localStorePath + _T(".files.rftp"));

	//char nameBuffer[100];
	//sprintf(nameBuffer, "%S", dirContentFile); // CString to char *
	
	CFileStatus status;
	if(CFile::GetStatus(dirContentFile, status))
	{
		CStdioFile file (dirContentFile, CStdioFile::modeRead | CFile::shareDenyNone);
		CString buffer;
		//CString string;
		while (file.ReadString(buffer))
		{
			CXml xml;

			CString type(_T(""));
			xml.GetTag(buffer, CString(_T("t")), type);
			bool isDir = false;
			if(type.Compare(_T("d")) == 0){
				isDir = true;
			}
			

			CString fileName(_T(""));
			xml.GetTag(buffer, CString(_T("n")), fileName);

			
			// size
			long size = 0;
			CString fileSize(_T(""));
			if(xml.GetTag(buffer, CString(_T("s")), fileSize)){
				fileSize = fileSize.Trim();
				if(fileSize.GetLength() > 0){
					size = _wtol(fileSize);
				}
			}

			// Date
			CString fileDate(_T(""));
			xml.GetTag(buffer, CString(_T("d")), fileDate);	
			fileDate = fileDate.Trim();
			

			// permissions
			CString filePermissions(_T(""));
			xml.GetTag(buffer, CString(_T("p")), filePermissions);	
			filePermissions = filePermissions.Trim();

			//TRACE1(" line: %s \n", buffer);
			CFileContainer f;
			if(isDir){
				f.dir = 1;
			} else {
				f.dir = 0;
			}
			f.name = fileName; 
			f.path = CString(dir); 
			f.localPath = CString(localStorePath);
			f.size = size;
			f.remoteDate = CString(fileDate);
			f.host = CString(con->host);
			//TRACE1("filePermissions %s \n " , filePermissions);
			f.permissions = CString(filePermissions);
			
			if(f.name.Trim().Compare(_T(".")) != 0 && f.name.Trim().Compare(_T("..")) != 0 ){
				int i = 0;
				files.push_back(f);
			}
		}
	} 
	else 
	{
		// Directory not read.
		//GetFtpDirectory(con, dir, NULL);
		TRACE1(" ReadLocalDirectory not found: %s \n", dir);
		result = false;
	}
	return result;
}

//bool CFtpManager::compareFiles( std::vector<CFileContainer> &filesA, std::vector<CFileContainer> &filesB )

/**
* GetFtpDirectory
*
* Description: 1) Conect to ftp server, change directory. 
*	2) Get dir listing.
*	3) Read existing listing file, 
*	4) If listing has changed, write new dir listing file.
*
* Parameters:	CConnection - describes ftp connection info, user, host etc.
*				dir - ftp remote directory
*				ftpClient - optional ftpclient. if null, will create, connect and close.
* Return:
*				bool - success
* ??? was it updated?
*/
bool CFtpManager::GetFtpDirectory(CConnection * con, CString &directory, CRealFtpClient * ftpClient)
{
	bool result = false;
	//TRACE1(" con %s \n ", con->name);

	//Connect(con, m_ftpClient);
	//if(!m_ftpClient.IsConnected()) // Not connected
	//{
	//	return;
	//}
	bool closeConnection = false;
	if(!ftpClient)
	{
		ftpClient = new CRealFtpClient();
		closeConnection = true;
	}
	if(!ftpClient->IsConnected()) // Not connected
	{
		int retcode = 11;
		retcode = ftpClient->FTPConnect(con->host, con->user, con->password, _T(""));
		if(retcode == UTE_SUCCESS)
		{
			ftpClient->SetFireWallMode(TRUE);
		}
	}

	//m_ftpClient.SetFireWallMode(TRUE);
	//ftpClient->SetFireWallMode(TRUE);

	CString localStorePath;
	GetLocalStorePath(con, localStorePath);
	localStorePath = localStorePath + directory + CString(_T(""));
	localStorePath.Replace('/', '\\');

	CString dirContentListing(_T(""));
	CString fileListing(_T(""));
	CString folderListing(_T(""));

	CUT_DIRINFO di;
	_TCHAR entry[HISTORY_ENTRY_SIZE];
	_TCHAR dir[MAX_PATH];
	int valRet;
	{
		valRet = ftpClient->ChDir( directory );
		if (valRet != UTE_SUCCESS){
			//m_history.AddLine(CUT_ERR::GetErrorString (valRet));
			TRACE1(" chdir failed: %s \n ", CUT_ERR::GetErrorString(valRet) );
		}
		else 
		{
			int dirInfo = ftpClient->GetDirInfo();

			int count = ftpClient->GetDirInfoCount();
			for(int t=0;t<count;t++){
				ftpClient->GetDirEntry(t, &di);
				// v4.2 dir info now has _TCHAR
				// Format: Name,size,time, time display
				_sntprintf(entry, 
					sizeof(entry)/sizeof(_TCHAR), 
					_T("<n>%s</n><s>%ld</s><d>%2.2d/%2.2d/%2.2d %2.2d:%2.2d</d><p>%d</p><o>%s</o><g>%s</g>"),
					di.fileName, di.fileSize,
					di.year, di.month, di.day, di.hour, di.minute, di.permissions, di.owner, di.group);
					
				//m_history.AddLine(entry);

				if(di.isDir)
				{
					CString folderPath(localStorePath + _T("") + di.fileName );
					if(GetFileAttributes(folderPath) == INVALID_FILE_ATTRIBUTES){
						CreateDirectory(folderPath, NULL);
					}

					CString fileInfo(_T(""));
					fileInfo = fileInfo + CString(_T("<t>d</t>")) + CString(entry) + _T("\n");
					folderListing = folderListing + fileInfo;
				} else {
					// File
					CString fileInfo(_T(""));
					fileInfo = fileInfo + CString(_T("<t>f</t>")) + CString(entry) + _T("\n");
					fileListing = fileListing + fileInfo;
				}
			}
			dirContentListing = CString(folderListing + fileListing);

			
			// write dir descriptor
			// dirContentListing
			CString dirContentFile(localStorePath + _T(".files.rftp"));


			// Check existing files.rftp. Don't rewrite if its the same. SSD safety.
			CString existingListing(_T(""));
			CFileStatus status;
			if(CFile::GetStatus(dirContentFile, status))
			{
				CStdioFile file (dirContentFile, CStdioFile::modeRead | CFile::shareDenyNone);
				CString buffer;
				while (file.ReadString(buffer))
				{
					existingListing = existingListing + buffer + _T("\n");
				}
			}

			
			if(existingListing.Compare(dirContentListing) != 0)
			{
				CFile theFile(dirContentFile, CFile::modeReadWrite | CFile::modeCreate);
				
				char buffer[200000];
				sprintf(buffer, "%S", dirContentListing); // CString to char *
				int len = strlen(buffer);
				//theFile.Write( (LPCTSTR) dirContentListing, dirContentListing.GetLength() * sizeof(TCHAR));
				
				//
				// Error here ***********
				//
				CString buffy(dirContentFile);
				if(buffy.Find(CString(_T("newsletter-form"))) != 1){
					int i  = 0;
				}

				theFile.Write(buffer, len);

				TRACE0(" GetFtpDirectory Updated \n");
				m_directoryUpdated = true;  
			} else {
				//TRACE0(" GetFtpDirectory Allready up to date \n");
				m_directoryUpdated = false;
			}

			//TRACE1(" file: %s \n", entry);
			result = true;
		} // chdir success		
	}
	if(closeConnection){
		ftpClient->Close();
		delete ftpClient;
	}
	return result;
}


bool CFtpManager::DirectoryUpdated()
{
	if(m_directoryUpdated){
		return true;
	}
	return false;
}


/**
* SendFolder
*
* Description: Send a folder and its contents to the ftp server.
*
* Param: con - describes the connection details to send.
*        file - describes the file, path must end in /. name and dir (is dir) are used.
*        waitDialog - message target to send progress updates. May or may not be displayed.
*/
bool CFtpManager::SendFolder(CConnection * con, CFileContainer & file, CWaitDialog & waitDialog)
{
	CString localStorePath;
	GetLocalStorePath(con, localStorePath);

	// Check
	if(file.dir == false)
	{
		return false;
	}

	// Create remote directory
	int r = CreateFolder(con, file.path, file.name);
	if(r == false)
	{
		// Error
		return false;
	}

	// Read files and folders in directory.
	localStorePath = localStorePath + CString(file.path + file.name) + _T("\\");
	localStorePath.Replace('/', '\\');
	CString localStoreSearchPath = CString(localStorePath + CString(_T("*.*")));

	// Read contents of folder
	CFileFind finder; 
	BOOL bWorking = finder.FindFile(localStoreSearchPath);
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if(finder.IsDirectory())
		{
			CFileContainer f;
			f.dir = 1;
			f.name = finder.GetFileName(); 
			f.path = CString(file.path + file.name + _T("/"));
			if(f.name.GetLength() > 0 && f.name.Compare(_T(".")) != 0 && f.name.Compare(_T("..")) != 0)
			{
				SendFolder(con, f, waitDialog);
			}
		} 
		else 
		{
			CFileContainer f;
			f.dir = 0;
			f.name = finder.GetFileName(); 
			f.path = CString(file.path + file.name + _T("/"));
			SendFile(con, f, waitDialog);
		}
	}
	finder.Close();
	return true;
}


/**
* SendFile
*  
* Description: Send a file from the local repository to the ftp server.
*
* Param con - describes the ftp server to send to.
*       file - describes file path and name to copy.
*       waitDialog - notification target.
*
* Return bool - not currently used.
*/
bool CFtpManager::SendFile(CConnection * con, CFileContainer & file, CWaitDialog & waitDialog)
{
	bool result = false;
	
	CString localStorePath;
	GetLocalStorePath(con, localStorePath);
	CString localPath(localStorePath + file.path + file.name);
	localPath.Replace('/', '\\');

	CString destinationPath(file.path + file.name);

	// Get file size
	CFileStatus filestatus;
	CFile::GetStatus( localPath, filestatus );

	//CUT_FTPClient * ftpClient = new CUT_FTPClient();
	CRealFtpClient * ftpClient = new CRealFtpClient();
	ftpClient->fileSize = filestatus.m_size;
	ftpClient->m_waitDialog = &waitDialog;
	ftpClient->con = con;							// depricate -> moved into FileTransfer

	CFileTransfer * transfer = new CFileTransfer();
	transfer->sending = true;
	CConnection * conClone = new CConnection(con);
	transfer->connection = conClone;
	transfer->fileName = file.name;
	ftpClient->m_transfer = transfer;

	// increment files being transfered count
	if(waitDialog){
		// inc
	}

	FTP_THREADPARAM * _param = new FTP_THREADPARAM;
	_param->c = this;
	_param->con = con;
	_param->localPath = localPath;
	_param->remotePath = destinationPath;
	_param->ftpClient = ftpClient;
	_param->fileSize = filestatus.m_size;
	_param->waitDialog = &waitDialog;

	//CWinThread *pSendFileThread;
	m_pSendFileThread = AfxBeginThread(CFtpManager::SendFileWorker, _param); 
	if (m_pSendFileThread != NULL)
	{
		//TRACE(_T("AfxBeginThread: 0x%08lX\n"), m_pSendFileThread->m_nThreadID);
		//m_transferThreads->push_back(pSendFileThread);
	}
	return result;
}


UINT CFtpManager::SendFileWorker(LPVOID pParam)
{
	CFtpManager::FTP_THREADPARAM *const pThreadParam = static_cast<CFtpManager::FTP_THREADPARAM *>(pParam);
	//pThreadParam->c->Connect(pThreadParam->con, pThreadParam->ftpClient);
	
	if(!pThreadParam->ftpClient->IsConnected()) // Not connected
	{
		int retcode = 11;
		retcode = pThreadParam->ftpClient->FTPConnect(pThreadParam->con->host, pThreadParam->con->user, pThreadParam->con->password, _T(""));
		if(retcode == UTE_SUCCESS)
		{
			pThreadParam->ftpClient->SetFireWallMode(TRUE);
		}
	}
	
	if(!pThreadParam->ftpClient->IsConnected()) // Not connected
	{
		return false;
	}

	int s = pThreadParam->ftpClient->SendFile(pThreadParam->localPath, pThreadParam->remotePath);
	TRACE1(" Done  %d \n", s);

	if(s == UTE_SUCCESS)
	{
		//result = true;
	}
    if(s == UTE_SVR_DATA_CONNECT_FAILED){TRACE0("  UTE_SVR_DATA_CONNECT_FAILED \n");}   
    if(s == UTE_PORT_FAILED ){TRACE0("  UTE_PORT_FAILED \n");}
    if(s == UTE_RETR_FAILED) {TRACE0("  UTE_RETR_FAILED \n");}
    if(s == UTE_CONNECT_TERMINATED) {TRACE0("  UTE_CONNECT_TERMINATED \n");}         
    if(s == UTE_STOR_FAILED) {TRACE0("  UTE_STOR_FAILED \n");}                 
    if(s == UTE_CONNECT_TIMEOUT) {TRACE0("  UTE_CONNECT_TIMEOUT \n");}

	// Notify wait dialog - transfer done
	if(pThreadParam->waitDialog){
		PostMessage( pThreadParam->waitDialog->GetSafeHwnd(), ID_TRANSFER_DONE, 0, 1);
	}

	// Cleanup
	pThreadParam->ftpClient->Close();

	//if( NULL != pThreadParam->ftpClient && NULL != pThreadParam->ftpClient->m_transfer)
	//	delete pThreadParam->ftpClient->m_transfer; // now done in RealFtpClient destructor

	if ( NULL != pThreadParam->ftpClient )
        delete pThreadParam->ftpClient;

	if ( NULL != pThreadParam )
		delete pThreadParam; 

	return 1;
}



/**
* IsFolderLocal
*
* Description: Determine if a folder is local and up to date. If it is not it may 
*   need to be downloaded.
*/
bool CFtpManager::IsFolderLocal(CConnection * con, CFileContainer & file) 
{
	bool result = true;

	CString localStorePath;
	GetLocalStorePath(con, localStorePath);
	CString localPath(localStorePath + file.path + file.name);
	localPath.Replace('/', '\\');

	CFtpManager ftp;

	// Read directory information, process subfolders.
	std::vector<CFileContainer> files;
	bool r = ftp.ReadLocalDirectory(con, CString( file.path + file.name + _T("/") ), files);
	if(r == false)  // failed because it hasn't been downloaded yet.
	{
		ftp.GetFtpDirectory(con, CString( file.path + file.name + _T("/") ), NULL);
		ftp.ReadLocalDirectory(con, CString( file.path + file.name + _T("/") ), files);
	}
	

	int i = 0;
	for(i = 0; i < files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)files[i];
		bool descend = true; 

		if(f.dir) // dir
		{
			//CString subFolder(CString(f.path + f.name + _T("/")));
			
			result = IsFolderLocal(con, f);
		} 
		else   // file
		{
			CString sFile;
			sFile = CString(localStorePath + f.path + f.name);
			sFile.Replace('/', '\\');
			if ( 0xFFFFFFFF == GetFileAttributes ( sFile ) && GetLastError() == ERROR_FILE_NOT_FOUND )
			{

				return false;
			}
		}		
	}
	return result;
}


/**
* ReceiveFolder
*
* Description: Download a folder and all of its contents to the local store.
*
* Param: con - describes the ftp server to connect to.
*		 file - describes the folder path + (folder)name to download.
*		 waitDialog - notification recipient.
*
* Return: bool - TODO...
*/
bool CFtpManager::ReceiveFolder(CConnection * con, CFileContainer & file, CWaitDialog & waitDialog) 
{
	bool result = false;
	
	CString localStorePath;
	GetLocalStorePath(con, localStorePath); // could always use file.localPath
	
	CString localStoreFolder = localStorePath + CString(file.path + file.name) + _T("\\");
	localStoreFolder.Replace('/', '\\');

	// Check
	if(file.dir == false)
	{
		return false;
	}

	// Create local folder
	WIN32_FIND_DATA  data; 
	//HANDLE handle = FindFirstFile(localStoreFolder, &data); 
	//if ( handle == INVALID_HANDLE_VALUE )
	if( GetFileAttributes(localStoreFolder) == INVALID_FILE_ATTRIBUTES )
	{
		int i = 0;
		CreateDirectory(localStoreFolder, NULL);
	}

	CFtpManager ftp;
	std::vector<CFileContainer> files;
	bool r = ftp.ReadLocalDirectory(con, CString(file.path + file.name + _T("/")) , files);
	if(r == false) // directory information not present.
	{
		GetFtpDirectory(con, CString(file.path + file.name + _T("/")), NULL);
		ftp.ReadLocalDirectory(con, CString(file.path + file.name + _T("/")) , files);
	}

	int i = 0;
	for(i = 0; i < files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)files[i];

		if(f.dir)
		{

			ReceiveFolder(con, f, waitDialog);
		}
		else 
		{
			// does the file exist, and is it current?
			bool download = false;
			CString localFile(localStorePath + f.path + f.name);
			localFile.Replace('/', '\\');
			if ( 0xFFFFFFFF == GetFileAttributes ( localFile ) && GetLastError() == ERROR_FILE_NOT_FOUND )
			{
				download = true;
			}

			if(download)
			{
				ReceiveFile(con, f, waitDialog);
			}
		}
	}
	return result;
}


/**
* ReceiveFile
*
* Description: Starts a receive transfer in a new thread. If multiple 
*  file transfers are requested in the same operation the same waitDialog is used to track them.
*/
bool CFtpManager::ReceiveFile(CConnection * con, CFileContainer & file, CWaitDialog & waitDialog) 
{
	bool result = false;

	CString localStorePath;
	GetLocalStorePath(con, localStorePath);
	CString localPath(localStorePath + file.path + file.name);
	localPath.Replace('/', '\\');

	CString remotePath(file.path + file.name);

	CRealFtpClient * ftpClient = new CRealFtpClient();
	ftpClient->fileSize = file.size;
	ftpClient->m_waitDialog = &waitDialog;
	ftpClient->con = con;

	CFileTransfer * transfer = new CFileTransfer();
	transfer->sending = false;
	CConnection * conClone = new CConnection(con);
	transfer->connection = conClone;
	transfer->fileName = file.name;
	ftpClient->m_transfer = transfer;

	FTP_THREADPARAM * _param = new FTP_THREADPARAM;
	_param->c = this;
	_param->con = con;
	_param->localPath = localPath;
	_param->remotePath = remotePath;
	_param->ftpClient = ftpClient;
	_param->fileSize = file.size; 
	_param->waitDialog = &waitDialog;
	
	m_pReceiveFileThread = AfxBeginThread(CFtpManager::ReceiveFileWorker, _param); 
	if (m_pReceiveFileThread != NULL)
	{
		//TRACE(_T("AfxBeginThread: 0x%08lX\n"), m_pReceiveFileThread->m_nThreadID);
		result = true;
	}

	return result;
}

UINT CFtpManager::ReceiveFileWorker(LPVOID pParam)
{
	CFtpManager::FTP_THREADPARAM *const pThreadParam = static_cast<CFtpManager::FTP_THREADPARAM *>(pParam);
	//CFtpManager::
	pThreadParam->c->m_activeTransfers++;
	//while(pThreadParam->c->m_activeTransfers > 3)
	//{
	//	Sleep(50);
	//}

	if(!pThreadParam->ftpClient->IsConnected()) // Not connected
	{
		int retcode = 11;
		retcode = pThreadParam->ftpClient->FTPConnect(pThreadParam->con->host, pThreadParam->con->user, pThreadParam->con->password, _T(""));
		if(retcode == UTE_SUCCESS)
		{
			pThreadParam->ftpClient->SetFireWallMode(TRUE);
		}
	}
	
	if(!pThreadParam->ftpClient->IsConnected()) // Not connected
	{
		pThreadParam->c->m_activeTransfers--;
		return false;
	}

	int r = pThreadParam->ftpClient->ReceiveFile(pThreadParam->remotePath, pThreadParam->localPath);
	if(r == UTE_SUCCESS)
	{
		//result = true;
	}

	// Notify wait dialog - transfer done
	if(pThreadParam->waitDialog){
		PostMessage( pThreadParam->waitDialog->GetSafeHwnd(), ID_TRANSFER_DONE, 0, 1);
	}

	pThreadParam->c->m_activeTransfers--;

	// Cleanup
	//pThreadParam->ftpClient->Close();
	
	//if( NULL != pThreadParam->ftpClient && NULL != pThreadParam->ftpClient->m_transfer)
	//	delete pThreadParam->ftpClient->m_transfer;  // deleted in RealFtpClient destructor

	if ( NULL != pThreadParam->ftpClient )
        delete pThreadParam->ftpClient;

	if ( NULL != pThreadParam )
		delete pThreadParam; 
	
	return 1;
}

/*
bool CFtpManager::ReceiveDone(CString & transaction)
{

	return false;
}
*/


/**
* RenameFile
*
* Description: 
*/
bool CFtpManager::RenameFile(CConnection * con, CString & filePath, CString & fileName, CString & newName)
{
	bool result = false;
	if(!m_ftpClient.IsConnected())
	{
		int retcode = 11;
		retcode = m_ftpClient.FTPConnect(con->host, con->user, con->password, _T(""));
		if(retcode == UTE_SUCCESS)
		{
			
		}
	}
	m_ftpClient.SetFireWallMode(TRUE);

	//CString localStorePath;
	//GetLocalStorePath(con, localStorePath);
	//CString localPath(localStorePath + filePath + fileName);
	//localPath.Replace('/', '\\');

	CString remotePath(filePath + fileName);
	CString renameToPath(filePath + newName);

	int r = m_ftpClient.RenameFile(remotePath, renameToPath);
	if(r == UTE_SUCCESS)
	{
		result = true;
	}

	if(r == UTE_NO_RESPONSE) {TRACE0(" Error: UTE_NO_RESPONSE \n");}
	if(r == UTE_RNFR_NA) {TRACE0(" Error: UTE_RNFR_NA \n");}
	if(r == UTE_RNTO_NA) {TRACE0(" Error: UTE_RNTO_NA \n");}
	
	return result;
}


/**
* ChangeFilePermissions
*
*/
bool CFtpManager::ChangeFilePermissions(CConnection * con, CFileContainer & file, CString & permissions, CWaitDialog & waitDialog)
{
	bool result = false;
	if(!m_ftpClient.IsConnected())
	{
		int retcode = 11;
		retcode = m_ftpClient.FTPConnect(con->host, con->user, con->password, _T(""));
		if(retcode == UTE_SUCCESS)
		{
			
		}
	}
	m_ftpClient.SetFireWallMode(TRUE);

	//CString localStorePath;
	//GetLocalStorePath(con, localStorePath);
	//CString localPath(localStorePath + filePath + fileName);
	//localPath.Replace('/', '\\');

	CString remotePath(file.path + file.name);
	//CString renameToPath(file.path + new.name);

	int r = m_ftpClient.ChmodFile( remotePath, permissions );
	if(r == UTE_SUCCESS)
	{
		result = true;
	}

	if(r == UTE_NO_RESPONSE) {TRACE0(" Error: UTE_NO_RESPONSE \n");}
	//if(r == UTE_RNFR_NA) {TRACE0(" Error: UTE_RNFR_NA \n");}
	//if(r == UTE_RNTO_NA) {TRACE0(" Error: UTE_RNTO_NA \n");}
	
	return result;
}



/**
* DeleteFile
*
* Description: Delete a file from the local system and remote FTP server.
*				TODO: multi file select
* Parameters: CFileContainer  contains identification for file to be removed
*
* Returns:    bool true if deleted, false if not deleted.
*/
bool CFtpManager::DeleteFile(CConnection * con, CFileContainer & file)
{
	bool result = false;
	
	CString localStorePath;
	GetLocalStorePath(con, localStorePath);
	CString localPath(localStorePath + file.path + file.name);
	localPath.Replace('/', '\\');

	CString remotePath(file.path + file.name);

	//TRACE2("localPath %s  remotePath %s  \n", localPath, remotePath);

	// Delete from FTP server
	if(!m_ftpClient.IsConnected())
	{
		int retcode = 11;
		retcode = m_ftpClient.FTPConnect(con->host, con->user, con->password, _T(""));
		if(retcode == UTE_SUCCESS)
		{
			
		}
	}
	m_ftpClient.SetFireWallMode(TRUE);
	int r = m_ftpClient.DeleteFileW(remotePath);
	if(r == UTE_SUCCESS)
	{
		result = true;
	}

	// Delete from local disk
	CFile::Remove(localPath);

	// Reload dir listing info from server
	GetFtpDirectory(con, file.path);

	return result;
}


/**
* CreateFolder
*
*/
bool CFtpManager::CreateFolder(CConnection * con, CString & path, CString & folder)
{
	bool result = false;
	
	CString remotePath(path + folder);

	if(!m_ftpClient.IsConnected())
	{
		int retcode = 11;
		retcode = m_ftpClient.FTPConnect(con->host, con->user, con->password, _T(""));
		if(retcode == UTE_SUCCESS)
		{
			
		}
	}
	m_ftpClient.SetFireWallMode(TRUE);
	int r = m_ftpClient.MkDir(remotePath);
	if(r == UTE_SUCCESS)
	{
		result = true;
	}
	// Allready exists = 10
	if(r == 10){
		result = true;
	}

	return result;
}


/**
* DeleteFolder
*
* Description: FAILS .... fix this
*/
bool CFtpManager::DeleteFolder(CConnection * con, CFileContainer & file)
{
	bool result = false;
	CString folderPath(file.path + file.name);
	if(!m_ftpClient.IsConnected())
	{
		int retcode = 11;
		retcode = m_ftpClient.FTPConnect(con->host, con->user, con->password, _T(""));
		if(retcode == UTE_SUCCESS)
		{
			
		}
	}
	m_ftpClient.SetFireWallMode(TRUE);
	int r = m_ftpClient.RmDir(folderPath);
	if(r == UTE_SUCCESS)
	{
		result = true;
	}
	if(r == UTE_NO_RESPONSE)
	{
		TRACE1(" Delete Folder %s Failed: No Response \n", folderPath);
	}
	if(r == UTE_SVR_REQUEST_DENIED)
	{
		TRACE1(" Delete Folder %s Failed: Request Denied \n", folderPath);
	}
	return result;
}



/**
* SearchLocalDirectory
*
* Description: Search a directory for files and folders containing a search string.
*/
bool CFtpManager::SearchLocalDirectory(CConnection * con, CString &dir, CString & search, std::vector<CFileContainer> &files)
{
	TRACE1("search: %s \n", dir);
	CFtpManager ftp;
	std::vector<CFileContainer> dir_files;
	bool r = ftp.ReadLocalDirectory(con, dir, dir_files);
	if(r == false)  // failed because it hasn't been downloaded yet.
	{
	//	ftp.GetFtpDirectory(con, dir, NULL);
	//	ftp.ReadLocalDirectory(con, dir, dir_files);
	}
	
	int i = 0;
	for(i = 0; i < dir_files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)dir_files[i];
		bool descend = true; 

		if( f.name.Find(search) != -1 )
		{
			files.push_back(f);
		}

		if(f.dir) // recurse directory
		{	
			SearchLocalDirectory(con, CString( f.path + f.name + _T("/") ), search, files);
		} 
	}
	return true;
}



/**
* SyncServerConnections
*
*/
void CFtpManager::SyncServerConnections()
{
	CConnectionManager connectionManager;
	connectionManager.Load();
	CObArray * listOfConnections;
	listOfConnections = connectionManager.GetConnections();
	int i = 0;
	for(i = 0; i <  listOfConnections->GetSize(); i++){
		CConnection * con = (CConnection *)listOfConnections->GetAt(i);
		if(con->host.GetLength() > 0)
		{
			SyncServerConnection(con);
		}
	}
}


/**
* SyncServerConnection
*
*/
void CFtpManager::SyncServerConnection(CConnection * con)
{
	CRealFtpClient * ftpClient = new CRealFtpClient();
	SyncServerFolder(con, ftpClient, CString(_T("/")));
	delete ftpClient;
}


void CFtpManager::SyncServerFolder(CConnection * con, CRealFtpClient * ftpClient, CString &folder)
{
	//TRACE2(" Synch folder %s  %s \n", con->name, folder);

	// Download directory information
	CFtpManager ftp;
	bool r = ftp.GetFtpDirectory(con, folder, ftpClient);
	if(!r){return;}

	CLocalStore localStore;
	bool local_store = false; 
	CFileContainer dir;
	dir.host = CString(con->host);
	dir.path = CString(folder);
	dir.name = CString(_T(""));
	local_store = localStore.IsFolderStored(dir);
	
	//TRACE2(" folder: %s  l: %d  \n ", folder, local_store);
	
	// Read directory information, process subfolders.
	std::vector<CFileContainer> files;
	ftp.ReadLocalDirectory(con, folder, files);

	int i = 0;
	for(i = 0; i < files.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)files[i];
		bool descend = true; 
		bool download = false;
		bool upload = false;
		//TRACE1("   name %s \n", f.name);
		// If store local, sync.... TODO
		if(f.dir == 0 && local_store)  // is file and local store
		{
			CFileStatus status;
			if(CFile::GetStatus(f.localPath + f.name, status) == 0) // not found, not local
			{
				download = true; 
			} else {  // File exists
				ULONGLONG local_size = status.m_size;

				// is local file stale?
				CString fileDate = f.remoteDate; // 2012/11/23 15:00
				COleDateTime myDtTime;
				CTime sysTime;
				if(myDtTime.ParseDateTime(fileDate))
				{
					SYSTEMTIME st_remote;
					if(myDtTime.GetAsSystemTime(st_remote))
					{
						sysTime = st_remote;
					}
					CTime ct_local = status.m_mtime;
					SYSTEMTIME st_local;
					ct_local.GetAsSystemTime(st_local);

					//DWORD x = ft_remote->
					_int64 d = Delta(st_local, st_remote);  //  
					d = d / 10000000;
					//TRACE1(" d:  %ld \n", d);  // -200859136
					//   105568  1 day - 
					if(d > 86400)  // remote file was updated a day later than the local. 
					{
						download = true;
					}

					//TRACE2(" size: %d : %d \n", f.size, local_size);
					if(d < 86400 && f.size != local_size)  // local file updated a day after the remote file.
					{
						upload = true;
					}
				}
			}
		}
		if(download)
		{
			CWaitDialog wait;
			ftp.ReceiveFile(con, f, wait); 
		}

		// if file is local but not remote send it
		if(upload)
		{
			CWaitDialog wait;
			//ftp.SendFile(con, f, wait);
		}

		// If older than a year skip, unless local store
		long age = f.GetAge();
		//TRACE2(" date %s   age: %d \n", f.remoteDate, age);
		if(age > 1000){  // this only applies if not local store directory
			descend = false;  
		}

		if(f.path.Compare(_T("/proc/")) == 0){
			descend = false;
		}
		if(f.path.Compare(_T("/sys/")) == 0){
			descend = false;
		}

		if(f.dir && descend)
		{
			CString subFolder(CString(f.path + f.name + _T("/")));
			SyncServerFolder(con, ftpClient, subFolder);

			if(f.path.Find(_T("newsletter-form")) != -1){
				int i = 0;
			}
		}
	}
}




_int64 CFtpManager::Delta(SYSTEMTIME st1, SYSTEMTIME st2)
{
    union timeunion {
        FILETIME fileTime;
        ULARGE_INTEGER ul;
    } ;
    
    timeunion ft1;
    timeunion ft2;

    SystemTimeToFileTime(&st1, &ft1.fileTime);
    SystemTimeToFileTime(&st2, &ft2.fileTime);

    return ft2.ul.QuadPart - ft1.ul.QuadPart;
}
//=================================================================
//  class: CUH_DataManager, CUT_UserManager
//  File:  fileman.cpp
// 
//  Purpose:
//
//  The CUT_DataManager class abstracts the email file storage from
//  the SMTP and POP3 protocols. This allows the storage method
//  to be easily changed without effecting the mail protocols.
//
//=================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
//=================================================================

#ifdef _WINSOCK_2_0_
#define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
/* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include <stdio.h>
#include "ut_srvr.h"
#include "fileman.h"
#include "UT_MailServer.h"

#ifdef UT_DISPLAYSTATUS
#include "uh_ctrl.h"
extern CUH_Control  *ctrlHistory;
#endif 

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

// these do not exist in core VC6 
// these do not exist in core VC6 
#if _MSC_VER < 1400
#if !defined ULONG_PTR
#define ULONG_PTR DWORD
#define LONG_PTR DWORD
#endif
#endif

/*****************************************
CUT_DataManager
Constructor. 
Params
ptrMailServer   - Mail Server class
Return
none
******************************************/
CUT_DataManager::CUT_DataManager(CUT_MailServer &ptrMailServer) :
m_listFileHandle(NULL),             // Initialize pointer to the file handle list
m_ptrMailServer(&ptrMailServer)     // Initialize pointer to the Mail Server class
{
}

/*****************************************
~CUT_DataManager
Destructor. 
Params
none
Return
none
******************************************/
CUT_DataManager::~CUT_DataManager()
{
}

/*****************************************
Que_AddFileHandle
Adds a file handle and name to a linked list
Params
none
Return
none
******************************************/
void CUT_DataManager::Que_AddFileHandle(long handle, LPCTSTR fileName) {

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	// Add item to the list
	m_listFileHandle.push_back(CUT_QUEFileHandle(handle, fileName));
}

/*****************************************
Que_GetFileName
Returns a filename from its handle, the 
name is retrieved from a linked list, see
the above function
******************************************/
LPCTSTR CUT_DataManager::Que_GetFileName(long handle){

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	// Find file handle in the list
	QUEUE_FILE_LIST::iterator       index;
	for(index = m_listFileHandle.begin(); index != m_listFileHandle.end() ; ++index) 
		if(index->m_lHandle == handle) 
			return index->m_szFileName;

	return NULL;
}

/*****************************************
Que_RemoveFileHandle
Removes the given handle from the file handle
linked list
******************************************/
void CUT_DataManager::Que_RemoveFileHandle(long handle) {

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	// Find file handle in the list and remove it
	QUEUE_FILE_LIST::iterator       index;
	for(index = m_listFileHandle.begin(); index != m_listFileHandle.end() ; ++index) 
		if(index->m_lHandle == handle) {
			// Remove item from the list
			m_listFileHandle.erase(index);
			break;
		}
}

/*****************************************
Que_OpenFile
-Opens a file from the QUE
-This function also checks for retries and
the retry time interval, therefore this funciton
may not open a file if one exists
-returns a file handle as a generic long value
******************************************/
long CUT_DataManager::Que_OpenFile(){

	WIN32_FIND_DATA wfd;    
	long            rt = -1;

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	// Find the first available file in the que
	_TCHAR path[WSS_BUFFER_SIZE + 1], file[WSS_BUFFER_SIZE + 1];

	_tcscpy(path,m_ptrMailServer->m_szRootPath);
	_tcscat(path,_T("QUE\\*.*"));


	HANDLE handle = FindFirstFile(path,&wfd);
	while(handle != INVALID_HANDLE_VALUE) {
		if(wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
			_tcscpy(file,m_ptrMailServer->m_szRootPath);
			_tcscat(file,_T("QUE\\"));
			_tcscat(file,wfd.cFileName);
			HANDLE hFile = CreateFile(file,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
			if(hFile !=  INVALID_HANDLE_VALUE) {

				// Read the time and retry count
				DWORD   numRetries = 0;
				DWORD   readLen = 0;
				time_t  tm = time(NULL);
				BOOLEAN closeFile = FALSE;

				// Read the time
				if(ReadFile(hFile,(LPBYTE)&tm,sizeof(time_t),&readLen,NULL)) {

					// Read the retry count
					if(ReadFile(hFile,(LPBYTE)&numRetries,sizeof(DWORD),&readLen,NULL)) {

						// Check to see if the file is being retried
						if(numRetries > 0) {
							time_t currentTime;
							time(&currentTime);
							currentTime -= m_ptrMailServer->m_dwRetryInterval;
							if(tm > currentTime )
								closeFile = TRUE;
						}

					}
					else {
						m_ptrMailServer->OnStatus(_T("ERROR: File Read Error"));
					}   

				}
				else {
					m_ptrMailServer->OnStatus(_T("ERROR: File Read Error"));
				}


				if(!closeFile) {
					rt = (long)(ULONG_PTR)hFile;
					Que_AddFileHandle(rt,file);

					m_ptrMailServer->OnStatus(_T("QueOpenFile"));
					m_ptrMailServer->OnStatus(file);

					FindClose(handle);

					break;
				}
				else
					CloseHandle(hFile);
			}
		}

		if(FindNextFile(handle,&wfd) != TRUE) {
			FindClose(handle);
			break;
		}
	}
	return rt;
}

/*****************************************
Que_CreateFile
-creates a new file for writing to in the QUE
-returns a file handle as a generic long value
(which may be INVALID_FILE_HANDLE)
******************************************/
long CUT_DataManager::Que_CreateFile(){


	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	// Find the first available filename in the que
	long filename = 0;
	long number = 0;
	_TCHAR path[WSS_BUFFER_SIZE + 1],file[WSS_BUFFER_SIZE + 1];

	_tcscpy(path,m_ptrMailServer->m_szRootPath);
	_tcscat(path,_T("QUE"));

	// Check if the path exists 
	CreateDirectory(path, NULL);
	_tcscat(path,_T("\\*.*"));


	WIN32_FIND_DATA wfd;
	HANDLE          handle = FindFirstFile(path,&wfd);

	while(handle != INVALID_HANDLE_VALUE) {
		if(wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
			number = _ttol(wfd.cFileName);
			if(number > filename)
				filename =number;
		}
		if(FindNextFile(handle,&wfd) != TRUE) {
			FindClose(handle);
			break;
		}
	}

// v4.2 removed loop - caused ureachable code warning
//	do {

	HANDLE hFile;

	filename++;
	_sntprintf(file,sizeof(file)/sizeof(_TCHAR)-1,_T("%sQue\\%06ld"),m_ptrMailServer->m_szRootPath,filename);
	hFile = CreateFile(file,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
	if(hFile !=  INVALID_HANDLE_VALUE) {
		Que_AddFileHandle((long)(ULONG_PTR)hFile,file);

		m_ptrMailServer->OnStatus(_T("QueCreateFile"));
		m_ptrMailServer->OnStatus(file);

	}
	else {

		LPVOID lpMsgBuf;
		FormatMessage(     FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,    0,    NULL );

		m_ptrMailServer->OnStatus(_T("Open File Error ***********"));
		m_ptrMailServer->OnStatus((LPCTSTR) lpMsgBuf);
		LocalFree( lpMsgBuf );

	}

	return (long)(ULONG_PTR)hFile;

//	} while(filename > 0);

//	m_ptrMailServer->OnStatus(_T("Open File Error ***********"));
//	return -1;                      // should never happen...
}

/*****************************************
Que_CloseFile
-Closes an open QUE file
-if the delete flag is set then the file
is also deleted from the que
-the sendFailure flag is set if the mail
could not be sent or returned
******************************************/
int  CUT_DataManager::Que_CloseFile(long fileHandle,BOOLEAN deleteFlag){

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	int rt = CUT_ERROR;

	m_ptrMailServer->OnStatus(_T("QueCloseFile"));
	if(CloseHandle((HANDLE)(ULONG_PTR)fileHandle)) {

		m_ptrMailServer->OnStatus(Que_GetFileName(fileHandle));

		if(deleteFlag) {
			_TCHAR *name = (LPTSTR)Que_GetFileName(fileHandle);
			if(name != NULL) {
				DeleteFile(name);

				m_ptrMailServer->OnStatus(_T("QueDeleteFile"));
				m_ptrMailServer->OnStatus(name);
			}
		}
		rt = CUT_SUCCESS;
	}

	Que_RemoveFileHandle(fileHandle);

	return rt;
}

/*****************************************
Que_ReadFile
-reads bytes from the given file
-the return value is the number of bytes read
******************************************/
int CUT_DataManager::Que_ReadFile(long fileHandle,LPBYTE buf,int bufLen){

	DWORD readLen;

	if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,buf,bufLen,&readLen,NULL))
		return (int)readLen;
	return FALSE;
}

/*****************************************
Que_ReadFileHeader
-reads the header information from the given
file
-header information includes, to, from, 
write time, retry count, and if the mail is
being returned
-returns 0 for success
******************************************/
int CUT_DataManager::Que_ReadFileHeader(long fileHandle,LPSTR to,int maxToLen,LPSTR from,
										int maxFromLen,time_t* tm,DWORD *numRetries,DWORD * returnMail){

											DWORD   readLen;
											int     error = CUT_SUCCESS;
											DWORD   len = 0;

											// Set the file pointer to the start of the file
											if( SetFilePointer((HANDLE)(ULONG_PTR)fileHandle,0,NULL,FILE_BEGIN) != 0)
												return 64;

											// Read the time
											if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,tm,sizeof(time_t),&readLen,NULL) != TRUE)
												error |=1;

											// Read the retry count
											if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,numRetries,sizeof(DWORD),&readLen,NULL) != TRUE)
												error |=2;

											// Read the returnMail flag
											if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,returnMail,sizeof(DWORD),&readLen,NULL) != TRUE)
												error |=2;

											// Read the to string
											if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,&len,sizeof(DWORD),&readLen,NULL) != TRUE)
												error |=4;
											else {
												if(len > (DWORD)maxToLen)
													error |=8;
												else
													if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,to,len,&readLen,NULL) != TRUE)
														error |=16;
											}

											// Read the from string
											if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,&len,sizeof(DWORD),&readLen,NULL) != TRUE)
												error |=32;
											else {
												if(len > (DWORD)maxFromLen)
													error |=64;
												else
													if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,from,len,&readLen,NULL) != TRUE)
														error |=128;
											}
											return error;
}

/*****************************************
Que_WriteFile
-writes the given data to an open QUE file
-returns the number of bytes written
******************************************/
int CUT_DataManager::Que_WriteFile(long fileHandle,LPBYTE buf,int bufLen){

	DWORD writeLen;

	if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,buf,bufLen,&writeLen,NULL))
		return (int)writeLen;
	else
		MessageBeep((UINT)-1);
	return FALSE;
}

/*****************************************
Que_WriteFileHeader
-writes a file header to a QUE file
-header items include to, form, retry count and
if the mail is being returned
-returns 0 for success
******************************************/
int CUT_DataManager::Que_WriteFileHeader(long fileHandle,LPCSTR to,LPCSTR from,
										 DWORD numRetries,DWORD returnMail){

											 DWORD   writeLen;
											 int     error = CUT_SUCCESS;
											 DWORD   len;

											 // Get the current time
											 time_t tm;
											 time(&tm);

											 // Set the file pointer to the start of the file
											 if( SetFilePointer((HANDLE)(ULONG_PTR)fileHandle,0,NULL,FILE_BEGIN) != 0)
												 return 128;

											 // Write the time
											 if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,(LPBYTE)&tm,sizeof(time_t),&writeLen,NULL) != TRUE)
												 error |=1;

											 // Write the retry count
											 if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,&numRetries,sizeof(DWORD),&writeLen,NULL) != TRUE)
												 error |=2;

											 // Write the returnMail flag
											 if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,&returnMail,sizeof(DWORD),&writeLen,NULL) != TRUE)
												 error |=4;

											 // Write the to string
											 len = (DWORD)strlen(to)+1;

											 m_ptrMailServer->OnStatus( to );

											 if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,&len,sizeof(DWORD),&writeLen,NULL) != TRUE)
												 error |=8;
											 if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,to,len,&writeLen,NULL) != TRUE)
												 error |=16;

											 // Write the from string
											 len = (DWORD)strlen(from)+1;
											 if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,&len,sizeof(DWORD),&writeLen,NULL) != TRUE)
												 error |=32;
											 if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,from,len,&writeLen,NULL) != TRUE)
												 error |=64;

											 return error;
}

/******************************************
******************************************/
int CUT_DataManager::Que_WriteFileHeader(long fileHandle,DWORD numRetries,DWORD returnMail){

	DWORD   writeLen;
	int     error = CUT_SUCCESS;

	// Get the current time
	time_t tm;
	time(&tm);

	// Set the file pointer to the start of the file
	if( SetFilePointer((HANDLE)(ULONG_PTR)fileHandle,0,NULL,FILE_BEGIN) != 0)
		return 128;

	// Write the time
	if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,(LPBYTE)&tm,sizeof(time_t),&writeLen,NULL) != TRUE)
		error |=1;

	// Write the retry count
	if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,&numRetries,sizeof(DWORD),&writeLen,NULL) != TRUE)
		error |=2;

	// Write the returnMail flag
	if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,&returnMail,sizeof(DWORD),&writeLen,NULL) != TRUE)
		error |=4;

	return error;
}

/*****************************************
Que_CarbonCopyFile
- copies a QUE file to another QUE file but
changes the TO: name to the given name
******************************************/
int  CUT_DataManager::Que_CarbonCopyFile(long origFileHandle,LPCSTR toAddr){

	char    buf[WSS_BUFFER_SIZE + 1];
	char    to[WSS_BUFFER_SIZE + 1];
	char    from[WSS_BUFFER_SIZE + 1];
	time_t  tm;
	DWORD   numRetries;
	DWORD   returnMail;

	int     result,len;

	long newFileHandle = Que_CreateFile();

	if((long)(ULONG_PTR)INVALID_HANDLE_VALUE == newFileHandle)
		return CUT_ERROR;

	result = Que_ReadFileHeader(origFileHandle,to,WSS_BUFFER_SIZE,from,WSS_BUFFER_SIZE,&tm,&numRetries,
		&returnMail);

	// Copy header with new to addr...
	result = Que_WriteFileHeader(newFileHandle,toAddr,from,numRetries,returnMail); 

	// Now copy the rest of the file
	for(;;) {
		// Read old...
		len = Que_ReadFile(origFileHandle,(LPBYTE)buf,sizeof(buf));
		if(len <= 0)
			break;

		// Write new...
		Que_WriteFile(newFileHandle,(LPBYTE)buf,len);
	}


	Que_CloseFile(newFileHandle);   // must leave the origfile alone, but this can
	// be closed.  For a long cc list, cc recipients may
	// get mail first as CUT_SMTPOut::SendMailThread kicks in...

	return CUT_SUCCESS;
}

/*****************************************
User_OpenUser
-opens up the given users account
-returns a -1 if the user does not exist
of if the user is already locked
******************************************/
long CUT_UserManager::User_OpenUser(LPCTSTR user,LPCTSTR password){

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	long    rt = -1;

	// Find the user
	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) {

		// Check to see if the user and password matches a user in the list
		if(_tcsicmp(user, index->szUserName) == 0) {
			if(_tcsicmp(password, index->szPassword) == 0) {

				// Check to see if the user is already locked
				if(index->bOpenFlag == TRUE) { 
					rt = -2;
					break;
				}

				// Open the user
				index->bOpenFlag = TRUE;

				// Get the files list
				User_GetFileList(&(*index));

				// Return the user handle
				rt = index->nID;
				break;
			}
		}
	}

	return rt;
}

/*****************************************
User_CloseUser
-closes an open users account
******************************************/
long CUT_UserManager::User_CloseUser(long userHandle){

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	// Find user by ID
	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
		if(index->nID == userHandle) {

			// Delete all files in the delete list
			User_DeleteAllFiles(&(*index));

			// Clear the information
			index->vcFileList.clear();
			index->vcFileDeleteList.clear();
			index->vcFileSizeList.clear();
			index->nFileListLength = 0;
			index->bOpenFlag = FALSE;
		}

		return CUT_SUCCESS;
}

/*****************************************
User_GetFileCount
-returns the number of files available in 
the the users email box
******************************************/
int CUT_UserManager::User_GetFileCount(long userHandle){
	int result = -1;

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
		if(index->nID == userHandle) {
			result = index->nFileListLength;
			break;
		}

		return result;
}

/*****************************************
User_GetFileSize
-returns the size of a file in the users email
box
******************************************/
long CUT_UserManager::User_GetFileSize(long userHandle, int nFileIndex){
	int result = -1;

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
		if(index->nID == userHandle) {
			if(nFileIndex >= index->nFileListLength)
				result = -1;
			else
				result = index->vcFileSizeList[nFileIndex];
			break;
		}

		return result;

}

/*****************************************
User_GetFileID
-returns the name of a file in the users
email box
******************************************/
long CUT_UserManager::User_GetFileID(long userHandle,int nFileIndex){
	int result = -1;

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
		if(index->nID == userHandle) {
			if(nFileIndex >= index->nFileListLength) 
				result = -1;
			else
				result = index->vcFileList[nFileIndex];
			break;
		}

		return result;

}

/*****************************************
User_OpenFile
-opens a file in the users email box
-returns -1 if the file could not be opened
******************************************/
long CUT_UserManager::User_OpenFile(long userHandle,int nFileIndex){
	int result = -1;

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index)
	{
		if(index->nID == userHandle) {
			_TCHAR    file[WSS_BUFFER_SIZE + 1];

			_sntprintf(file,sizeof(file)/sizeof(_TCHAR)-1,_T("%s%s\\%06ld"),m_ptrMailServer->m_szRootPath,
				index->szMailDir,
				index->vcFileList[nFileIndex]);

			HANDLE hFile = CreateFile(file,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
			if(hFile !=  INVALID_HANDLE_VALUE) {
				result = (long)(ULONG_PTR)hFile;
				break;
			}
		}
	}

	return result;
}

/*****************************************
User_ReadFile
-reads bytes from the open USER file
******************************************/
int CUT_UserManager::User_ReadFile(long fileHandle,LPBYTE buf,int bufLen){

	DWORD readLen;

	if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,buf,bufLen,&readLen,NULL))
		return (int)readLen;
	return FALSE;
}


/*****************************************
User_CreateUserFile
-creates a new file for writing in a USER
email box 
-returns -1 if the user box cannot be 
found
-returns -1 if the file cannot be created
******************************************/
long CUT_UserManager::User_CreateUserFile(LPCTSTR emailAddress){

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	long            filename = 0;
	long            number;
	_TCHAR          path[WSS_BUFFER_SIZE + 1];
	_TCHAR			file[WSS_BUFFER_SIZE + 1];
	_TCHAR          szMailDir[MAX_PATH + 1];
	_TCHAR			szMailRootDir[MAX_PATH + 1] = {_T("")};
	_TCHAR			addr[WSS_BUFFER_SIZE + 1];
	int             addrIndex;
	WIN32_FIND_DATA wfd;
	HANDLE          handle,hFile;
	int             found = FALSE;

	// Chop the domain off the address
	_tcsncpy(addr,emailAddress,sizeof(addr));
	addr[WSS_BUFFER_SIZE] = 0;

	for(int i=0;i < sizeof(addr);i++) {
		if(addr[i]=='@') {
			addr[i] = 0;
			break;
		}
	}

	// Find the user that matches the email address
	USER_LIST::iterator     index;
	for(index = m_userList.begin(); !found && index != m_userList.end() ; ++index) {
		for(addrIndex = 0; !found && addrIndex < index->listEmailAddresses.GetCount(); addrIndex++) {

			// Check mailbox name
			LPCTSTR name = index->listEmailAddresses.GetString(addrIndex);
			if(_tcsicmp(addr, name) == 0) {
				//            if(_stricmp(addr, index->listEmailAddresses.GetString(addrIndex)) == 0) {
				_tcscpy(szMailDir, index->szMailDir);
				found = TRUE;
				break;
			}

			// Special mailbox found with name "ROOT" which must be used if address is not found
			if(_tcsicmp(index->listEmailAddresses.GetString(addrIndex), _T("ROOT")) == 0) {
				_tcscpy(szMailRootDir, index->szMailDir);
			}
		}   
	}

	// If the email address was not found 
	if(found == FALSE) {
		_sntprintf(file,sizeof(file)/sizeof(_TCHAR)-1,_T("Error: Mailbox %s not found."), addr);
		m_ptrMailServer->OnStatus(file);

		// Use ROOT mailbox
		if(_tcslen(szMailRootDir) > 0) {
			m_ptrMailServer->OnStatus(_T("Info: Redirecting mail to the ROOT."));
			_tcscpy(szMailDir, szMailRootDir);
			found = TRUE;
		}

		// Return error
		else {
			return -1;
		}
	}

	// Check if the user directory exists 
	_sntprintf(path,sizeof(path)/sizeof(_TCHAR)-1,_T("%s%s"),m_ptrMailServer->m_szRootPath, szMailDir);
	CreateDirectory(path, NULL);

	// Find the first available filename in the que
	_sntprintf(path,sizeof(path)/sizeof(_TCHAR)-1,_T("%s%s\\*.*"),m_ptrMailServer->m_szRootPath, szMailDir);

	handle = FindFirstFile(path,&wfd);
	while(handle != INVALID_HANDLE_VALUE) {
		if(wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
			number = _ttol(wfd.cFileName);
			if(number > filename)
				filename = number;
		}
		if(FindNextFile(handle,&wfd) != TRUE) {
			FindClose(handle);
			break;
		}
	}
	do {
		filename++;
		_sntprintf(file,sizeof(file)/sizeof(_TCHAR)-1,_T("%s%s\\%06ld"),m_ptrMailServer->m_szRootPath,szMailDir,filename);
		hFile = CreateFile(file,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
		if(hFile !=  INVALID_HANDLE_VALUE) {
			return (long)(ULONG_PTR)hFile;
		}
	} while(filename > 0);

	return -1;
}

/*****************************************
User_WriteFile
-writes bytes to an open USER file
Returns # of bytes written or 0 for failure
******************************************/
int CUT_UserManager::User_WriteFile(long fileHandle,LPBYTE buf,int bufLen){
	DWORD writeLen;

	if(WriteFile((HANDLE)(ULONG_PTR)fileHandle,buf,bufLen,&writeLen,NULL)) {
		return (int)writeLen;
	}
	else {
		// v4.2 - removed
		// MessageBeep((UINT)-1);
	}
	return 0;
}

/*****************************************
User_CloseFile
-closes an open USER file
******************************************/
int CUT_UserManager::User_CloseFile(long fileHandle){
	if(CloseHandle((HANDLE)(ULONG_PTR)fileHandle))
		return CUT_SUCCESS;
	return CUT_ERROR;
}

/*****************************************
User_DeleteFile
-puts the given file into the delete list
******************************************/
int CUT_UserManager::User_DeleteFile(long userHandle,int nFileIndex){
	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
		if(index->nID == userHandle) 
			if(nFileIndex >= index->nFileListLength || nFileIndex < 0)
				return CUT_ERROR;
			else {
				index->vcFileDeleteList[nFileIndex] = TRUE;
				return CUT_SUCCESS;
			}

			return CUT_ERROR;
}

/*****************************************
User_ResetDelete
-clears all user files from the delete list
******************************************/
int CUT_UserManager::User_ResetDelete(long userHandle){
	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
		if(index->nID == userHandle) 
			for(int i = 0; i < index->nFileListLength; i++)
				index->vcFileDeleteList[i] = FALSE;

	return CUT_SUCCESS;
}

/*****************************************
User_GetFileList
Returns a list of available files in a users
account
******************************************/
int CUT_UserManager::User_GetFileList(CUT_UserInfo *info)
{
	long            lVectorSize = 25;
	WIN32_FIND_DATA wfd;
	HANDLE          handle;
	HANDLE          hFile;

	// Clear the data
	info->nFileListLength = 0;
	info->vcFileList.clear();
	info->vcFileDeleteList.clear();
	info->vcFileSizeList.clear();

	// Resize the vectors
	info->vcFileList.resize(lVectorSize);
	info->vcFileDeleteList.resize(lVectorSize);
	info->vcFileSizeList.resize(lVectorSize);

	// Create the path to search in
	_TCHAR path[WSS_BUFFER_SIZE+1], file[WSS_BUFFER_SIZE+1];
	_sntprintf(path,sizeof(path)/sizeof(_TCHAR)-1,_T("%s%s\\*.*"),m_ptrMailServer->m_szRootPath, info->szMailDir);

	// Find the available files
	handle = FindFirstFile(path,&wfd);
	while(handle != INVALID_HANDLE_VALUE){
		if(wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {

			// The name of the file MUST be 6 digits
			BOOL	bFileNameOK = FALSE;
			if(_tcslen(wfd.cFileName) == 6) {
				bFileNameOK = TRUE;
				for(int i=0; i<6; i++) {
					if(!isdigit(wfd.cFileName[i])) 
						bFileNameOK = FALSE;
				}
			}

			if(bFileNameOK) {
				// Check to see if the file can open - if so then add it to the list
				_sntprintf(file,sizeof(file)/sizeof(_TCHAR)-1,_T("%s%s\\%s"),m_ptrMailServer->m_szRootPath,info->szMailDir,wfd.cFileName);
				hFile = CreateFile(file,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
				if(hFile !=  INVALID_HANDLE_VALUE){
					// Close the file
					CloseHandle(hFile);

					// Add it to the list
					info->vcFileList[info->nFileListLength]         = _ttoi(wfd.cFileName);
					info->vcFileDeleteList[info->nFileListLength]   = FALSE;
					info->vcFileSizeList[info->nFileListLength]     = wfd.nFileSizeLow;

					++(info->nFileListLength);

					// Adjust vector size
					if(info->nFileListLength >= lVectorSize  ) {
						lVectorSize += 25;
						info->vcFileList.resize(lVectorSize);
						info->vcFileDeleteList.resize(lVectorSize);
						info->vcFileSizeList.resize(lVectorSize);
					}
				}
			}
			else {
				// We have a problem with the file name format
				_sntprintf(file,sizeof(file)/sizeof(_TCHAR)-1,_T("ERROR: User \"%s\". Invalid file name format: %s"), info->szUserName, wfd.cFileName);
				m_ptrMailServer->OnStatus(file);
			}
		}

		if(FindNextFile(handle,&wfd) != TRUE) {
			FindClose(handle);
			break;
		}
	}

	// Resize the vectors
	info->vcFileList.resize(info->nFileListLength);
	info->vcFileDeleteList.resize(info->nFileListLength);
	info->vcFileSizeList.resize(info->nFileListLength);

	// Put the number of files into the log
	_sntprintf(file,sizeof(file)/sizeof(_TCHAR)-1,_T("User \"%s\". Number of messages found: %d"), info->szUserName, info->nFileListLength);
	m_ptrMailServer->OnStatus(file);

	return CUT_SUCCESS;
}

/*****************************************
User_DeleteAllFiles
-deletes all the files found the the USER
delete file list
******************************************/
int CUT_UserManager::User_DeleteAllFiles(CUT_UserInfo *info){

	_TCHAR  path[WSS_BUFFER_SIZE+1];
	int     error = CUT_SUCCESS;

	for(int i = 0; i < info->nFileListLength; i++) {
		if(info->vcFileDeleteList[i] == TRUE) {
			_sntprintf(path,sizeof(path)/sizeof(_TCHAR)-1,_T("%s%s\\%06ld"),m_ptrMailServer->m_szRootPath, info->szMailDir, info->vcFileList[i]);
			if(DeleteFile(path) != TRUE)
				error += 1;
		}
	}

	return error;
}

/*****************************************
LoadUserInfo

-retrieves program settings from the registry
Registry Entries

NumberUsers             number of users in the system
XXXUser                 username where XXX starts from 0
XXXPassword             password where XXX starts from 0
XXXNumberEmailAddresses number of email addresses where XXX starts from 0
XXXEmailAddressYYY      email address(s) where XXX starts from 0
YYY starts from 0
******************************************/
int CUT_UserManager::LoadUserInfo(LPCTSTR subKey) {

	_TCHAR  szTemp[WSS_BUFFER_SIZE+1];
	_TCHAR  buf[WSS_BUFFER_SIZE+1];
	int     count,count2;
	DWORD   items;
	DWORD   size;

	HKEY    key;
	int     error = CUT_SUCCESS;

	if(subKey == NULL)
		_tcscpy(buf, _T("SOFTWARE\\EMAIL_S\\USER\\"));
	else {
		_tcsncpy(buf, subKey, WSS_BUFFER_SIZE);
		if(*(buf + _tcslen(buf) - 1) != _T('\\'))
			_tcscat(buf, _T("\\"));
		_tcscat(buf, _T("USER\\"));
	}

	//open up the key for the email config info
	if(RegCreateKey(HKEY_LOCAL_MACHINE, buf, &key) != ERROR_SUCCESS)
		return CUT_ERROR; 

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	m_ptrMailServer->OnStatus(_T("Loading user information..."));

	// Get the user list infomation length
	size = sizeof(DWORD);
	int     nUserListLength = 0;
	if(RegQueryValueEx(key,_T("NumberUsers"),NULL,NULL,(LPBYTE)&nUserListLength,&size) != ERROR_SUCCESS)
		error |= 64;

	// Get the user list infomation
	for(count = 0;count < nUserListLength; count++) {
		CUT_UserInfo    UserInfo;

		// Set user ID
		UserInfo.nID = count;

		// Get the username
		size = MAX_USER_NAME_LENGTH;
		_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1,_T("%dUser"),count);
		if(RegQueryValueEx(key,buf,NULL,NULL,(LPBYTE)szTemp, &size) != ERROR_SUCCESS) {
			error |= 128;
		}
		else {
			szTemp[size] = _T('\0');
			_tcscpy(UserInfo.szUserName, szTemp);
		}

		UserInfo.szUserName[size] = _T('\0');
		_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1, _T("User: %s"), UserInfo.szUserName); 
		m_ptrMailServer->OnStatus(buf);

		// Get the password
		size = MAX_USER_NAME_LENGTH;
		_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1,_T("%dPassword"),count);
		if(RegQueryValueEx(key,buf,NULL,NULL,(LPBYTE)szTemp, &size) != ERROR_SUCCESS){
			error |= 256;
		}
		else {
			szTemp[size] = _T('\0');
			_tcscpy(UserInfo.szPassword, szTemp);
		}

		// Get the email addresses
		size = sizeof(DWORD);
		_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1,_T("%dNumberEmailAddresses"),count);
		if(RegQueryValueEx(key,buf,NULL,NULL,(LPBYTE)&items, &size) != ERROR_SUCCESS)
			error |= 512;

		// Get user mail directory
		size = MAX_PATH;
		_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1,_T("%dUserMailDir"),count);
		if(RegQueryValueEx(key,buf,NULL,NULL,(LPBYTE)szTemp, &size) != ERROR_SUCCESS) {
			User_GetFreeMailDir(UserInfo);
			error |= 1024;
		}
		else {
			szTemp[size] = _T('\0');
			_tcscpy(UserInfo.szMailDir, szTemp);
		}

		for(count2 = 0; count2 < (int)items; count2++) {

			// Get the username
			size = WSS_BUFFER_SIZE;
			_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1,_T("%dEmailAddress%d"),count,count2);
			if(RegQueryValueEx(key,buf,NULL,NULL,(LPBYTE)szTemp,&size) != ERROR_SUCCESS){
				error |= 1024;
			}
			else {
				szTemp[size] = _T('\0');

				_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1, _T("Mailbox: %s"), szTemp); 
				m_ptrMailServer->OnStatus(buf);

				UserInfo.listEmailAddresses.AddString(szTemp);
			}
		}

		// Add item to the list
		m_userList.push_back(UserInfo);
	}

	RegCloseKey(key);

	bUserInfoChanged = FALSE;

	return error;
}

/***************************************************
SaveUserInfo
Saves program settings to the registry    
Params
subKey      - registry key to load from
Return
CUT_SUCCESS - if success
****************************************************/
int CUT_UserManager::SaveUserInfo(LPCTSTR subKey) {
	int     count,count2;
	_TCHAR  buf[WSS_BUFFER_SIZE + 1];

	HKEY    key;
	int     error = CUT_SUCCESS;

	if(subKey == NULL)
		_tcscpy(buf, _T("SOFTWARE\\EMAIL_S\\USER\\"));
	else {
		_tcsncpy(buf, subKey, WSS_BUFFER_SIZE);
		if(*(buf + _tcslen(buf) - 1) != _T('\\'))
			_tcscat(buf, _T("\\"));
		_tcscat(buf, _T("USER\\"));
	}

	// Open/Create up the key for the email config info
	if(RegCreateKey(HKEY_LOCAL_MACHINE, buf, &key) != ERROR_SUCCESS)
		return CUT_ERROR; 

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	// Clean up registry first
	DWORD   cbValue;    
	long    lRet;

	for( ;; ) {
		// Remove this keys old values
		cbValue = (DWORD)WSS_BUFFER_SIZE;
		lRet    = RegEnumValue(key, 0, buf, &cbValue, NULL, NULL, NULL, NULL);                  // address for size of data buffer

		if( lRet == ERROR_SUCCESS) 
			lRet = RegDeleteValue(key, buf);
		else
			break;
	}

	_TCHAR szTemp[WSS_BUFFER_SIZE+1];

	// Put the user list infomation length
	int nUserListLength = (int)m_userList.size();
	if(RegSetValueEx(key,_T("NumberUsers"),NULL,REG_DWORD,(LPBYTE)&nUserListLength, sizeof(DWORD)) != ERROR_SUCCESS)
		error |= 64;

	// Put the user list infomation
	USER_LIST::iterator     index;
	for(count = 0, index = m_userList.begin(); index != m_userList.end() ; ++index, count++) {

		// Put the username
		_sntprintf(buf,sizeof(buf)-1,_T("%dUser"),count);
		_stprintf(szTemp, index->szUserName);
		if(RegSetValueEx(key,buf,NULL,REG_SZ,(LPBYTE)szTemp, (DWORD)_tcslen(szTemp)*sizeof(_TCHAR)) != ERROR_SUCCESS)
			error |= 128;

		// Put the password
		_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1,_T("%dPassword"),count);
		_stprintf(szTemp, index->szPassword);
		if(RegSetValueEx(key,buf,NULL,REG_SZ,(LPBYTE)szTemp, (DWORD)_tcslen(szTemp)*sizeof(_TCHAR)) != ERROR_SUCCESS)
			error |= 256;

		// Put the email addresses
		_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1,_T("%dNumberEmailAddresses"),count);
		DWORD   dwEmailAddressesNumb = index->listEmailAddresses.GetCount();
		if(RegSetValueEx(key,buf,NULL,REG_DWORD,(LPBYTE)&dwEmailAddressesNumb, sizeof(DWORD)) != ERROR_SUCCESS)
			error |= 512;

		// Put mail dir
		_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1,_T("%dUserMailDir"),count);
		if(RegSetValueEx(key,buf,NULL,REG_SZ,(LPBYTE)index->szMailDir,(DWORD)_tcslen(index->szMailDir)*sizeof(_TCHAR)) != ERROR_SUCCESS)
			error |= 256;

		for(count2 = 0;(unsigned ) count2 <  dwEmailAddressesNumb; count2++) {
			// Put the email addresses (may be several aliases)
			_sntprintf(buf,sizeof(buf)/sizeof(_TCHAR)-1,_T("%dEmailAddress%d"),count,count2);
			_stprintf(szTemp, index->listEmailAddresses.GetString(count2));
			if(RegSetValueEx(key,buf,NULL,REG_SZ,(LPBYTE)szTemp, (DWORD)_tcslen(szTemp)*sizeof(_TCHAR)) != ERROR_SUCCESS)
				error |= 1024;
		}
	}

	RegCloseKey(key);

	bUserInfoChanged = FALSE;

	return error;
}

/*****************************************
CUT_UserManager
-initializes the class variables
******************************************/
CUT_UserManager::CUT_UserManager(CUT_MailServer &ptrMailServer) :
bUserInfoChanged(FALSE),
m_ptrMailServer(&ptrMailServer)     // Initialize pointer to the Mail Server class
{
}

/*****************************************
~CUT_UserManager
-frees any allocated memory and objects
******************************************/
CUT_UserManager::~CUT_UserManager(){

	// Save user info if it was changed
	if(bUserInfoChanged)
		SaveUserInfo(m_ptrMailServer->m_szRootKey);
}

/*****************************************
User_Add
Adds new user to the list
******************************************/
int CUT_UserManager::User_Add(LPCTSTR lpszName, LPCTSTR szPassword, LPCTSTR MailAddress) 
{
	_TCHAR    buffer[101];

	if(MailAddress == NULL)
		return m_ptrMailServer->OnError(UTE_PARAMETER_INVALID_VALUE);

	CUT_TStringList  listMailAddresses;

	int i = 0;
	while(CUT_StrMethods::ParseString(MailAddress, _T(","), i++, buffer, 100) == UTE_SUCCESS)
		listMailAddresses.AddString(buffer);

	return User_Add(lpszName, szPassword, listMailAddresses);
}

/*****************************************
User_Add
Adds new user to the list
******************************************/
int CUT_UserManager::User_Add(LPCTSTR lpszName, LPCTSTR szPassword, CUT_TStringList &MailAddresses) 
{
	if(lpszName == NULL || szPassword == NULL)
		return m_ptrMailServer->OnError(UTE_PARAMETER_INVALID_VALUE);

	// Check if this user name is alredy used and find maximum ID number
	USER_LIST::iterator     index;
	int                     addrIndex, nMaxID = 0;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) {

		// Search for duplicate names
		if(_tcsicmp(lpszName, index->szUserName) == 0) 
			return m_ptrMailServer->OnError(UTE_USER_NAME_ALREDY_EXIST);

		// Search for duplicate mailboxes
		for(int i = 0; i < MailAddresses.GetCount(); i++) {
			for(addrIndex = 0; addrIndex < index->listEmailAddresses.GetCount(); addrIndex++) {
				if(_tcsicmp(MailAddresses.GetString(i), index->listEmailAddresses.GetString(addrIndex)) == 0) {
					return m_ptrMailServer->OnError(UTE_MAILBOX_NAME_ALREDY_EXIST);
				}
			}   
		}


		if(index->nID >= nMaxID)
			nMaxID = index->nID + 1;
	}


	// Initialize user info class
	CUT_UserInfo    UserInfo;
	_tcsncpy(UserInfo.szUserName, lpszName, MAX_USER_NAME_LENGTH);
	_tcsncpy(UserInfo.szPassword, szPassword, MAX_USER_NAME_LENGTH);
	UserInfo.listEmailAddresses = MailAddresses;
	UserInfo.nID = nMaxID;
	User_GetFreeMailDir(UserInfo);

	// Add item to the list
	m_userList.push_back(UserInfo);

	// Set changed flag
	bUserInfoChanged = TRUE;

	return m_ptrMailServer->OnError(UTE_SUCCESS);
}

/*****************************************
User_GetNumber
Returns number of users in the list
******************************************/
int CUT_UserManager::User_GetCount() 
{
	return (int)m_userList.size();
}

/*****************************************
User_GetUser
Returns the user from the list
******************************************/
CUT_UserInfo * CUT_UserManager::User_GetUser(int ID) 
{
	// Find user by ID
	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
		if(index->nID == ID) {
			return &(*index);
		}

		return NULL;
}

/*****************************************
User_Delete
Delete user from the list
******************************************/
int CUT_UserManager::User_Delete(LPCTSTR lpszName) 
{
	if(lpszName == NULL)
		return m_ptrMailServer->OnError(UTE_PARAMETER_INVALID_VALUE);

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	// Find user by name
	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
		if(_tcsicmp(lpszName, index->szUserName) == 0) {
			// Remove item from the list
			m_userList.erase(index);

			// Set changed flag
			bUserInfoChanged = TRUE;

			break;
		}

		// Change User IDs
		int     nID = 0;
		for(index = m_userList.begin(); index != m_userList.end() ; ++index) {
			index->nID = nID;
			++nID;
		}

		return m_ptrMailServer->OnError(UTE_SUCCESS);
}

/*****************************************
User_Modify
Modifies user in the list
******************************************/
int CUT_UserManager::User_Modify(LPCTSTR lpszName, LPCTSTR lpszNewName, LPCTSTR lpszNewPassword, CUT_TStringList *listNewAddresses) 
{
	if(lpszName == NULL)
		return m_ptrMailServer->OnError(UTE_PARAMETER_INVALID_VALUE);

	// Enter into a critical section
	CUT_CriticalSection CriticalSection(m_criticalSection);

	// Find user by name
	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
		if(_tcsicmp(lpszName, index->szUserName) == 0) {

			// Change iformation
			if(lpszNewName != NULL)
				_tcscpy(index->szUserName, lpszNewName);
			if(lpszNewPassword != NULL)
				_tcscpy(index->szPassword, lpszNewPassword);
			if(listNewAddresses != NULL)
				index->listEmailAddresses = *listNewAddresses;

			// Set changed flag
			bUserInfoChanged = TRUE;

			break;
		}

		return m_ptrMailServer->OnError(UTE_SUCCESS);
}

/*****************************************
GetFreeMailDir
Gets free user mail directory name
******************************************/
int CUT_UserManager::User_GetFreeMailDir(CUT_UserInfo &info) 
{
	_TCHAR    buffer[20];

	for(int i = 0; i <= 99; i++) {
		BOOL    bExist = FALSE;

		// Generate a new name
		_tcsncpy(buffer, info.szUserName, 4);
		buffer[4] = 0;
		while(_tcslen(buffer) < 4)
			_tcscat(buffer, _T("0"));
		_stprintf(buffer+4, _T("%02d"), i);

		// Check if the name alredy exsists
		USER_LIST::iterator     index;
		for(index = m_userList.begin(); index != m_userList.end() ; ++index) 
			if(_tcsicmp(buffer, index->szMailDir) == 0) {
				bExist = TRUE;
				continue;
			}

			if(!bExist) {
				_tcscpy(info.szMailDir, buffer);
				return m_ptrMailServer->OnError(UTE_SUCCESS);
			}
	}

	return m_ptrMailServer->OnError(UTE_ERROR);
}

/*****************************************
User_IsFileDeleted
checks if file marked as deleted
******************************************/
BOOL CUT_UserManager::User_IsFileDeleted(long userHandle, int nFileIndex)
{
	USER_LIST::iterator     index;
	for(index = m_userList.begin(); index != m_userList.end() ; ++index) {
		if(index->nID == userHandle) {
			if(nFileIndex >= index->nFileListLength)
				return TRUE;
			else
				return index->vcFileDeleteList[nFileIndex];
		}
	}

	return TRUE;
}

#pragma warning ( pop )
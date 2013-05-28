//////////////////////////////////////////////////////////////////////
// ==================================================================
//  class:  CUT_MailServer
//  File:   UT_MailServer.cpp
//  
//  Purpose:
//
//  Mail server class
//
// ==================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// ==================================================================

#ifdef _WINSOCK_2_0_
#define _WINSOCKAPI_	/* Prevent inclusion of winsock.h in windows.h   */
/* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include <stdio.h>

#include "UT_MailServer.h"

#include "ut_strop.h"

#ifdef UT_DISPLAYSTATUS
#include "uh_ctrl.h"
extern CUH_Control	*ctrlHistory;
#endif 

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

// these do not exist in core VC6 
#if _MSC_VER < 1400
#if !defined ULONG_PTR
#define ULONG_PTR DWORD
#define LONG_PTR DWORD
#endif
#endif

/***************************************************
CUT_MailServer
Constructor. Initialize mail server classes
Params
none
Return
none
****************************************************/
CUT_MailServer::CUT_MailServer() :
m_bClassesInitialized(FALSE),

m_bSystemInfoChanged(FALSE),
m_bDestroyingObject(FALSE),

// Initialize classes cleanup flags
m_bDeletePOP3(FALSE),
m_bDeleteSMTP(FALSE),
m_bDeleteSMTPOut(FALSE),
m_bDeleteDataManager(FALSE),
m_bDeleteUserManager(FALSE),

m_dwMaxOut(10),
m_dwMaxRetries(10),
m_dwRetryInterval(15)

{
	m_szRootKey[0] = NULL;
}

/***************************************************
~CUT_MailServer
Destructor. Cleans up allocated resources.
Params
none
Return
none
****************************************************/
CUT_MailServer::~CUT_MailServer()
{
	m_bDestroyingObject = TRUE;

	// Save user info if it was changed
	if(m_bSystemInfoChanged)
		SaveSystemInfo((m_szRootKey[0] == NULL) ? NULL : m_szRootKey);

	// If we were using default classes - clean them up
	if(m_bDeleteSMTPOut && m_ptrSMTPOutServer)
		delete m_ptrSMTPOutServer;
	if(m_bDeletePOP3 && m_ptrPOP3Server)
		delete m_ptrPOP3Server;
	if(m_bDeleteSMTP && m_ptrSMTPServer)
		delete m_ptrSMTPServer;
	if(m_bDeleteDataManager && m_ptrDataManager)
		delete m_ptrDataManager;
	if(m_bDeleteUserManager && m_ptrUserManager)
		delete m_ptrUserManager;

	// Clear local domain names list
	m_listLocalNames.ClearList();

	// Clear DNS names list
	m_listDNSNames.ClearList();

	m_listRelayingIPAddresses.ClearList ();
}

/***************************************************
SetServerClasses
Set server classes pointers
Params
[ptrPOP3Server]		- pointer to custom POP3 class
[ptrSMTPServer]		- pointer to custom SMTP class
[ptrSMTPOutServer]	- pointer to custom SMTPOut class
[ptrDataManager]	- pointer to custom DataManager class
[ptrUserManager]	- pointer to custom UserManager class
Return
CUT_SUCCESS	- success
CUT_ERROR	- error
****************************************************/
int CUT_MailServer::SetServerClasses(	CUT_POP3Server	*ptrPOP3Server,
									 CUT_SMTPServer	*ptrSMTPServer,
									 CUT_SMTPOut		*ptrSMTPOutServer,
									 CUT_DataManager	*ptrDataManager,
									 CUT_UserManager	*ptrUserManager)
{
	if(m_bClassesInitialized)
		return CUT_ERROR;

	// Initialize pointers to the POP3, SMTP, SMTPOut, DataManager and UserManager classes
	// Use default classes if constructor pointers are NULLs

	if(ptrUserManager == NULL) {
		m_ptrUserManager = new CUT_UserManager(*this);
		m_bDeleteUserManager = TRUE;
	}
	else 
		m_ptrUserManager = ptrUserManager;

	if(ptrDataManager == NULL) {
		m_ptrDataManager = new CUT_DataManager(*this);
		m_bDeleteDataManager = TRUE;
	}
	else 
		m_ptrDataManager = ptrDataManager;

	if(ptrPOP3Server == NULL) {
		m_ptrPOP3Server = new CUT_POP3Server(*this);
		m_bDeletePOP3 = TRUE;
	}
	else
		m_ptrPOP3Server = ptrPOP3Server;

	if(ptrSMTPServer == NULL) {
		m_ptrSMTPServer = new CUT_SMTPServer(*this);
		m_bDeleteSMTP = TRUE;
	}
	else 
		m_ptrSMTPServer = ptrSMTPServer;

	if(ptrSMTPOutServer == NULL) {
		m_ptrSMTPOutServer = new CUT_SMTPOut(*this);
		m_bDeleteSMTPOut = TRUE;
	}
	else
		m_ptrSMTPOutServer = ptrSMTPOutServer;

	m_bClassesInitialized = TRUE;

	return CUT_SUCCESS;
}
/********************************
Start
Loads user and system information and 
starts up the SMTP, SMTPOut, POP3 
server classes.
Params
none
Return
CUT_SUCCESS	- success
*********************************/
int CUT_MailServer::Start() {



	int	rt = CUT_SUCCESS;

	if(!m_bClassesInitialized)
		SetServerClasses();

	// Load user info
	if(m_ptrUserManager->LoadUserInfo((m_szRootKey[0] == NULL) ? NULL : m_szRootKey) != CUT_SUCCESS) 
		return OnError(UTE_USER_INFO_LOAD_FAILED);

	// Load system info
	if(LoadSystemInfo((m_szRootKey[0] == NULL) ? NULL : m_szRootKey) != CUT_SUCCESS) 
		return OnError(UTE_SYSTEM_INFO_LOAD_FAILED);

	// Start POP3 Server
	if(rt == CUT_SUCCESS && m_ptrPOP3Server)
		rt = m_ptrPOP3Server->Start();

	// Start SMTP Server
	if(rt == CUT_SUCCESS && m_ptrSMTPServer)
		rt = m_ptrSMTPServer->Start();

	// Start SMTPOut Server
	if(rt == CUT_SUCCESS && m_ptrSMTPOutServer)
		rt = m_ptrSMTPOutServer->Start();

	return OnError(rt);
}
/***************************************************
OnError
This virtual function is called each time we return
a value. It's a good place to put in an error messages
or trace.
Params
error - error code
Return
error code
****************************************************/
int CUT_MailServer::OnError(int error) {
	return error;
}

/***************************************************
OnStatus
This virtual function is called each time we have any
status information to display.
Params
StatusText	- status text
Return
UTE_SUCCESS - success   
****************************************************/
int CUT_MailServer::OnStatus(LPCTSTR StatusText)
{
#ifdef UT_DISPLAYSTATUS 
	if(!m_bDestroyingObject) {
		// Add Thread ID at the begging of the status message
		_TCHAR szBuffer[WSS_BUFFER_SIZE+1];
		_sntprintf(szBuffer,sizeof(szBuffer)/sizeof(_TCHAR)-1,_T("[%d] "), ::GetCurrentThreadId());

		// Add status text
		_tcsncat(szBuffer, StatusText, WSS_BUFFER_SIZE - 10);

		// Display status message in the LOG
		ctrlHistory->AddLine(szBuffer);
	}
#endif

	return UTE_SUCCESS;
}
// overload for char * requests 
#if defined _UNICODE
int CUT_MailServer::OnStatus(LPCSTR StatusText)
{
#ifdef UT_DISPLAYSTATUS 
	_TCHAR sz[WSS_BUFFER_SIZE+1];
	_TCHAR szBuffer[WSS_BUFFER_SIZE+1];
	int size = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,StatusText,(int)strlen(StatusText),sz,sizeof(sz)/sizeof(_TCHAR)-1);
	if(size == 0) {
		ctrlHistory->AddLine(_T("Error in mb call"));
	}
	else {
		sz[size] = _T('\0');
		_sntprintf(szBuffer,sizeof(szBuffer)/sizeof(_TCHAR)-1,_T("[%d] "), ::GetCurrentThreadId());

		// Add status text
		_tcsncat(szBuffer, sz, WSS_BUFFER_SIZE - 10);

		// Display status message in the LOG
		ctrlHistory->AddLine(szBuffer);
	}
#endif

	return UTE_SUCCESS;
}
#endif
/***************************************************
OnCanAccept
This virtual function is called each time the server 
accepting new connection.
Params
LPCSTR	    - IP address of connecting user
Return
TRUE        - accept 
FALSE       - cancel
****************************************************/
long CUT_MailServer::OnCanAccept(LPCSTR /* address */ ){
	return TRUE;
}

/***************************************************
SaveSystemInfo
Saves program settings to the registry    
Params
subKey		- registry key to load from
Return
CUT_SUCCESS	- if success
****************************************************/
int CUT_MailServer::SaveSystemInfo(LPCTSTR subKey){
	int     count, len, error = CUT_SUCCESS;
	_TCHAR    buf[WSS_BUFFER_SIZE + 1];
	HKEY    key;

	if(subKey == NULL)
		_tcscpy(buf, _T("SOFTWARE\\EMAIL_S"));
	else
		_tcsncpy(buf, subKey, WSS_BUFFER_SIZE);

	// Open/Create up the key for the email config info
	if(RegCreateKey(HKEY_LOCAL_MACHINE, buf, &key) != ERROR_SUCCESS)
		return CUT_ERROR;

	// Clean up registry first
	DWORD	cbValue;	
	long	lRet;
	for (;;) {
		// Remove this keys old values
		cbValue	= (DWORD)WSS_BUFFER_SIZE;
		lRet	= RegEnumValue(key, 0, buf, &cbValue, NULL, NULL, NULL, NULL);					// address for size of data buffer

		if( lRet == ERROR_SUCCESS) 
			lRet = RegDeleteValue(key, buf);
		else
			break;
	}

	// Put the maxout params
	if(RegSetValueEx(key, _T("MaxOut"), NULL, REG_DWORD, (LPBYTE)&m_dwMaxOut, sizeof(DWORD)) != ERROR_SUCCESS)
		m_dwMaxOut = 20;
	if(RegSetValueEx(key,_T("MaxRetries"),NULL,REG_DWORD,(LPBYTE)&m_dwMaxRetries,sizeof(DWORD)) != ERROR_SUCCESS)
		m_dwMaxRetries = 10;
	if(RegSetValueEx(key,_T("RetryInterval"),NULL,REG_DWORD,(LPBYTE)&m_dwRetryInterval,sizeof(DWORD)) != ERROR_SUCCESS)
		m_dwRetryInterval = 5;

	// Put the program root path
	if(RegSetValueEx(key,_T("RootPath"),NULL,REG_SZ,(LPBYTE)m_szRootPath,(DWORD)_tcslen(m_szRootPath)*sizeof(_TCHAR)) != ERROR_SUCCESS)
		error |=2;

	len = (int)_tcslen(m_szRootPath);
	if(m_szRootPath[len-1] != _T('\\'))
		_tcscat(m_szRootPath,_T("\\"));

	// Put the local domain names count
	DWORD		NamesCount = m_listLocalNames.GetCount();
	if(RegSetValueEx(key,_T("NumberDomainNames"),NULL,REG_DWORD,(LPBYTE)&NamesCount,sizeof(DWORD)) != ERROR_SUCCESS)
		error |= 4;

	_TCHAR szTemp[WSS_BUFFER_SIZE+1];

	// Put the local domain names
	for(count = 0;count < (int)NamesCount;count++) {
		_stprintf(buf,_T("DomainName%d"),count);
		if(m_listLocalNames.GetString(count) != NULL) {
			_tcscpy(szTemp, m_listLocalNames.GetString(count));
			if(RegSetValueEx(key,buf,NULL,REG_SZ,(LPBYTE)szTemp,(DWORD)_tcslen(szTemp)*sizeof(_TCHAR)) != ERROR_SUCCESS)
				error |= 8;
		}
	}

	// Put the DNS names count
	NamesCount = m_listDNSNames.GetCount();
	if(RegSetValueEx(key,_T("NumberDNSNames"),NULL,REG_DWORD,(LPBYTE)&NamesCount,sizeof(DWORD)) != ERROR_SUCCESS)
		error |=16;

	// Put the DNS names
	for(count = 0;count < (int)NamesCount; count++) {
		_stprintf(buf,_T("DNSName%d"),count);
		if(m_listDNSNames.GetString(count) != NULL) {
			_tcscpy(szTemp, m_listDNSNames.GetString(count));
			if(RegSetValueEx(key,buf,NULL,REG_SZ,(LPBYTE)szTemp ,(DWORD) _tcslen(szTemp)*sizeof(TCHAR)) != ERROR_SUCCESS)
				error |= 32;
		}
	}

	// Put the Relay IP addresses count
	NamesCount = m_listRelayingIPAddresses.GetCount();
	if(RegSetValueEx(key,_T("NumberRelayingIPAddresses"),NULL,REG_DWORD,(LPBYTE)&NamesCount,sizeof(DWORD)) != ERROR_SUCCESS)
		error |=64;

	// Put the Relay IP addresses
	for(count = 0;count < (int)NamesCount; count++) {
		_stprintf(buf,_T("RelayingIPAddresses%d"),count);
		if( m_listRelayingIPAddresses.GetString(count) != NULL) {
			_tcscpy(szTemp, m_listRelayingIPAddresses.GetString(count));
			if(RegSetValueEx(key,buf,NULL,REG_SZ,(LPBYTE)szTemp ,(DWORD) _tcslen(szTemp)*sizeof(_TCHAR)) != ERROR_SUCCESS)
				error |= 156;
		}
	}


	RegCloseKey(key);

	m_bSystemInfoChanged = FALSE;

	return error;
}

/*****************************************
LoadSystemInfo
Loads program settings from the registry

Registry Entries

RootPath        root path for the que and user mail

NumberDomainNames
DomainNameXXX   local domain name(s), where XXX is a number 
starting from 0

NumberDNSNames
DNSNameXXX      Name server name(s), whre XXX starts from 0

MaxOut          max. mail out at once

MaxRetries      max. times to try and resend the mail
RetryInterval   seconds between retries

Params
subKey		- registry key to load from
Return
CUT_SUCCESS	- if success
******************************************/
int CUT_MailServer::LoadSystemInfo(LPCTSTR subKey){

	_TCHAR  szTemp[WSS_BUFFER_SIZE+1];
	int     count = 0, len, error = CUT_SUCCESS;
	_TCHAR  buf[WSS_BUFFER_SIZE + 1];
	_TCHAR	buf2[WSS_BUFFER_SIZE + 1];
	DWORD   size;
	HKEY    key;

	if(subKey == NULL)
		_tcscpy(buf, _T("SOFTWARE\\EMAIL_S"));
	else
		_tcsncpy(buf, subKey, WSS_BUFFER_SIZE);

	// Open up the key for the email config info
	if(RegCreateKey(HKEY_LOCAL_MACHINE, buf, &key) != ERROR_SUCCESS)
		return CUT_ERROR;

	// Get the maxout params
	size = sizeof(DWORD);
	if(RegQueryValueEx(key,_T("MaxOut"),NULL,NULL,(LPBYTE)&m_dwMaxOut,&size) != ERROR_SUCCESS)
		m_dwMaxOut = 10;
	size = sizeof(DWORD);
	if(RegQueryValueEx(key,_T("MaxRetries"),NULL,NULL,(LPBYTE)&m_dwMaxRetries,&size) != ERROR_SUCCESS)
		m_dwMaxRetries = 10;
	size = sizeof(DWORD);
	if(RegQueryValueEx(key,_T("RetryInterval"),NULL,NULL,(LPBYTE)&m_dwRetryInterval,&size) != ERROR_SUCCESS)
		m_dwRetryInterval = 2;

	// Get the program root path
	size = WSS_BUFFER_SIZE;
	if(RegQueryValueEx(key,_T("RootPath"),NULL,NULL,(LPBYTE)m_szRootPath,&size) != ERROR_SUCCESS) {
		_tcscpy(m_szRootPath, _T("c:\\temp\\Mail_Serv\\"));
		error |=2;
	}

	len = (int)_tcslen(m_szRootPath);
	if(m_szRootPath[len-1] != _T('\\'))
		_tcscat(m_szRootPath,_T("\\"));

	// Get the local domain names count
	size = sizeof(DWORD);
	int		NamesCount = 0;
	if(RegQueryValueEx(key,_T("NumberDomainNames"),NULL,NULL,(LPBYTE)&NamesCount,&size) != ERROR_SUCCESS)
		error |= 4;
	if(NamesCount <= 0)
		error |= 4;

	// Get the local domain names
	m_listLocalNames.ClearList();
	for(count = 0;count < (int)NamesCount;count++) {
		size = WSS_BUFFER_SIZE;
		_stprintf(buf,_T("DomainName%d"),count);
		if(RegQueryValueEx(key,buf,NULL,NULL,(LPBYTE)szTemp,&size) != ERROR_SUCCESS){
			error |= 8;
		}
		else {
			szTemp[size] = _T('\0');
			_tcscpy(buf2, szTemp);
			m_listLocalNames.AddString(buf2);
		}
	}



	// Get the list of Allowed relay IP addresses
	size = sizeof(DWORD);
	int		IpAddressesCount = 0;
	if(RegQueryValueEx(key,_T("NumberRelayingIPAddresses"),NULL,NULL,(LPBYTE)&IpAddressesCount,&size) != ERROR_SUCCESS)
		error |= 64;
	if(IpAddressesCount < 0)
		error |= 64;

	// Get the current list of IP addresses
	m_listRelayingIPAddresses.ClearList();

	// now get the the list
	for(count = 0;count < (int)IpAddressesCount;count++)
	{
		size = WSS_BUFFER_SIZE;
		_stprintf(buf,_T("RelayingIPAddresses%d"),count);
		if(RegQueryValueEx(key,buf,NULL,NULL,(LPBYTE)szTemp,&size) != ERROR_SUCCESS){
			error |= 156;
		}
		else {
			szTemp[size] = _T('\0');
			_tcscpy(buf2, szTemp);
			m_listRelayingIPAddresses.AddString(buf2);
		}
	}

	// Get the DNS names count
	size = sizeof(DWORD);
	NamesCount = 0;
	if(RegQueryValueEx(key,_T("NumberDNSNames"),NULL,NULL,(LPBYTE)&NamesCount,&size) != ERROR_SUCCESS)
		error |=16;

	// Get the DNS names
	m_listDNSNames.ClearList();
	for(count = 0;count < (int)NamesCount; count++) {
		size = WSS_BUFFER_SIZE;
		_stprintf(buf,_T("DNSName%d"),count);
		if(RegQueryValueEx(key,buf,NULL,NULL,(LPBYTE)szTemp,&size) != ERROR_SUCCESS){
			error |= 32;
		}
		else {
			szTemp[size] = '\0';
			_tcscpy(buf2, szTemp);
			m_listDNSNames.AddString(buf2);
		}
	}

	RegCloseKey(key);

	m_bSystemInfoChanged = FALSE;

	return error;
}

/*****************************************
Sys_GetLocalNamesCount
Returns the number of local domain names
for the email server
Params
none
Return
number of local domain names
******************************************/
DWORD CUT_MailServer::Sys_GetLocalNamesCount() const
{
	return m_listLocalNames.GetCount();
}

/*****************************************
Sys_GetLocalName
Gets a local domain name by index
Params
index	- index of the name
Return
local domain name
******************************************/
LPCTSTR CUT_MailServer::Sys_GetLocalName(int index) const
{
	return m_listLocalNames.GetString(index);
}

/*****************************************
Sys_AddLocalName
Adds a local domain name
Params
name	- local domain name to add
Return
TRUE	- if success
******************************************/
BOOL CUT_MailServer::Sys_AddLocalName(LPCTSTR name) {
	m_bSystemInfoChanged = TRUE;
	return m_listLocalNames.AddString(name);
}

/*****************************************
Sys_DeleteLocalName
Deletes a local domain name from the list
Params
index	- index of the name
Return
TRUE	- if success
******************************************/
BOOL CUT_MailServer::Sys_DeleteLocalName(int index) {
	m_bSystemInfoChanged = TRUE;
	return m_listLocalNames.DeleteString(index);
}

/*****************************************
Sys_GetDNSNamesCount
Returns the number of DNS servers that 
the email system can use for name resolution
Params
none
Return
number of DNS servers 
******************************************/
DWORD CUT_MailServer::Sys_GetDNSNamesCount() const
{
	return m_listDNSNames.GetCount();
}

/*****************************************
Sys_GetDNSName
Returns a DNS server name 
Params
index	- index of the name
Return
DNS name
******************************************/
LPCTSTR CUT_MailServer::Sys_GetDNSName(int index) const
{
	return m_listDNSNames.GetString(index);
}

/*****************************************
Sys_AddDNSName
Adds a DNS server name
Params
name	- DNS server name to add
Return
TRUE	- if success
******************************************/
BOOL CUT_MailServer::Sys_AddDNSName(LPCTSTR name) {
	m_bSystemInfoChanged = TRUE;
	return m_listDNSNames.AddString(name);
}

/*****************************************
Sys_DeleteDNSName
Deletes a DNS name from the list
Params
index	- index of the name
Return
TRUE if success
******************************************/
BOOL CUT_MailServer::Sys_DeleteDNSName(int index) 
{
	m_bSystemInfoChanged = TRUE;
	return m_listDNSNames.DeleteString(index);
}
/*****************************************
Sys_GetRelayAddressCount


Params
none
Return

******************************************/
DWORD CUT_MailServer::Sys_GetRelayAddressCount() const
{
	return m_listRelayingIPAddresses.GetCount();
}

/*****************************************
Sys_AddDNSName
Adds a DNS server name
Params
name	- DNS server name to add
Return
TRUE	- if success
******************************************/
BOOL CUT_MailServer::Sys_AddRelayAddress(LPCTSTR name)
{
	m_bSystemInfoChanged = TRUE;
	return m_listRelayingIPAddresses.AddString(name);
}


/*****************************************
Sys_DeleteRelayAddress


Params
index	- index of the name
Return
TRUE if success
******************************************/
BOOL CUT_MailServer::Sys_DeleteRelayAddress(int index)
{
	m_bSystemInfoChanged = TRUE;
	return m_listRelayingIPAddresses.DeleteString(index);
}
/*****************************************
Sys_GetRelayAddress

Params
index	- index of the name
Return
DNS name
******************************************/
LPCTSTR CUT_MailServer::Sys_GetRelayAddress(int index) const
{
	return m_listRelayingIPAddresses.GetString(index);
}
/*****************************************
Sys_GetRetryInterval
Returns the Retry Interval
Params
none
Return
DWORD
******************************************/
DWORD CUT_MailServer::Sys_GetRetryInterval() const
{
	return m_dwRetryInterval;
}

/*****************************************
Sys_SetRetryInterval
Sets the Retry Interval
Params
newVal	- new value to set
Return
none
******************************************/
void CUT_MailServer::Sys_SetRetryInterval(DWORD newVal){
	m_bSystemInfoChanged = TRUE;
	m_dwRetryInterval = newVal;
}

/*****************************************
Sys_GetRootKey
Returns the Root key
Params
none
Return
LPCSTR
******************************************/
LPCTSTR CUT_MailServer::Sys_GetRootKey() const
{
	return m_szRootKey;
}

/*****************************************
Sys_SetRootKey
Sets the Root key
Params
newVal	- new value to set
Return
none
******************************************/
void CUT_MailServer::Sys_SetRootKey(LPCTSTR newVal)
{
	if(newVal == NULL)
		return;

	_tcsncpy(m_szRootKey, newVal, WSS_BUFFER_SIZE);
	m_szRootKey[WSS_BUFFER_SIZE - 1] = 0;
}

/*****************************************
Sys_GetRootPath
Returns the Root Path
Params
none
Return
LPCSTR
******************************************/
LPCTSTR CUT_MailServer::Sys_GetRootPath() const
{
	return m_szRootPath;
}

/*****************************************
Sys_SetRootPath
Sets the Root Path
Params
newVal	- new value to set
Return
none
******************************************/
void CUT_MailServer::Sys_SetRootPath(LPCTSTR newVal)
{
	if(newVal == NULL)
		return;

	_tcsncpy(m_szRootPath, newVal, WSS_BUFFER_SIZE);
	m_szRootPath[WSS_BUFFER_SIZE - 1] = 0;
}
/*****************************************
Sys_GetMaxOut
Gets maximum number of mail sending threads
Params
none
Return
DWORD
******************************************/
DWORD CUT_MailServer::Sys_GetMaxOut() const
{
	return m_dwMaxOut;
}

/*****************************************
Sys_SetMaxOut
Sets maximum number of mail sending threads
Params
newVal	- new value to set
Return
none
******************************************/
void CUT_MailServer::Sys_SetMaxOut(DWORD newVal){
	m_bSystemInfoChanged = TRUE;
	m_dwMaxOut = newVal;
}

/*****************************************
Sys_GetMaxRetries
Returns the maximum number of retries for
outgoing mail
Params
none
Return
DWORD
******************************************/
DWORD CUT_MailServer::Sys_GetMaxRetries() const
{
	return m_dwMaxRetries;
}
/*****************************************
Sys_SetMaxRetries
Sets the maximum number of retries for
outgoing mail
Params
newVal	- new value to set
Return
none
******************************************/
void CUT_MailServer::Sys_SetMaxRetries(DWORD newVal){
	m_bSystemInfoChanged = TRUE;
	m_dwMaxRetries = newVal;
}

/*****************************************
Sys_IsDomainLocal
Checks if the domain name is local
Params
domain	- domain name to check
Return
TRUE	- if the given domain name is
in the local domain names list
******************************************/
int CUT_MailServer::Sys_IsDomainLocal(LPCTSTR domain) const
{
	for(int index=0;index < m_listLocalNames.GetCount();index++){
		if(_tcsicmp(domain,m_listLocalNames.GetString(index))==0)
			return TRUE;
	}
	return FALSE;
}

/*****************************************
ReadLineFromFile
Reads header line by line
Probably best to call this with a buffer of 512 - header lines
are often limited to 65 or 72 chars, but no guarantees.

If an EOL cannot be identified, return all.  Message is probably
malformed.
Params
fileHandle	- file handle to read from
buf			- read buffer
bufLen		- read buffer length
Return
length of line for success.  On success
buf passed in will point to a zero-terminated
string.  (We assume text file)

0 indicates empty and no more to read - this will
happen when the end of the header is reached, or
(malformed message) end of file is reached.

We assume that this will be called consecutively with
no intervening file pointer operations.

-1 = ReadFile error
******************************************/
int CUT_MailServer::ReadLineFromFile(long fileHandle, LPBYTE buf,int bufLen)
{
	DWORD readLen;

	if(ReadFile((HANDLE)(ULONG_PTR)fileHandle,buf,(DWORD)bufLen,&readLen,NULL) == TRUE) {

		// No more ?
		if(0 == readLen) return 0;

		int plus = 0;

		// Find first of cr or lf - according to the RFCs, crlf should 
		// terminate lines, but MLM types don't know that...
		unsigned int pos;
		for (pos = 0; pos < readLen; pos++) {
			// Check for cr, crlf, or lf.
			// We want to return what we find without the 
			// crlf, but set file pointer to the byte beyond whatever
			// terminates the string...
			if(13 == *(buf+pos)) {          // cr found...
				*(buf+pos) = 0;             // terminate
				if(10 == *(buf+pos+1)) {    // followed by linefeed - as expected
					plus = 2;
					break;
				}
				else {                      // no linefeed - rather odd
					plus = 1;
					break;
				}
			}
			else {
				if(10 == *(buf+pos)) {      // possibly lf alone....
					*(buf+pos) = 0;         // terminate
					plus = 1;
					break;
				}
			}
		}

		// If pos == readlen at this point then the loop failed to find 
		// a line termination.  For our purposes, assuming the call was
		// made with a large enough buffer, we can assume the message is
		// malformed and return 0.
		if(pos == readLen)
			return 0;

		// Set the file pointer sso that next call picks-up where we left off.
		(void) SetFilePointer(  (HANDLE)(ULONG_PTR) fileHandle, pos+plus - readLen, 
			NULL, FILE_CURRENT);

		return pos;
	}

	return -1;      // file read error
}

/***************************************************
GetUIDL
Retrieve the message id for a particular file
and store in string.
Params
fileHandle	- file handle to read from
buf			- read buffer
bufLen		- read buffer length
msg			- message number
Return
UT_SUCCESS	- success
UT_ERROR	- error
***************************************************/
int  CUT_MailServer::GetUIDL(long fileHandle, int /* msg */, LPBYTE buf,int bufLen)
{

	int		len = 1, len2;
	int		done = 0, ok = 0;
	char	buffer[WSS_BUFFER_SIZE + 1];

	while (len > 0 && !done) 
	{   // while still header lines....
		// Read a line
		ok = 0;
		len = ReadLineFromFile(fileHandle,(LPBYTE)buffer, WSS_BUFFER_SIZE);

		// Does this contain the message id?
		if(strnicmp(buffer, "Message-Id:",11) == 0)
		{

			// Ok - some messages might leave the id on the next line, so
			// lets see if buffer+13 is blank.  (there is another problem
			// here in that if there is no space after the ':' we won't
			// even find the Message-Id: string, but we can fall back
			// to no id found.  (The rfc requires a space so we'll argue
			// the message header is malformed)

			// check for at least one non-blank char in this line...
			len2 = (int)strlen(buffer+11);
			for(int i = 12; i < len2; i++) 
			{
				if(!isspace(*(buffer+i)) )
				{
					ok = 1;
					break;

				}
			}
			if(ok) 
			{
				// Valid chars - we have our id.
				// if buffer too small, truncate - what else can we do?
				if((int)strlen(buffer+12) > bufLen) 
				{
					strncpy((char*)buf, buffer+12, bufLen-1);
					*(buf+bufLen-1) = 0;
				}
				else 
					strcpy((char*)buf, buffer+12);       
				done = TRUE;
			}
			else 
			{
				// Message id must be on next line
				len = ReadLineFromFile(fileHandle,(LPBYTE)buffer, WSS_BUFFER_SIZE);
				if(len > 0) 
				{
					// remove the leading spaces
					int loop = 0;
					while (isspace((int)buffer[loop]) && loop < len)
						loop++;
					// make sure the line is not empty
					// and that the line we read is not one of the headers 
					// i.e it must have started with a white space
					if (loop < len && loop != 0)  
					{
						strncpy((char*)buf, &buffer[loop], bufLen-1); 
						buf[bufLen] = 0;
						done = TRUE;
					}
				}                               
			}
		}
	}
	if(len > 0)     // oops - got through header with no id found.
		return CUT_SUCCESS;
	else
		return CUT_ERROR;
}


/***************************************************
BuildUniqueID
Create the time/rand portion of a new 
message id.  Note that while this is fine for 
SMTP/POP3, USENET has slightly more particular
requirements for the machine.domain.name
Params
buf			- read buffer
bufLen		- read buffer length
Return
UT_SUCCESS	- success
UT_ERROR	- error
***************************************************/
int CUT_MailServer::BuildUniqueID(char* buf, int bufLen) {

	if(bufLen < 72)
		return CUT_ERROR;

	SYSTEMTIME s;
	GetSystemTime(&s);
	_snprintf(buf,bufLen, "<%d%d%d%d%d%d%d",s.wYear,s.wMonth,s.wDay,s.wHour,s.wMinute,s.wSecond,s.wMilliseconds);

	srand(GetTickCount());
	int pos = (int)strlen(buf);
	// add random characters onto the end of the string
	int i;
	for (i = pos; i < (pos+12); i++) 
		buf[i]= (char)((rand()%25) + 65);

	buf[i] = 0;

	strcat(buf, "@");
	if(m_listLocalNames.GetString(0L))
		// v4.2 using AC here - local names now _TCHAR")
		strcat(buf, AC(m_listLocalNames.GetString(0L)));
	else
		strcat(buf, "ret.mail.unknown");

	strcat(buf, ">");

	return CUT_SUCCESS;
}

/***************************************************
GetPOP3Server
Returns pointer to the POP3 server class
Params
none
Return
pointer to the POP3 server class
****************************************************/
CUT_POP3Server * CUT_MailServer::GetPOP3Server() 
{
	if(!m_bClassesInitialized)
		CUT_MailServer::SetServerClasses();

	return m_ptrPOP3Server;
}

/***************************************************
GetSMTPServer
Returns pointer to the SMTP server class
Params
none
Return
pointer to the SMTP server class
****************************************************/
CUT_SMTPServer * CUT_MailServer::GetSMTPServer() 
{
	if(!m_bClassesInitialized)
		SetServerClasses();
	return m_ptrSMTPServer;
}

/***************************************************
GetSMTPOutServer
Returns pointer to the SMTPOut server class
Params
none
Return
pointer to the SMTPOut server class
****************************************************/
CUT_SMTPOut * CUT_MailServer::GetSMTPOutServer() 
{
	if(!m_bClassesInitialized)
		SetServerClasses();
	return m_ptrSMTPOutServer;
}

/***************************************************
GetDataManager
Returns pointer to the DataManager class
Params
none
Return
pointer to the DataManager class
****************************************************/
CUT_DataManager * CUT_MailServer::GetDataManager() 
{
	if(!m_bClassesInitialized)
		SetServerClasses();
	return m_ptrDataManager;
}

/***************************************************
GetUserManager
Returns pointer to the UserManager class
Params
none
Return
pointer to the UserManager class
****************************************************/
CUT_UserManager * CUT_MailServer::GetUserManager() 
{
	if(!m_bClassesInitialized)
		SetServerClasses();
	return m_ptrUserManager;
}

#pragma warning ( pop )
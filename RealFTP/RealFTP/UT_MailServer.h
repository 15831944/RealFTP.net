//////////////////////////////////////////////////////////////////////
// ==================================================================
//  class:  CUT_MailServer
//  File:   UT_MailServer.h
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

#if !defined(AFX_UT_MAILSERVER_H__3605C750_175B_11D3_A47A_0080C858F182__INCLUDED_)
#define AFX_UT_MAILSERVER_H__3605C750_175B_11D3_A47A_0080C858F182__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "UTErr.h"

#include "pop3_s.h"
#include "smtp_s.h"
#include "smtp_o.h"

class CUT_DataManager;

class CUT_MailServer {

// Define friend classes
friend  CUT_DataManager;
friend  CUT_UserManager;
friend  CUT_POP3Thread;
friend  CUT_POP3Server;
friend  CUT_SMTPThread;
friend  CUT_SMTPServer;
friend  CUT_SMTPOut;

protected:
    BOOL    m_bClassesInitialized;              // TRUE if server classes pointers are initialized

    _TCHAR    m_szRootPath[WSS_BUFFER_SIZE + 1];  // Mail server root path
    _TCHAR    m_szRootKey[WSS_BUFFER_SIZE + 1];   // Root registry key to load/save data

	// 4.2 - all _TCHAR now...
    CUT_TStringList     m_listLocalNames;       // List of local names
    CUT_TStringList     m_listDNSNames;         // List of DNS names
	CUT_TStringList		m_listRelayingIPAddresses ; // list of allowed IP Addresses to relay

    DWORD               m_dwMaxOut;             // Maximum piece of mail that can be sent out at once
    DWORD               m_dwMaxRetries;         // Maximum number of retries for outgoing mail
    DWORD               m_dwRetryInterval;      // Retry interval

    CUT_POP3Server      *m_ptrPOP3Server;       // Pointer to the POP3 server class
    BOOL                m_bDeletePOP3;          // If TRUE we use default class, so we need to clean it up

    CUT_SMTPServer      *m_ptrSMTPServer;       // Pointer to the SMTP server class
    BOOL                m_bDeleteSMTP;          // If TRUE we use default class, so we need to clean it up
    
    CUT_SMTPOut         *m_ptrSMTPOutServer;    // Pointer to the SMTPOut server class
    BOOL                m_bDeleteSMTPOut;       // If TRUE we use default class, so we need to clean it up

    CUT_DataManager     *m_ptrDataManager;      // Pointer to the Data managment class
    BOOL                m_bDeleteDataManager;   // If TRUE we use default class, so we need to clean it up

    CUT_UserManager     *m_ptrUserManager;      // Pointer to the Users managment class
    BOOL                m_bDeleteUserManager;   // If TRUE we use default class, so we need to clean it up

    BOOL                m_bSystemInfoChanged;   // If TRUE system info was changed
    BOOL                m_bDestroyingObject;    // If TRUE we are in process of desroying object

protected:
    // Reads line from the file
    virtual int     ReadLineFromFile(long fileHandle,LPBYTE buf,int bufLen);

    // Retrieve the message id for a particular file and store in string
    virtual int     GetUIDL(long fileHandle, int msg,LPBYTE buf,int bufLen);

    // Create the time/rand portion of a new message id
    virtual int     BuildUniqueID(char* buf, int bufLen);

public:
    // Constructor
    CUT_MailServer();

    // Destructor
    virtual ~CUT_MailServer();

    // Set server classes pointers
    virtual int SetServerClasses(   CUT_POP3Server  *ptrPOP3Server      = NULL,
                                    CUT_SMTPServer  *ptrSMTPServer      = NULL,
                                    CUT_SMTPOut     *ptrSMTPOutServer   = NULL,
                                    CUT_DataManager *ptrDataManager     = NULL,
                                    CUT_UserManager *ptrUserManager     = NULL);

    // Starts up the Mail Server.
    virtual int     Start();

    // Load/Save system information
    virtual int     LoadSystemInfo(LPCTSTR subKey);
    virtual int     SaveSystemInfo(LPCTSTR subKey);

    // Gets number of local domain names
    virtual DWORD   Sys_GetLocalNamesCount() const;
    // Adds a local domain name to the list
    virtual BOOL    Sys_AddLocalName(LPCTSTR name);
    // Deletes a local domain name from the list
    virtual BOOL    Sys_DeleteLocalName(int index);
    // Gets a local domain name from the list
    virtual LPCTSTR  Sys_GetLocalName(int index) const;

    // Gets number of DNS names
    virtual DWORD   Sys_GetDNSNamesCount() const;
    // Adds a DNS name to the list
    virtual BOOL    Sys_AddDNSName(LPCTSTR name);
    // Deletes a DNS name from the list
    virtual BOOL    Sys_DeleteDNSName(int index);
    // Gets a DNS name from the list
    virtual LPCTSTR  Sys_GetDNSName(int index) const;

	 // Gets number of Allowed relaying IP Addreses
    virtual DWORD   Sys_GetRelayAddressCount() const;
    // Adds an Allowed relaying IP Addreses
    virtual BOOL    Sys_AddRelayAddress(LPCTSTR name);
    // Deletes a DNS name from the list
    virtual BOOL    Sys_DeleteRelayAddress(int index);
    // Gets a DNS name from the list
    virtual LPCTSTR  Sys_GetRelayAddress(int index) const;

    // Checks if the given domain name is in the local domain names list
    virtual int     Sys_IsDomainLocal(LPCTSTR domain) const;

    // Get/Set Maximum piece of mail that can be sent out at once
    DWORD           Sys_GetMaxOut() const;
    void            Sys_SetMaxOut(DWORD newVal);

    // Get/Set Maximum number of retries for outgoing mail
    DWORD           Sys_GetMaxRetries() const;
    void            Sys_SetMaxRetries(DWORD newVal);

    // Get/Set Retry interval
    DWORD           Sys_GetRetryInterval() const;
    void            Sys_SetRetryInterval(DWORD newVal);

    // Get/Set Mail server root registry key for saving/loading data
    LPCTSTR          Sys_GetRootKey() const;
    void            Sys_SetRootKey(LPCTSTR newVal);

    // Get/Set Mail server root path for saving/loading messages
    LPCTSTR         Sys_GetRootPath() const;
    void            Sys_SetRootPath(LPCTSTR newVal);

    // Returns pointer to the specified mail server class
    CUT_POP3Server  *GetPOP3Server();
    CUT_SMTPServer  *GetSMTPServer();
    CUT_SMTPOut     *GetSMTPOutServer();
    CUT_DataManager *GetDataManager();
    CUT_UserManager *GetUserManager();

protected:        

    /////////////////////////////////////////////////////////////
    // Overridables
    /////////////////////////////////////////////////////////////

    // On display status
    virtual int     OnStatus(LPCTSTR StatusText);
#if defined _UNICODE
    virtual int     OnStatus(LPCSTR StatusText);
#endif
    // Return the error code passed. Called each time error occured
    virtual int     OnError(int error);

    // This virtual function is called each time the server 
    // accepting new connection.
    virtual long    OnCanAccept(LPCSTR address);

};

#endif // !defined(AFX_UT_MAILSERVER_H__3605C750_175B_11D3_A47A_0080C858F182__INCLUDED_)

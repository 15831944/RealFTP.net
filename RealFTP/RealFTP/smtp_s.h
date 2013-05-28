// =================================================================
//  class: CUT_SMTPServer
//  class: CUT_SMTPThread
//  File:  smtp_s.h
//  
//  Purpose:
//
//      Declares CUT_SMTPServer and CUT_SMTPThread classes
//      for SMTP server implementation.
//
// =================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// =================================================================

#ifndef SMTP_SERVER_H
#define SMTP_SERVER_H

#include "ut_srvr.h"
#include "fileman.h"
#include <stdio.h>


class CUT_SMTPThread;
class CUT_MailServer;

// SMTP commands ID enumeration
typedef enum SMTPCommandIDTag {
    CMD_UNKNOWN_SMTP,
    CMD_SMTPHELO,
    CMD_SMTPMAIL,
    CMD_SMTPRCPT,
    CMD_SMTPDATA,
    CMD_SMTPHELP,
    CMD_SMTPQUIT,
    CMD_SMTPRSET,
    CMD_SMTPNOOP,
	CMD_SMTPSTARTTLS,
	CMD_SMTPEHLO
    } SMTPCommandID;

// ===================================
//   CUT_SMTPServer class
// ===================================
class CUT_SMTPServer : public CUT_WSServer {

friend CUT_SMTPThread;                          // Make SMTP thread class a friend

protected:
    CUT_MailServer      *m_ptrMailServer;       // Pointer to the Mail Server class

#ifdef CUT_SECURE_SOCKET
	BOOL				m_bImmediateNegotiation;
#endif

public:

    CUT_SMTPServer(CUT_MailServer &ptrMailServer);
    virtual ~CUT_SMTPServer();

    // Create class instance callback, this is where a C_WSTHREAD clss is created
    virtual         CUT_WSThread    *CreateInstance();

    // This function is called so that the instance created above can be released
    virtual void    ReleaseInstance(CUT_WSThread *ptr);

    // Starts up the SMTP Server.
    virtual int     Start();

#ifdef CUT_SECURE_SOCKET
	// Set immediate security negotiation flag
	virtual	void	SetImmediateNegotiation(BOOL bFlag);
#endif

protected:
    // This virtual function is called each time the server 
    // accepting new connection.
    virtual long    OnCanAccept(LPCSTR address);
};


// ===================================
//   CUT_SMTPThread class
// ===================================
class CUT_SMTPThread : public CUT_WSThread {

protected:

    // Returns a command ID from the given command line    
    SMTPCommandID   GetCommand(LPSTR data);

    // This function is called whenever a new connection is made
    // and the max. number of connections has already been reached
    virtual void    OnMaxConnect();

    // This function is called whenever a new connection is made
    virtual void    OnConnect();

    // Sends out help information when a help command is issued
    virtual void    OnHelpCommand();

#ifdef CUT_SECURE_SOCKET

	// Non secure client trying to connect to the secure server
	virtual BOOL	OnNonSecureConnection(LPCSTR IpAddress);

	// On socket connection
	virtual int		SocketOnConnected(SOCKET s, const char *lpszName);

#endif

public:
	BOOL CheckRelay(LPCSTR szFrom, LPCSTR szTo, LPCSTR szHost);
	// virtual function to allow for aborting connections from specific email address 
	// such as empty from fields or known Spammers
	// also it is good place to log the from addresses
	virtual int OnFrom(LPSTR szMailFrom, LPCSTR szMailhost);
};

#endif

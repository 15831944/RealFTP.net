//=================================================================
//  class: CUT_POP3Server
//  class: CUT_POP3Thread
//  File:  pop3_s.h
//  
//  Purpose:
//
//      POP3 server and thread classes declared.
//      
// ===================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// ===================================================================

#ifndef POP3_SERVER_H
#define POP3_SERVER_H


#include "ut_srvr.h"
#include "fileman.h"
#include <stdio.h>
#include <limits.h>

class CUT_POP3Thread;
class CUT_MailServer;

// Command ID enumeration
typedef enum POP3CommandIDTag {
    CMD_UNKNOWN_POP3,
    CMD_POP3USER,
    CMD_POP3PASS,
    CMD_POP3STAT,
    CMD_POP3LIST,
    CMD_POP3RETR,
    CMD_POP3DELE,
    CMD_POP3NOOP,
    CMD_POP3RSET,
    CMD_POP3QUIT,
    CMD_POP3TOP,
    CMD_POP3UIDL,
	CMD_STLS,
	CMD_CAPA
} POP3CommandID;

// ===================================
//  CUT_POP3Server class
// ===================================
class CUT_POP3Server : public CUT_WSServer {

friend CUT_POP3Thread;                      // Make POP3 thread class a friend

protected:
    CUT_MailServer  *m_ptrMailServer;       // Pointer to the Mail Server class

public:
    // Constructor/Destructor
    CUT_POP3Server(CUT_MailServer &ptrMailServer);
    virtual ~CUT_POP3Server();
    
    // Starts up the POP3 Server.
    virtual int             Start();

#ifdef CUT_SECURE_SOCKET
	// Set immediate security negotiation flag
	virtual	void	SetImmediateNegotiation(BOOL bFlag);
#endif

protected:
    // This virtual function is called each time the server 
    // is about to accept a new connection.
    virtual long    OnCanAccept(LPCSTR address);

    // Create class instance callback, this is where a C_WSTHREAD class is created
    virtual CUT_WSThread    *CreateInstance();

    // This function is called so that the instance created above can be released
    virtual void            ReleaseInstance(CUT_WSThread *ptr);

#ifdef CUT_SECURE_SOCKET
	BOOL				m_bImmediateNegotiation;
#endif

};

// ===================================
//  CUT_POP3Thread class
// ===================================
class CUT_POP3Thread : public CUT_WSThread {

protected:

    // Returns a command ID from the given command line
    POP3CommandID   GetCommand(LPSTR data);

    // This function is called whenever a new connection is made
    // and the max. number of connections has already been reached
    virtual void    OnMaxConnect();

    // This function is called whenever a new connection is made
    virtual void    OnConnect();

#ifdef CUT_SECURE_SOCKET

	// Non secure client trying to connect to the secure server
	virtual BOOL	OnNonSecureConnection(LPCSTR IpAddress);

	// On socket connection
	virtual int		SocketOnConnected(SOCKET s, const char *lpszName);

#endif
};

#endif

// =================================================================
//  class: CUT_POP3Server
//  class: CUT_POP3Thread
//  File:  pop3_s.cpp
//
//  Purpose:
//
//  Implementation of the POP3 server and thread classes.
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

#ifdef _WINSOCK_2_0_
#define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
/* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include "pop3_s.h"
#include "UT_MailServer.h"

#include "ut_strop.h"

#ifdef UT_DISPLAYSTATUS
#include "uh_ctrl.h"
extern CUH_Control  *ctrlHistory;
#endif 

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

// =================================================================
// CUT_POP3Server class
// =================================================================

/***************************************
Constructor
****************************************/
CUT_POP3Server::CUT_POP3Server(CUT_MailServer &ptrMailServer) : 
m_ptrMailServer(&ptrMailServer) // Initialize pointer to the Mail Server class
{
    m_nPort = 110; // Set default port number to 110

#ifdef CUT_SECURE_SOCKET
   m_bImmediateNegotiation = FALSE;
#endif
}

/***************************************
Destructor
****************************************/
CUT_POP3Server::~CUT_POP3Server(){

    m_bShutDown = TRUE;
    while(GetNumConnections())
	    Sleep(50);

}

/********************************
CreateInstance
Creates an Instance of a
CUT_POP3Thread class and return
a pointer to it
Params
none
Return
pointer to an instance of a 
CUT_POP3Thread
*********************************/
CUT_WSThread * CUT_POP3Server::CreateInstance(){
    return new CUT_POP3Thread;
}

/********************************
ReleaseInstance
Releases a given instance of a
CUT_POP3Thread class
Params
ptr - pointer to the instance
Return
none
*********************************/
void CUT_POP3Server::ReleaseInstance(CUT_WSThread *ptr){
    if(ptr != NULL)
        delete ptr;
}

/********************************
Start
Starts up the POP3 Server.
Opens the port and listens for connections.
Params
none
Return
UTE_SUCCESS         - success
UTE_CONNECT_FAILED  - connection failed
UTE_ACCEPT_FAILED   - start accepting connections failed
*********************************/
int CUT_POP3Server::Start(){
    if(ConnectToPort(m_nPort) != CUT_SUCCESS)
        return m_ptrMailServer->OnError(UTE_CONNECT_FAILED);
    if(StartAccept() != CUT_SUCCESS)
        return m_ptrMailServer->OnError(UTE_ACCEPT_FAILED);
    
    return m_ptrMailServer->OnError(CUT_SUCCESS);
}

/***************************************************
OnCanAccept
This virtual function is called each time the server 
accepting new connection.
Params
LPCSTR      - IP address of connecting user
Return
TRUE        - accept 
FALSE       - cancel
****************************************************/
long CUT_POP3Server::OnCanAccept(LPCSTR address){
    if(m_ptrMailServer == NULL)
        return TRUE;
    else
        return m_ptrMailServer->OnCanAccept(address);
}

#ifdef CUT_SECURE_SOCKET

/*******************************************
SetImmediateNegotiation
    Set immediate security negotiation flag
Params
    bFlag	- TRUE if security negotiation should
				start right after connection
Return
    none
*******************************************/
void CUT_POP3Server::SetImmediateNegotiation(BOOL bFlag)
{
   m_bImmediateNegotiation = bFlag;
}

#endif

// =================================================================
// CUT_POP3Thread class
// =================================================================

/********************************
OnMaxConnect
This function is called when a
new connection is made and the
system has already reached its
max. number of allowed connections
Params
none
Return 
none
*********************************/
void CUT_POP3Thread::OnMaxConnect(){
    // Send too busy message
    Send("-ERR  System Too Busy Please Try Later\r\n");
    ((CUT_POP3Server *)(m_winsockclass_this))->m_ptrMailServer->OnStatus("-ERR  System Too Busy Please Try Later\r\n");
}

/********************************
OnConnect
This function is called when a new 
connection is made. This function
receives all of the POP3 commands
and processes them
Params
none
Return
none
*********************************/
void CUT_POP3Thread::OnConnect(){
    char            szUserName[WSS_BUFFER_SIZE + 1];
    char            szPassword[WSS_BUFFER_SIZE + 1];
    char            szBuffer[WSS_LINE_BUFFER_SIZE + 1];
    char            messageStatus[MAX_PATH + 1];
    char            ipbuf[32];
    int             nLength;
    BOOL            bLogInState     = FALSE;
    long            lUserHandle     = -1;
    POP3CommandID   nCommand        = CMD_UNKNOWN_POP3;
    BOOL            bQuit           = FALSE;
    CUT_MailServer  *ptrMailServer  = ((CUT_POP3Server *)m_winsockclass_this)->m_ptrMailServer;
    CUT_UserManager *ptrUserManager = ptrMailServer->m_ptrUserManager;

#ifdef CUT_SECURE_SOCKET

	BOOL            bHandshakeDone	= FALSE;	

	// Disable security while connecting
	BOOL bSecureFlag = GetSecurityEnabled();
	if(!ptrMailServer->GetPOP3Server()->m_bImmediateNegotiation)
		SetSecurityEnabled(FALSE);
	else
		bHandshakeDone = TRUE;

#endif

    // Initialize user name and password
    szUserName[0]   = 0;
    szPassword[0]   = 0;
    
    // Initialize client IP address
    _snprintf(ipbuf,sizeof(ipbuf)-1, "%d.%d.%d.%d", m_clientSocketAddr.sin_addr.S_un.S_un_b.s_b1,
        m_clientSocketAddr.sin_addr.S_un.S_un_b.s_b2,
        m_clientSocketAddr.sin_addr.S_un.S_un_b.s_b3,
        m_clientSocketAddr.sin_addr.S_un.S_un_b.s_b4);
    
    // Display status - client IP address 
    ptrMailServer->OnStatus( " " );
    ptrMailServer->OnStatus( ipbuf );
    
    // Send inital message
    Send("+OK  POP3 server ready\r\n");
    ptrMailServer->OnStatus("+OK  POP3 server ready");
    
    // Main loop
    while(bQuit == FALSE) {
        
        // Receive a line from the client
        nLength = ReceiveLine(szBuffer, WSS_LINE_BUFFER_SIZE);
        
        // Exit on an error
        if(nLength <= 0) {
            bQuit = TRUE;
            break;  
        }
        
        CUT_StrMethods::RemoveCRLF(szBuffer);
        if(0 == _strnicmp(szBuffer, "PASS", 4)) 
            ptrMailServer->OnStatus( "PASS *********" );
        else {
            ptrMailServer->OnStatus( "Command rec'd: " );
            ptrMailServer->OnStatus( szBuffer );
        }
        
        // Get the command that was sent
        nCommand = GetCommand(szBuffer);
        
        // Check to see if it is a command that can run without logging in
        switch(nCommand){
            
        case CMD_POP3USER:
			{
#ifdef CUT_SECURE_SOCKET

			if(!bHandshakeDone  && bSecureFlag )
			{
				if(!OnNonSecureConnection(GetClientAddress()))
				{
					Send("-ERR Secure connection required\r\n");
					ptrMailServer->OnStatus("Non secure connection rejected");
					bQuit = TRUE;
					continue;
				}
			}
            
#endif

            // Save the user name
            CUT_StrMethods::ParseString(szBuffer," ",1,szUserName, WSS_BUFFER_SIZE);
            
            // Send back a notification
            // always send OK even if the wrong user is entered
            // this way password cracking is harder
            Send("+OK\r\n");
            strcpy(messageStatus,szUserName);
            strcat(messageStatus," POP3: USER NAME OK");
            ptrMailServer->OnStatus(messageStatus);
            continue;
            }
            
        case CMD_POP3PASS:
			{
#ifdef CUT_SECURE_SOCKET

			if(!bHandshakeDone  && bSecureFlag )
			{
				if(!OnNonSecureConnection(GetClientAddress()))
				{
					Send("-ERR Secure connection required\r\n");
					ptrMailServer->OnStatus("Non secure connection rejected");
					bQuit = TRUE;
					continue;
				}
			}
				
#endif

			// Save the user name
            CUT_StrMethods::ParseString(szBuffer," ",1,szPassword,WSS_BUFFER_SIZE);
            
            // Check to see if the user exists
			// v4.2 using WC here - user info is now _TCHAR
            lUserHandle = ptrUserManager->User_OpenUser(WC(szUserName),WC(szPassword));
            
            if(lUserHandle < 0) {
                
                // Send back a access denied notification
                Send("-ERR\r\n");
                strcpy(messageStatus,szUserName);
                strcat(messageStatus," POP3: PASSWORD ERROR");
                ptrMailServer->OnStatus(messageStatus);
            }
            else {
                
                // Send back a notification
                Send("+OK\r\n");
                strcpy(messageStatus,szUserName);
                strcat(messageStatus," POP3:  PASSWORD OK");
                ptrMailServer->OnStatus(messageStatus);
                bLogInState = TRUE;
            }
            continue;
                          }
            
        case CMD_POP3NOOP: 
            {
                // Just send an OK notify
                Send("+OK\r\n");
                strcpy(messageStatus,szUserName);
                strcat(messageStatus," POP3: NOOP OK");
                ptrMailServer->OnStatus(messageStatus);
                continue;
            }
            
        case CMD_POP3QUIT:
            {
                // Send an OK command
                Send("+OK\r\n");
                strcpy(messageStatus,szUserName);
                strcat(messageStatus," POP3: QUIT OK");
                ptrMailServer->OnStatus(messageStatus);
                
                // Exit the message pump loop
                bQuit = TRUE;
                continue;
            }
		
		case CMD_CAPA:
			{
				// Start capabilties list
				Send("+OK Capability list follows\r\n");
				Send("TOP\r\n");
				Send("USER\r\n");
				Send("UIDL\r\n");

#ifdef CUT_SECURE_SOCKET

				// Add TLS in the secure server
				if(bSecureFlag && !bHandshakeDone)
					Send("STARTTLS\r\n");
				
#endif

				// End of capabilities list
				Send(".\r\n");

				continue;
			}

        case CMD_STLS:
            {

#ifdef CUT_SECURE_SOCKET

				if(bSecureFlag)
				{
					// Already done
					if(bHandshakeDone)
					{
						Send("-ERR Command not permitted when TLS active\r\n");
						continue;
					}

					// Send an OK command
					Send("+OK Begin TLS negotiation\r\n");

					// Set security flag
					SetSecurityEnabled(TRUE);

					// Start negotiation
					int nResult = CUT_SecureSocket::SocketOnConnected(m_clientSocket, "");
					if (nResult != UTE_SUCCESS)
					{
						SetSecurityEnabled(FALSE);									
						Send("-ERR  The SSL negotiation failed during the handshake");
						bQuit = TRUE;
						continue;
					} 
					else
					{
						Send("+OK\r\n");
						bHandshakeDone =  TRUE;
						continue;
					}
				}
				else
					Send("+ERR TLS is not supported\r\n");

#else

				Send("+ERR TLS is not supported\r\n");

#endif

                continue;
            }
		default:
                    {
                        if(bLogInState == FALSE) {
                            Send("-ERR authentication exchange failed\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus,"POP3 -ERR Unknown Command");
                            ptrMailServer->OnStatus(messageStatus);
						    continue;
                            }
                    }

        }
        
        // Check to see if the client is logged in
        if(bLogInState == TRUE) {
            
            // Check the rest of the commands
            switch(nCommand) {
                
            case CMD_POP3STAT:
                {
                    // Get the size of all messages
                    long totalSize =0;
                    long index;
                    long numDeleted = 0, numMsgs = ptrUserManager->User_GetFileCount(lUserHandle);
                    for(index = 0; index < numMsgs; index++)
                        if(!ptrUserManager->User_IsFileDeleted(lUserHandle,index))
                            totalSize += ptrUserManager->User_GetFileSize(lUserHandle,index);
                        else
                            ++numDeleted;
                        
                        _snprintf(szBuffer,sizeof(szBuffer)-1,"+OK %d %ld\r\n",numMsgs - numDeleted,totalSize);
                        Send(szBuffer);
                        
                        strcpy(messageStatus,szUserName);
                        strcat(messageStatus," POP3: STAT ");
                        ptrMailServer->OnStatus(messageStatus);
                        continue;
                }
            case CMD_POP3LIST:
                {
                    // Find which message to show
                    long param;
                    if(CUT_StrMethods::ParseString(szBuffer," ",1,&param) == CUT_SUCCESS){
                        param --;  // POP3 starts from one CDataManager starts from 0
                        
                        // Show the single message size
                        if(param < 0 || param >= ptrUserManager->User_GetFileCount(lUserHandle)) {
                            Send("-ERR  Invalid Message Number\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: -ERR  Invalid Message Number");
                            ptrMailServer->OnStatus(messageStatus);
                        }
                        else if(ptrUserManager->User_IsFileDeleted(lUserHandle,param)) {
                            Send("-ERR  Message marked as deleted\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: -ERR  Message marked as deleted");
                            ptrMailServer->OnStatus(messageStatus);
                        }
                        else {
                            _snprintf(szBuffer,sizeof(szBuffer)-1,"+OK %d %ld\r\n",param+1,
                                ptrUserManager->User_GetFileSize(lUserHandle,param));
                            Send(szBuffer);
                            
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: Message Size");
                            ptrMailServer->OnStatus(messageStatus);
                        }
                    }
                    else {
                        // Show the total size for all the messages
                        long totalSize =0;
                        long index;
                        long numDeleted = 0, numMsgs = ptrUserManager->User_GetFileCount(lUserHandle);
                        for(index = 0; index < numMsgs; index++)
                            if(!ptrUserManager->User_IsFileDeleted(lUserHandle,index))
                                totalSize += ptrUserManager->User_GetFileSize(lUserHandle,index);
                            else
                                ++numDeleted;
                            
                            _snprintf(szBuffer,sizeof(szBuffer)-1,"+OK %d messages (%ld octets)\r\n",numMsgs - numDeleted,totalSize);
                            Send(szBuffer);
                            
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: Total size of all messages ");
                            ptrMailServer->OnStatus(messageStatus);  
                            
                            // Now show the size of each
                            for(index = 0; index < numMsgs; index++) {
                                if(!ptrUserManager->User_IsFileDeleted(lUserHandle,index)) {
                                    _snprintf(szBuffer,sizeof(szBuffer)-1,"%d %ld\r\n",index+1,
                                        ptrUserManager->User_GetFileSize(lUserHandle,index));
                                    Send(szBuffer);
                                }
                            }
                            
                            Send(".\r\n");
                    }
                    continue;
                }
                
            case CMD_POP3RETR:
                {
                    // Find which message to send
                    long param;
                    if(CUT_StrMethods::ParseString(szBuffer," ",1,&param) == CUT_SUCCESS) {
                        param --;  // POP3 starts from one CDataManager starts from 0
                        
                        // Check to see if the message is valid
                        if(param < 0 || param >= ptrUserManager->User_GetFileCount(lUserHandle)) {
                            Send("-ERR  Invalid Message Number\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: -ERR  Invalid Message Number");
                            ptrMailServer->OnStatus(messageStatus);  
                            continue;
                        }
                        else if(ptrUserManager->User_IsFileDeleted(lUserHandle,param)) {
                            Send("-ERR  Message marked as deleted\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: -ERR  Message marked as deleted");
                            ptrMailServer->OnStatus(messageStatus);
                            continue;
                        }
                        
                        
                        // Send the message size
                        _snprintf(szBuffer,sizeof(szBuffer)-1,"+OK %ld octets\r\n",
                            ptrUserManager->User_GetFileSize(lUserHandle,param));
                        Send(szBuffer);
                        
                        strcpy(messageStatus,szUserName);
                        strcat(messageStatus,szBuffer);
                        ptrMailServer->OnStatus(messageStatus);
                        
                        // Send the message
                        long fileHandle;
                        int  len,lastLen = 0;
                        fileHandle = ptrUserManager->User_OpenFile(lUserHandle,param);
                        if(fileHandle < 0) {
                            Send("-ERR  Invalid Message Number\r\n");
                            
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: -ERR  Invalid Message Number");
                            ptrMailServer->OnStatus(messageStatus);
                            continue;
                        }   
                        
						for (;;) {
							// Read with space for null...
                            len = ptrUserManager->User_ReadFile(fileHandle,(LPBYTE)szBuffer, WSS_LINE_BUFFER_SIZE);
                            if(len <= 0)
                                break;
                            Send(szBuffer,len);
                            lastLen = len;
                        }
                        
                        // Check for crlf.crlf - not foolproof, since we could have
                        // a small read ( < 5 bytes) 
                        if(lastLen >= 5) {
                            *(szBuffer+lastLen) = 0; // terminate string
                            if(!strstr(szBuffer, "\r\n.\r\n")) {
                                ptrMailServer->OnStatus( "ALERT: Malformed closure on message!");
                                Send("\r\n\r\n.\r\n");
                            }
                        }
                        
                        ptrUserManager->User_CloseFile(fileHandle);
                    }
                    else {
                        Send("-ERR  Invalid Message Number\r\n");
                        strcpy(messageStatus,szUserName);
                        strcat(messageStatus," POP3: -ERR  Invalid Message Number");
                        ptrMailServer->OnStatus(messageStatus);
                    }
                    continue;
                }
                
            case CMD_POP3UIDL: 
                {
                    // Client requests unique ID of message
                    // is there a second param?
                    long param;
                    if(CUT_StrMethods::ParseString(szBuffer," ",1,&param) == CUT_SUCCESS){
                        // Yes - send message id for this message..
                        param --;  // POP3 starts from one CDataManager starts from 0
                        
                        // Check to see if the message is valid
                        if(param < 0 || param >= ptrUserManager->User_GetFileCount(lUserHandle)) {
                            Send("-ERR  Invalid Message Number\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: TOP -ERR  Invalid Message Number");
                            ptrMailServer->OnStatus(messageStatus);
                            continue;
                        }
                        else if(ptrUserManager->User_IsFileDeleted(lUserHandle,param)) {
                            Send("-ERR  Message marked as deleted\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: -ERR  Message marked as deleted");
                            ptrMailServer->OnStatus(messageStatus);
                            continue;
                        }
                        
                        
                        long fileHandle;
                        fileHandle = ptrUserManager->User_OpenFile(lUserHandle,param);
                        if(fileHandle < 0) {
                            Send("-ERR  Invalid Message Number\r\n");
                            
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: UIDL -ERR  Invalid Message Number");
                            ptrMailServer->OnStatus(messageStatus);
                            continue;
                        }
                        
                        char buf[20];
                        if(CUT_SUCCESS == ptrMailServer->GetUIDL(fileHandle,param, (LPBYTE)szBuffer, WSS_LINE_BUFFER_SIZE)) {
                            _snprintf(buf,sizeof(buf)-1, "+OK %d ", param+1);
                            Send(buf);
                            Send(szBuffer);
                            Send("\r\n");
                            
                            ptrMailServer->OnStatus(buf);
                            ptrMailServer->OnStatus(szBuffer);
                        }
                        else {          
                            _snprintf(buf,sizeof(buf)-1, "-ERR %d ", param+1);      // should never happen!  Incoming mail
                            Send(buf);                              // has Message-ID: attached if none exists!
                            Send(" message ID not found\r\n");
                            ptrMailServer->OnStatus(buf);
                            ptrMailServer->OnStatus("POP3 UIDL N **Failed to get UIDL**");
                        }
                        
                        ptrUserManager->User_CloseFile(fileHandle);
                        
                    }
                    else {
                        // Get multiline response - message ids for all messages
                        // Get number of messages for user...
                        int fileCount = ptrUserManager->User_GetFileCount(lUserHandle);
                        
                        // Should always be at least one file if client calls this, 
                        // but...
                        if(-1 == fileCount) {
                            Send("-ERR no messages");
                            ptrMailServer->OnStatus("UIDL called with no mail on server");
                            continue;
                        }
                        else
                            Send("+OK unique-id listing follows\r\n");
                        
                        // Ok - loop through file count, sending id's
                        long fileHandle;
                        char buf[20];
                        for(int i = 0; i < fileCount; i++) {
                            
							// if the file is not deleted
                            if(!ptrUserManager->User_IsFileDeleted(lUserHandle,i)) {
                              
								// the following error should never happen
								// unless a file which is not six degits is being added to the users inbox
								// perhapse we should add some alert to the administrator
								// instead of shutting off the server
								// also it will not compile on an alpha machine
								// INTERNAL ERROR 
								if(-1 == (fileHandle = ptrUserManager->User_OpenFile(lUserHandle,i))) {
                                    ptrMailServer->OnStatus("Error processing UIDL...");
						
									
									_snprintf(buf,sizeof(buf)-1, "-ERR %d ", param+1);      
									Send(buf);                            
									Send(" system error in UIDL\r\n");
									ptrMailServer->OnStatus(buf);
									ptrMailServer->OnStatus("POP3 UIDL N **Failed to get UIDL - bad filename**");
									
						//			_asm int 3; // need to test...
                                    continue;
								}
                                
                                _snprintf(buf,sizeof(buf)-1, "%d ", i+1);
                                if(CUT_SUCCESS == ptrMailServer->GetUIDL(fileHandle, i,(LPBYTE) szBuffer, WSS_LINE_BUFFER_SIZE)) {
                                    Send(buf);
                                    Send(szBuffer);
                                    Send("\r\n");
                                    ptrMailServer->OnStatus(szBuffer);
                                }
                                else {
                                    Send(buf);
                                    Send("no message id");
                                    Send("\r\n");
                                    ptrMailServer->OnStatus("POP3 UIDL ERROR retrieving Message-ID");
                                }
                                
                                ptrUserManager->User_CloseFile(fileHandle);
                            }
                        }
                        
                        // End multiline response
                        Send(".\r\n");
                    }
                    continue;
                    }
                    
                case CMD_POP3TOP:
                    {
                        
                        // Find which message to send the top of 
                        long param;
                        long lines = 1;
                        if(CUT_StrMethods::ParseString(szBuffer," ",1,&param) == CUT_SUCCESS){
                            param --;  //POP3 starts from one CDataManager starts from 0
                            
                            // Check to see if the message is valid
                            if(param < 0 || param >= ptrUserManager->User_GetFileCount(lUserHandle)) {
                                Send("-ERR  Invalid Message Number\r\n");
                                strcpy(messageStatus,szUserName);
                                strcat(messageStatus," POP3: TOP -ERR  Invalid Message Number");
                                ptrMailServer->OnStatus(messageStatus);
                                continue;
                            }
                            else if(ptrUserManager->User_IsFileDeleted(lUserHandle,param)) {
                                Send("-ERR  Message marked as deleted\r\n");
                                strcpy(messageStatus,szUserName);
                                strcat(messageStatus," POP3: -ERR  Message marked as deleted");
                                ptrMailServer->OnStatus(messageStatus);
                                continue;
                            }
                            
                            // Get the number of message lines to send
                            if(CUT_StrMethods::ParseString(szBuffer," ",2,&lines) != CUT_SUCCESS)
                                lines = 0;
                            
                            // Send the message top
                            long fileHandle;
                            fileHandle = ptrUserManager->User_OpenFile(lUserHandle,param);
                            if(fileHandle < 0) {
                                Send("-ERR  Invalid Message Number\r\n");
                                strcpy(messageStatus,szUserName);
                                strcat(messageStatus," POP3: TOP -ERR  Invalid Message Number");
                                ptrMailServer->OnStatus(messageStatus);
                                continue;
                            }
                            
                            Send("+OK Top of Message Follows\r\n");
                            ptrMailServer->OnStatus("+OK Top of Message Follows\r\n");
                            
                            int  len = -2;
                            while (len != 0 && len != -1) { // crlf indicates blank line (EOH)
                                len = ptrMailServer->ReadLineFromFile(fileHandle,(LPBYTE)szBuffer, WSS_LINE_BUFFER_SIZE);
                                if(len > 0) {
                                    Send(szBuffer, len);   // avoid call to strlen
                                    Send("\r\n");       // readline strips these
                                }
                            }
                            
                            Send("\r\n");       // send blank line - header done
                            
                            // Ok - got header and blank - now simply send number of lines.
                            BOOL sentLast = false;
                            for (int i = 0; i < lines; i++) {
                                len = ptrMailServer->ReadLineFromFile(fileHandle,(LPBYTE)szBuffer, WSS_LINE_BUFFER_SIZE);
                                if(len > 0) {
                                    Send(szBuffer, len);
                                    Send("\r\n");
                                    if(!strcmp(szBuffer, "."))
                                        sentLast = true;
                                }
                                else if(len == 0) {
                                    Send("\r\n");
                                    }
                                else {
                                    break;  // may have asked for too many lines...
                                    }
                            }
                            
                            if(!sentLast) 
                                Send(".\r\n");
                            
                            
                            ptrUserManager->User_CloseFile(fileHandle);
                        }
                        else {
                            Send("-ERR  Invalid Message Number\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: TOP -ERR  Invalid Message Number");
                            ptrMailServer->OnStatus(messageStatus);
                        }
                        continue;
                    }
                    
                case CMD_POP3DELE:
                    {
                        // Find which message to delete
                        long param;
                        if(CUT_StrMethods::ParseString(szBuffer," ",1,&param) == CUT_SUCCESS){
                            param --;  //POP3 starts from one CDataManager starts from 0
                            
                            // Check to see if the message is valid
                            if(param < 0 || param >= ptrUserManager->User_GetFileCount(lUserHandle)){
                                Send("-ERR  Invalid Message Number\r\n");
                                strcpy(messageStatus,szUserName);
                                strcat(messageStatus," POP3: DELE -ERR  Invalid Message Number");
                                ptrMailServer->OnStatus(messageStatus);
                                continue;
                            }
                            else if(ptrUserManager->User_IsFileDeleted(lUserHandle,param)) {
                                Send("-ERR  Message marked as deleted\r\n");
                                strcpy(messageStatus,szUserName);
                                strcat(messageStatus," POP3: -ERR  Message marked as deleted");
                                ptrMailServer->OnStatus(messageStatus);
                                continue;
                            }
                            
                            // Mark the file for deletion
                            ptrUserManager->User_DeleteFile(lUserHandle,param);
                            
                            // Send a notify back
                            Send("+OK\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: DELE OK");
                            ptrMailServer->OnStatus(messageStatus);
                        }
                        else {
                            Send("-ERR  Invalid Message Number\r\n");
                            strcpy(messageStatus,szUserName);
                            strcat(messageStatus," POP3: DELE -ERR  Invalid Message Number");
                            ptrMailServer->OnStatus(messageStatus);
                        }
                        continue;
                    }
                    
                case CMD_POP3RSET:
                    {
                        // Reset the deletion list
                        ptrUserManager->User_ResetDelete(lUserHandle);
                        Send("+OK\r\n");
                        continue;
                    }
                default:
                    {
                        Send("-ERR Unknown Command\r\n");
                        strcpy(messageStatus,szUserName);
                        strcat(messageStatus,"POP3 -ERR Unknown Command");
                        ptrMailServer->OnStatus(messageStatus);
                    }
                }
            }   
        }
        
        // Close the user, close also deletes all marked files
        if(lUserHandle >= 0)
            ptrUserManager->User_CloseUser(lUserHandle);
        
        _snprintf(messageStatus,sizeof(messageStatus)-1,"POP3  %s has disconnected" ,szUserName);    
        ptrMailServer->OnStatus(messageStatus);
        
}

/********************************
GetCommand
Returns a command ID from the given
command line
Params
data - command line
Return
CMD_UNKNOWN - Unknown command
valid commands:
CMD_POP3USER,CMD_POP3PASS,CMD_POP3STAT
CMD_POP3LIST,CMD_POP3RETR,CMD_POP3DELE
CMD_POP3NOOP,CMD_POP3RSET,CMD_POP3QUIT  
*********************************/
POP3CommandID CUT_POP3Thread::GetCommand(LPSTR data){
    
    char    buf[10];
    char    *lpszCommandsList[] = { "USER", "PASS", "STAT", "LIST", "RETR", 
        "DELE", "NOOP", "RSET", "QUIT", "TOP ", "UIDL", "STLS", "CAPA"};
    
    strncpy(buf, data, 5);
    buf[4] = 0;
    _strupr(buf);
    
    for(int i = (int)CMD_POP3USER; i <= (int)CMD_CAPA ; i++) 
        if(strcmp(buf, lpszCommandsList[i - (int)CMD_POP3USER]) == 0)
            return (POP3CommandID) i;
        
        return CMD_UNKNOWN_POP3;
}

#ifdef CUT_SECURE_SOCKET
/*******************************************
	When security is enabled some ftp clients may 
	try to connect to the secure FTP
	This flag attempt enable or disable non secure connection on a secure server

PARAM
	flag	-	 TRUE = allow non secure clients to connect
			-	 FALSE =	 Only secure clients allowed
*******************************************/
BOOL CUT_POP3Thread::OnNonSecureConnection(LPCSTR /* IpAddress */)
{
   return FALSE;
}

int CUT_POP3Thread::SocketOnConnected(SOCKET s, const char *lpszName)
{
	CUT_MailServer  *ptrMailServer  = ((CUT_POP3Server *)m_winsockclass_this)->m_ptrMailServer;
	if(ptrMailServer->GetPOP3Server()->m_bImmediateNegotiation) {
		// v4.2 note : notifications still taking char
		return CUT_SecureSocket::SocketOnConnected(s, lpszName);
	}
	else
		return UTE_SUCCESS;
}

#endif

#pragma warning ( pop )
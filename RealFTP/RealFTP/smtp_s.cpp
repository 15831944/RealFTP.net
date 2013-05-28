//=================================================================
//  class: CUT_SMTPServer
//  class: CUT_SMTPThread
//  File:  smtp_s.cpp
//
//  Purpose:
//
//  Implementation of the SMTP server and thread classes.
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

#include "smtp_s.h"
#include "UT_MailServer.h"

#include "ut_strop.h"

#ifdef UT_DISPLAYSTATUS
    #include "uh_ctrl.h"
    extern CUH_Control  *ctrlHistory;
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

// =================================================================
// CUT_SMTPServer class
// =================================================================

/***************************************
Constructor
****************************************/
CUT_SMTPServer::CUT_SMTPServer(CUT_MailServer &ptrMailServer) :
    m_ptrMailServer(&ptrMailServer) // Initialize pointer to the Mail Server class
{
    m_nPort = 25; // Set default port number to 25

#ifdef CUT_SECURE_SOCKET
   m_bImmediateNegotiation = FALSE;
#endif

}

/***************************************
Destructor
****************************************/
CUT_SMTPServer::~CUT_SMTPServer(){
    m_bShutDown = TRUE;
    while(GetNumConnections())
	    Sleep(50);

}

/********************************
CreateInstance
    Creates and returns a pointer to
    an instance of the CUT_SMTPThread
    class
Params
    none
Return
    pointer to an instance of a
    CUT_SMTPThread class
*********************************/
CUT_WSThread * CUT_SMTPServer::CreateInstance(){
    return new CUT_SMTPThread;
}

/********************************
ReleaseInstance
    Releases an instance of the given
    CUT_SMTPThread class
Params
    ptr - pointer to an instance of a
    CUT_SMTPThread class
Return
    none
*********************************/
void CUT_SMTPServer::ReleaseInstance(CUT_WSThread *ptr){
    if(ptr != NULL)
        delete ptr;
}

/********************************
Start
    Starts up the SMTP server class
    Connects to port 25 and starts
    listening for connections
Params
    none
Return
    UTE_SUCCESS         - success
    UTE_CONNECT_FAILED  - connect to port 25 failed
    UTE_ACCEPT_FAILED   - start accepting failed
*********************************/
int CUT_SMTPServer::Start(){

    if(ConnectToPort(m_nPort) != UTE_SUCCESS)
        return m_ptrMailServer->OnError(UTE_CONNECT_FAILED);
    if(StartAccept() != UTE_SUCCESS)
        return m_ptrMailServer->OnError(UTE_ACCEPT_FAILED);
    return m_ptrMailServer->OnError(UTE_SUCCESS);
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
long CUT_SMTPServer::OnCanAccept(LPCSTR address)
{
    if(m_ptrMailServer == NULL)
        return TRUE;
    else
	{
		if(GetShutDownFlag())
			return FALSE;
	}
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
void CUT_SMTPServer::SetImmediateNegotiation(BOOL bFlag)
{
   m_bImmediateNegotiation = bFlag;
}

#endif


// =================================================================
// CUT_SMTPThread class
// =================================================================

/********************************
OnMaxConnect
    This function is called when a new
    connection comes in and the 
    max number of connections has already
    been reached
Params
    none
Return
    none
*********************************/
void CUT_SMTPThread::OnMaxConnect(){
    // Send too busy message
    Send("-ERR  System Too Busy Please Try Later\r\n");
    ((CUT_SMTPServer *)m_winsockclass_this)->m_ptrMailServer->OnStatus("-ERR  System Too Busy Please Try Later\r\n");
}

/********************************
OnConnect
    This function is called when a new
    connection comes in. This routine
    processes all commands from the 
    connection and only returns when
    the connection is finished
Params
    none
Return
    none

Revisions:
	Added string length limitations 
	as enforced by RFC 821

	There are several objects that have required minimum maximum
	sizes.  That is, every implementation must be able to receive
	objects of at least these sizes, but must not send objects
	larger than these sizes.


	****************************************************
	*                                                  *
	*  TO THE MAXIMUM EXTENT POSSIBLE, IMPLEMENTATION  *
	*  TECHNIQUES WHICH IMPOSE NO LIMITS ON THE LENGTH *
	*  OF THESE OBJECTS SHOULD BE USED.                *
	*                                                  *
	****************************************************
	user
	The maximum total length of a user name is 64 characters.

	domain
	The maximum total length of a domain name or number is 64
	 characters.

	path

	   The maximum total length of a reverse-path or
	   forward-path is 256 characters (including the punctuation
	   and element separators).

	command line

	   The maximum total length of a command line including the
	   command word and the <CRLF> is 512 characters.

	reply line

	   The maximum total length of a reply line including the
	   reply code and the <CRLF> is 512 characters.

	recipients buffer

               The maximum total number of recipients that must be
               buffered is 100 recipients.

*********************************/
void CUT_SMTPThread::OnConnect()
{
    char            szMailFrom[WSS_BUFFER_SIZE + 1];
    char            szHeloName[WSS_BUFFER_SIZE + 1];     
    char            szBuffer[WSS_LINE_BUFFER_SIZE + 1];
    char            szRcpt[WSS_LINE_BUFFER_SIZE + 1];
    int             nLength             = 0;
    int             nNumBadCommands     = 0;
    int             bQuit               = FALSE;
    SMTPCommandID   nLastCommand        = CMD_UNKNOWN_SMTP;
    SMTPCommandID   nCommand            = CMD_UNKNOWN_SMTP;
    CUT_MailServer  *ptrMailServer  = ((CUT_SMTPServer *)m_winsockclass_this)->m_ptrMailServer;
    CUT_DataManager     *ptrDataManager     = ptrMailServer->m_ptrDataManager;
    CUT_StringList  m_listRcpt;   
	BOOL			greeted = FALSE;
	time_t			ltime;
		

	char szReceivedHeader[2*WSS_LINE_BUFFER_SIZE];

	if(ptrMailServer->GetSMTPServer()->GetShutDownFlag())
		return;


	time( &ltime );
	ZeroMemory( szBuffer, sizeof(szBuffer));
	_snprintf(szBuffer,sizeof(szBuffer)-1, "**** Client [%s] connected at (%s) ***** ", GetClientAddress(), (const char *)ctime( &ltime ));
	CUT_StrMethods::RemoveCRLF (szBuffer);
	ptrMailServer->OnStatus(szBuffer);

#ifdef CUT_SECURE_SOCKET

	BOOL            bHandshakeDone	= FALSE;	

	// Disable security while connecting
	BOOL bSecureFlag = GetSecurityEnabled();

	if(!ptrMailServer->GetSMTPServer()->m_bImmediateNegotiation)
		SetSecurityEnabled(FALSE);
	else
		bHandshakeDone = TRUE;

#endif


    // Send inital message
	ZeroMemory( szBuffer, sizeof(szBuffer));
    _snprintf(szBuffer,sizeof(szBuffer)-1,"220 %s Service Ready. Ultimate TCP/IP Enterprise Edition\r\n",ptrMailServer->Sys_GetLocalName(0));
    Send(szBuffer);

    // Main message pump
    while(bQuit == FALSE) {
		if(ptrMailServer->GetSMTPServer()->GetShutDownFlag())
		{
			ZeroMemory( szBuffer, sizeof(szBuffer));
			_snprintf(szBuffer,sizeof(szBuffer)-1,"421 %s Service not available, closing transmission channel\r\n",ptrMailServer->Sys_GetLocalName(0));
			Send(szBuffer);	
			break;             
		}
		
        ptrMailServer->OnStatus("NEW Command:");

		if (nNumBadCommands > 10)
			break; 

        // Receive a line from the client
		ZeroMemory( szBuffer, sizeof(szBuffer));
        nLength = ReceiveLine(szBuffer, WSS_LINE_BUFFER_SIZE);
	
	

        // Exit on an error
        if(nLength <= 0)
            break;  

		// command line
		// The maximum total length of a command line including the
		// command word and the <CRLF> is 512 characters.
		 if(nLength > 512)
		{
			Send("500 Line too long.\r\n");
			ptrMailServer->OnStatus("500 Line too long.");
			continue;
		}

        CUT_StrMethods::RemoveCRLF(szBuffer);
        ptrMailServer->OnStatus(szBuffer);

        // Store the last command
        nLastCommand = nCommand;

        // Get the command that was sent
        nCommand = GetCommand(szBuffer);




#ifdef CUT_SECURE_SOCKET

		// Only this commands allowed without establishing secure connection
		if(	nCommand != CMD_SMTPNOOP && 
			nCommand != CMD_SMTPSTARTTLS && 
			nCommand != CMD_SMTPEHLO && 
			nCommand != CMD_SMTPQUIT)
		{
			if(!bHandshakeDone  && bSecureFlag )
			{
				if(!OnNonSecureConnection(GetClientAddress()))
				{
					Send("530 Must issue a STARTTLS command first\r\n");
					ptrMailServer->OnStatus("Must issue a STARTTLS command first");
					continue;
				}
			}
		}
				
#endif


        // Check to see if it is a command that can run without logging in
        switch(nCommand) {

            case CMD_SMTPHELO:
				{

                // Get the param
                *szHeloName = NULL;
				
				//\b Backspace  \f Formfeed \n New line \r Carriage return \t Horizontal tab \v 
				// 

                CUT_StrMethods::ParseString(szBuffer," \t\b\v\f",1, szHeloName,sizeof(szHeloName)-1);

				// the domain name must be sorter than 
               // The maximum total length of a domain name or number is 64
				// characters
				//  see rfc 821 section 
				// 4.5.3.  SIZES 
				if (szHeloName == 0 || strlen(szHeloName) < 2 )
				{
					 Send("501 Syntax Error In Parameters Or Arguments.\r\n");
					 nNumBadCommands++;
					 ptrMailServer->OnStatus("501 Syntax Error In Parameters Or Arguments.");
					  nCommand = nLastCommand ;
					  continue;				
				}
				CUT_StrMethods::RemoveCRLF (szHeloName);
				CUT_StrMethods::RemoveSpaces (szHeloName);
				
				if (szHeloName == 0 || strlen(szHeloName)  > 64 )
				{
					 Send("501 Syntax Error In Parameters Or Arguments.\r\n");
					 ptrMailServer->OnStatus("501 Syntax Error In Parameters Or Arguments");
					 nNumBadCommands++;
					  nCommand = nLastCommand ;
					  continue;				
				}

                // Send acknowlegment				
				time( &ltime );
				char szTimeStr[29];
				ZeroMemory( szTimeStr, 29 );
				
				
				//	The string result produced by ctime contains exactly 26 characters and has the form: 
				// Wed Jan 02 02:03:55 1980\n\0 
				// So we need 29
				_snprintf(szTimeStr,sizeof(szTimeStr)-1,"at %s", (const char *)ctime( &ltime ));
				// A 24-hour clock is used.
				// All fields have a constant width.
				// The newline character ('\n') and the null character 
				// ('\0') occupy the last two positions of the string.
				CUT_StrMethods::RemoveCRLF (szTimeStr);		
				// ok you may wonder what was that about
				//I found out that ctime adds a \n to the end 
				// so I wanted to remove any \r or \n 
				// the add a new one
				ZeroMemory( szReceivedHeader, sizeof(szReceivedHeader)-1 );
				_snprintf(szReceivedHeader,sizeof(szReceivedHeader)-1,"Received: from %s [%s];\r\n\tby %s %s;\r\n\twith Codepro Mail Server  (R2hhemkgVyBOb3YgMDYA);\r\n\tPowered by Ultimate TCP-IP v4.2 Enterprise Edition (www.theultimatetoolbox.com).\r\n", szHeloName, GetClientAddress(), ptrMailServer->Sys_GetLocalName(0), szTimeStr);

				ZeroMemory( szBuffer, sizeof(szBuffer));
			    _snprintf(szBuffer,sizeof(szBuffer)-1,"250 %s %s [%s] \r\n",ptrMailServer->Sys_GetLocalName(0), szHeloName, GetClientAddress());

				greeted = TRUE;
                Send(szBuffer);
				ptrMailServer->OnStatus(szBuffer);
                continue;
		}

            case CMD_SMTPHELP:
                OnHelpCommand();
                continue;
                
            case CMD_SMTPQUIT:
                Send("221 Goodbye\r\n");
                ptrMailServer->OnStatus("QUIT");
                bQuit = TRUE;
                break;

            case CMD_SMTPNOOP:
                Send("250 Requested mail action ok, completed\r\n");
                continue;

            case CMD_SMTPRSET:
                m_listRcpt.ClearList();
                Send("250 Ok\r\n");
                continue;

            case CMD_SMTPEHLO:
				{

		
                // Get the param
                // Get the param
                *szHeloName = NULL;
				
				//\b Backspace  \f Formfeed \n New line \r Carriage return \t Horizontal tab \v 
				// 

                CUT_StrMethods::ParseString(szBuffer," \t\b\v\f",1, szHeloName,sizeof(szHeloName)-1);

				// the domain name must be sorter than 
               // The maximum total length of a domain name or number is 64
				// characters
				//  see rfc 821 section 
				// 4.5.3.  SIZES 
				if (szHeloName == 0 || strlen(szHeloName) < 2 )
				{
					 Send("501 Syntax Error In Parameters Or Arguments.\r\n");
					 nNumBadCommands++;
					 ptrMailServer->OnStatus("501 Syntax Error In Parameters Or Arguments.");
					  nCommand = nLastCommand ;
					  continue;				
				}
				CUT_StrMethods::RemoveCRLF (szHeloName);
				CUT_StrMethods::RemoveSpaces (szHeloName);
				
				if (szHeloName == 0 || strlen(szHeloName)  > 64 )
				{
					 Send("501 Syntax Error In Parameters Or Arguments.\r\n");
					 ptrMailServer->OnStatus("501 Syntax Error In Parameters Or Arguments");
					 nNumBadCommands++;
					  nCommand = nLastCommand ;
					  continue;				
				}
				char szTimeStr[29];
				ZeroMemory( szTimeStr, sizeof(szTimeStr));
				
			     // Send acknowlegment				
				time( &ltime );
		
				_snprintf(szTimeStr,sizeof(szTimeStr)-1,"at %s", (const char *)ctime( &ltime ));
				// A 24-hour clock is used.
				// All fields have a constant width.
				// The newline character ('\n') and the null character 
				// ('\0') occupy the last two positions of the string.
				CUT_StrMethods::RemoveCRLF (szTimeStr);		
				// ok you may wonder what was that about
				//I found out that ctime adds a \n to the end 
				// so I wanted to remove any \r or \n 
				// the add a new one
				ZeroMemory( szReceivedHeader, sizeof(szReceivedHeader)-1 );
				_snprintf(szReceivedHeader,sizeof(szReceivedHeader)-1,"Received: from %s [%s];\r\n\tby %s %s;\r\n\twith Ultimate TCP/IP Mail Server ;\r\n\tPowered by Ultimate TCP/IP v4.2 Enterprise Edition (www.theultimatetoolbox.com).\r\n", szHeloName, GetClientAddress(), ptrMailServer->Sys_GetLocalName(0), szTimeStr);		
				

                // Send acknowlegment
#ifdef CUT_SECURE_SOCKET
				ZeroMemory( szBuffer, sizeof(szBuffer));
                _snprintf(szBuffer,sizeof(szBuffer)-1,"250-%s %s [%s] \r\n250 STARTTLS\r\n",ptrMailServer->Sys_GetLocalName(0), szHeloName, GetClientAddress());
#else
				ZeroMemory( szBuffer, sizeof(szBuffer));
				_snprintf(szBuffer,sizeof(szBuffer)-1,"250 %s %s [%s] \r\n",ptrMailServer->Sys_GetLocalName(0), szHeloName, GetClientAddress());
#endif
				greeted = TRUE;
                Send(szBuffer);
				ptrMailServer->OnStatus(szBuffer);

                continue;
				}

			case CMD_SMTPSTARTTLS:

#ifdef CUT_SECURE_SOCKET

				if(bSecureFlag)
				{
					// Already done
					if(bHandshakeDone)
					{
						Send("502 Command not permitted when TLS active\r\n");
						continue;
					}

					// Send an OK command
					Send("220 Ready to start TLS\r\n");

					// Set security flag
					SetSecurityEnabled(TRUE);

					// Start negotiation
					int nResult = CUT_SecureSocket::SocketOnConnected(m_clientSocket, "");
					if (nResult != UTE_SUCCESS)
					{
						SetSecurityEnabled(FALSE);									
						Send("554  The SSL negotiation failed during the handshake");
						bQuit = TRUE;
						continue;
					} 
					else
					{
						bHandshakeDone =  TRUE;
						continue;
					}
				}
				else
					Send("454 TLS not available due to temporary reason\r\n");
#else

				Send("454 TLS not available due to temporary reason\r\n");
#endif

				continue;

			
			case CMD_SMTPMAIL:
				{
					if (!greeted)
					{
					  Send("503 Bad sequence of commands. Nice People Say Helo first\r\n");
					   continue;
					}

				// 				
				char firstString[WSS_LINE_BUFFER_SIZE];
                // Special case "MAIL FROM: <>"
                if (strstr(szBuffer,"<>")) {
                    strcpy(szMailFrom,szHeloName);
                }
				else {
					// Get number of components
					int counter = 0;
					int piecesCount= 0;
					piecesCount = CUT_StrMethods::GetParseStringPieces (szBuffer, "\t\b\v\f: ,\r\n");
					for (; counter < piecesCount;counter ++)
					{
						if ( CUT_StrMethods::ParseString (szBuffer,"\t\b\v\f: ,\r\n",counter, firstString,sizeof(firstString)) == UTE_SUCCESS)
						{
							// does it include an @ sign
							// then there should be 2 peices
							if (  CUT_StrMethods::GetParseStringPieces (firstString, "@")  > 1){
								// now remove the > and < signs from the email
								// this function call should pass even if the line does not include 
								// a <> tag
								CUT_StrMethods::ParseString (firstString,"<>",0, szMailFrom,sizeof(szMailFrom));
								break;
							}
						}
					}
					// if we have reached the number of peices 
					// then the from is not found
					// then we should let the client know of the error
					if (counter == piecesCount)
					{
					  Send("501 Syntax Error In Parameters Or Arguments.\r\n");
					  nCommand = nLastCommand ;
					  continue;
					}
				}
			    m_listRcpt.ClearList();

                // Send acknowlegment
				ZeroMemory( szBuffer, sizeof(szBuffer));
                _snprintf(szBuffer,sizeof(szBuffer)-1,"250 OK it is from %s\r\n",szMailFrom);
                Send(szBuffer);
				ptrMailServer->OnStatus(szBuffer);              
                continue;
				}
                
            case CMD_SMTPRCPT:
				{
				
					if (!greeted)
					{
					  Send("503 Bad sequence of commands. Nice People Say Helo first\r\n");
					  ptrMailServer->OnStatus("503 Bad sequence of commands. Nice People Say Helo first");;              
					   continue;
					}
					
                if(nLastCommand != CMD_SMTPRCPT && nLastCommand != CMD_SMTPMAIL) {
                    Send("503 Bad Sequence Of Commands.\r\n");
				   ptrMailServer->OnStatus("503 Bad sequence of commands.");;              
				
                    nCommand = nLastCommand ;
                    continue;
                    }

				// 				
				char firstString[WSS_LINE_BUFFER_SIZE];
				// Get number of components
				int counter = 0;
				int piecesCount= 0;
				piecesCount = CUT_StrMethods::GetParseStringPieces (szBuffer, "\t\b\v\f: ,\r\n");
				for (; counter < piecesCount;counter ++)
				{
					if ( CUT_StrMethods::ParseString (szBuffer,"\t\b\v\f: ,\r\n",counter, firstString,sizeof(firstString)) == UTE_SUCCESS)
					{
						// does it include an @ sign
						// then there should be 2 peices
						if (  CUT_StrMethods::GetParseStringPieces (firstString, "@")  > 1){
							// now remove the > and < signs from the email address
							// this function call should pass even if the line does not include 
							// a <> tag
							CUT_StrMethods::ParseString (firstString,"<>",0, szRcpt,sizeof(szRcpt));
							break;
						}
					}
				}
				// if we have reached the number of peices 
				// then the to is not found
				// then we should let the client know of the error
				if (counter == piecesCount)
				{
				  Send("501 Syntax Error In Parameters Or Arguments.\r\n");
			  	   ptrMailServer->OnStatus("501 Syntax Error In Parameters Or Arguments..");;              			
				  nCommand = nLastCommand ;
				  continue;
				}


				//	recipients buffer
				//	The maximum total number of recipients that must be
				//	buffered is 100 recipients.
				if (m_listRcpt.GetCount () >= 100)
				{
					Send("552 Too many recipients.\r\n");
					ptrMailServer->OnStatus("552 Too many recipients.");
					nCommand = nLastCommand ;
					continue;
				}

                // Add the name to the rcpt list
				// GetRelay here
				if (!CheckRelay(szMailFrom,szRcpt,szHeloName))
				{
					 Send("550 Relaying not allowed .\r\n");
				  	 ptrMailServer->OnStatus("550 Relaying not allowed.");				
					 nNumBadCommands++;
				 	  nCommand = nLastCommand ;
					  continue;
				}


                m_listRcpt.AddString(szRcpt);
                // Send acknowlegment
                Send("250 OK it is for ");
                Send(szRcpt);
			    Send(" \r\n");

				ZeroMemory( szBuffer, sizeof(szBuffer));
             	_snprintf(szBuffer,sizeof(szBuffer)-1,"Ok so it is for %s" ,szRcpt);
				ptrMailServer->OnStatus(szBuffer);								 
                continue;
				}
            
		           
            case CMD_SMTPDATA:
                {
                if(nLastCommand != CMD_SMTPRCPT) {
                    Send("503 Bad Sequence Of Commands.\r\n");
					ptrMailServer->OnStatus("503 Bad Sequence Of Commands");
					 nNumBadCommands++;
			         continue;
                    }			

                Send("354 Start mail input, end with <CRLF>.<CRLF>\r\n");

                long fileHandle = ptrDataManager->Que_CreateFile();
                if(INVALID_HANDLE_VALUE == (void*)(ULONG_PTR)fileHandle) {

                    bQuit = TRUE;
                    // Hard kill - read what's left of transaction and discard...
					ZeroMemory( szBuffer, sizeof(szBuffer));
                    while(ReceiveLine(szBuffer,sizeof(szBuffer)-1));

                    Send("451 Requested action aborted - error in processing\r\n");
                    ptrMailServer->OnStatus("SMTPDATA error - Requested action aborted ");

                    break;
                    }

                BOOLEAN deleteFlag = FALSE;

                // Write the message header
                ptrDataManager->Que_WriteFileHeader(fileHandle,m_listRcpt.GetString(0L),szMailFrom,0,0);

                // Read lines in until  <CRLF>.<CRLF> and save it
                // Check header portion for message id - if none, 
                // add one.
                int     len;
                BOOL    IdOk            = FALSE;
                BOOL    EndOfHeaderSeen = FALSE;
				
				// the return path is the first thing to be written
				ptrDataManager->Que_WriteFile(fileHandle,(LPBYTE)szReceivedHeader,(int)strlen(szReceivedHeader));
				for(;;) {
					if(ptrMailServer->GetSMTPServer()->GetShutDownFlag())
						{
                        Send("421 Service not available, closing transmission channel\r\n");
                        ptrMailServer->OnStatus("421  Service not available, closing transmission channel");
                        deleteFlag = TRUE;
                        break;
                        }


					ZeroMemory( szBuffer, sizeof(szBuffer)-1);
                    len = ReceiveLine(szBuffer,sizeof(szBuffer)-1);

                    if(len <= 0) {
                        Send("554 Transaction failed\r\n");
                        ptrMailServer->OnStatus("SMTPDATA error 554 Transaction failed");
                        deleteFlag = TRUE;
                        break;
                        }
					


					// make sure we have enough chars
					if (szBuffer[0] != 0 && strlen(szBuffer) > 11) 
					{
						if(!EndOfHeaderSeen) 
						{
							if(_strnicmp(szBuffer, "Message-Id:",11) == 0)
								IdOk = TRUE;
                        }
					}

                 if(len < 3) 
				 {
                        EndOfHeaderSeen = TRUE;
                       
						   // Did this message have a valid ID?
                        if(!IdOk) {
                            char    szMsgID[WSS_LINE_BUFFER_SIZE + 1];      

                            // Create a unique ID and write to the file!
                            ptrMailServer->BuildUniqueID(szMsgID, sizeof(szMsgID)-1);
                            ptrDataManager->Que_WriteFile(fileHandle,(LPBYTE)"Message-ID: ",12);
                            strcat(szMsgID, "\r\n");
                            ptrDataManager->Que_WriteFile(fileHandle,(LPBYTE)szMsgID, (int)strlen(szMsgID));
                            IdOk = TRUE;
                            }
				 }
				

                    ptrDataManager->Que_WriteFile(fileHandle,(LPBYTE)szBuffer,len);

                    if(len == 3)
					{
                        if(szBuffer[0]=='.' && szBuffer[1]=='\r' && szBuffer[2]=='\n')
						{
                            Send("250 Mail Received OK\r\n");
                            break;
						}
					}
                    
                }
                ptrMailServer->OnStatus("Data finished");
				
				// Make sure that the server is not relaying messages to itself,
				// it is possible for the server to relay a message to itself
				// when the destination of the email contains a domain name
				// that resolves to the same IP as the server but the names
				// are different.

				// v4.2 using AC here - names now _TCHAR")
				if ( strcmp( AC(ptrMailServer->Sys_GetLocalName(0)), szHeloName ) == 0 )
				{
					// Bad message found, receive it but also delete it from the queue
					// to prevent further resending of the message.
					deleteFlag = TRUE;
				}

                // Get the number of rcpt to lines
                // Copy the original file for each rcpt in the rcpt list
                if(!deleteFlag)
                    for(int loop = 1;loop < m_listRcpt.GetCount(); loop++)      // bypass first - it's done
                        ptrDataManager->Que_CarbonCopyFile(fileHandle,m_listRcpt.GetString(loop));
                
                // Close the file                
                ptrDataManager->Que_CloseFile(fileHandle,deleteFlag);

                continue;
                }           
            default:
                Send("502 Command not implemented\r\n");

                // Break after 5 unreconized commands
                nNumBadCommands ++;
                if(nNumBadCommands == 5) {
                    bQuit = TRUE;
                    break;
                    }
            
            }
        }
		CloseConnection();
}

/********************************
GetCommand
    Returns a command ID from the given
    command line
Params
    data - command line string
Return
    command ID number 
    CMD_UNKNOWN_SMTP - bad command
    CMD_SMTPHELO,CMD_SMTPMAIL,CMD_SMTPRCPT
    CMD_SMTPDATA,CMD_SMTPHELP,CMD_SMTPQUIT
    CMD_SMTPNOOP,CMD_SMTPRSET
*********************************/
SMTPCommandID CUT_SMTPThread::GetCommand(LPSTR data){

    char    buf[11];
    char    *lpszCommandsList[] = { "HELO", "MAIL", "RCPT", "DATA", 
                                    "HELP", "QUIT", "RSET", "NOOP", "STARTTLS", "EHLO"};

    strncpy(buf, data, 10);
	buf[10] = 0; // NULLIFY IT

    char *space = strchr(buf, ' ');
	if(space != NULL)
		*space = NULL;
    _strupr(buf);
    
    for(int i = (int)CMD_SMTPHELO; i <= (int)CMD_SMTPEHLO ; i++)
	{
        if(strcmp(buf, lpszCommandsList[i - (int)CMD_SMTPHELO]) == 0)
            return (SMTPCommandID) i;
	}



    return CMD_UNKNOWN_SMTP;
}

/********************************
OnHelpCommand
    Sends out help information when
    a help command is issued
Params
    none
Return
    none
*********************************/
void CUT_SMTPThread::OnHelpCommand() {
    Send("214- 32bit Windows E-mail Server From The Ultimate Toolbox \r\n");
    Send("214- ================================================ \r\n");
    Send("214- Commands Accepted: \r\n");
    Send("214-    HELO   MAIL   RCPT   DATA   HELP   QUIT \r\n");
    Send("214-    NOOP   RSET  \r\n");
    Send("214- Normal Communication Sequence: \r\n");
    Send("214-    HELO yourdomain.name \r\n");
    Send("214-    MAIL From:<you@yourdomain.name> \r\n");
    Send("214-    RCPT To:<destination@destinationdomain.name> \r\n");
    Send("214-    DATA \r\n");
    Send("214-          << send email message here>> \r\n");
    Send("214-		QUIT \r\n");
    Send("214  ================================================ \r\n");
}
/****************************************************
 virtual function to allow for aborting connections from specific email address 
 such as empty from fields or known Spammers
 also it is good place to log the from addresses

PARAMETERS:
	szMailFrom   - the From field of the MAIL command
	szMailhost	 - the host name as sent  by the mail client
RETURN
	TRUE		- Continue
	FALSE		- ABORT process
*****************************************************/

int CUT_SMTPThread::OnFrom(LPSTR /* szMailFrom */, LPCSTR  /* szMailhost */)
{
	return TRUE;
}

/*****************************************************

The suggested algorithm is:

   a)  If "RCPT To:" is one of "our" domains, local or a domain that
       we accept to forward to (alternate MX), then accept to Relay.

   b)  If SMTP_Caller is authorized, either its IP.src or its FQDN
       (depending on if you trust the DNS), then accept to Relay.

   c)  Else refuse to Relay.



  550 <bar@domain.example>... Denied due to spam list

*****************************************************/
BOOL CUT_SMTPThread::CheckRelay(LPCSTR /* szFrom */, LPCSTR szTo, LPCSTR /* szHost */)
{
	

	CUT_MailServer  *ptrMailServer  = ((CUT_SMTPServer *)m_winsockclass_this)->m_ptrMailServer;
	char szStrFromTempDomain[WSS_BUFFER_SIZE+1];
	char szStrToTempDomain[WSS_BUFFER_SIZE+1];


	if (szTo == NULL || szTo[0] == 0 || strlen(szTo) < 3)
		return FALSE;	
	

	// i am not going to relay to other server
	if (CUT_StrMethods::GetParseStringPieces  (szTo, "@") > 2) 
	{
		return FALSE;
	}
	
	
	if (CUT_StrMethods::ParseString (szTo, "@",1,szStrToTempDomain,WSS_BUFFER_SIZE) != CUT_SUCCESS) 
	{
		// this block should never be hit
		  return FALSE;					
	}


	// if the domain name of the to is local 
	// then return true
	// v4.2 using WC here - domains now held as _TCHAR
	if (ptrMailServer->Sys_IsDomainLocal(WC(szStrToTempDomain)))
	{
		char szLogLine[WSS_BUFFER_SIZE +  20];
		_snprintf(szLogLine,sizeof(szLogLine)-1, "RELAY CHECKED [%s]  %s",GetClientAddress(),szStrToTempDomain);
		ptrMailServer->OnStatus(szLogLine);
		return TRUE;
	}


		
	// now we have the address of the recepient 
	//  and the address of the sender and the address of the 
	// host machine
	// if the client address is in the allowed relay IP addresses 
	
	// if it does not exist then let see if it is part of relay IP addresses list
	// now lets see if the client is in the relay list
	// Get the list of Relay from the mail server
	if ( ptrMailServer->Sys_GetRelayAddressCount () <= 0)
		return TRUE; // the server is not set up to prevent relaying
	
	int peices = 0;
	char szClientAddress[32];
	in_addr inRangeStart, inRangeEnd ;
	in_addr	clientAddr;

	// first get the address of the connected client.
	strcpy(szClientAddress, GetClientAddress());

	// change the client address into a winsock 	
	clientAddr.S_un.S_addr  = inet_addr (szClientAddress);
	// v4.2 changed these to unsigned long to allow ranges up to 255.255.255
	unsigned long lstart = 0;
	unsigned long lend = 0;
	unsigned long lclient = 0;
	
	// v4.2 relays now held as _TCHAR so convert...
	char szAnsiRelay[MAX_PATH+1];
	for (int index = 0; index <  (int) ptrMailServer->Sys_GetRelayAddressCount (); index++)
	{
		// for each item 
		// check first if it contains -
		CUT_Str::cvtcpy(szAnsiRelay,MAX_PATH,ptrMailServer->Sys_GetRelayAddress (index));
		peices = CUT_StrMethods::GetParseStringPieces (szAnsiRelay,"-");
		// if peices are 0 or less then it is an internal error
		if (peices >1)
		{
			// parse the avilable list for  the range
			// get the first piece
			// v4.2 using AC here - relays now _TCHAR
			CUT_StrMethods::ParseString (szAnsiRelay, "-", 0, szStrFromTempDomain, WSS_BUFFER_SIZE);
			inRangeStart.S_un.S_addr  = inet_addr ( szStrFromTempDomain);
			// get the end piece
			// v4.2 using AC here - relays now _TCHAR
			CUT_StrMethods::ParseString (szAnsiRelay, "-",1,szStrToTempDomain,WSS_BUFFER_SIZE);
			inRangeEnd.S_un.S_addr  = inet_addr ( szStrToTempDomain);


			lstart = inRangeStart.S_un.S_un_b.s_b1 * 16777216 + 
				inRangeStart.S_un.S_un_b.s_b2 * 65536 +
				inRangeStart.S_un.S_un_b.s_b3 * 256 +
				inRangeStart.S_un.S_un_b.s_b4;

			lend = inRangeEnd.S_un.S_un_b.s_b1 * 16777216 + 
				inRangeEnd.S_un.S_un_b.s_b2 * 65536 +
				inRangeEnd.S_un.S_un_b.s_b3 * 256 +
				inRangeEnd.S_un.S_un_b.s_b4;

			lclient = clientAddr.S_un.S_un_b.s_b1 * 16777216 + 
				clientAddr.S_un.S_un_b.s_b2 * 65536 +
				clientAddr.S_un.S_un_b.s_b3 * 256 +
				clientAddr.S_un.S_un_b.s_b4;
				if (	( lend >= lclient ) && (lstart <= lclient)) 
				{
				char szLogLine[WSS_BUFFER_SIZE +  20];
				_snprintf(szLogLine,sizeof(szLogLine)-1, "IP RANGE OK %s < [%s] > %s",szStrFromTempDomain ,GetClientAddress() ,szStrToTempDomain );
				ptrMailServer->OnStatus(szLogLine);
				return TRUE;

				}
			
		}
		else
		if (_stricmp(szAnsiRelay,szClientAddress) == 0)
		{
			char szLogLine[WSS_BUFFER_SIZE +  20];
			_snprintf(szLogLine,sizeof(szLogLine)-1, "IP OK [%s] %s",GetClientAddress(),szStrToTempDomain);
			ptrMailServer->OnStatus(szLogLine);			
			return TRUE;
		}
	}
	// you may notice that I did not test the 
	// from if it is part of the domain
	// this is simply because the message data may contain diffrent from feild than the actual sender

	return FALSE; // nothing found
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
BOOL CUT_SMTPThread::OnNonSecureConnection(LPCSTR /* IpAddress */)
{
   return FALSE;
}

int CUT_SMTPThread::SocketOnConnected(SOCKET s, const char *lpszName)
{
	CUT_MailServer  *ptrMailServer  = ((CUT_SMTPServer *)m_winsockclass_this)->m_ptrMailServer;
	if(ptrMailServer->GetSMTPServer()->m_bImmediateNegotiation)
		return CUT_SecureSocket::SocketOnConnected(s, lpszName);
	else
		return UTE_SUCCESS;
}

#endif

#pragma warning ( pop )
// =================================================================
//  class: CUT_FTPServer 
//  class: CUT_FTPThread
//  File:  ftp_s.cpp
//
//  Purpose:
//
//      Implementation of FTP server and thread classes.
//
// File Transfer Protocol - RFC 959 and its references
// ===================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// ===================================================================

#ifdef _WINSOCK_2_0_
    #define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
                            /* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include "ftp_s.h"

#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

//=================================================================
//  class: CUT_FTPServer
//=================================================================

/********************************
Constructor
*********************************/
CUT_FTPServer::CUT_FTPServer() :
        m_MaxTimeOut(60),
		m_bDataPortProtection(TRUE) // default for back compatibility is FALSE
{
    // Initialize path with current working directory
    m_szPath[0] = 0;
    _getcwd(m_szPath, MAX_PATH);
    if(m_szPath[strlen(m_szPath) - 1] != '\\')
        strcat(m_szPath,"\\");

#ifdef CUT_SECURE_SOCKET
		m_FtpSSLConnectionType = FTP_SSL_EXPLICIT;
#endif //


}

/********************************
Destructor
*********************************/
CUT_FTPServer::~CUT_FTPServer(){
    
    // Give connection threads a chance to close...
    m_bShutDown = TRUE;
    while(GetNumConnections())
        Sleep(500);
}

/********************************
CreateInstance
    Creates an instance of the
    derived CUT_WSThread class
PARAM
    none
Return
    instance of a CUT_FTPThread class
*********************************/
CUT_WSThread * CUT_FTPServer::CreateInstance(){
    return new CUT_FTPThread;
}

/********************************
ReleaseInstance
    Deletes the instance of the 
    derived CUT_Thread class
Params
    ptr - pointer to a CUT_FTPThread class
Return
    none
*********************************/
void CUT_FTPServer::ReleaseInstance(CUT_WSThread *ptr){
    if(ptr != NULL)
        delete ptr;
}

/********************************
SetPath
    Stores the root path 
PARAM
    LPCSTR path - the full path name
RETURN
    UTE_SUCCESS                 - success
    UTE_PARAMETER_INVALID_VALUE - invalid parameter
*********************************/
#if defined _UNICODE
int CUT_FTPServer::SetPath(LPCWSTR path) {
	return SetPath(AC(path));}
#endif
int CUT_FTPServer::SetPath(LPCSTR path) {

    if(path == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);  
	
	if (strlen (path) > MAX_PATH)
		return  OnError(UTE_PARAMETER_INVALID_VALUE);
    
    strncpy(m_szPath, path, MAX_PATH);

    if(m_szPath[strlen(m_szPath) - 1] != '\\')
        strcat(m_szPath,"\\");
    
    return OnError(UTE_SUCCESS);
}

/********************************
GetPath
    Gets the root path
PARAM
    none
RETURN
    full path name
*********************************/
LPCSTR CUT_FTPServer::GetPath() const
{
    return m_szPath;
}
/*************************************************
GetPath()
Retrieves root server path.

PARAM
path	- [out] pointer to buffer to receive text
maxSize - length of buffer
size    - [out] length of message text

  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- path and/or size is a null pointer
  UTE_BUFFER_TOO_SHORT  - space in path buffer indicated by maxSize insufficient, realloc 
							based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
int CUT_FTPServer::GetPath(LPSTR path, size_t maxSize, size_t *size)
{
	int retval = UTE_SUCCESS;
	
	if(path == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetPath();
		
		if(str == NULL) {
			*path = 0;
		}
		else {
			*size = strlen(str);
			if(*size >= maxSize) {
				++(*size);
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				strcpy(path, str);
			}
		}
	}
	return retval;
}
#if defined _UNICODE
int CUT_FTPServer::GetPath(LPWSTR path, size_t maxSize, size_t *size)
{
	int retval;
	
	if(maxSize > 0) {
		char * pathA = new char [maxSize];
		
		if(pathA != NULL) {
			retval = GetPath( pathA, maxSize, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(path, maxSize, pathA);
			}
			delete [] pathA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetPath();
			if(lpStr != NULL) {
				*size = strlen(lpStr)+1;
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				retval = UTE_INDEX_OUTOFRANGE;
			}
		}
	}
	return retval;
}
#endif

/***************************************************
OnStatus
    This virtual function is called each time we have any
    status information to display.
Params
    StatusText  - status text
Return
    UTE_SUCCESS - success   
****************************************************/
int CUT_FTPServer::OnStatus(LPCSTR StatusText)
{
    #ifdef UT_DISPLAYSTATUS 
        ctrlHistory->AddLine(StatusText);
    #endif

    return UTE_SUCCESS;
}

/********************************************
SetMaxTimeOut()
  Set the maximum time to wait for client intervention before 
  remotely disconnecting the client
PARAM
    int miliSec - time out in seconds
RETURN
    VOID
********************************************/
void CUT_FTPServer::SetMaxTimeOut(int Sec)
{
    EnterCriticalSection(&m_criticalSection);
    m_MaxTimeOut = Sec;
    LeaveCriticalSection(&m_criticalSection);
}

/********************************************
GetMaxTimeOut()
  Get the maximum time to wait for client intervention
  before  remotely disconnecting the client
PARAM
    NONE
RETURN
    maximum time in seconds 
********************************************/
int CUT_FTPServer::GetMaxTimeOut() const
{
    return m_MaxTimeOut;
}

/***************************************************
GetCmdNameFromID
    Gets name of the command by ID
Params
    CommandID   - command ID
Return
    pointer to the name or NULL
****************************************************/
LPCSTR CUT_FTPServer::GetCommandName(CommandID ID)
{
    static char *szCommandNames[] = {
        "Unknown command",
        "USER",     "PASS",     "ACCT",     "CWD",      "CDUP",
        "SMNT",     "QUIT",     "REIN",     "PORT",     "PASV",
        "TYPE",     "STRU",     "MODE",     "RETR",     "STOR",
        "STOU",     "APPE",     "ALLO",     "REST",     "RNFR",
        "RNTO",     "ABOR",     "DELE",     "RMD",      "MKD",
        "PWD",      "LIST",     "NLST",     "SITE",     "SYST",
        "STAT",     "HELP",     "AUTH" , "FEAT" ,"NOOP", "ABORT"
    };

    if(ID > CMD_UNKNOWN && ID <= NOOP)
        return szCommandNames[ID];
    else
        return szCommandNames[CMD_UNKNOWN];
}


//=================================================================
//  class: CUT_FTPThread 
//=================================================================

/***************************************************
OnMaxConnect
    This function is called when a new client connects
    to the server and the server has already reached
    its max. connection limit.
Params
    none
Return 
    none
****************************************************/
void CUT_FTPThread::OnMaxConnect(){

    char    data[WSS_BUFFER_SIZE + 1];
    Send("550 Too Many Connections, Please Try Later.\r\n",0);

    _snprintf(data,sizeof(data)-1,"(%d) Max Connections Reached",
        m_winsockclass_this->GetNumConnections());

    ((CUT_FTPServer *)m_winsockclass_this)->OnStatus(data);
}

int CUT_FTPThread::SocketOnConnected(SOCKET /* s */, const char * /* lpszName */)
{
	return UTE_SUCCESS;
}

/***************************************************
OnConnect
    This function is called when a new client 
    connects to the FTP server. This function
    processes all commands from the client until
    the connection is terminated or until the client
    sends a QUIT command.
Params
    none
Return
    none
****************************************************/
void CUT_FTPThread::OnConnect(){

  // Set up the variables
    m_nExit         = FALSE;

	
	m_nLoginState   = 0;                // FTP progress state 0- user needed 1-pass needed 2 -logged in
    m_nSuccess      = -1;               // Command success flag

	m_bPASV			= FALSE;			//  passive mode is disabled
	
    m_szUser[0]     = NULL;
	m_wscDataConnection = NULL;


#ifdef CUT_SECURE_SOCKET
	
	m_bHandshakeDone = FALSE;
  
	BOOL bSecureFlag = GetSecurityEnabled();

	// 4.2  Implicit Session Security
	//In this scenario, the server listens on a distinct port {FTP-
    //  TLSPORT} to the normal unsecured ftp server.  Upon connection, the
    //  client is expected to start the TLS negotiation.  If the
    //  negotiation fails or succeeds at an unacceptable level of security
    //  then it will be a client and/or server policy decision to
    //  disconnect the session.
	if (bSecureFlag)
		if 	( ((CUT_FTPServer*) m_winsockclass_this)->m_FtpSSLConnectionType == FTP_SSL_IMPLICIT)
		{
			int nResult = CUT_SecureSocket::SocketOnConnected(m_clientSocket, "");
			if (nResult != UTE_SUCCESS)
				return ;
			else
				m_bHandshakeDone = TRUE;
		}
		else
				SetSecurityEnabled(FALSE);	

	// otherwise we will just wait for the negotiation

	
#endif


	//Get the default data port and address
    m_nDataPort = m_clientSocketAddr.sin_port;
    strcpy(m_szDataIPAddress,inet_ntoa (m_clientSocketAddr.sin_addr));
	
    // Set up the initial directory
    CUT_FTPServer *fs = (CUT_FTPServer*) m_winsockclass_this;
    strcpy(m_szPath,fs->GetPath());
    strcpy(m_szRootPath,fs->GetPath());
	
	
    // Send a connect notification
    if(OnConnectNotify(GetClientAddress()) == FALSE) 
	{
        Send("500 Disconnecting\r\n");
		m_nSuccess = false;
        return;
	}
    else
        // Send connect message to the client program
        Send("220 Connected to - Win32 FTP server\r\n");
    
	
    // Main command loop
    while(!m_nExit) 
	{
		
		
        // Run the command finished notification
        // but skip the first time, since no command has run yet
        // (m_nSuccess is set to -1 for the first round)
        if(m_nSuccess >= 0)
            OnCommandFinished(m_nCommand,m_szData,m_nSuccess);
		
        // Receive a line.
        // Break max timout into smaller parts so as to avoid a 
        // lengthy wait on closing down of the server.
        int scs =fs->GetMaxTimeOut();
        if(0 >= scs) 
		{
            for(;;) 
			{
                m_nLen = ReceiveLine(m_szData,sizeof(m_szData),1);
                if( 0 != m_nLen) 
				{
                    break;
                }
				// server is shutting down
				if (fs->m_bShutDown )
				{
				  Send("500 Disconnecting\r\n");
				  return ;
				}
				// lost connection
				if (!IsConnected())
				{
					OnDisconnect();
					return;

				}
            }
        }
        else 
		{
            for (;scs;scs--) 
			{
                m_nLen = ReceiveLine(m_szData,sizeof(m_szData),1);
               	// server is shutting down
				if (fs->m_bShutDown )
				{
				  Send("500 Disconnecting\r\n");
				  return ;
				}
                if(0 != m_nLen) 
				{
                    // got a command from the user
                    break;
                }	// lost connection
				if (!IsConnected())
				{
					OnDisconnect();
					return;

				}
            }
            // out of read loop - assume if m_nLen = 0 we've timed out
            if(0 == m_nLen)
			{
				m_nExit = TRUE;
				continue;
            }
        }
		
		
		
        // Before we process the command lets see
        // if the server admin has issued shut down command
        if (m_winsockclass_this->GetShutDownFlag() == TRUE)  
            break;
		
        // Exit on an error
        if(m_nLen <= 0) 
            break;  
		
        CUT_StrMethods::RemoveCRLF(m_szData);
	
        // Get the command that was sent
        m_nCommand = GetCommand(m_szData);
		
        // Send a command notification
        if(OnCommandStart(m_nCommand,m_szData) == FALSE) 
		{
            Send("550 Access is denied\r\n");
            m_nSuccess = FALSE;
            continue;
        }
		
        if(m_nCommand == CMD_UNKNOWN) 
		{
            Send("502 Command not implemented\r\n");
            m_nSuccess = FALSE;
            continue;
        }
        
        // Set the success flag to TRUE
        m_nSuccess = TRUE;
		
        // Process commands that can be accessed without logging in
        switch(m_nCommand) 
		{
			
            // USER ///////////////////////////////////
		case USER:
			{
				// Get the user name
				if(CUT_StrMethods::ParseString(m_szData," ",1,m_szParam,sizeof(m_szParam))==CUT_SUCCESS) 
				{
					// Store the username
					strcpy(m_szUser,m_szParam);
					m_nLoginState = 1;
					Send("331 anonymous access allowed, send email name as PASS\r\n");
				}
				else 
				{
					Send("500 USER: command not understood\r\n");
					m_nSuccess = FALSE;
				}
				continue;
			}
			
            // PASS ///////////////////////////////////
		case PASS:
			{
				// Check to see if the USER command was already given
				if(m_nLoginState < 1)
				{
					Send("500 PASS: USER must be sent first\r\n");
					m_nSuccess = FALSE;
				}
				
				// Get the password
				if(CUT_StrMethods::ParseString(m_szData," ",1,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
				{
					// Store the password
					strcpy(m_szPass,m_szParam);
					// Check the password
					if(OnCheckPassword(m_szUser,m_szPass)==TRUE)
					{
						// Good password
						m_nLoginState =2;
						OnSetUserPath(m_szRootPath);
						// Make the current directory the default directory at the start  
						strcpy(m_szPath,m_szRootPath);


#ifdef CUT_SECURE_SOCKET						
						// if the security is enabled and the 
						// client did not log in with secure negotiation then 
						// check with the user if he wants to continue with unsecure connection
						// in this security enabled server
						
						/***********************************************
						4.1.2  Server wants a secured session
						
						  The FTP protocol does not allow a server to directly dictate
						  client behaviour, however the same effect can be achieved by
						  refusing to accept certain FTP commands until the session is
						  secured to an acceptable level to the server.
						  
						*************************************************/

						if (!m_bHandshakeDone  && bSecureFlag )
							if (!OnNonSecureConnection(GetClientAddress()))
							{
								Send("530 Please login with USER and PASS\r\n");
								m_nSuccess = FALSE;
								continue;
							}
#endif // CUT_SECURE_SOCKET

						
						// Change these lines to present your own server login message
						Send("230-logon successful\r\n");
						m_nSuccess = TRUE;
						OnWelcome();
						Send("230 \r\n");
						m_nLoginState =2;
					}
					else 
					{
						// Bad password
						Send("530 User cannot log in\r\n");
						m_nSuccess = FALSE;
					}
				}
				else 
				{
					Send("500 PASS: command not understood\r\n");
					m_nSuccess = FALSE;
				}
				
				continue;
			}
			
            // ACCT ///////////////////////////////////
		case ACCT: 
			{
				Send("502 Command not implemented\r\n");
				m_nSuccess = FALSE;
				continue;
			}
			
            // QUIT ///////////////////////////////////
		case QUIT: 
			{
				Send("221 Goodbye\r\n");
				m_nExit = TRUE;
				continue;
			}
			
            // HELP ///////////////////////////////////
		case HELP: 
			{
				Send("214-The following commands are implemented\r\n");
				Send("     USER  PASS  ACCT  QUIT  PORT  RETR\r\n");
				Send("     STOR  DELE  RNFR  PWD   CWD   CDUP\r\n");
				Send("     MKD   RMD   NOOP  TYPE  MODE  STRU\r\n");
				Send("     LIST  HELP  FEAT	 UTF8	PASV  ABORT\r\n");
				Send("214  HELP command successful\r\n");
				
				continue;
			}
			
            // NOOP ///////////////////////////////////
		case NOOP:
			{
				Send("200 NOOP command successful\r\n");
				continue;
			}
			
			// Display the feature commands list
		case FEAT:
			{
				Send("211-Extensions supported\r\n");
				Send("  AUTH SSL\r\n");
				Send("211 END\r\n");
				continue;
			}
		case AUTH:
			{
				
#ifndef	 CUT_SECURE_SOCKET
				Send("503 unknown security mechanism\r\n");
				m_nSuccess = FALSE;
				continue;

#else //CUT_SECURE_SOCKET	
				
				// check the parameters if we support the auth mechanism
				// check if security is enabled
				if(bSecureFlag)
				{
					// parse the data for the authintication parameter
					if(CUT_StrMethods::ParseString(m_szData," ",1,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						// we will support SSL for now until the draft is done
						if ((stricmp(m_szParam, "SSL") == 0) || (stricmp(m_szParam, "TLS-P") == 0) )
						{
							Send ("234 SSL enabled start the negotiation\r\n");
							// set security flag
							SetSecurityEnabled(TRUE);
							// start negotiation
							int nResult = CUT_SecureSocket::SocketOnConnected(m_clientSocket, "");

							// when we fail the hand shake 
							// we should start speaking plain text
							// so disable encryption 
							if (nResult != UTE_SUCCESS)
							{
								SetSecurityEnabled(FALSE);									
								// send the error 
								Send("421  The SSL negotiation failed during the SSL handshake");
								// make sure we exit
								m_nExit = TRUE;
								continue;
							} // good
							else		// check results
							{
								m_bHandshakeDone =  TRUE;
								continue;
							}
							
						}
						else
						{
							Send("504 Unknown security mechanism");
							m_nSuccess = FALSE;
							continue;
						}
						
					}
					else
					{
						Send("500 AUTH: command not understood\r\n");
						m_nSuccess = FALSE;
						continue;
					}
					
				}
				else
				{
					Send("500 AUTH: command not understood\r\n");
					m_nSuccess = FALSE;
					continue;
				}
#endif			//  CUT_SECURE_SOCKET						
				continue;
			}


		default:
				{
					// Only continue if logged in
					if(m_nLoginState < 2) 
					{
						Send("530 Please login with USER and PASS\r\n");
						m_nSuccess = FALSE;
						continue;
					}
				}

		}

			
			// Process commands that can be accessed after logging in
			switch(m_nCommand) 
			{
				
				// CWD ///////////////////////////////////
            case CWD: 
				{
					char szPath[WSS_BUFFER_SIZE+1];
					memset(szPath,0,sizeof(szPath));

					// Get the directory
					if(CUT_StrMethods::ParseString(m_szData," ",0,m_szParam,sizeof(m_szParam))!=CUT_SUCCESS)
						strcpy(szPath,"\\");
					else
						strncpy(szPath,&m_szData[strlen(m_szParam)],sizeof(szPath)-1);

					
					// Set the directory
					if(SetDir(szPath)==TRUE)
					{
						Send("250 CWD command successful\r\n");
						m_nSuccess = TRUE;
                    }
					else 
					{ 
						Send("550 CWD failed, path not found\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// CDUP ///////////////////////////////////
            case CDUP:
				{
					if(SetDir("..") == TRUE) 
					{
						Send("250 CWD command successful\r\n");
						m_nSuccess = TRUE;
                    }
					else 
					{
						Send("550 Access is denied\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// SMNT ///////////////////////////////////
            case SMNT: 
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
				
				// REIN ///////////////////////////////////
            case REIN: 
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
				
				// PORT ///////////////////////////////////
            case PORT: 
				{
					  
					m_bPASV = FALSE;

					if(CUT_StrMethods::ParseString(m_szData," ",1,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						if( SetDataPort(m_szParam))
							Send("250 PORT command successful\r\n");
						else {
							Send("550 PORT failed\r\n");
							m_nSuccess = FALSE;
                        }
                    }
					else 
					{
						Send("500 PORT: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// PASV ///////////////////////////////////
            case PASV:
				{
	
					char buf[WSS_BUFFER_SIZE + 1];
				    if ( SetDataPortPASV() )
					{
						_snprintf(buf,sizeof(buf)-1,"227 Entering Passive Mode (%s).\r\n",m_szDataPASVAddress);
						Send(buf);
						m_bPASV = TRUE;
					}
					else
					{
						Send("550 PASV failed");
						m_bPASV = FALSE;
						m_nSuccess = FALSE;
					}

					
					continue;				
				}
				
				// TYPE ///////////////////////////////////
            case TYPE: 
				{
					if(CUT_StrMethods::ParseString(m_szData," ",1,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						_strupr(m_szParam);
						if(m_szParam[0]=='A' || m_szParam[0] =='I')
							Send("250 TYPE command successful\r\n");
						else 
						{
							Send("550 TYPE failed\r\n");
							m_nSuccess = FALSE;
                        }
                    }
					else
					{
						Send("500 TYPE: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// STRU ///////////////////////////////////
            case STRU:
				{
					if(CUT_StrMethods::ParseString(m_szData," ",1,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						_strupr(m_szParam);
						if(m_szParam[0]=='F')
							Send("250 STRU command successful\r\n");
						else
						{
							Send("550 STRU failed\r\n");
							m_nSuccess = FALSE;
                        }
                    }
					else
					{
						Send("500 STRU: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// MODE ///////////////////////////////////
            case MODE:
				{
					Send("502 MODE command not implemented\r\n");
					m_nSuccess = FALSE;
					break;
                }
				
				// RETR ///////////////////////////////////
            case RETR:
				{
					char szPath[WSS_BUFFER_SIZE+1];
					memset(szPath,0,sizeof(szPath));

					// Get the directory
					if(CUT_StrMethods::ParseString(m_szData," ",0,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						strncpy(szPath,&m_szData[strlen(m_szParam)],sizeof(szPath)-1);
						if(RetrFile(szPath))
						{
							Send("226 Transfer Complete\r\n");
							m_nSuccess = TRUE;
						}
						else
						{
							Send("550 RETR failed\r\n");
							m_nSuccess = FALSE;
                        }
                    }
					else 
					{
						Send("500 RETR: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// STOR ///////////////////////////////////
            case STOR: 
				{
					char szFileName[WSS_BUFFER_SIZE+1];
					memset(szFileName,0,sizeof(szFileName));

					if(CUT_StrMethods::ParseString(m_szData," ",0,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						strncpy(szFileName,&m_szData[strlen(m_szParam)],sizeof(szFileName)-1);
						Send("150 STOR command started\r\n");
						if(StorFile(szFileName))
						{
							Send("226 Transfer Complete\r\n");
							m_nSuccess = TRUE;
						}
						else
						{
							Send("550 STOR failed\r\n");
							m_nSuccess = FALSE;
                        }
                    }
					else
					{
						Send("500 STOR: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// STOU ///////////////////////////////////
            case STOU: 
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
				
				// APPE ///////////////////////////////////
            case APPE: 
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
				
				// ALLO ///////////////////////////////////
            case ALLO: 
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
				
				// REST ///////////////////////////////////
            case REST: 
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
				
				// RNFR ///////////////////////////////////
            case RNFR: 
				{
					char szFileName[WSS_BUFFER_SIZE+1];
					memset(szFileName,0,sizeof(szFileName));

					if(CUT_StrMethods::ParseString(m_szData," ",0,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						strncpy(szFileName,&m_szData[strlen(m_szParam)],sizeof(szFileName)-1);
			
						strcpy(m_szFrom,szFileName);
						Send("350 requested file action pending further information\r\n");
                    }
					else 
					{
						Send("500 RNFR: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// RNTO ///////////////////////////////////
            case RNTO: 
				{
					char szFileName[WSS_BUFFER_SIZE+1];
					memset(szFileName,0,sizeof(szFileName));

					if(CUT_StrMethods::ParseString(m_szData," ",0,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						strncpy(szFileName,&m_szData[strlen(m_szParam)],sizeof(szFileName)-1);
			
						if(RenameFile(m_szFrom,szFileName))
							Send("250 RNTO command successful\r\n");
						else {
							Send("550 RNTO failed\r\n");
							m_nSuccess = FALSE;
                        }
                    }
					else
					{
						Send("500 RNTO: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// ABOR ///////////////////////////////////
            case ABOR:
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
				
				// DELE ///////////////////////////////////
            case DELE: 
				{
					char szFileName[WSS_BUFFER_SIZE+1];
					memset(szFileName,0,sizeof(szFileName));

					if(CUT_StrMethods::ParseString(m_szData," ",0,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						strncpy(szFileName,&m_szData[strlen(m_szParam)],sizeof(szFileName)-1);
			
						if(DeleteFile(szFileName))
							Send("250 DELE command successful\r\n");
						else 
						{
							Send("550 DELE failed\r\n");
							m_nSuccess = FALSE;
                        }
                    }
					else
					{
						Send("500 DELE: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// RMD ///////////////////////////////////
            case RMD: 
				{
					char szDirName[WSS_BUFFER_SIZE+1];
					memset(szDirName,0,sizeof(szDirName));
					if(CUT_StrMethods::ParseString(m_szData," ",0,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						strncpy(szDirName,&m_szData[strlen(m_szParam)],sizeof(szDirName)-1);
			
						if(RemoveDirectory(szDirName))
							Send("250 RMD command successful\r\n");
						else 
						{
							Send("550 RMD failed\r\n");
							m_nSuccess = FALSE;
                        }
                    }
					else
					{
						Send("500 RMD: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// MKD ///////////////////////////////////
            case MKD: 
				{
					char szDirName[WSS_BUFFER_SIZE+1];
					memset(szDirName,0,sizeof(szDirName));
					if(CUT_StrMethods::ParseString(m_szData," ",0,m_szParam,sizeof(m_szParam))==CUT_SUCCESS)
					{
						strncpy(szDirName,&m_szData[strlen(m_szParam)],sizeof(szDirName)-1);
			
						if(MakeDirectory(szDirName) )
							Send("250 MKD command successful\r\n");
						else 
						{
							Send("550 MKD failed\r\n");
							m_nSuccess = FALSE;
                        }
                    }
					else
					{
						Send("500 MKD: command not understood\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// PWD ///////////////////////////////////
            case PWD: 
				{
					// v4.2 Changed to size_t
					size_t len = strlen(m_szRootPath) -1;
					char buf[2 * MAX_PATH];
					_snprintf(buf,sizeof(buf)-1,"257 \"%s\" is current directory\r\n",&m_szPath[len]);
					Send(buf);
					continue;
                }
				
				// LIST ///////////////////////////////////
            case LIST: 
				{
					Send("150 Opening ASCII mode data connection\r\n");
					if(ListDirectory())
						Send("226 Transfer Complete\r\n");
					else 
					{
						Send("550 LIST failed\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// NLST ///////////////////////////////////
            case NLST: 
				{
					Send("150 Opening ASCII mode data connection\r\n");
					if(NListDirectory())
						Send("226 Transfer Complete\r\n");
					else 
					{
						Send("550 NLST failed\r\n");
						m_nSuccess = FALSE;
                    }
					continue;
                }
				
				// SITE ///////////////////////////////////
            case SITE: 
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
				
				// SYST ///////////////////////////////////
            case SYST: 
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
				
				// STAT ///////////////////////////////////
            case STAT: 
				{
					Send("502 Command not implemented\r\n");
					m_nSuccess = FALSE;
					continue;
                }
		
				
				// Default command ////////////////////////
            default: 
				{
					Send("500 command not understood\r\n");
					m_nSuccess = FALSE;
                }
            }
        
	}
	delete m_wscDataConnection;
	m_wscDataConnection = NULL;
	OnDisconnect();
}
/********************************
GetCommand  
    Parses a command line received 
    from the client and returns a 
    number that matches the command
    issued. If the command issued is
    invalid then 0 is returned
Params
    data - command line received from
        the client connection
Return
    command number or zero
*********************************/
CommandID CUT_FTPThread::GetCommand(LPSTR data){

    char buf[5];

    buf[0] = data[0];
    buf[1] = data[1];
    buf[2] = data[2];
    buf[3] = data[3];
    buf[4] = 0;
    if(buf[3]==32)
        buf[3]= 0;

    _strupr(buf);

    if(strcmp(buf,"USER") == 0)
        return USER;
    else if(strcmp(buf,"PASS") == 0)
        return PASS;
    else if(strcmp(buf,"ACCT") == 0)
        return ACCT;
    else if(strcmp(buf,"CWD") == 0)
        return CWD;
    else if(strcmp(buf,"CDUP") == 0)
        return CDUP;
    else if(strcmp(buf,"SMNT") == 0)
        return SMNT;
    else if(strcmp(buf,"QUIT") == 0)
        return QUIT;
    else if(strcmp(buf,"REIN") == 0)
        return REIN;
    else if(strcmp(buf,"PORT") == 0)
        return PORT;
    else if(strcmp(buf,"PASV") == 0)
        return PASV;
    else if(strcmp(buf,"TYPE") == 0)
        return TYPE;
    else if(strcmp(buf,"STRU") == 0)
        return STRU;
    else if(strcmp(buf,"MODE") == 0)
        return MODE;
    else if(strcmp(buf,"APPE") == 0)
        return APPE;
    else if(strcmp(buf,"ALLO") == 0)
        return ALLO;
    else if(strcmp(buf,"REST") == 0)
        return REST;
    else if(strcmp(buf,"RNFR") == 0)
        return RNFR;
    else if(strcmp(buf,"RNTO") == 0)
        return RNTO;
    else if(strcmp(buf,"ABOR") == 0 || strcmp(buf,"ABORT") == 0)
        return ABOR;
    else if(strcmp(buf,"DELE") == 0)
        return DELE;
    else if(strcmp(buf,"RMD") == 0)
        return RMD;
    else if(strcmp(buf,"MKD") == 0)
        return MKD;
    else if(strcmp(buf,"PWD") == 0 || strcmp(buf,"XPWD") == 0)
        return PWD;
    else if(strcmp(buf,"LIST") == 0)
        return LIST;
    else if(strcmp(buf,"NLST") == 0)
        return NLST;
    else if(strcmp(buf,"SITE") == 0)
        return SITE;
    else if(strcmp(buf,"SYST") == 0)
        return SYST;
    else if(strcmp(buf,"STAT") == 0)
        return STAT;
    else if(strcmp(buf,"HELP") == 0)
        return HELP;
    else if(strcmp(buf,"NOOP") == 0)
        return NOOP;
    else if(strcmp(buf,"RETR") == 0)
        return RETR;
    else if(strcmp(buf,"STOR") == 0)
        return STOR;
	else if (strcmp(buf,"AUTH") == 0)
		return AUTH;
	else if (strcmp(buf,"FEAT") == 0)
		return FEAT;
    return CMD_UNKNOWN;
}

/***************************************************
SetDir
    Sets the directory for the current client
Params
    adjustPath - relative path to move into
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::SetDir(LPSTR adjustPath)
{

    int     tempPos, loop, pos = 0;
    int     len = (int)strlen(adjustPath);
    char    tempPath[WSS_BUFFER_SIZE];

    // Change all '/' to '\'
    for(loop = 0; loop < len; loop++) 
	{    
        if(adjustPath[loop] == '/')
            adjustPath[loop] = '\\';
	}
		// skip all leading spaces
	while (adjustPath[pos] == ' ' && pos < len)
			pos++;
	
    // Check to see if the adjustPath starts with an '\'
    if(adjustPath[pos] == '\\') 
	{
        pos++;
         // Setup the temp path
        strcpy(tempPath,m_szRootPath);
	}
    else 
	{	
        // Setup the temp path
        strcpy(tempPath,m_szPath);
	}
    
    // Append the adjusted path
    while(pos < len) 
	{

        // Check for "\\"
        if(adjustPath[pos]=='\\')
		{
            // Do nothing
		}
		// Add in anything else to the temp path
		else
		{
			tempPos = (int)strlen(tempPath);
			if(tempPath[tempPos-1] != '\\') 
			{
				tempPath[tempPos] = '\\';
				tempPos++;
			}

			while(adjustPath[pos] != '\\' && adjustPath[pos] != 0) 
			{
                tempPath[tempPos] = adjustPath[pos];
                tempPos++;
                pos++;
                if(tempPos > 252)
                    return FALSE;
			}

             tempPath[tempPos]      = '\\';
             tempPath[tempPos+1]    = 0;
            }

        // Find the next section in the adjust path
        while(pos < len) 
		{
          if(adjustPath[pos] == '\\') 
		  {
            pos++;
                break;
		  }
		  pos++;
		}
	} 

    if(_chdir(tempPath) == 0) 
	{
        // Final check... if m_szRootPath not contained in the current
        // directory string, user has hacked our checks - revert to 
        // previous...
        char    buf[MAX_PATH + 1];
        (void)GetCurrentDirectoryA(MAX_PATH, buf);

        // Make sure that the path ends with a back slash
        int len = (int)strlen(buf);
        if(buf[len-1] != '\\')
            strcat(buf,"\\");

        if(!strstr(buf, m_szRootPath)) 
		{
            _chdir(m_szPath);  // revert to prev
            return FALSE;
		}

        // Now lets copy back the current directory to the m_szPath in case we have any ".." operations 
        (void)GetCurrentDirectoryA(MAX_PATH, buf);
        strcpy(m_szPath,buf);
        return TRUE;
    }
    return FALSE;
}

/***************************************************
DeleteFile
    Deletes the specified file
Params
    file - file to delete
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::DeleteFile(LPSTR file)
{

    char buf[WSS_BUFFER_SIZE + 1];

    strcpy(buf,m_szPath);
    if(MakePath(buf,file,WSS_BUFFER_SIZE) != TRUE)
        return FALSE;

    // Check if the file was deleted successfully
    if (remove(buf) == 0)
        return TRUE;

     return FALSE;
}

/***************************************************
RenameFile
    Renames the specified file
Params
    oldfile - file to rename
    newfile - the new name of the file
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::RenameFile(LPSTR oldfile,LPSTR newfile)
{

    char    buf[WSS_BUFFER_SIZE + 1];
    char    buf2[WSS_BUFFER_SIZE + 1];

    strcpy(buf,m_szPath);
    strcpy(buf2,m_szPath);

    if(MakePath(buf,oldfile, WSS_BUFFER_SIZE) != TRUE)
        return FALSE;

    if(MakePath(buf2,newfile, WSS_BUFFER_SIZE) != TRUE)
        return FALSE;

    if(rename(buf,buf2) != 0)
        return FALSE;

    return TRUE;
}

/***************************************************
MakeDirectory
    Creates a new directory
Params
    dir - name of new directory
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::MakeDirectory(LPSTR dir)
{

    char buf[WSS_BUFFER_SIZE + 1];

    strcpy(buf,m_szPath);
    if(MakePath(buf,dir, WSS_BUFFER_SIZE) != TRUE)
        return FALSE;

    if(_mkdir(buf) != 0)
        return FALSE;

    return TRUE;
}

/***************************************************
RemoveDirectory
    removes an existing directory
Params
    dir - name of directory to remove
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::RemoveDirectory(LPSTR dir)
{
    
    char buf[WSS_BUFFER_SIZE + 1];

    strcpy(buf,m_szPath);
    if(MakePath(buf,dir, WSS_BUFFER_SIZE) != TRUE)
        return FALSE;

    if(_rmdir(buf) != 0)
        return FALSE;

    return TRUE;
}

/***************************************************
ListDirectory
    Sends the current directory information to 
    the client on the data channel. The server
    connects to the client on the data channel
    then sends the information and closes
Params
    none
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::ListDirectory()
{

    WIN32_FIND_DATAA wfd;
    SYSTEMTIME      st;
    HANDLE          h;
    time_t          tim;
    struct tm       *tm;
    int             loop, len;
    char            buf[WSS_BUFFER_SIZE + 1];
    char            buf2[WSS_BUFFER_SIZE + 1];
    static char         *Months[]={"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

	// Set the security attributes of the new connection 
	// to be the same as this class's attributes

	if (CreateDataConnection () !=UTE_SUCCESS)
        return FALSE;

    strcpy(buf,m_szPath);

    // If the path does not end with a \ then 
    if (buf[strlen(buf)-1] != '\\')
        strcat(buf,"\\*.*");
    else
        strcat(buf,"*.*");

    h = FindFirstFileA(buf,&wfd);

    if( h == INVALID_HANDLE_VALUE) 
	{
        m_wscDataConnection->CloseConnection();
        return FALSE;
	}

    do 
	{
        // Clear the buffer
        for(loop=0;loop<64;loop++)
            buf[loop]=32;

        // Setup the attribs
        strncpy(buf,"-r-xr-x---",10);

        // Directory attribs
        if(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
            buf[0] = 'd';

        // Read attribs      
        if(!(wfd.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN))
            buf[7] = 'r';   //general user read access

        // Write attribs
        if(!(wfd.dwFileAttributes&FILE_ATTRIBUTE_READONLY))
            buf[8] = 'w';   //general user write access

        // Execute attribs
        if(!(wfd.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN))
            buf[9] = 'x';   //general user execute

        //Blocks
        buf[13] = '1';

        // Owner
        strncpy(buf+15,"owner",5);

        // Group
        strncpy(buf+24,"group",5);

        // File size
        _ltoa(wfd.nFileSizeLow,buf2,10);
        len = (int)strlen(buf2);
        for(loop = 0;loop < len; loop++)
            buf[44 - loop] = buf2[len-loop-1];

        // Date
        FileTimeToSystemTime(&wfd.ftCreationTime,&st);
        strncpy(buf+46,Months[st.wMonth],3);
        _itoa(st.wDay,buf2,10);
        if(st.wDay >9)
            strncpy(buf+50,buf2,2);
        else
            strncpy(buf+51,buf2,1);

        // Check to see if the files year is the same as the system
        time(&tim);
        tm = localtime(&tim);
        if((tm->tm_year+1900) == st.wYear) 
		{
            if(st.wHour <10) 
			{
                _itoa(st.wHour,buf2,10);
                buf[53] = '0';
                buf[54] = buf2[0];
			}
            else 
			{
                _itoa(st.wHour,buf2,10);
                strncpy(buf+53,buf2,2);
			}
            if(st.wMinute < 10) 
			{
                _itoa(st.wMinute,buf2,10);
                buf[56] = '0';
                buf[57] = buf2[0];
			}
            else
			{
                _itoa(st.wMinute,buf2,10);
                strncpy(buf+56,buf2,2);
			}
            buf[55] = ':';
		}
        else 
		{
            _itoa(st.wYear,buf2,10);
            strncpy(buf+54,buf2,4);
		}

        // Filename
		// make sure that the file name can fit in the buffer 
		strncpy(buf+59,wfd.cFileName, WSS_BUFFER_SIZE- 61);
	    strcat(buf,"\r\n");

        m_wscDataConnection->Send(buf);

    } while(FindNextFileA(h,&wfd) == TRUE);

    FindClose(h);
	m_wscDataConnection->ReleaseDataConnection();
	m_bPASV = FALSE;
	if (m_wscDataConnection)	
	{
		delete m_wscDataConnection;
		m_wscDataConnection = NULL;

	}
    

    return TRUE;
}

/***************************************************
ListDirectory
    Sends only the names in the current directory to 
    the client on the data channel. The server
    connects to the client on the data channel
    then sends the information and closes
Params
    none
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::NListDirectory()
{

    WIN32_FIND_DATAA wfd;
    HANDLE          h;
    char            buf[WSS_BUFFER_SIZE];

	// Set the security attributes of the new connection 
	// to be the same as this class's attributes
	if (CreateDataConnection () !=UTE_SUCCESS)
        return FALSE;
     
    strcpy(buf,m_szPath);

    // If the path does not end with a \ then 
    if (buf[strlen(buf)-1] != '\\')
        strcat(buf,"\\*.*");
    else
        strcat(buf,"*.*");

    h = FindFirstFileA(buf,&wfd);

    if( h == INVALID_HANDLE_VALUE) 
	{
        m_wscDataConnection->CloseConnection();
        return FALSE;
	}

    do 
	{
        // Filename
     	// make sure that the file name can fit in the buffer 
		strncpy(buf+59,wfd.cFileName, WSS_BUFFER_SIZE- 61);
	    strcat(buf,"\r\n");

        m_wscDataConnection->Send(buf);
	} while(FindNextFileA(h,&wfd) == TRUE);

    FindClose(h);
    m_wscDataConnection->ReleaseDataConnection();
	delete m_wscDataConnection;
	m_wscDataConnection = NULL;

	m_bPASV = FALSE;

    return TRUE;
}

/***************************************************
SetDataPort
    Sets the port and address to use as the data channel 
    to send information over to the client
Params
    param - contains the IP address and port in a string 
        where each number is separated by a comma
        eg. '123,134,145,178,2,34'
            address = 123.134.145.178
            port = 2 *256 + 34 = 546  (hibyte,lobyte)
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::SetDataPort(LPSTR param)
{

    int     count, loop;
    int     len = (int)strlen(param);
    char    buf[30];
    int     hibytepos = 0,lobytepos = 0;
    BYTE    hibyte,lobyte;

    // Check to see if the param is too long - if it is then it must be invalid
    if(len > 25)
        return FALSE;

    // Copy the string over
    strcpy(buf,param);

    // Get the IP address
    count = 0;
    for(loop = 0; loop < len; loop++) 
	{
        if(buf[loop] == ',') 
		{
            buf[loop] = '.';
            count++;
            if(count == 4) 
			{
                buf[loop] = 0;
                strcpy(m_szDataIPAddress,buf);
                hibytepos = loop+1;
			}
            if(count == 5) 
			{
                buf[loop] =0;
                lobytepos = loop+1;
			}
		}
	}

   // Check to see if the lobytepos and hibytepos vars have been set
    if(hibytepos == 0 || lobytepos == 0)
        return FALSE;
	
	// v4.2 Casts to avoid conversion warning")
	hibyte = (BYTE)atoi(&buf[hibytepos]);
    lobyte = (BYTE)atoi(&buf[lobytepos]);

	// if the programmer have allowed bounce attacks then it 
	// is fine other wise make sure the number is greater than 1024
	CUT_FTPServer *fs = (CUT_FTPServer*) m_winsockclass_this;
	if (hibyte >= 4 || !fs->m_bDataPortProtection)
	{
		   m_nDataPort = MAKEWORD(lobyte,hibyte);
		    return TRUE;
	}
	return FALSE;

}

/***************************************************
RetrFile
    Sends the specified file to the client over
    the data channel
Params
    filename - name of file to send
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::RetrFile(LPSTR filename)
{

    char            buf[WSS_BUFFER_SIZE + 1];
    WIN32_FIND_DATAA wfd;
    HANDLE          h;
    int             rt = TRUE;


    // Make the path
    strcpy(buf,m_szPath);
    if(MakePath(buf,filename, WSS_BUFFER_SIZE) != TRUE)
        return FALSE;

    // Check to see if the file exists
    h = FindFirstFileA(buf,&wfd);
    if( h == INVALID_HANDLE_VALUE)
        return FALSE;
    
    if(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
        return FALSE;
 
	// We will inform the client before attempting to connect
    Send("150 RETR command started\r\n"); 

	// Set the security attributes of the new connection 
	// to be the same as this class's attributes
	if (CreateDataConnection () !=UTE_SUCCESS)
        return FALSE;
 
	long	bytesSent = 0;
	// v4.2 using WC() here 
	rt =  m_wscDataConnection->SendFile(WC(buf),&bytesSent);

    m_wscDataConnection->ReleaseDataConnection();
	delete m_wscDataConnection;
	m_wscDataConnection = NULL;
	m_bPASV = FALSE;

    FindClose(h);

    if(rt != CUT_SUCCESS)
        return FALSE;

    return TRUE;
}

/***************************************************
StorFile
    Retrieves a specified file from the client over
    the data channel
Params
    filename - name of file to retrieve
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::StorFile(LPSTR filename)
{

    char            buf[WSS_BUFFER_SIZE + 1];
    int             rt = TRUE;

	
	if (CreateDataConnection () !=UTE_SUCCESS)
        return FALSE;

   
    strcpy(buf,m_szPath);
    if(MakePath(buf,filename, WSS_BUFFER_SIZE) != TRUE)
        return FALSE;

	long	bytesReceived = 0;
	// v4.2 using WC() here 
	rt = m_wscDataConnection->ReceiveToFile(WC(buf), UTM_OM_WRITING, &bytesReceived);

   
    m_wscDataConnection->ReleaseDataConnection();
	m_bPASV = FALSE;
    if(rt != CUT_SUCCESS)
        return FALSE;

    return TRUE;
}

/***************************************************
MakePath
    Creates the absolute path to a file for a given
    filename.
Params
    path - buffer where results are returned in
    filename - name of file to create the path for
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::MakePath(LPSTR path,LPSTR filename, int nBufferSize) 
{

    int loop;
    int len = (int)strlen(filename);

    // Do not allow .. in the path - for security purposes
    for(loop = 0; loop < len; loop++) 
	{
        if(filename[loop] == '.')
            if(filename[loop+1] == '.')
                return FALSE;
    }
	// trim away leading spaces
	loop = 0;
	while (filename[loop] == ' ')
		loop++;
	
	// Check to see if the adjustPath starts with an '\'
    if(filename[loop]=='\\') 
	{
        // Setup the temp path
        strncpy(path,m_szRootPath,nBufferSize);
        strncat(path,&filename[loop],nBufferSize - strlen(m_szRootPath));
    }
    else 
	{ 
        if(path[strlen(path)-1] != '\\')
            strncat(path,"\\",nBufferSize - strlen(path));
        strncat(path,&filename[loop],nBufferSize - strlen(path));
    }

    return TRUE;
}

/***************************************************
OnCheckPassword - Overridable
    Checks the given user name and password to see
    if the client is allowed onto the FTP server
Params
    user - user name string
    pass - password string
Return
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::OnCheckPassword(LPSTR user,LPSTR pass)
{

    int rt = FALSE;

    // Check the user name
    _strupr(user);
    if(strcmp(user,"ANONYMOUS") ==0)
        rt = TRUE;
    if(strcmp(user,"GUEST") == 0 )
        rt = TRUE;

    if(rt == FALSE)
        return FALSE;

    // Check the password  - make sure something is entered
    _strupr(pass);
    if(strlen(pass) < 1)
        return FALSE;

    strncpy(m_szUser,user,MAX_PATH);
    m_szUser[MAX_PATH] = '\0';

    return TRUE;
}

/***************************************************
OnCommandStart - Overridable
    notification that a command that was just sent 
    You can reject the command by simply returning 
    FALSE from this notification - which means you
    can usurp default implementation.
Params
    command - the command that was just sent
    data    - the complete command line
Return
    TRUE - to allow the command
    FALSE - to deny the command
****************************************************/
int CUT_FTPThread::OnCommandStart(CommandID command,LPCSTR data){
    char buf[WSS_BUFFER_SIZE + 1];

	
	// check that the user did not cause a buffer over flow by his  command
	if (data == NULL)
	{
		_snprintf(buf,sizeof(buf)-1,"Client [%s] issued invalid command",GetClientAddress());
		((CUT_FTPServer *)m_winsockclass_this)->OnStatus(buf);
		return FALSE;
	}
	if (strlen (data) > WSS_BUFFER_SIZE - 40  )
	{
		_snprintf(buf,sizeof(buf)-1,"Client [%s] is Sent too large data",GetClientAddress());
		((CUT_FTPServer *)m_winsockclass_this)->OnStatus(buf);
		return FALSE;
	}
    _snprintf(buf,sizeof(buf)-1,"[%s] issued command %d ' %s ' ",m_szDataIPAddress, (int)command, data);
    ((CUT_FTPServer *)m_winsockclass_this)->OnStatus(buf);
    return TRUE;
}

/***************************************************
OnCommandFinished - Overridable
    notification that a command just completed 
Params
    command - the command that was just sent
    data    - the complete command line
    success - TRUE if the command completed succfully
              FALSE if the command failed
Return
    none
****************************************************/
void CUT_FTPThread::OnCommandFinished(CommandID command,LPCSTR data,int success){
    char buf[WSS_BUFFER_SIZE + 1];
	
	// check for potential buffer overflow atack
	if (data == NULL)
	{
		_snprintf(buf,sizeof(buf)-1,"Client [%s] issued invalid command",GetClientAddress());
		((CUT_FTPServer *)m_winsockclass_this)->OnStatus(buf);
		return ;
	}
	if (strlen (data) > WSS_BUFFER_SIZE - 40  )
	{
		_snprintf(buf,sizeof(buf)-1,"Client [%s] sent too much data",GetClientAddress());
		((CUT_FTPServer *)m_winsockclass_this)->OnStatus(buf);
		return ;
	}

    if(success)
        _snprintf(buf,sizeof(buf)-1,"[%s] command %d ' %s ' was successful ",m_szUser, (int)command, data);
    else
        _snprintf(buf,sizeof(buf)-1,"[%s] command %d ' %s ' has failed",m_szUser, (int)command, data);
	((CUT_FTPServer *)m_winsockclass_this)->OnStatus(buf); 
}

/***************************************************
OnConnectNotify - Overridable
    notification that a connection was just started
Params
    ipAddress - IP address of the client that just 
                connected
Return
    TRUE - to allow the connect
    FALSE - to deny the connect
****************************************************/
int CUT_FTPThread::OnConnectNotify(LPCSTR /* ipAddress */){
  	char szClientconnectMsg[MAX_PATH];
	_snprintf(szClientconnectMsg,sizeof(szClientconnectMsg )-1,"Client [%s] has connected ", GetClientAddress());
	((CUT_FTPServer *)m_winsockclass_this)->OnStatus(szClientconnectMsg);
  return TRUE;
}

/***************************************************
OnDisconnect - Overridable
    notification that a client is disconnecting
Params
    none
Return 
    none
****************************************************/
void CUT_FTPThread::OnDisconnect(){
	char szClientDisconnectMsg[MAX_PATH];
	if (m_winsockclass_this != NULL)
	{
		_snprintf(szClientDisconnectMsg,sizeof(szClientDisconnectMsg)-1,"Client [%s] has disconnected ", GetClientAddress());
		((CUT_FTPServer *)m_winsockclass_this)->OnStatus(szClientDisconnectMsg);
	}
}

/***************************************************
SetRootPath
    Sets the root path for all clients
PARAM
    LPCSTR rootPath - path 
****************************************************/
int CUT_FTPThread::SetRootPath(LPCSTR rootPath){

    if(rootPath == NULL)
        return FALSE;

    if(strlen(rootPath) >= (MAX_PATH-1))
        return FALSE;

    strcpy(m_szRootPath,rootPath);

    // Make sure that the rootpath ends with a back slash
    int len = (int)strlen(m_szRootPath);
    if(m_szRootPath[len-1] != '\\')
        strcat(m_szRootPath,"\\");

    return TRUE;
}

/******************************************
Virtual OnSetUserPath() - Overridable

  Override this function to have a personal path 
  for each user upon connection. 

  You may want to set the path for each user depending on his/her
  user name or ip address. 
PARAM
    LPSTR userPath - the user's path if needed to be modified
RETURN
    void 
*******************************************/
void CUT_FTPThread::OnSetUserPath(LPSTR /* userPath */ )
{

}

/******************************************
OnWelcome
    Override this function to implement your 
    own server login message
PARAM
    none
RETURN
    void 
*******************************************/
void CUT_FTPThread::OnWelcome()
{
    Send("230-==========================\r\n");
    Send("Welcome to my server\r\n");
    
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
BOOL CUT_FTPThread::OnNonSecureConnection(LPCSTR IpAddress){

   char buf[WSS_BUFFER_SIZE + 1];
   _snprintf(buf,sizeof(buf)-1,"A client [%s]Attempted to connect non-secure. Rejected ", IpAddress);
   ((CUT_FTPServer *)m_winsockclass_this)->OnStatus(buf);
   return FALSE;
}

#endif

/******************************************
SetDataPortProtection 
	This function was added to prevent 
	The Bounce Attack (outlined  by RFC 2577)
PARAM 
	
	enable - When the parameter is TRUE port command received from the client 
				will not be premeted when port number are less than 1024.
				the port command must use ports greater than the well known 
				port  numbers.
REMARK
	See Also RFC 2577.

******************************************/
void CUT_FTPServer::SetDataPortProtection(BOOL enable)
{
	m_bDataPortProtection = enable;

}
/***************************************************
SetDataPortPASV
    Sets the port and address to use as the data channel to send information 
	over to the client.
	Prior to doing so we must make sure that the client did not send an
	other PASV command without closing it. As that may lead to DOS attack

PARAM
    param - contains the IP address and port in a string where each number is
			separated by a comma
			eg. '123,134,145,178,2,34'
            address = 123.134.145.178
            port = 2 * 256 + 34 = 546  (hibyte,lobyte)
RET
    TRUE  - successful
    FALSE - failed
****************************************************/
int CUT_FTPThread::SetDataPortPASV()
{

	if (m_wscDataConnection)
		delete m_wscDataConnection;

	m_wscDataConnection= new CUT_FTPThreadData;
	m_wscDataConnection->m_cutParent = this;

	// Set security properties
	 
	// If the client enabled security and wants to allow non secure connection
	// m_bAllowNonSecure
#ifdef CUT_SECURE_SOCKET
		
		SetSecurityAttrib(m_winsockclass_this, m_wscDataConnection);
		  if (!m_bHandshakeDone  )
			  m_wscDataConnection->SetSecurityEnabled (FALSE);
#endif

	  int     count, loop;
	  int     len;// = strlen(param);
	  char    buf[30];
	  BYTE    hibyte,lobyte;


	  if ( m_wscDataConnection->WaitForConnect(0) != UTE_SUCCESS )
	  {
		return FALSE;
	  }

	  // get the accept port (generated by WaitForConnect)
	  m_nDataPort = m_wscDataConnection->m_nDataPort;

	  hibyte = HIBYTE(m_nDataPort);
	  lobyte = LOBYTE(m_nDataPort);
	  // generate the string to PASV string to return to the client
	  // copy the string over

	  
	  if ( strcmp(inet_ntoa ( ( (CUT_FTPServer *) m_winsockclass_this )->m_serverSockAddr.sin_addr) ,"0.0.0.0") == 0 )
	  {
		  // since the name of the server is not specified then we will connect to the 
		  // first one available on the host name adderss
		  CUT_WSClient ws;
		  ws.GetHostAddress( buf, 29 ,FALSE);
	  }
	  else
		  strcpy(buf,inet_ntoa(((CUT_FTPServer *)m_winsockclass_this)->m_serverSockAddr.sin_addr));



	  len = (int)strlen(buf);
  
	  // get the IP address
	  count = 0;
	  for( loop = 0; loop < len; loop++ )
	  {
		if( buf[loop] == '.' )
		{
		  buf[loop] = ',';
		  count++;
		}
	  }

	  strcpy( m_szDataPASVAddress, buf);

	  char hb[10];
	  char lb[10];

	  _itoa(hibyte,hb,10);
	  _itoa(lobyte,lb,10);

	  strcat(m_szDataPASVAddress,",");
	  strcat(m_szDataPASVAddress,hb);
	  strcat(m_szDataPASVAddress,",");
	  strcat(m_szDataPASVAddress,lb);

	  // if the programmer has allowed bounce attacks then it
	  // is fine otherwise make sure the number is greater than 1024
	  CUT_FTPServer *fs = (CUT_FTPServer*) m_winsockclass_this;
	  if (hibyte >= 4 || !fs->m_bDataPortProtection)
	  {
		m_nDataPort = MAKEWORD(lobyte,hibyte);
		m_nHiByte = hibyte;
		m_nLoByte = lobyte;
		return TRUE;
	  }
//#endif

	  return FALSE;
}
/***************************************************
CreateDataConnection
    Returns a pointer to a client thread that
    according to whether or not the client is using
    passive ftp mode.  If so, then the server waits
    for the client to connect to the port that was
    specified in the response to the PASV command.
    Othwerwise, regular PORT ftp mode is used and
    the server connect to the client specified port.
Params
    None
Return
    Pointer to data connection or NULL
****************************************************/
int CUT_FTPThread::CreateDataConnection()
{


	  // check if we are in passive mode
	  // if so then we will use the socket created in 
	  // SetDataPortPASV
	  if ( m_bPASV )
	  {
		// wait for the incomming connection
		if ( m_wscDataConnection->WaitForAccept(GetReceiveTimeOut()) != CUT_SUCCESS )
		{
			//close the connection down
			m_wscDataConnection->ReleaseDataConnection();
			m_bPASV = FALSE;
			return UTE_ERROR;
		}
		// ok go ahead with accepting the connection
		//accept a connection on the data port
		if ( m_wscDataConnection->AcceptConnection() != CUT_SUCCESS )
		{
			//close the connection down
			m_wscDataConnection->ReleaseDataConnection();
			m_bPASV = FALSE;
			return UTE_ERROR;
		}	
	  }
	  else
	  {

		  // ok so we are not in passive mode 
		  // then we need to create a new connection
		  if (m_wscDataConnection)
			  delete m_wscDataConnection;
		  m_wscDataConnection = new CUT_FTPThreadData();
		  m_wscDataConnection->m_cutParent = this; 

		  // if we are secure then let's copy the parents connection state
#ifdef CUT_SECURE_SOCKET
		  if ( GetSecurityEnabled() )//
		  {
			  SetSecurityAttrib(m_winsockclass_this, m_wscDataConnection);
		  }
#endif
		  // ok se we need to connect to the remote client 
		  if( m_wscDataConnection->Connect(m_nDataPort, m_szDataIPAddress) != CUT_SUCCESS)
		  {
			  m_wscDataConnection->ReleaseDataConnection();
			  return UTE_ERROR;
		  }
	  }

	  // good
	  return UTE_SUCCESS;
}


//=================================================================
//  class:class CUT_FTPThreadData 
// Added to allow for PASSIVE mode for Secure and none secure versions
//=================================================================

CUT_FTPThreadData::CUT_FTPThreadData()
{
	
		m_serverSocket = INVALID_SOCKET;
}
// detor
CUT_FTPThreadData::~CUT_FTPThreadData()
{
	// make sure we close connections
	ReleaseDataConnection();
}

/***************************************************
m_wscDataConnection->ReleaseDataConnection
		Release the memory allocated for the data
		connection
PARAM
		none
RET
		None
****************************************************/
void CUT_FTPThreadData::ReleaseDataConnection()
{
	// reallocate resources for the data connection
	int rt =0; // debugging;

	CloseConnection ();
	SocketClose(m_serverSocket);
	if(m_serverSocket != INVALID_SOCKET)
	{
		if(m_nSockType == SOCK_STREAM)
		{
			if(SocketShutDown(m_serverSocket, 2) == SOCKET_ERROR)
			{
				rt = UTE_ERROR;	//1 
			}
		}
		if(SocketClose(m_serverSocket) == SOCKET_ERROR)
		{
			rt = UTE_ERROR;		//2
		}
	}
	m_serverSocket        = INVALID_SOCKET;
	
}
/*****************************************************
AcceptConnection
    This function is used to accept a connection
    once a socket is set up for listening see:
    WaitForConnect
    To see if a connection is waiting call:
    WaitForAccept
PARAM	
    none
RET
    UTE_ERROR	- error
	UTE_SUCCESS	- success
******************************************************/
int CUT_FTPThreadData::AcceptConnection()
{

    int addrLen = sizeof(SOCKADDR_IN);


    // close the socket if open - may be blocking though
    if(m_clientSocket != INVALID_SOCKET)
	{
        if(m_nSockType == SOCK_STREAM)
		{
            if(SocketShutDown(m_clientSocket, 2) == SOCKET_ERROR)
			{
                return OnError(UTE_ERROR);	
            }
        }
        if(SocketClose(m_clientSocket) == SOCKET_ERROR)
		{
            return OnError(UTE_ERROR);		
        }
    }
 

    // accept the connection request when one is received
    m_clientSocket = accept(m_serverSocket,(LPSOCKADDR)&m_clientSocketAddr,&addrLen);
    if(m_clientSocket == INVALID_SOCKET)
        return OnError(UTE_ERROR);

    strcpy(m_szAddress, inet_ntoa(m_clientSocketAddr.sin_addr));
    
    // save the local port
    SOCKADDR_IN sa;
    int len = sizeof(SOCKADDR_IN);
    getsockname(m_clientSocket, (SOCKADDR*) &sa, &len);
    m_nDataPort = ntohs(sa.sin_port);

	// v4.2 Was using WC() macro here, but decided against wchar in notifications
	return CUT_WSThread::SocketOnConnected(m_clientSocket, m_szAddress);
}
/***************************************************
WaitForAccept
    Waits up til the specified time to see if there
    is an incomming connection waiting with a read set. If this 
    function returns UTE_SUCCESS then call 
    AcceptConnection to connect.
PARAM
    secs - the max. number of seconds to wait
RET
    UTE_ERROR	- error
    UTE_SUCCESS - success
****************************************************/
int CUT_FTPThreadData::WaitForAccept(long secs)
{
    
	fd_set readSet;
	struct timeval tv;

	tv.tv_sec = secs;
	tv.tv_usec = 0;

	FD_ZERO(&readSet);

	// W level 4 
	// warning C4127: conditional expression is constant (in FD_SET macro)
#pragma warning (disable : 4127 )
	FD_SET(m_serverSocket,&readSet);
#pragma warning (default : 4127 )

	//wait up to the specified time to see if data is avail
	if( select(-1,&readSet,NULL,NULL,&tv)!= 1)
	{
		return OnError(UTE_ERROR);
	}

	return OnError(UTE_SUCCESS);
}
/*************************************************
WaitForConnect
    Creates a socket and binds it to the given port
    and address. Then puts the socket in a listening state
Params
    port		- port to listen on.  If 0, winsock
                    will assign chose a port between 
                    1024 and 5000.  Call GetAcceptPort
                    to retrieve this port number.
    [queSize]	- que size for incomming connections
    [family]	- protocol family: AF_INET, AF_AF_IPX, etc.
    [address]	- address to listen on
Return
    UTE_SOCK_ALREADY_OPEN	- socket already open or in use
    UTE_SOCK_CREATE_FAILED	- socket creation failed
    UTE_SOCK_BIND_FAILED	- socket binding to local port failed
    UTE_SOCK_LISTEN_ERROR	- listen failed
    UTE_SUCCESS				- success
**************************************************/
int CUT_FTPThreadData::WaitForConnect(unsigned short port,int queSize,short family,
        unsigned long address) 
{

    if(m_serverSocket != INVALID_SOCKET)
        return OnError(UTE_SOCK_ALREADY_OPEN);
     SOCKADDR_IN		m_serverSockAddr;		// Server socket address
    //Set up the serverSockAddr structure
    memset(&m_serverSockAddr, 0, sizeof(m_serverSockAddr)); //clear all
    m_serverSockAddr.sin_port           = htons(port);      //port
    m_serverSockAddr.sin_family         = family;           //Internet family
    m_serverSockAddr.sin_addr.s_addr    = address;          //address (any)

    //create a socket
	CUT_WSClient clnt;
    if(clnt.CreateSocket(m_serverSocket, family, SOCK_STREAM, 0) == UTE_ERROR)
	{
        return OnError(UTE_SOCK_CREATE_FAILED);   // ERROR: socket unsuccessful 
    }

    //associate the socket with the address and port
    if(bind(m_serverSocket,(LPSOCKADDR)&m_serverSockAddr,sizeof(m_serverSockAddr))
     == SOCKET_ERROR)
	{
        return OnError(UTE_SOCK_BIND_FAILED);  // ERROR: bind unsuccessful 
    }
    //allow the socket to take connections
    if( listen(m_serverSocket, queSize ) == SOCKET_ERROR)
	{
        return OnError(UTE_SOCK_LISTEN_ERROR);   // ERROR: listen unsuccessful 
    }
    
    // save the port number for GetAcceptPort
    SOCKADDR_IN sa;
    int len = sizeof(SOCKADDR_IN);
    getsockname(m_serverSocket, (SOCKADDR*) &sa, &len);
    m_nDataPort = ntohs(sa.sin_port);

    return OnError(UTE_SUCCESS);
}
/***************************************************
ReceiveFileStatus
    This virtual function is called during a 
    ReceiveToFile function.
Params
    bytesReceived - number of bytes received so far
Return
    TRUE - allow the receive to continue
    FALSE - abort the receive
****************************************************/
int CUT_FTPThreadData::ReceiveFileStatus(long /* BytesReceived */)
{
	// right here lets look at what the other side is  telling the 
	// control connection
	// if we are being told to abort 
	// then just do that
	// and return FALSE 
	// other wise 
	// continue
	
	 if (m_cutParent->IsDataWaiting ())
	 {
		 char szData[MAX_PATH];
		 m_cutParent->ReceiveLine(szData,sizeof(szData),1);
		 CUT_StrMethods::RemoveCRLF (szData);
		 CUT_StrMethods::RemoveSpaces (szData);

		 if ( _stricmp(szData,"ABORT") == 0 || _stricmp(szData,"ABOR") == 0)
		 {
			 return FALSE;
		 }
	 }
    return TRUE;
}
/***************************************************
SendFileStatus
    This virtual function is called during a 
    SendFile function.
Params
    bytesSent - number of bytes sent so far
Return
    TRUE - allow the send to continue
    FALSE - abort the send
****************************************************/
int CUT_FTPThreadData::SendFileStatus(long /* BytesSent */)
{
	// right here lets look at what the other side is  telling the 
	// control connection
	// if we are being told to abort 
	// then just do that
	// and return FALSE 
	// other wise 
	// continue
	
	 if (m_cutParent->IsDataWaiting ())
	 {
		 char szData[MAX_PATH];
		 m_cutParent->ReceiveLine(szData,sizeof(szData),1);
		 CUT_StrMethods::RemoveCRLF (szData);
		 CUT_StrMethods::RemoveSpaces (szData);
		 if ( _stricmp(szData,"ABORT") == 0 || _stricmp(szData,"ABOR") == 0)
		 {
			 return FALSE;
		 }
	 }
    return TRUE;

}

#pragma warning ( pop )
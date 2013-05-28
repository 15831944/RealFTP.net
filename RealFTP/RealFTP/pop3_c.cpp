// =================================================================
//  class: CUT_POP3Client
//  File:  pop3_c.cpp
//
//  Purpose:
//
//  POP3 Client Class 
//  Implementation of Post Office version 3 Protocol
//
//  RFC  1939
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

#ifdef _WINSOCK_2_0_
    #define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
                            /* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include "pop3_c.h"

#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)


/********************************
*********************************/
CUT_POP3Client::CUT_POP3Client() : 
        m_nPort(110),               // Set default connection port to 110
        m_nConnectTimeout(5),       // Connection time out. Default 5 sec.
        m_nPOP3TimeOut(10),         // POP3 time out. Default 10 sec.
        m_bMsgOpen(FALSE),          // no mesage open for reading 
        m_bTopOpen(FALSE),          // No message open for TOP command 
        m_bReadMsgFinished(FALSE),
        m_bReadTopFinished(FALSE)
{
}
/********************************
*********************************/
CUT_POP3Client::~CUT_POP3Client(){
}
/********************************
POP3Connect()
    Connect to port 110 on a POP3 server informing it of the 
    user name and password. 
PARAM:
    mailHost    - server ip address or name.
    user        - user account id
    password    - account password
RETURN:
    UTE_SOCK_ALREADY_OPEN   - socket already open or in use
    UTE_SOCK_CREATE_FAILED  - socket creation failed
    UTE_SOCK_CONNECT_FAILED - socket connection failed
    UTE_INVALID_ADDRESS     - invalid address
    UTE_SUCCESS             - success
    UTE_CONNECT_TIMEOUT     - connect time out
    UTE_USER_FAILED         - user command has timed out or rejected 
    UTE_PASS_FAILED         - passwrod command Timed out or rejected
    UTE_ABORTED             - aborted
*********************************/
#if defined _UNICODE 
int CUT_POP3Client::POP3Connect(LPCWSTR mailHost, LPCWSTR user, LPCWSTR password) {
	return POP3Connect( AC(mailHost), AC(user), AC(password));}
#endif 
int CUT_POP3Client::POP3Connect(LPCSTR mailHost, LPCSTR user, LPCSTR password) {

    int     error;

    //close any open connection
    POP3Close();

    //connect
    if((error = Connect(m_nPort, mailHost, m_nConnectTimeout)) != UTE_SUCCESS) 
        return OnError(error);
    
    //get the response code
	if(GetResponseCode(m_nPOP3TimeOut) == FALSE)
		return OnError(UTE_CONNECT_TIMEOUT);

    //send the user name
    Send("USER ");
    Send(user);
    Send("\r\n");

    if(GetResponseCode(m_nPOP3TimeOut) == FALSE)
        return OnError(UTE_USER_FAILED);

    if(IsAborted())         // Test for abortion flag
        return OnError(UTE_ABORTED);

    //send the password
    Send("PASS ");
    Send(password);
    Send("\r\n");

    if(GetResponseCode(m_nPOP3TimeOut) == FALSE)
        return OnError(UTE_PASS_FAILED);


    return OnError(UTE_SUCCESS);
}
/********************************
POP3Close() 
    Close connection to the pop3 server by
    issuing a QUIT command
PARAM:
    NONE
RETURN:
    UTE_SUCCESS     - success
    UTE_ERROR       - failed
*********************************/
int CUT_POP3Client::POP3Close(){

    Send("Quit\r\n");

    return CloseConnection();
}

/********************************************************************
GetNumMsgs
    The objective of this function is to retrieve the number 
    of messages stored in the server for the user maildrop. 

NOTE: Server should not include any messages marked for deletion in the response

PARAM:
    int *number - pionter to the number of available messages
RETURN:
    UTE_NOCONNECTION        - NOT connected
    UTE_SVR_NO_RESPONSE     - no response from Server
    UTE_STAT_FAILED         - STAT command was not reponded to positivly
    UTE_INVALID_RESPONSE    - Malformed response
*********************************************************************/
int CUT_POP3Client::GetNumMsgs(int *number){

    char param[MAX_PATH+1];

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);

    //send the STAT command
    Send("STAT\r\n");

    //get the response
    if(ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut) <= 0)
        return OnError(UTE_SVR_NO_RESPONSE);

    if(m_szBuf[0] != '+')
        return OnError(UTE_STAT_FAILED);

    CUT_StrMethods::RemoveCRLF(m_szBuf);

    if(CUT_StrMethods::ParseString(m_szBuf," ",1,param,sizeof(param)) != UTE_SUCCESS)
        return OnError(UTE_INVALID_RESPONSE);

    *number = atoi(param);

    return OnError(UTE_SUCCESS);
}

/********************************
GetMsgSize()
    Retrieve the size of the message specified by the index.
    When this function is invoked on a valid connection and the message 
    specified by the Index is not marked for deletion, The POP3 server 
    issues a positive response with a line containing 
    information for the message.
PARAM
    int index - the message number 
    long *size - pointer to the message size
RETURN
    UTE_NOCONNECTION        - NOT connected
    UTE_SVR_NO_RESPONSE     - no response from Server
    UTE_LIST_FAILED         - LIST command was not reponded to positivly
    UTE_INVALID_RESPONSE    - Malformed response
*********************************/
int CUT_POP3Client::GetMsgSize(int index,long *size){

    char param[MAX_PATH+1];

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);

    //send the LIST command
    _snprintf(m_szBuf,sizeof(m_szBuf)-1,"LIST %d\r\n",index);
    Send(m_szBuf);

    //get the response
    if(ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut) <= 0)
        return OnError(UTE_SVR_NO_RESPONSE);

    if(m_szBuf[0] != '+')
        return OnError(UTE_LIST_FAILED);

    CUT_StrMethods::RemoveCRLF(m_szBuf);
    
    // a server response to a LIST command that includes an argument
    // should have +OK[SPACE]messageNumber[SPACE]size
    // Hence we will be looking for the third octet of the server response
    if(CUT_StrMethods::ParseString(m_szBuf," ()",2,param,sizeof(param)) != UTE_SUCCESS)
        return OnError(UTE_INVALID_RESPONSE);

    *size = atol(param);

    return OnError(UTE_SUCCESS);
}

/********************************
SaveMsg()
    Retrives (RETR) and Saves the specified message to disk storage
NOTE
    Message index is 1 based NOT zero based
PARAM   
    index       - Index of the message to be saved
    fileName    - name and path of the destination file to save the message to
RETURN
    UTE_SUCCESS         - Success
    UTE_NOCONNECTION    - NOT connected
    UTE_SVR_NO_RESPONSE - no response from Server
    UTE_FILE_OPEN_ERROR - Not able to open file for writing
    UTE_RETR_FAILED     - RETR command has timed out without a response from the server
    UTE_SOCK_TIMEOUT    - Timed out while receiving parts of the message
    UTE_ABORTED         - User canceled the Save to file process
*********************************/
int CUT_POP3Client::SaveMsg(int index, LPCTSTR fileName) {
    CUT_FileDataSource  dsFile(fileName);
    return SaveMsg(index, dsFile);
}

/********************************
SaveMsg()
    Retrives (RETR) and Saves the specified message to 
    the data source
NOTE
    Message index is 1 based NOT zero based
PARAM   
    index       - Index of the message to be saved
    dest        - destination data source
RETURN
    UTE_SUCCESS         - Success
    UTE_NOCONNECTION    - NOT connected
    UTE_SVR_NO_RESPONSE - no response from Server
    UTE_FILE_OPEN_ERROR - Not able to open file for writing
    UTE_RETR_FAILED     - RETR command has timed out without a response from the server
    UTE_SOCK_TIMEOUT    - Timed out while receiving parts of the message
    UTE_ABORTED         - User canceled the Save to file process
*********************************/
int CUT_POP3Client::SaveMsg(int index, CUT_DataSource & dest) {

    int         error = UTE_SUCCESS;
    int         len;
    UINT        bytesReceived = 0;

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);

    //open the destination file
    if(dest.Open(UTM_OM_WRITING) == -1)
        return OnError(UTE_FILE_OPEN_ERROR);

    //send the RETR command
    _snprintf(m_szBuf,sizeof(m_szBuf)-1,"RETR %d\r\n",index);
    Send(m_szBuf);
    

    //get the response
    if(ReceiveLine(m_szBuf,sizeof(m_szBuf)-1, m_nPOP3TimeOut) <= 0)
        return OnError(UTE_SVR_NO_RESPONSE);

    if(m_szBuf[0] != '+')
        return OnError(UTE_RETR_FAILED);

    //read in and save the message
	// v4.2 change to eliminate C4127: conditional expression is constant
    for(;;) {
        if(IsAborted()) {       // Test for abortion flag
            error = UTE_ABORTED;
            break;
            }
    
        //read in a line
        len = ReceiveLine(m_szBuf,sizeof(m_szBuf)-1,m_nPOP3TimeOut);


        if( len  <= 0 ) {
            error = UTE_SOCK_TIMEOUT;
            break;
            }

        bytesReceived += len;

        if (OnSaveMsg(bytesReceived) != TRUE) {
            error = UTE_ABORTED;       // User cancel the save message
            break; // call status func
            }

        //if a period is found at the beginning of a line then
        //check to see if it is the end of the message
        if(m_szBuf[0] == '.') {
            //end of message
            if(len == 3)       
                break;
            
            //save line
            else
                dest.Write(&m_szBuf[1], len-1);
            }
        else
            dest.Write(m_szBuf, len);
    }

    // Close data source
    dest.Close();

    return OnError(error);
}

/********************************
OpenMsg(int index)
    To allow us read part of the message without having to download it all
    we will issue a RETR command and receive only the response from the server
    If the server response is positive then we will flag the MessageOpen.
    By doing so the message will be available to us on the receive buffer where we can 
    either read the whole message or receive it by line by line
NOTE 
    This function must be called prior to a call to ReadMsgLine(). 
    A message can't be opened if another message is already open. The other message should be closed first 
PARAM
    index   - The index of the message to be opened ( 1 based ) 
RETURN
    UTE_SUCCESS         - Success
    UTE_NOCONNECTION    - NOT connected
    UTE_SVR_NO_RESPONSE - no response from Server
    UTE_RETR_FAILED     - RETR command has timed out without a response from the server
*********************************/
int CUT_POP3Client::OpenMsg(int index){

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);

    //send the RETR command
    _snprintf(m_szBuf,sizeof(m_szBuf)-1,"RETR %d\r\n",index);
    Send(m_szBuf);

    //get the response
    if(ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut) <= 0)
        return OnError(UTE_SVR_NO_RESPONSE);

    if(m_szBuf[0] != '+')
        return OnError(UTE_RETR_FAILED);

    m_bMsgOpen = TRUE;
    m_bReadMsgFinished = FALSE;

    return OnError(UTE_SUCCESS);
}

/********************************
ReadMsgLine
    Read a single line of the Already open message
    A call to OpenMsg() should be made prior to calling this function
PARAM
    buf     - A string buffer to hold the received line
    bufLen  - Maximum length of the buffer
RETURN
    Number of bytes read
    -1 No message open 
*********************************/
#if defined _UNICODE
int CUT_POP3Client::ReadMsgLine(LPWSTR buf,int bufLen){
	char * bufA = (char*) alloca(bufLen+1);
	*bufA = '\0';
	int result = ReadMsgLine(bufA, bufLen);
	if (result != -1) {
		CUT_Str::cvtcpy(buf, bufLen, bufA);
	}
	return result;}
#endif
int CUT_POP3Client::ReadMsgLine(LPSTR buf,int bufLen){

    if(m_bMsgOpen == FALSE) 
        return -1;

    int     rt = ReceiveLine(buf,bufLen);

    if(buf[0] == '.' && rt <= 3) {
        m_bReadMsgFinished = TRUE;
        return 0;
        }

    return rt;
}

/********************************
CloseMsg()
    Close the currently open message by reading all
    availabe lines of the messages if any.
PARAM
    NONE
RETURN
    UTE_SUCCESS         - Success
    UTE_NOCONNECTION    - NOT connected
*********************************/
int CUT_POP3Client::CloseMsg(){

    int     error = UTE_SUCCESS;

    if(m_bMsgOpen == FALSE)
        return OnError(UTE_SUCCESS);

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);
        
    int len;

    while(!m_bReadMsgFinished) {

        if(IsAborted()) {       // Test for abortion flag
            error = UTE_ABORTED;
            break;
            }

        //read in a line
        len = ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut);
        
        if( len <= 0 ) 
            break;

        if(m_szBuf[0] == '.' && len <= 3)
            break;
        }

    m_bMsgOpen          = FALSE;
    m_bReadMsgFinished  = FALSE;

    return OnError(error);
}

/********************************
OpenTop(int index)
    To allow us to read the top part of the message without having to download it all.
    We will issue a TOP command and receive only the response from the server
    If the server response is positive then we will flag the TopOpen.
    By doing so the message will be available to us on the receive buffer where we can 
    either read the whole message Top or receive it by line by line.
NOTE 
    This function must be called prior to a call to ReadTopLine(). 
    A message for TOP can't be opened if another message is already open for TOP.
    The other message should be closed first 
PARAM
    index - The index of the message to be opened ( 1 based ) 
RETURN
    UTE_SUCCESS         - Success
    UTE_NOCONNECTION    - NOT connected
    UTE_SVR_NO_RESPONSE - no response from Server
    UTE_TOP_FAILED      - TOP command has timed out without a response from the server
*********************************/
int CUT_POP3Client::OpenTop(int index,int msgLines){

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);

    //send the TOP command
    _snprintf(m_szBuf,sizeof(m_szBuf)-1,"TOP %d %d\r\n",index,msgLines);
    Send(m_szBuf);

    //get the response
    if(ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut) <= 0)
        return OnError(UTE_SVR_NO_RESPONSE);

    if(m_szBuf[0] != '+')  // '-' response probably flags 'not implemented'.
        return OnError(UTE_TOP_FAILED);        // TOP was/is an optional POP3 option 

    m_bTopOpen = TRUE;
    m_bReadTopFinished = FALSE;

    return OnError(UTE_SUCCESS);
}

/********************************
RetrieveUID
    Retrieves the unique-id of one or all messages. You can use
    the GetUID(...) method to get the unique-id string.

    "The unique-id of a message is an arbitrary server-determined
    string, consisting of one to 70 characters in the range 0x21
    to 0x7E, which uniquely identifies a message within a
    maildrop and which persists across sessions.  This
    persistence is required even if a session ends without
    entering the UPDATE state.  The server should never reuse an
    unique-id in a given maildrop, for as long as the entity
    using the unique-id exists.

    Note that messages marked as deleted are not listed." RFC 1939

PARAM
    [msgNumber] - the number of message to get UID for.
                  If set to -1 (default) retieve UIDs for all messages
RETURN
    UTE_SUCCESS         - Success
    UTE_NOCONNECTION    - NOT connected
    UTE_SVR_NO_RESPONSE - no response from Server
    UTE_UIDL_FAILED     - UIDL command has timed out without a response from the server
*********************************/
int CUT_POP3Client::RetrieveUID(int msgNumber)
{
    int error = UTE_SUCCESS;

    // clear the current vector
    m_vecUID.erase(m_vecUID.begin(), m_vecUID.end());

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);

    //send the UIDL command
    if(msgNumber >= 1)
        _snprintf(m_szBuf,sizeof(m_szBuf)-1,"UIDL %d\r\n", msgNumber);
    else 
        _snprintf(m_szBuf,sizeof(m_szBuf)-1,"UIDL\r\n");
    Send(m_szBuf);

    //get the response
    if(ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut) <= 0)
        return OnError(UTE_SVR_NO_RESPONSE);

    if(m_szBuf[0] != '+')  // '-' response probably flags 'not implemented'.
        return OnError(UTE_UIDL_FAILED);        // UIDL was/is an optional POP3 option 

    // If we are getting only one UID - get it from the response
    char        szBuffer[MAX_UID_STRING_LENGTH + 1];
    MessageUID  messageID;
    if(msgNumber >= 1) {
        if(CUT_StrMethods::ParseString(m_szBuf," \r\n", 2, szBuffer, MAX_UID_STRING_LENGTH) != UTE_SUCCESS)
            return OnError(UTE_INVALID_RESPONSE);

        // Initialize UID structure
        messageID.m_nMessageNumber = msgNumber;
        messageID.m_strUID = szBuffer;

        // Add UID structure to the vector
        m_vecUID.push_back(messageID);
        }

    // We should get the multiline response with UIDs
    else {
        int len;
        while(!m_bReadTopFinished) {
            if(IsAborted()) {       // Test for abortion flag
                error = UTE_ABORTED;
                break;
                }

            // read in a line
            len = ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut);
            if( len  <= 0 ) {
                error = UTE_SOCK_TIMEOUT;
                break;
                }

            // end of multiline response
            if(m_szBuf[0] == '.' && len <= 3)
                break;

            // Get message number
            if(CUT_StrMethods::ParseString(m_szBuf," \r\n", 0, &messageID.m_nMessageNumber) != UTE_SUCCESS) {
                error = UTE_INVALID_RESPONSE;
                break;
                }

            // Get message UID
            if(CUT_StrMethods::ParseString(m_szBuf," \r\n", 1, szBuffer, MAX_UID_STRING_LENGTH) != UTE_SUCCESS) {
                error = UTE_INVALID_RESPONSE;
                break;
                }

            // Initialize UID structure
            messageID.m_strUID = szBuffer;

            // Add UID structure to the vector
            m_vecUID.push_back(messageID);
            }
        }

    return OnError(error);
}

/********************************
GetUID
    Returns the message unique-id. You MUST retrieve these
    UID first using RetrieveUID(...) method
PARAM
    msgNumber - the number of message to get UID string for.
RETURN
    NULL    - no UID string was retrieved for this message
    string  - UID string of the message
*********************************/
LPCSTR CUT_POP3Client::GetUID(int msgNumber) 
{
    MESSAGE_UID_VEC::iterator theIterator;
    for (theIterator = m_vecUID.begin(); theIterator != m_vecUID.end(); theIterator++) {
        if(msgNumber == theIterator->m_nMessageNumber)
            return theIterator->m_strUID.c_str();
        }
    return NULL;
}

/*************************************************
GetUID()
Get the UID of the message specified by message number - 
	- call RetrieveUID before callin this function.
PARAM
uid		- [out] pointer to buffer to receive uid
maxSize - length of buffer
msgNumber   - index of the message
size    - [out] length of title

  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- uid and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - uid not found
  UTE_BUFFER_TOO_SHORT  - space in uid buffer indicated by maxSize insufficient, realloc 
  based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
int CUT_POP3Client::GetUID(LPSTR uid, size_t maxSize, int msgNumber, size_t *size) {

	int retval = UTE_SUCCESS;
	
	if(uid == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetUID(msgNumber);
		
		if(str == NULL) {
			retval = UTE_INDEX_OUTOFRANGE;
		}
		else {
			*size = strlen(str);
			if(*size >= maxSize) {
				++(*size);
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				strcpy(uid, str);
			}
		}
	}
	return retval;

}
#if defined _UNICODE
int CUT_POP3Client::GetUID(LPWSTR uid, size_t maxSize, int msgNumber, size_t *size) {

	int retval;
	
	if(maxSize > 0) {
		char * uidA = new char [maxSize];
		
		if(uidA != NULL) {
			retval = GetUID( uidA, maxSize, msgNumber, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(uid, maxSize, uidA);
			}
			
			delete [] uidA;
			
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetUID(msgNumber);
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

/********************************
ReadTopLine()
    Read a single line of the already open message Top.
    A call to OpenTop() should be made prior to calling this function
PARAM
    buf     - A string buffer to hold the received line
    bufLen  - Maximum length of the buffer
RETURN
    Number of bytes read
    -1 No message Top open 
*********************************/
#if defined _UNICODE
int CUT_POP3Client::ReadTopLine(LPWSTR buf,int bufLen){
	char * bufA = (char*) alloca(bufLen+1);
	*bufA = '\0';
	int result = ReadTopLine(bufA, bufLen);
	if (result != -1) {
		CUT_Str::cvtcpy(buf, bufLen, bufA);
	}
	return result;}
#endif
int CUT_POP3Client::ReadTopLine(LPSTR buf,int bufLen){

    if (m_bTopOpen == FALSE) 
        return -1;

    int rt = ReceiveLine(buf,bufLen);
    if(buf[0] == '.' && rt <= 3) {
        m_bReadTopFinished = TRUE;
        return 0;
        }

    return rt;
}

/********************************
CloseTop()
    Close the currently open message by reading all
    availabe lines of the Top response if any.
PARAM
    NONE
RETURN
    UTE_SUCCESS         - Success
    UTE_NOCONNECTION    - NOT connected
*********************************/
int CUT_POP3Client::CloseTop(){
    int     error = UTE_SUCCESS;

    if(m_bTopOpen == FALSE)
        return OnError(UTE_SUCCESS);

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);
    
    int len;

    while(!m_bReadTopFinished) {
        if(IsAborted()) {       // Test for abortion flag
            error = UTE_ABORTED;
            break;
            }

        //read in a line
        len = ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut);
        
        if( len  <= 0 )
            break;
        
        if(m_szBuf[0] == '.' && len <= 3)
            break;
        }

    m_bTopOpen = FALSE;
    m_bReadTopFinished = FALSE;

    return OnError(error);
}

/********************************
DeleteMsg()
  Mark a message for deletion.
  the message will be deleted after the transaction closure
PARAM
    index   - index of the message to be marked for deletion
RETURN
    UTE_SUCCESS         - Success
    UTE_NOCONNECTION    - NOT connected
    UTE_SVR_NO_RESPONSE - no response from Server
    UTE_DELE_FAILED     - DELE command has timed out without a response from the server
*********************************/
int CUT_POP3Client::DeleteMsg(int index){

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);
    
    //send the DELE command
    _snprintf(m_szBuf,sizeof(m_szBuf)-1,"DELE %d\r\n",index);
    Send(m_szBuf);

    //get the response
    if(ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut) <= 0)
        return OnError(UTE_SVR_NO_RESPONSE);

    if(m_szBuf[0] != '+')
        return OnError(UTE_DELE_FAILED);

    return OnError(UTE_SUCCESS);
}

/********************************
ResetDelete()
     If any messages have been marked as deleted 
    by the pop3 server they are unmarked.
PARAM
    NONE
RETURN
    UTE_SUCCESS         - Success
    UTE_NOCONNECTION    - NOT connected
    UTE_SVR_NO_RESPONSE - no response from Server
    UTE_RSET_FAILED     - RSET command has timed out without a response from the server
*********************************/
int CUT_POP3Client::ResetDelete(){

    //check to see if the connection is made
    if(!IsConnected())
        return OnError(UTE_NOCONNECTION);

    //send the DELE command
    Send("RSET\r\n");
    
    //get the response
    if(ReceiveLine(m_szBuf,sizeof(m_szBuf),m_nPOP3TimeOut) <= 0)
        return OnError(UTE_SVR_NO_RESPONSE);

    if(m_szBuf[0] != '+')
        return OnError(UTE_RSET_FAILED);

    return OnError(UTE_SUCCESS);
}

/********************************
    Virtual function designed to be overridden to  
    receive report of progress of save message process.

    To abort the Save Message process return false

PARAM
    bytesRetrieved - Number of bytes received so far
RETURN
    TRUE    - to continue
    FALSE   - to cancel
*********************************/
BOOL CUT_POP3Client::OnSaveMsg(long /*  bytesRetrieved */ ){
    return !IsAborted();
}

/********************************
*********************************/
BOOL CUT_POP3Client::GetResponseCode(int timeOut){

    if(ReceiveLine(m_szBuf,sizeof(m_szBuf), timeOut) <=0)
        return FALSE;

    if(m_szBuf[0] == '+')
        return TRUE;
    
    return FALSE;
}
/*********************************************
SetPort
    Sets the port number to connect to
Params
    newPort     - connect port
Return
    UTE_SUCCESS - success
**********************************************/
int CUT_POP3Client::SetPort(unsigned int newPort) {
    m_nPort = newPort;
    return OnError(UTE_SUCCESS);
}
/*********************************************
GetPort
    Sets the port number to connect to
Params
    none
Return
    connect port
**********************************************/
unsigned int  CUT_POP3Client::GetPort() const
{
    return m_nPort;
}
/*********************************************
SetConnectTimeout
    Sets the time to wait for a connection 
    in seconds
    5 seconds is the default time
Params
    secs - seconds to wait
Return
    UTE_SUCCESS - success
    UTE_ERROR   - invalid input value
**********************************************/
int CUT_POP3Client::SetConnectTimeout(int secs){
    
    if(secs <= 0)
        return OnError(UTE_ERROR);

    m_nConnectTimeout = secs;

    return OnError(UTE_SUCCESS);
}
/*********************************************
GetConnectTimeout
    Gets the time to wait for a connection 
    in seconds
Params
    none
Return
    current time out value in seconds
**********************************************/
int CUT_POP3Client::GetConnectTimeout() const
{
    return m_nConnectTimeout;
}
/********************************
SetPOP3TimeOut
    Sets POP3 time out value
PARAM:
    timeout 
RETURN:
    UTE_SUCCESS - success
    UTE_ERROR   - invalid input value
*********************************/
int CUT_POP3Client::SetPOP3TimeOut(int timeout) {
    if(timeout <= 0)
        return OnError(UTE_ERROR);

    m_nPOP3TimeOut = timeout;

    return OnError(UTE_SUCCESS);
}
/********************************
GetPOP3TimeOut
    Gets POP3 time out value
PARAM:
    none
RETURN:
    timeout 
*********************************/
int CUT_POP3Client::GetPOP3TimeOut() const
{
    return m_nPOP3TimeOut;
}



/**************************************************************
SocketOnConnected(SOCKET s, const char *lpszName)
	
		If the security is enabled then perform te SSL neogotiation
		otherwise just return a success and let the plain text FTP handles
		the comunication

		To let the server know that we are looking for SSL or TLS we need to
		send the following command
		FEAT  
		The Feature negotiation mechanism for the File Transfer Protocol 
		
		To ask the server for SSL or TLS negotiation 
		we will send the AUTH command.

		A parameter for the AUTH command to indicate that TLS is
		required.  It is recommended that 'TLS', 'TLS-C', 'SSL' and 'TLS-P'
		are acceptable, and mean the following :-

		 'TLS' or 'TLS-C' - the TLS protocol or the SSL protocol will be
			negotiated on the control connection.  The default protection
			setting for the Data connection is 'Clear'.

			'SSL' or 'TLS-P' - the TLS protocol or the SSL protocol will be
			negotiated on the control connection.  The default protection
			setting for the Data connection is 'Private'.  This is primarily
			for backward compatibility.

	Notice that we will first send a TLS P to select The highest implementation.
	the server might response with the response
	503 unknown security mechanism 

	if this happened we will issue an Auth SSL command.

Param:
	SOCKET s		- The newly created socket 
	lpszName		- apointer to the host name we are attempting to connect to


Return:
	UTE_NO_RESPONSE - Server did not response to our command
	UTE_CONNECT_FAIL_NO_SSL_SUPPORT - Server does not support SSL.
	UTE_CONNECT_FAILED	-	 The connection have failed
	security  errors   - This function may fail with other error
			UTE_LOAD_SECURITY_LIBRARIES_FAILED
			UTE_OUT_OF_MEMORY
			UTE_FAILED_TO_GET_SECURITY_STREAM_SIZE
			UTE_OUT_OF_MEMORY
			UTE_FAILED_TO_QUERY_CERTIFICATE
			UTE_NULL_PARAM
			UTE_PARAMETER_INVALID_VALUE
			UTE_FAILED_TO_GET_CERTIFICATE_CHAIN
			UTE_FAILED_TO_VERIFY_CERTIFICATE_CHAIN
			UTE_FAILED_TO_VERIFY_CERTIFICATE_TRUST
	UTE_POP3_TLS_NOT_SUPPORTED - server does not support the STLS command	
**************************************************************/
int CUT_POP3Client::SocketOnConnected(SOCKET s, const char *lpszName){

	int		rt = UTE_SUCCESS;	

#ifdef CUT_SECURE_SOCKET
	
	BOOL bSecFlag = GetSecurityEnabled();
	
	if(bSecFlag)
	{
		
		// disable the secure comunication so we can send the plain data 
		SetSecurityEnabled(FALSE);
		
		// get server response
		if( !GetResponseCode(m_nPOP3TimeOut))
			rt = UTE_CONNECT_FAILED;
		else{
			
			
			Send("STLS\r\n");
			
			// get response
			if( GetResponseCode(m_nPOP3TimeOut))
			{
				// reset back the security so we can proceed with negotiation
				SetSecurityEnabled(bSecFlag);
				rt =  CUT_SecureSocketClient::SocketOnConnected(s, lpszName);
			}
			else
				rt = UTE_POP3_TLS_NOT_SUPPORTED;
		}
		
		return rt;
		
	}
	else
	{
		
		return UTE_SUCCESS;
	}

#else 
// v4.2 non-secure builds result in unref'd param warns VC6
	UNREFERENCED_PARAMETER(s);
	UNREFERENCED_PARAMETER(lpszName);

// v4.2 unreachable code in secure build
	return OnError(rt);
// v4.22 check this - was retuning UTE_SUCCESS
#endif

}

#pragma warning ( pop )



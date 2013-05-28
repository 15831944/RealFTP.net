//=================================================================
//  class: CUT_SMTPClient
//  File:  smtp_c.cpp
//
//  Purpose:
//
//  Implementation of the SPTP client class.
//
//=================================================================
// Ultimate TCP/IP v4.2
// Copyright © The Ultimate Toolbox 1995-2007, all rights reserverd
//=================================================================

#ifdef _WINSOCK_2_0_
#define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
/* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include <stdio.h>

#include "smtp_c.h"
//#include "Base64.h"
#include "UT_CramMd5.h"

#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

/********************************
Constructor
*********************************/
CUT_SMTPClient::CUT_SMTPClient() :
m_nPort(25),            // Initialize default port 25
m_nSMTPTimeOut(10),     // Set SMTP timeout to 10 sec.
m_nConnectTimeout(5) ,   // Set connect timeout to 5 sec.
m_bSMTPLogin(FALSE)
{
	m_lpszPassword = NULL;
	m_lpszUserName = NULL;
	m_loginMechanism.ClearList();
}

/********************************
Destructor
*********************************/
CUT_SMTPClient::~CUT_SMTPClient(){
	ClearHeaderTags();

	if (m_lpszPassword != NULL)
	{
		// prior to attempting to delete a critical data we must fill it in with 
		// unused data so we can make sure it does not get read latter on
		memset(m_lpszPassword,0,strlen(m_lpszPassword));
		delete m_lpszPassword;
	}
	if (m_lpszUserName != NULL)
	{
		// prior to attempting to delete a critical data we must fill it in with 
		// unused data so we can make sure it does not get read latter on
		memset(m_lpszUserName,0,strlen(m_lpszUserName));
		delete m_lpszUserName;
	}
}

/********************************
GetRcptName
Internal function:
Find the recpient name  for the RCPT command

PARAM:
rcptIn      - source buffer 
rcptOut     - target buffer
rcptOutLen  - length of the target buffer

RETURN:
UTE_ERROR   - Invalid email string
UTE_SUCCESS - Success
*********************************/
int CUT_SMTPClient::GetRcptName(LPCSTR rcptIn, LPSTR rcptOut, int rcptOutLen) {

	int loop;
	int openQuote = FALSE;
	int len = (int)strlen(rcptIn);
	int startPos = 0;

	//the address must be at least 3 chars long eg. x@y
	if(len < 3)
		return OnError(UTE_ERROR);

	//find the first <
	for(loop = 0; loop < len; loop++) {
		if(rcptIn[loop] == '<' && openQuote == FALSE) {
			startPos = loop + 1;
			break;
		}
		if(rcptIn[loop] == '\"')
			openQuote = openQuote ? 0 : 1; //toggle openQuote On and Off
	}

	//if no < is found then get the first piece of the address string
	if(startPos == 0) {
		CUT_StrMethods::ParseString(rcptIn, " ", 0, rcptOut, rcptOutLen);
		return OnError(UTE_SUCCESS);
	}

	CUT_StrMethods::ParseString(&rcptIn[startPos], ">" , 0, rcptOut, rcptOutLen);

	return OnError(UTE_SUCCESS);
}

/***********************************************
SMTPConnect
Connects to an SMTP server informing it of 
which machine the user is trying to access 
the connection to the server from.

Some SMTP servers do not allow user to
connect unless the service access is being 
made from specific machines.
If the SMTP server does not require this
information you set the second parameter
"localName" to "local.com" or "127.0.0.1".
The connection is made through port 25 on the server
PARAM:
hostName    - Server name or IP address
localName   - local host name or IP address
RETURN:
UTE_SUCCESS         - success  
UTE_INVALID_ADDRESS - invalid address
UTE_HELLO_FAILED    - HELO command was rejected or not responded to by the server
UTE_ABORTED         - aborted
UTE_CONNECT_TIMEOUT - connection timeout
UTE_CONNECT_FAILED  - Attempt to connect to the server  failed
UTE_SMTP_TLS_NOT_SUPPORTED - Server does not  support TLS
************************************************/
#if defined _UNICODE
int CUT_SMTPClient::SMTPConnect(LPCWSTR hostName,LPCWSTR localName){
	return SMTPConnect(AC(hostName), AC(localName));}
#endif
int CUT_SMTPClient::SMTPConnect(LPCSTR hostName,LPCSTR localName){

	int     rt, error = UTE_SUCCESS;
	char    buf[MAX_PATH+1];

	// clear list of supported login mechanism 
	m_loginMechanism.ClearList();


#ifdef CUT_SECURE_SOCKET
	// keep track of the security settings
	BOOL bFlagSecurity = GetSecurityEnabled(); // 
	// disable the security so we comunicate in plain
	SetSecurityEnabled(FALSE);

#endif // CUT_SECURE_SOCKET


	//close any open connection
	SMTPClose();


	// make sure we don't cache anything
	m_bServerSupportTLS = FALSE;

	//get the host IP Address
	if(IsIPAddress(hostName) != TRUE) {
		if(GetAddressFromName(hostName, buf, sizeof(buf)) != UTE_SUCCESS)
			return OnError(UTE_INVALID_ADDRESS);
	}
	else
		strncpy(buf, hostName, sizeof(buf));

	//connect using a timeout
	error = Connect(m_nPort, buf, m_nConnectTimeout);


	if(IsAborted())                         // Test abort flag
		error = UTE_ABORTED;                // Aborted

	// if everything still Ok.
	if(error == UTE_SUCCESS) {

		//get the header
		rt = GetResponseCode(m_nSMTPTimeOut);
		if( rt < 200 || rt > 299)	
		{
			error = UTE_CONNECT_TIMEOUT;
		}
		else
		{			
			char        szTempBuff[MAX_ADDRESS_SIZE+MAX_PATH]; //temporary buffer to send whole command as one line 

			// ESMTP Extension work
			_snprintf(szTempBuff,sizeof(szTempBuff)-1,"EHLO %s\r\n",localName);
			Send (szTempBuff);

			// we will try to receive a single line
			if(ReceiveLine(buf,sizeof(buf),m_nSMTPTimeOut) <= 0)
			{
				error = UTE_HELLO_FAILED;

			}
			else if (strlen(buf) < 3 )
			{
				error = UTE_HELLO_FAILED;			
			}
			else if ( (buf[3] != '-')) 
			{// in case the server understands EHLO but does not support ESMTP

				// ---- Fix Correction 
				// before september 24th we have assumed that
				// We should get a multiline response from the server here 
				// However, as I was pointed out by our customers, this assumption is not correct.
				// As some Email servers may respond to an EHLO message by sending back the ok 250
				// notice this is not containing a "-" sign, since the response is not a multiline response.
				// this will indicate that the server is only supporting SMTP not ESMTP commands
				// -------------                      GW 

				// So we will check for this case to fix this problem.
				buf[3] = 0;

				rt = atoi(buf);
				if( rt > 199 && rt < 300)
				{
#ifdef			CUT_SECURE_SOCKET
					if (!bFlagSecurity)
					{
#endif			
						error = UTE_SUCCESS; // this will be changed if the security was enabled

#ifdef			CUT_SECURE_SOCKET
						error = UTE_SMTP_TLS_NOT_SUPPORTED;
					}			
#endif			
				} // rt < || rt > 300
				else
				{
					// ehlo failed so lets try 
					// HELO
#ifdef			CUT_SECURE_SOCKET

					if (!bFlagSecurity)
					{
#endif			
						// if the server does not support EHLO 
						// then just send a HELO
						//send the HELO command
						_snprintf(szTempBuff,sizeof(szTempBuff)-1,"HELO %s\r\n",localName);
						Send (szTempBuff);		

						rt = GetResponseCode(m_nSMTPTimeOut);
						if( rt < 200 || rt > 299)
							error = UTE_HELLO_FAILED;
#ifdef			CUT_SECURE_SOCKET
					}
					else // security was set and server does not support TLS so we will just return
						error = UTE_SMTP_TLS_NOT_SUPPORTED;
#endif		

				}
			}// (buf[3] == '-')

			else 
			{
				buf[3] = 0;					
				rt = atoi(buf);
				if( rt > 199 && rt < 300)
				{

					// the server supports the ehlo command so lets see what 
					// command intrest us
					do{  


						if(ReceiveLine(buf,sizeof(buf),m_nSMTPTimeOut) <= 0)
						{
							error = UTE_HELLO_FAILED;
							break;
						}				
						// we are looking for this tag 250-STARTTLS
						CUT_StrMethods::ParseString(buf,"- \r\n",1,szTempBuff,sizeof(szTempBuff));
#ifdef			CUT_SECURE_SOCKET

						if (strcmp(szTempBuff,"STARTTLS") == 0 )
						{
							// the server supports TLS
							m_bServerSupportTLS = TRUE;
						}
#endif	// CUT_SECURE_SOCKET
						// find the list of supported authentication mechanisms
						if (strcmp(szTempBuff,"AUTH") == 0 )
						{
							// right here lets call the add AuthMechanism to set the list of 
							// authintications supported by the server
							// we are contacting
							// the current supported mechanisms are
							// LOGIN and CRAM-MD5 for now 


							// get the number of supported mechanisms
							// they are space delimetted paramateres 
							int numPeices = CUT_StrMethods::GetParseStringPieces (buf," ");

							// now for each mechanism supported add it to the list
							for (int loop = 1; loop < numPeices; loop++)
							{
								CUT_StrMethods::ParseString(buf," \r\n",loop,szTempBuff,sizeof(szTempBuff));
								// we will use only the highest of the ones we support								
								m_loginMechanism.AddString(szTempBuff);
							}						

						}
						//check to see if there is a '-' in the return code
						if(CUT_StrMethods::GetParseStringPieces (buf,"-") <2)
							break;

					}while(IsDataWaiting());
				} // ( rt > 199 && rt < 300)
				else
					error = UTE_HELLO_FAILED;

			}


		}
	}	

#ifdef CUT_SECURE_SOCKET
	if (bFlagSecurity)
	{
		if (m_bServerSupportTLS)
		{
			Send("STARTTLS\r\n");
			rt = GetResponseCode(m_nSMTPTimeOut);
			if( rt < 200 || rt > 299)
			{
				error = UTE_HELLO_FAILED;
			}
			SetSecurityEnabled(bFlagSecurity);
			error =  CUT_SecureSocketClient::SocketOnConnected(m_socket, hostName);

		}else{

			error = UTE_HELLO_FAILED;
			SetSecurityEnabled(bFlagSecurity);
		}

	}
#endif // CUT_SECURE_SOCKET

	// before we use the login let's check if the server did not faile yet
	// or the server actually support the mechanism we need
	//
	// Note that if the login is turned on and the server allows use without Login
	// this function will return an error.
	if (error == UTE_SUCCESS && m_bSMTPLogin == TRUE)
	{
		error = DoSASLAuthentication();

	}
	return OnError(error); 
}

/********************************
SMTPClose() 
Close the coinnection to the SMTP server by 
Issuing a QUIT command 
PARAM:
NONE
RETURN
UTE_SUCCESS - success
UTE_ERROR   - failed
*********************************/
int CUT_SMTPClient::SMTPClose(){

	if(GetSocket() != INVALID_SOCKET)
		Send("QUIT\r\n");

	return CloseConnection();
}
/********************************
ClearHeaderTags()
Remove the header tags added previously
NOTE: Headers that added to SMTP object will be added to each
message sent afterward unless you call this function
PARAM:
NONE
RETURN  
UTE_SUCCESS - success
*********************************/
int CUT_SMTPClient::ClearHeaderTags(){
	m_listHeader.ClearList();
	return UTE_SUCCESS;
}

/********************************
SendMail
This function sends the message with or
without an attachment to an SMTP server.

As you can see, there are various versions
of this function due to different function signatures.
Select the version that will best meets your needs.
You may report the progress of the send process by 
overriding the OnSendMailProgress(long bytesSent, long totalBytes) 
virtual function.

You can also cancel the send at any time during the send process by
returning FALSE from the OnSendMailProgress() function.

See also: OnSendMailProgress(long bytesSent, long totalBytes)
PARAM:
to              - The destination address (ie- testuser@theultimatetoolbox.com)
from            - The address of the sender. (ie- testuserll@theultimatetoolbox.com)
subject         - The subject of the message.
message         - The actual message that is to be sent.
cc              - The address of another destination for the message to be sent.
bcc             - The address of another blind destination for the message to be sent.
attachFilename  - Pointer to the attachments file name.
numAttach       - The number of attachments to send.
RETURN:
UTE_SUCCESS                 - This function was successful.
UTE_ERROR                   - Error
UTE_NOCONNECTION            - There is no Connection.
UTE_INVALID_ADDRESS         - The address is not valid.
UTE_SVR_NO_RESPONSE         - There was no response from the server. 
UTE_RCPT_FAILED             - There was a problem with the server with respect to the RCPT command.
UTE_DATA_FAILED             - The DATA command failed.
UTE_SOCK_SEND_ERROR         - Winsock send command failed.
UTE_TEMP_FILENAME_FAILED    - get temporary file name failed
UTE_PARAMETER_INVALID_VALUE - Invalid values of the parameters
UTE_MSG_BODY_TOO_BIG        - Message body is too big. See CUT_Msg::SetMaxMessageBodySize()
UTE_MSG_ATTACHMENT_ADD_FAILED - Failed to add attachments to the message
UTE_ABORTED                 - User terminated the process.
*********************************/
#if defined _UNICODE

int CUT_SMTPClient::SendMail(   LPCWSTR to, LPCWSTR from, LPCWSTR subject, LPCWSTR message_body, LPCWSTR cc, 
							 LPCWSTR bcc, LPCWSTR* attachFilenames, int numAttach) 
{
	LPCSTR * attFilesA = NULL;

	// convert attachment file names if any
	if( numAttach > 0 ) {
		attFilesA = new LPCSTR[numAttach];
		for (int i = 0; i < numAttach; ++i) {
			attFilesA[i] = AC(attachFilenames[i]);
		}
	}
	int retval = SendMail(AC(to), AC(from), AC(subject), AC(message_body), AC(cc), AC(bcc), attFilesA, numAttach);
	if(attFilesA != NULL) {
		delete [] attFilesA;
	}
	return retval;
}
#endif
int CUT_SMTPClient::SendMail(   LPCSTR to,
							 LPCSTR from,
							 LPCSTR subject,
							 LPCSTR message_body,
							 LPCSTR cc,
							 LPCSTR bcc,
							 LPCSTR* attachFilenames,
							 int numAttach) 

{

	CUT_Msg     message;
	BOOL        bResult = TRUE;

	// Check parameters
	if(to == NULL || from == NULL || subject == NULL || message_body == NULL)
		return OnError(UTE_PARAMETER_INVALID_VALUE);

	//*******************************
	//** Initialize message object **
	//*******************************

	// Set message headers fields
	bResult &= message.AddHeaderField(to, UTM_TO);
	bResult &= message.AddHeaderField(from, UTM_FROM);
	bResult &= message.AddHeaderField(subject, UTM_SUBJECT);

	if(cc != NULL && strlen(cc) > 0)
		bResult &= message.AddHeaderField(cc, UTM_CC);
	if(bcc != NULL && strlen(bcc) > 0)
		bResult &= message.AddHeaderField(bcc, UTM_BCC);

	if(!bResult)
		return OnError(UTE_ERROR);

	// Set message body
	if(!message.SetMessageBody(message_body))
		return OnError(UTE_MSG_BODY_TOO_BIG);


	// Set message attachments
	int     index;
	if(attachFilenames != NULL)
		for(index = 0; index < numAttach; index ++) {
			if(attachFilenames[index] != NULL)
			{
				// v4.2 using WC() macro here - file cd now takes LPCTSTR
				CUT_FileDataSource dsTemp(WC(attachFilenames[index]));
				if(message.AddAttachment(dsTemp, attachFilenames[index]) != UTE_SUCCESS)
					return OnError(UTE_MSG_ATTACHMENT_ADD_FAILED);
			}
		}

		// Send message
		return SendMail(message);
}

/********************************
SendMail
This function sends the message with or
without an attachment to an SMTP server.

As you can see, there are various versions
of this function due to different function signatures.
Select the version that will best meets your needs.
You may report the progress of the send process by 
overriding the OnSendMailProgress(long bytesSent, long totalBytes) 
virtual function.

You can also cancel the send at any time during the send process by
returning FALSE from the OnSendMailProgress() function.

See also: OnSendMailProgress(long bytesSent, long totalBytes)
PARAM:
message     - message to send

RETURN:
UTE_SUCCESS                 - This function was successful.
UTE_NOCONNECTION            - There is no Connection.
UTE_INVALID_ADDRESS         - The address is not valid.
UTE_SVR_NO_RESPONSE         - There was no response from the server. 
UTE_RCPT_FAILED             - There was a problem with the server with respect to the RCPT command.
UTE_DATA_FAILED             - The DATA command failed.
UTE_SOCK_SEND_ERROR         - Winsock send command failed.
UTE_BUFFER_TOO_SHORT        - Buffer too small
UTE_FILE_TMP_NAME_FAILED    - Can't create temporary file name
UTE_ABORTED                 - User terminated the process.
*********************************/
int CUT_SMTPClient::SendMail(CUT_Msg & message) {

	char    workAddress[MAX_ADDRESS_SIZE + 1];
	char    szBuffer[MAX_LINE_LENGTH + 1];
	int     i, index = 0, retVal = UTE_SUCCESS;
	UINT    loop;
	LPCSTR  lpszString, lpszPrevCustFieldName = NULL;

	// Prepare message data source
	LPSTR   msg_body = const_cast<LPSTR> (message.GetMessageBody());
	CUT_BufferDataSource    MsgBodySource(msg_body, strlen(msg_body));

	// Create attachments data source
	CUT_MapFileDataSource   AttachSource(0, message.GetAttachmentSize() + 4096, "Attachments");

	//////////////////////////////////////////////////////////////

	// Clear headers fields
	m_listHeader.ClearList();

	// If there is an HTML body add it as an attachment for now and remove it after
	// the message is sent
	bool bMustRemoveHTMLAttachment = false;
	if (message.m_lpszHtmlMessageBody != NULL)
	{
		message.SetHtmlMessageBody(message.m_lpszHtmlMessageBody);
		bMustRemoveHTMLAttachment = true;
	}

	// If we have attachments or an html body or both
	if (message.GetAttachmentNumber() > 0)
	{
		// Encode attachments
		if((retVal = message.Encode(MsgBodySource, AttachSource)) != UTE_SUCCESS)
			return OnError(retVal);

		// Add encoding header fields
		index = 0;
		while((lpszString = message.GetGlobalHeader(index++)) != NULL) {
			m_listHeader.AddString(lpszString);     
		}	
	}

	// v4.2 added check for UTM_REPLY_TO 
	char buf[MAX_ADDRESS_SIZE+1];
	if(message.GetHeaderByType(UTM_REPLY_TO, buf, MAX_ADDRESS_SIZE)) {
		LPCSTR lpszName = message.GetFieldName(UTM_REPLY_TO);
		if(lpszName != NULL) {
			strcpy(szBuffer, lpszName);
			strcat(szBuffer, " ");
			CUT_StrMethods::RemoveSpaces(buf);
			CUT_StrMethods::RemoveCRLF(buf);
			strcat(szBuffer, buf);
			m_listHeader.AddString(szBuffer);
		}
	}


	// Add custom headers fields
	index = 0;
	while((lpszString = message.GetHeaderByType(UTM_CUSTOM_FIELD, index)) != NULL)  {
		LPCSTR lpszName = message.GetCustomFieldName(index);
		if(lpszName != NULL) {
			BOOL    bAdd = TRUE;

			*szBuffer = NULL;

			// Add the name of the first custom field line
			if(lpszPrevCustFieldName == NULL || _stricmp(lpszPrevCustFieldName, lpszName) != 0)
				strcpy(szBuffer, lpszName);

			// Check if this field is duplicated as encoding header
			int     nHeaderIndex = 0;
			LPCSTR  lpszEncHeader;
			while((lpszEncHeader = message.GetGlobalHeader(nHeaderIndex++)) != NULL)
			{
				char    szHDField[MAX_LINE_LENGTH + 1];                

				CUT_StrMethods::ParseString(lpszEncHeader, ":", 0, szHDField, sizeof(szHDField)-2);
				strcat(szHDField, ":");
				if(_stricmp(lpszName, szHDField) == 0)
				{
					// Note: According to RFC 1049 we can have a Content-Type header
					// in a non-multipart message, so if a Content-Type header is found
					// do not remove it unless this is a multipart message
					if (_stricmp(lpszName, "Content-Type:") == 0)
					{
						if (message.GetAttachmentNumber() > 0)
						{
							// Remove the Content-Type header since this is a multipart message
							bAdd = FALSE;
							break;
						}
					}
					else
					{
						// Another duplicate header, so remove it
						bAdd = FALSE;
						break;
					}
				}
			}

			// Add the custom field data
			if(bAdd) {
				strcat(szBuffer, lpszString);
				m_listHeader.AddString(szBuffer);
			}
			lpszPrevCustFieldName = lpszName;
		}
		index++;
	}

	/////////////////////////////////////////////////////////////////////////////

	// Parse all addresses and save them in the TO, CC and BCC lists
	CUT_StringList listTo, listCc, listBcc;

	for( i = 0; i < message.GetHeaderCount(UTM_TO); i++) {
		LPCSTR fieldTo = message.GetHeaderByType(UTM_TO, i);
		loop = 0;
		if(fieldTo != NULL) 
			while(CUT_StrMethods::ParseString(fieldTo, "\r\n\t,", loop++ ,workAddress, sizeof(workAddress)) == UTE_SUCCESS ) 
				listTo.AddString(workAddress);
	}

	for( i = 0; i < message.GetHeaderCount(UTM_CC); i++) {
		LPCSTR fieldCc = message.GetHeaderByType(UTM_CC, i);
		loop = 0;
		if(fieldCc != NULL) 
			while(CUT_StrMethods::ParseString(fieldCc, "\r\n\t,", loop++ ,workAddress, sizeof(workAddress)) == UTE_SUCCESS ) 
				listCc.AddString(workAddress);
	}

	for( i = 0; i < message.GetHeaderCount(UTM_BCC); i++) {
		LPCSTR fieldBcc = message.GetHeaderByType(UTM_BCC, i);
		loop = 0;
		if(fieldBcc != NULL) 
			while(CUT_StrMethods::ParseString(fieldBcc, "\r\n\t,", loop++ ,workAddress, sizeof(workAddress)) == UTE_SUCCESS ) 
				listBcc.AddString(workAddress);
	}

	// Calculate the total number of addresses
	long lAddressesNumber = listTo.GetCount() + listCc.GetCount() + listBcc.GetCount();

	// Initialize number of bytes send
	long lNumberBytesSend = 0;

	// cycle through a list of 'to' addresses, each separated by a comma,
	// each address must be mailed to individually.
	// The ParseString function will return each address by an index value, 
	// if we request an item past the end of the list, the function 
	// returns UTE_ERROR.

	for( i = 0; retVal == UTE_SUCCESS && !IsAborted() && i < listTo.GetCount(); i++) 
		retVal = ProcessMail(   listTo.GetString(i),
		message,
		listTo,
		listCc,
		lAddressesNumber,
		lNumberBytesSend,
		MsgBodySource,
		AttachSource,
		message.GetAttachmentNumber());


	// loop through all CC's

	// cycle through a list of 'cc' addresses, each separated by a comma,
	// each address must be mailed to individually.
	// The ParseString function will return each address by an index value, 
	// if we request an item past the end of the list, the function 
	// returns UTE_ERROR.
	// CC's are different in that the address "sent" to changes to reflect
	// each of the CC's, the actual addressee of the mail does not change.

	for( i = 0; retVal == UTE_SUCCESS && !IsAborted() && i < listCc.GetCount(); i++) 
		retVal = ProcessMail(   listCc.GetString(i),
		message,
		listTo,
		listCc,
		lAddressesNumber,
		lNumberBytesSend,
		MsgBodySource,
		AttachSource,
		message.GetAttachmentNumber());


	// loop through all BCC's

	// cycle through a list of 'bcc' addresses, each separated by a comma,
	// each address must be mailed to individually.
	// BCC's are different in that the address "sent" to changes to reflect
	// each of the BCC's, the actual addressee of the mail does not change.
	// BCC's are not listed in the "Copies To:" header of the final message.

	for( i = 0; retVal == UTE_SUCCESS && !IsAborted() && i < listBcc.GetCount(); i++) 
		retVal = ProcessMail(   listBcc.GetString(i),
		message,
		listTo,
		listCc,
		lAddressesNumber,
		lNumberBytesSend,
		MsgBodySource,
		AttachSource,
		message.GetAttachmentNumber());

	if (bMustRemoveHTMLAttachment)
		message.SetHtmlMessageBody((LPCSTR)NULL);

	return OnError(retVal);

}
/********************************
ProcessMail - Internal method
Sends each recepient a copy of the message 
NOTE:
During this process the return value 
OnSendMailProgress() is tested to check if
the user wants to cancel the process
PARAM:
mailto              - The destination address (ie- testuser@theultimatetoolbox.com)
message             - Message to send
listTo              - List of addresses in TO field
listCc              - List of addresses in CC field
lTotalAddressNumber - Total number of addresses in message TO + CC + BCC
lNumberBytesSend    - Number of bytes send for all destination addresses.
MessageBodySource   - Data source for the message body
AttachSource        - Data source for all encoded attachments
nAttachNumber       - number of attachments

RETURN:
UTE_SUCCESS                 - This function was successful.
UTE_DS_OPEN_FAILED          - Data source opening failed
UTE_NOCONNECTION            - There is no Connection.
UTE_INVALID_ADDRESS         - The address is not valid.
UTE_SVR_NO_RESPONSE         - There was no response from the server. 
UTE_RCPT_FAILED             - There was a problem with the server with respect to the RCPT command.
UTE_DATA_FAILED             - The DATA command failed.
UTE_SOCK_SEND_ERROR         - Winsock send command failed.
UTE_TEMP_FILENAME_FAILED    - get temporary file name failed
UTE_ABORTED                 - User terminated the process.
UTE_FILE_OPEN_ERROR         - File open error
*********************************/
int CUT_SMTPClient::ProcessMail(    LPCSTR mailto, 
								CUT_Msg & message,
								CUT_StringList &listTo,
								CUT_StringList &listCc,
								long lTotalAddressNumber,
								long &lNumberBytesSend,
								CUT_DataSource & MessageBodySource, 
								CUT_DataSource & AttachSource,
								int nAttachNumber)
{

	int         size, i, rt;
	char        from[MAX_ADDRESS_SIZE+1], rcptTo[MAX_ADDRESS_SIZE+1];
	char        buf[MAX_ADDRESS_SIZE+1];
	char        szTempBuff[MAX_ADDRESS_SIZE+MAX_PATH]; //temporary buffer to send whole command as one line 
	long        lengthOfMail; 
	LPCSTR      lpszAddress;


	if(!IsConnected())
		return OnError(UTE_NOCONNECTION);

	// Open data sources
	if(MessageBodySource.Open(UTM_OM_READING) == -1)
		return OnError(UTE_DS_OPEN_FAILED);

	if(nAttachNumber > 0) {
		if(AttachSource.Open(UTM_OM_READING) == -1)
			return OnError(UTE_DS_OPEN_FAILED);
	}

	// calculate total send size for send status reporting.
	// for efficiency, ignore headers in calculation.
	lengthOfMail = MessageBodySource.Seek(0,SEEK_END);
	MessageBodySource.Seek(0,SEEK_SET);
	if(nAttachNumber > 0) {
		lengthOfMail += AttachSource.Seek(0,SEEK_END);
		AttachSource.Seek(0,SEEK_SET);
	}
	lengthOfMail = lengthOfMail * lTotalAddressNumber;

	//get the to address
	if(GetRcptName(mailto, rcptTo, sizeof(rcptTo)) != UTE_SUCCESS)
		return OnError(UTE_INVALID_ADDRESS);

	//send the MAIL command
	if(!message.GetHeaderByType(UTM_FROM, from, MAX_ADDRESS_SIZE))
		return OnError(UTE_BUFFER_TOO_SHORT);
	{
		//get the from address
		if(GetRcptName(from, buf, sizeof(buf)) != UTE_SUCCESS)
			return OnError(UTE_INVALID_ADDRESS);
		sprintf(szTempBuff,"MAIL FROM:<%s>\r\n", buf);
	}
	Send (szTempBuff);

	//get the response code
	rt = GetResponseCode(m_nSMTPTimeOut);
	if(rt < 200 || rt > 299)
		return OnError(UTE_SVR_NO_RESPONSE);

	//send the RCPT command
	_snprintf(szTempBuff,sizeof(szTempBuff)-1,"RCPT TO:<%s>\r\n",rcptTo);
	Send (szTempBuff);

	//get the response code
	rt = GetResponseCode(m_nSMTPTimeOut);
	if(rt < 200 || rt > 299)
		return OnError(UTE_RCPT_FAILED);

	//send the DATA command
	Send("DATA\r\n");

	//get the response code
	rt = GetResponseCode(m_nSMTPTimeOut);
	if(rt != 354)   //if the response is not 354 then return
		return OnError(UTE_DATA_FAILED);

	// Send the message headers

	// Send From field
	Send("From: ");
	Send(from);
	if(!CUT_StrMethods::IsWithCRLF(from))
		Send("\r\n");

	// Send To field
	Send( "To: " );
	size = 0;
	for( i = 0; i < listTo.GetCount(); i++) 
		if((lpszAddress = listTo.GetString(i)) != NULL) {
			if(i != 0)
				Send( "," );

			size += (int)strlen(lpszAddress) + 1;
			if(size > 250) {
				Send( "\r\n\t" );
				size = (int)strlen(lpszAddress) + 1;
			}

			Send( lpszAddress );
		}

		if(i > 0)
			Send("\r\n");

		// Send Subject field
		Send("Subject: ");
		if(!message.GetHeaderByType(UTM_SUBJECT, buf, MAX_ADDRESS_SIZE))
			return OnError(UTE_BUFFER_TOO_SHORT);
		Send(buf);
		if(!CUT_StrMethods::IsWithCRLF(buf))
			Send("\r\n");

		// Send Date field
		Send("Date: ");
		GetGMTStamp(buf,sizeof(buf));
		Send(buf);
		Send("\r\n");

		// Send CC field
		size = 0;
		for( i = 0; i < listCc.GetCount(); i++) {
			if( i == 0)
				Send( "Cc: " );
			if((lpszAddress = listCc.GetString(i)) != NULL) {
				if(i != 0)
					Send( "," );

				size += (int)strlen(lpszAddress) + 1;
				if(size > 250) {
					Send( "\r\n\t" );
					size = (int)strlen(lpszAddress) + 1;
				}

				Send( lpszAddress );
			}
		}

		if(i > 0)
			Send("\r\n");


		// Send other header information - these should have been 
		// gleaned from the message in the SendMail method.
		LPCSTR      lpszHeader;
		for(i = 0; i < m_listHeader.GetCount(); i++) {
			lpszHeader = m_listHeader.GetString(i);
			if(lpszHeader != NULL) {
				Send(lpszHeader);
				if(!CUT_StrMethods::IsWithCRLF(lpszHeader))
					Send("\r\n");
			}
		}

		Send("\r\n");

		// If we don't have any attachments- just send the message
		char    szMessageLine[MAX_LINE_LENGTH + 1];
		int     nLineSize, nMaxSend = GetMaxSend();
		int     nBytesSend, nSendPos = 0;
		if(nAttachNumber <= 0) {

			// Send message line by line
			while( (nLineSize = MessageBodySource.ReadLine(szMessageLine, MAX_LINE_LENGTH)) > 0) {

				if(IsAborted())
					return OnError(UTE_ABORTED);

				if(szMessageLine[0] == '.')
					Send(".", 1);

				// If line is too big to send - split it
				nSendPos = 0;
				while(nLineSize > 0) {

					if((nBytesSend = Send((szMessageLine + nSendPos), min(nLineSize, nMaxSend))) <= 0)
						return OnError(UTE_SOCK_SEND_ERROR);

					nSendPos += nBytesSend;
					nLineSize -= nBytesSend;
					lNumberBytesSend += nBytesSend;

					if (OnSendMailProgress(lNumberBytesSend, lengthOfMail) == FALSE)
						return OnError(UTE_ABORTED);
				}

				// Add CR LF if nesessary
				if(!CUT_StrMethods::IsWithCRLF(szMessageLine))
					if(Send("\r\n", 2) != 2)
						return OnError(UTE_SOCK_SEND_ERROR);

			}
		}

		// Send attachments data
		// NOTE! The message body is alredy added to the attachments data source
		else {
			while( (nLineSize = AttachSource.ReadLine(szMessageLine, min(nMaxSend, MAX_LINE_LENGTH-1))) > 0) {

				if(IsAborted())
					return OnError(UTE_ABORTED);

				if(szMessageLine[0] == '.')
					Send(".", 1);


				if((nBytesSend = Send(szMessageLine, nLineSize)) != nLineSize)
					return OnError(UTE_SOCK_SEND_ERROR);

				lNumberBytesSend += nBytesSend;

				if (OnSendMailProgress(lNumberBytesSend, lengthOfMail) == FALSE)
					return OnError(UTE_ABORTED);
			}
		}

		//finish the message
		Send("\r\n.\r\n");

		rt = GetResponseCode(m_nSMTPTimeOut);
		if(rt < 200 || rt > 299)
			return OnError(UTE_SVR_NO_RESPONSE);

		return OnError(UTE_SUCCESS);
}



/********************************
OnSendMailProgress
Virtual function to be overridden to inform the user
of the send mail progress and to check if the user wants
to cancel the process.
PARAM:
long bytesSent  - number of bytes sent 
long totalBytes - Total number of bytes for the message being processed
RETURN
FALSE - cancel the send process
TRUE  - Continue
*********************************/
BOOL CUT_SMTPClient::OnSendMailProgress(long /*bytesSent*/,long /*totalBytes*/){
	return !IsAborted();
}

/********************************
GetResponseCode
Internal function
// routine to parse the response from the 
server and return the response code
PARAM:
int timeOut- secs to wait for incomming data
RETURN:
0 -  No response
RESPONSE CODE depending on the last SMTP command
( SEE RFC 821 for a complete listing of the SMTP return code)
*********************************/
int CUT_SMTPClient::GetResponseCode(int timeOut) {

	char buf[MAX_PATH+1];
	char buf2[MAX_PATH+1];

	// v4.2 change to eliminate C4127: conditional expression is constant
	for (;;) {
		if(ReceiveLine(buf,sizeof(buf),timeOut) <= 2)
			return 0;

		// if '-' follows the return code we have a multuline response
		if(buf[3] == '-')
			continue;

		// get the response code
		CUT_StrMethods::ParseString(buf," ",0,buf2,sizeof(buf2));
		return atoi(buf2);
	}
}

/**********************************************
GetGMTStamp
Internal
Generate a date stamp for the MAil message 
PARAM:
buf - Target Buffer
bufLen - buffer length  
RETURN:
UTE_ERROR   - the buffer length is too short
UTE_SUCCESS - Success
***********************************************/
int CUT_SMTPClient::GetGMTStamp(LPSTR buf,int bufLen){

	time_t  t;
	struct tm *systime;
	char *days[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	char *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
		"Sep","Oct","Nov","Dec"};

	if(bufLen < 32)
		return UTE_ERROR;

	_tzset();

	//get the time/date
	t = time(NULL);
	systime = localtime(&t);    // fails after 2037 - t will be NULL (0)


	char cSign = '-';
	int  nGmtHrDiff  = _timezone / 3600;
	int  nGmtMinDiff = abs((_timezone % 3600) / 60);
	if ( systime->tm_isdst > 0 )
		nGmtHrDiff -= 1;
	if ( nGmtHrDiff < 0 )
	{
		// because VC++ doesn't do "%+2d"
		cSign = '+';
		nGmtHrDiff *= -1;
	}

	_snprintf(buf,bufLen,"%s, %2.2d %s %d %2.2d:%2.2d:%2.2d %c%2.2d%2.2d",
		days[systime->tm_wday],
		systime->tm_mday, months[systime->tm_mon], systime->tm_year+1900,
		systime->tm_hour, systime->tm_min, systime->tm_sec,
		cSign, nGmtHrDiff, nGmtMinDiff );

	return UTE_SUCCESS;
}

/********************************
SetSMTPTimeOut
Sets SMTP time out value
PARAM:
timeout 
RETURN:
UTE_SUCCESS - success
UTE_ERROR   - invalid input value
*********************************/
int CUT_SMTPClient::SetSMTPTimeOut(int timeout) {
	if(timeout <= 0)
		return OnError(UTE_ERROR);

	m_nSMTPTimeOut = timeout;

	return OnError(UTE_SUCCESS);
}
/********************************
GetSMTPTimeOut
Gets SMTP time out value
PARAM:
none
RETURN:
timeout 
*********************************/
int CUT_SMTPClient::GetSMTPTimeOut() const
{
	return m_nSMTPTimeOut;
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
int CUT_SMTPClient::SetConnectTimeout(int secs){

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
int CUT_SMTPClient::GetConnectTimeout() const
{
	return m_nConnectTimeout;
}
/*********************************************
SetPort
Sets the port number to connect to
Params
newPort     - connect port
Return
UTE_SUCCESS - success
**********************************************/
int CUT_SMTPClient::SetPort(unsigned int newPort) {
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
unsigned int  CUT_SMTPClient::GetPort() const
{
	return m_nPort;
}

/*******************************************
Set the password to be used for the response 
PARAM
LPCSTR - a string describing the password
********************************************/
#if defined _UNICODE
void CUT_SMTPClient::SetPassword(LPCWSTR pass) {
	SetPassword(AC(pass));}
#endif
void CUT_SMTPClient::SetPassword(LPCSTR pass)
{
	if (pass == NULL || pass[0] == 0)		
		return;

	if (m_lpszPassword!= NULL)
		delete m_lpszPassword;
	m_lpszPassword = new char [strlen(pass)+1];
	strcpy(m_lpszPassword,pass);		
}

/*******************************************
Set the user name to be used for the response 
PARAM
LPCSTR - a string describing the user name
********************************************/
#if defined _UNICODE
void CUT_SMTPClient::SetUserName(LPCWSTR name) {
	SetUserName(AC(name));}
#endif
void CUT_SMTPClient::SetUserName(LPCSTR name)
{
	if ( name == NULL || name[0] == 0)
		return;	


	if (m_lpszUserName!= NULL)
		delete m_lpszUserName;
	m_lpszUserName = new char [strlen(name)+1];
	strcpy(m_lpszUserName,name);		
}
/*******************************************
Do the SASL authentication if the server supports it
based on the server's response we will atempt to comunicate 
with it using one of the supported authentication mechanisms
So far we support two mechanisms "CRAM-MD5" and "LOGIN"

If the server support CRAM-MD5 (the higher of the two) we
will atempt to start the authentication Challenge response
Otherwise, we will atempt a LOGIN approach.
PARAM
NONE
RETURN

UTE_SUCCESS	- Operation Succeded
UTE_USER_ERROR - User name is not correct
UTE_PASSWORD_ERROR - Password is not correct
UTE_INVALID_RESPONSE - Server returned an Invalid Response
UTE_ERROR	-	 Internal error 
UTE_LOGIN_FAILED - login rejected by server 
UTE_SVR_NOT_SUPPORTED - The login mechanism is not supported by the server	
UTE_ERROR - Internal Error as it should have not reached here if the condition met!
*******************************************/
int		CUT_SMTPClient::DoSASLAuthentication()
{
	int loginLevel = 0;

	// if thier is no authentication mechanism 
	// return error
	//Internal Error as it should have not reached here if the condition met!
	if (m_loginMechanism.GetCount() == 0)
		return UTE_ERROR;

	// we will look for CRAM-MD5 first 
	for (int loop = 0; loop < m_loginMechanism.GetCount(); loop++)
	{
		// does the server speak MD5 ish?
		if (_stricmp(m_loginMechanism.GetString(loop), "CRAM-MD5") == 0)
		{
			loginLevel = 1;
			// this is the higher mecanism we support so far so break
			break;
		}
		if (_stricmp(m_loginMechanism.GetString(loop), "LOGIN") == 0)
		{

			loginLevel = 2;
			// we don't want to break in case there is higher mechanism		
		}
	}
	// use the highest login mechanism supported 
	switch (loginLevel)
	{
	case 1:
		return CRAMAuthenticate();
		break;
	case 2:
		return LOGINAuthenticate();
		break;
	default :
		return UTE_ERROR; // Unsupported mechanism for AUTH command
	}
}
/*****************************************
LOGINAuthenticate()
Login Authentication is the a mechanism that we 
don't recomend unless the TLS is used.
However it is employed by many servers out there.

The server will response with the request for continue
in BASE 64
we send the user name base64
to which the server must reponse with 334 unless not authorized
then we send the password base64
to which the server must reponse with 235 unless not authorized

The AUTH command indicates an authentication mechanism to the
server.  If the server supports the requested authentication
mechanism, it performs an authentication protocol exchange to
authenticate and identify the user.  Optionally, it also
negotiates a security layer for subsequent protocol
interactions.  If the requested authentication mechanism is not
supported, the server rejects the AUTH command with a 504
reply.

REMARK
The functions SetUserName (), SetPassword (), and EnableSMTPLogin(TRUE)
must be called prior to calling this function
PARAM:
NONE
RET:
UTE_SUCCESS	- Operation Succeded
UTE_USER_ERROR - User name is not correct
UTE_PASSWORD_ERROR - Password is not correct
UTE_INVALID_RESPONSE - Server returned an Invalid Response
UTE_ERROR	-	 Internal error 
UTE_LOGIN_FAILED - login rejected by server 
UTE_SVR_NOT_SUPPORTED - The login mechanism is not supported by the server	
*****************************************/
#pragma warning ( push )
#pragma warning (disable : 4702)
int		CUT_SMTPClient::LOGINAuthenticate()
{
	char data[500];
	int rt =   0;
	
	if (m_lpszUserName == NULL || m_lpszUserName[0] == '\0')
		return UTE_USER_ERROR;
	
	if (m_lpszPassword == NULL || m_lpszPassword[0] == '\0')
		return UTE_PASSWORD_ERROR;
	
	// we speak LOGIN can you ?
	Send ("AUTH LOGIN\r\n");
	
	// server answer
	rt = GetResponseCode(m_nSMTPTimeOut);
	
	// does the server speak LOGIN?
	if( rt == 334)
	{
		CBase64 B64;
		char result[500];
		B64.EncodeData ((const unsigned char *)m_lpszUserName, (int)strlen(m_lpszUserName),result,500);
		// send the username bas64 Encoded
		_snprintf(data,sizeof(data)-1,"%s\r\n", result);
		Send (data); 
		
		// are we ok so far ?
		rt = GetResponseCode(m_nSMTPTimeOut);
		if( rt == 334)
		{
			// send the pasword base64 ed
			B64.EncodeData ((const unsigned char *)m_lpszPassword, (int)strlen(m_lpszPassword),result,500);
			// send the username bas64 Encoded
			_snprintf(data,sizeof(data)-1,"%s\r\n", result);
			Send (data); 
			// did we login ?
			rt = GetResponseCode(m_nSMTPTimeOut);
			if( rt == 235)
			{
				return UTE_SUCCESS;
				
			}
			else
			{
				return UTE_PASSWORD_ERROR;
			}
		}
		else
		{
			return UTE_USER_ERROR;
		}
	// v4.2 Gives unreachable code warn in VC6 (Release)
	}
	else if (rt == 504 )
	{
		return UTE_SVR_NOT_SUPPORTED;
	}
	// the server does not support the login
	return UTE_LOGIN_FAILED;
}
#pragma warning (pop)
/*****************************************
CRAMAuthenticate()
The AUTH command indicates an authentication mechanism to the
server.  If the server supports the requested authentication
mechanism, it performs an authentication protocol exchange to
authenticate and identify the user.  Optionally, it also
negotiates a security layer for subsequent protocol
interactions.  If the requested authentication mechanism is not
supported, the server rejects the AUTH command with a 504
reply.

REMARK
The functions SetUserName (), SetPassword (), and EnableSMTPLogin(TRUE)
must be called prior to calling this function
PARAM:
NONE
RET:
UTE_SUCCESS	- Operation Succeded
UTE_USER_ERROR - User name is not correct
UTE_PASSWORD_ERROR - Password is not correct
UTE_INVALID_RESPONSE - Server returned an Invalid Response
UTE_ERROR	-	 Internal error 
UTE_LOGIN_FAILED - login rejected by server 
UTE_SVR_NOT_SUPPORTED - The login mechanism is not supported by the server	
*****************************************/
#pragma warning ( push )
#pragma warning ( disable : 4702 )
int		CUT_SMTPClient::CRAMAuthenticate()
{
	char data[500];
	long rt =   0;
	
	
	if (m_lpszUserName == NULL || m_lpszUserName[0] == '\0')
		return UTE_USER_ERROR;
	
	if (m_lpszPassword == NULL || m_lpszPassword[0] == '\0')
		return UTE_PASSWORD_ERROR;
	
	// we speak CRAM-MD5 can you ?
	Send ("AUTH CRAM-MD5\r\n");
	
	// server answer
	if(ReceiveLine(data,sizeof(data),m_nSMTPTimeOut) <= 2)
		return UTE_INVALID_RESPONSE;
	
	// the server response should be 334 [BASE64 Challenge]
	CUT_StrMethods::ParseString (data," \r\n",0, &rt);
	
	// the response of the server should start with 334
	// does the server speak CRAM-MD5?
	if( rt == 334)
	{
		
		char szChallenge[500];
		CUT_CramMd5 cramMd5;
		
		// prepare the CRAM-MD5 for encryption
		cramMd5.SetPassword (m_lpszPassword);
		cramMd5.SetUserName (m_lpszUserName);
		
		// get the server's challenge
		CUT_StrMethods::ParseString (data," \r\n",1, szChallenge, sizeof(szChallenge)-1);
		
		// check if the response is NULL
		if (cramMd5.GetClientResponse (szChallenge) == NULL)
			return UTE_ERROR;			 // internal error
		
		_snprintf(data,sizeof(data)-1,"%s\r\n",cramMd5.GetClientResponse (szChallenge) );
		Send (data); 
		// are we ok so far ?
		rt = GetResponseCode(m_nSMTPTimeOut);
		if( rt == 235)
		{
			return UTE_SUCCESS;
			
		}
		else
		{
			return UTE_LOGIN_FAILED;
		}
	// v4.2 Gives unreachable code warn in VC6 (Release)
	}
	else 
	{
		if(rt == 504)// somthing wrong happened with our login 
		{
			return UTE_SVR_NOT_SUPPORTED;
		}
	}
	
	return UTE_LOGIN_FAILED;
}
#pragma warning ( pop )
/************************************************
Enables the SMTP authorization process to take place
PARAM
bFlag - TRUE the server requires the client to authenticate
- FALSE don't attempt to authorize
*************************************************/
void CUT_SMTPClient::EnableSMTPLogin(BOOL bFlag)
{
	m_bSMTPLogin = bFlag;
}

#pragma warning ( pop )
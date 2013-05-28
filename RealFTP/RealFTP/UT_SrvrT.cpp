//=================================================================
//  class: CUT_WSThread
//  File:  UT_SrvrT.cpp
//
//  Purpose:
//
//      CUT_WSThread class derivations implement connection 
//      servicing for particular protocols.  
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
    #define _WINSOCKAPI_	/* Prevent inclusion of winsock.h in windows.h   */
	    					/* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include <stdio.h>
#include <process.h>

#include "ut_srvr.h"

#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)


/***************************************************
CUT_WSThread
    purpose
        Constructor
    params
        None
    return
        None
****************************************************/
CUT_WSThread::CUT_WSThread() : 
		m_lSendTimeOut(0),				    // Initialize send timeout
		m_lReceiveTimeOut(0),			    // Initialize receive timeout
		m_bBlockingMode(FALSE),				// Set non blocking mode
		m_nShutDownFlag(FALSE)				// Clear shutdown flag
{

// The send/receive time out values are use with the WINSOCK ver 2.0 or later
#ifdef _WINSOCK_2_0_
    SetSendTimeOut(30000);
    SetReceiveTimeOut(30000);
#endif
	m_clientSocket = INVALID_SOCKET;
	m_nSockType = 0;
}

/***************************************************
~CUT_WSThread
    purpose
        Destructor
    params
        None
    return
        None
****************************************************/
CUT_WSThread::~CUT_WSThread(){
}

/***************************************************
StartShutdown
    purpose
        Tells the thread that it should shutdown
        immediatly, also sets a shutdown flag that
        the rest of the class must monitor
    params
        None
    return
        UTE_SUCCESS  if successful - this does not mean the thread has
            shutdown, but that the process has started
****************************************************/
int CUT_WSThread::StartShutdown(){
    WSACancelBlockingCall();
    m_nShutDownFlag = TRUE;
    return OnError(UTE_SUCCESS);
}

/***************************************************
Send
    purpose
        Sends the given data to the connected client.
        if 'len' is 0 or less then the given data is
        assumed to be 0 terminated
    params
        data        -pointer to the data to send
        len         -length of the data to send
    return
        number of bytes sent
        SOCKET_ERROR - on fail
****************************************************/
int CUT_WSThread::Send(LPCSTR data, int len, int flags){
    if(len < 0)
        len = (int)strlen((char *)data);
    return SocketSend(m_clientSocket, (char *)data, len, flags);
}

/***************************************************
SendBlob
    Sends information to the currently connected
    client. The receive function can block until the 
	specified timeout.  Unlike Receive, this method 
	waits until all the data is sent or until an 
	error occurs.
Params
    data    - pointer to data to send
    dataLen - buffer length
    timeOut - timeout in seconds ( 0- for no time-out)
Return
    number of bytes send
    0 if the connection was closed or timed-out
    any other error returns SOCKET_ERROR 
****************************************************/
int CUT_WSThread::SendBlob(LPBYTE data,int dataLen,int flags){

	LPBYTE position = data;
	int dataRemain = dataLen;

	// Try to send all data
	while(dataRemain > 0){

		// Send as much data as we can
		int dataSent = Send((LPCSTR)position, dataRemain, flags);

		if(dataSent < 0)
			break;

		// Move past the data sent
		position += dataSent;
		dataRemain -= dataSent;

		// Check if server is shutting down
		if(m_winsockclass_this->GetShutDownFlag())
			break;
	}

	return (int)(position - data);
}

/***************************************************
SendAsLine
    purpose
        Sends the given data and breaks the data up into
        lines of the specified length
    params
        data            - data to send
        len
        linelen
    return
        number of bytes sent
        SOCKET_ERROR - on fail
****************************************************/
long CUT_WSThread::SendAsLine(LPCSTR data, long len,int linelen){
    long	pos, endpos;
    char	temp, *line	= new char[linelen+5];
    long	rt, bytessent =0;
    LPSTR	d = (LPSTR)data;

    if(len <=0)
        len = (int)strlen(data);

    for(pos = 0; pos < len; pos += linelen) {
        endpos = pos+linelen;

        if(endpos > len)
            endpos =len;

        temp = d[endpos];
        d[endpos] = 0;
        strcpy(line,&d[pos]);
        d[endpos] = temp;

        strcat(line,"\r\n");

        rt = Send(line);

        if(rt <= 0) {
            bytessent = rt;
            break;
			}

        bytessent += rt;
		}

    delete[] line;

    return bytessent;
}

/***************************************************
SendFile
    Sends a given file to a currently connected client
Params
    filename - name of file to send
Return
	UTE_SUCCESS				- success
    UTE_DS_OPEN_FAILED		- unable to open specified data source
    UTE_CONNECT_TERMINATED	- remote connection terminated
****************************************************/
int CUT_WSThread::SendFile(LPCTSTR filename,long *bytesSent){
	CUT_FileDataSource ds(filename);
	return Send(ds, bytesSent);
}
/***************************************************
Send
    Sends a given datasource to a currently connected client
Params
    source		- datasource to send
	bytesSent	- number of bytes sent
Return
	UTE_SUCCESS				- success
    UTE_DS_OPEN_FAILED		- unable to open specified data source
    UTE_CONNECT_TERMINATED	- remote connection terminated
****************************************************/
int CUT_WSThread::Send(CUT_DataSource & source,long *bytesSent){

    int     len;
    char    buf[WSS_BUFFER_SIZE + 1];
    int     error = FALSE;

    *bytesSent = 0;

    // Open data source for reading
	if(source.Open(UTM_OM_READING) == -1)
        return OnError(UTE_DS_OPEN_FAILED);
    
    // Send the file
  do {
		if((len = source.Read(buf, WSS_BUFFER_SIZE - 1)) <= 0)
			break;

        if( Send(buf,len) == SOCKET_ERROR) {
            error = TRUE;
            break;
			}
        
        *bytesSent += len;

        // Send notify
        if(SendFileStatus(*bytesSent)==FALSE)
            break;
		}while (len >0);

	// Close data source
	source.Close();

    // Return an error status if the error flag is set
	if(error == TRUE)
        return OnError(UTE_CONNECT_TERMINATED);
    
    // Return success
    return OnError(UTE_SUCCESS);
}

/***************************************************
Receive
    receives information for the currently connected
    client. Up to 'maxDataLen' can be received at once.
    The receive function can block until the specified
    timeout.
Params
    data - buffer for received data
    maxDataLen - buffer length
    timeOut - timeout in seconds ( 0- for no time-out)
Return
    number of bytes received
    0 if the connection was closed or timed-out
    any other error returns SOCKET_ERROR 
****************************************************/
int CUT_WSThread::Receive(LPSTR data,int maxLen,int timeOut){
    
    if(timeOut > 0)
        if(WaitForReceive(timeOut,0) != UTE_SUCCESS)
            return 0;
    
    return SocketRecv(m_clientSocket,data,maxLen,0);
}

/***************************************************
ReceiveBlob
    Receives information for the currently connected
    client. The receive function can block until the 
	specified timeout.  Unlike Receive, this method 
	waits until all the data is received or until an 
	error occurs.
Params
    data	- buffer for received data
    dataLen - buffer length
    timeOut - timeout in seconds ( 0- for no time-out)
Return
    number of bytes received
    0 if the connection was closed or timed-out
    any other error returns SOCKET_ERROR 
****************************************************/
int CUT_WSThread::ReceiveBlob(LPBYTE data,int dataLen,int timeOut){

    LPBYTE position = data;
	int dataRemain = dataLen;

	// Try to get all the data
	while(dataRemain > 0){

		// Wait for any timeout until data is ready
		if(timeOut > 0)
			if(WaitForReceive(timeOut,0) != UTE_SUCCESS)
				break;

		// Receive any data that's available
		int received = SocketRecv(m_clientSocket,(char *)position,dataRemain,0);

		if(received == 0 && !IsConnected())
			break;

		if(received < 0)
			break;

		// Move past the data returned
		position += received;
		dataRemain -= received;

		// Check if server is shutting down
		if(m_winsockclass_this->GetShutDownFlag())
			break;
	}
    
    return (int)(position - data);
}

/***************************************************
SetReceiveTimeOut
    Sets the max. receive time-out. NOTE:This function 
    may not work on all stack implementations.
Params
    milisecs - timeout value (ms)
        a value of 0 is infinate
        a value of less than 500 is set to 500
Return
    0 - (UTE_SUCCESS) success
    any other value is an error
****************************************************/
int CUT_WSThread::SetReceiveTimeOut(long milisecs){
	int rt = setsockopt(m_clientSocket,SOL_SOCKET,SO_RCVTIMEO,(char *)&milisecs,sizeof(milisecs));
	if(rt == UTE_SUCCESS)
		m_lReceiveTimeOut = milisecs;
    return rt;
}

/***************************************************
GetReceiveTimeOut
    Gets the max. receive time-out. NOTE:This function 
    may not work on all stack implementations.
Params
    none
Return
    timeout value (ms)
****************************************************/
long CUT_WSThread::GetReceiveTimeOut() const
{
    return m_lReceiveTimeOut;
}

/***************************************************
SetSendTimeOut
    Sets the max. send time-out. NOTE:This function 
    may not work on all stack implementations.
Params
    milisecs - timeout value (ms)
    a value of 0 is infinate
    a value of less than 500 is set to 500
Return
    0 - (UTE_SUCCESS) success
    any other value is an error
****************************************************/
int CUT_WSThread::SetSendTimeOut(long milisecs){
	int rt = setsockopt(m_clientSocket,SOL_SOCKET,SO_SNDTIMEO,(char *)&milisecs,sizeof(milisecs));
	if(rt == UTE_SUCCESS)
		m_lSendTimeOut = milisecs;
    return rt;
}

/***************************************************
GetSendTimeOut
    Gets the max. send time-out. 
Params
    none
Return
    timeout value (ms)
****************************************************/
long CUT_WSThread::GetSendTimeOut() const
{
    return m_lSendTimeOut;
}

/***************************************************
SetBlockingMode
    Turns blocking mode on and off
Params
    mode -  1:no blocking  0:blocking on (default);
Return
    0 - (UTE_SUCCESS) success
    any other value is an error 
****************************************************/
int CUT_WSThread::SetBlockingMode(int mode){
    m_bBlockingMode =(BOOLEAN) mode;
    unsigned long lmode = mode; 
    return ioctlsocket(m_clientSocket,FIONBIO,&lmode);
}

/***************************************************
GetBlockingMode
    Gets current blocking mode settings
Params
    none
Return
    FALSE	- no blocking  
	TRUE	- blocking on 
****************************************************/
BOOL CUT_WSThread::GetBlockingMode() const
{
    return m_bBlockingMode;
}

/***************************************************
ReceiveLine
    Receives a line of data from the currently connected
    client, or receives until the maxDataLen is reached, or
    if the time-out has been reached.
Params
    data - buffer for incomming data
    maxDataLen - length of the buffer
    timeOut - timeout in seconds ( 0- for no time-out)
Return
    number of bytes received
    0 if the connection was closed or timed-out
    any other error returns SOCKET_ERROR 
****************************************************/
int CUT_WSThread::ReceiveLine(LPSTR data,int maxDataLen,int timeOut){

    int		x, rt, count = 0;
    LPSTR	dataPtr = data;

    maxDataLen --;

    while(maxDataLen > 0) {

        // Wait for a data up til the timeout value
        if(timeOut > 0)
            if(WaitForReceive(timeOut,0) != UTE_SUCCESS)
                return count;
        
        // Look at the data coming in
        rt = SocketRecv(m_clientSocket,(char *)dataPtr,maxDataLen, MSG_PEEK);

        // Error checking
        if(rt < 1)
            return count;
        
        // Find the fist CR/LF 
        for(x = 0; x < rt; x++) {
            if(dataPtr[x] == '\r' && dataPtr[x+1] == '\n') {
                rt = SocketRecv(m_clientSocket,(char *)dataPtr,x+2,0);
                dataPtr[x+2]=0;
                count += x+2;
				if(count == 0)
					count = 0;
                return count;
				}
			}   

        // If the CR/LF is not found then copy what is available
        rt = SocketRecv(m_clientSocket,(char *)dataPtr,rt,0);
        count += rt;
        dataPtr[rt]=0;
        dataPtr = &dataPtr[rt];
        maxDataLen -= rt;
		}

    return count;
}

/***************************************************
OnConnect
    This function needs to be overridden for the server
    to do anything useful. 
    This function is called whenever a new connection is
    made. This function should not return until the 
    connection is to be closed.
Params
    none
Return
    none
****************************************************/
void CUT_WSThread::OnConnect(){

    // This is a minimal default implementation
    long	len		= 100;
    LPSTR	data	=  new char[100];


    // Get the first line
    len = ReceiveLine(data,len);

    if(len > 0) {
        // Remove all outstanding data
        ClearReceiveBuffer();

        // Send a message back
        Send("Connect Successful ... Now Disconnecting.");
		}

    delete[] data;
}

/***************************************************
OnMaxConnect
    This function needs to be over-ridden for the server
    to do anything useful. 
    This function is called whenever a new connection is
    made and the max. number of connections has already
    been reached. This function should not return until the 
    connection is to be closed
Params
    none
Return
    none
****************************************************/
void CUT_WSThread::OnMaxConnect(){

    char data[WSS_BUFFER_SIZE + 1];
    if(Receive(data,WSS_BUFFER_SIZE) > 0)
        Send("Too Many Connections, Please Try Later.",0);
}

/***************************************************
OnUnAuthorizedConnect
    This function needs to be over-ridden for the server
    to do anything useful. 
    This function is called whenever a new connection is
    made and the client has been flagged as un-Authorized
    This function should not return until the connection 
    is to be closed
Params
    none
Return
    none
****************************************************/
void CUT_WSThread::OnUnAuthorizedConnect(){
}

/***************************************************
GetClientName
    Returns the name of the connected client connection
Params
    name - buffer where the name is copied into
    maxlen - length of the buffer
Return
    UTE_SUCCESS				- success
    UTE_BUFFER_TOO_SHORT	- buffer too short
    UTE_NAME_LOOKUP_FAILED	- failure to resolve name
****************************************************/
int CUT_WSThread::GetClientName(LPSTR name,int maxlen){

    hostent *pHostent;
    int		len;

    // Address = m_clientSocketAddr.sin_addr.s_addr  (member of CUT_WSThread)
    pHostent = gethostbyaddr((char*)&m_clientSocketAddr.sin_addr.s_addr,4,PF_INET);

    if(pHostent == NULL)
        return OnError(UTE_NAME_LOOKUP_FAILED);

    len = (int)strlen(pHostent->h_name);

    if(len > (maxlen -1))
        return OnError(UTE_BUFFER_TOO_SHORT);

    strcpy(name,pHostent->h_name);

    return OnError(UTE_SUCCESS);
}

/***************************************************
GetClientAddress
    Returns the addess of the connected client
Params
    address - buffer for the address
    maxlen - length of the buffer
Return
    UTE_SUCCESS				- success
    UTE_BUFFER_TOO_SHORT	- buffer too short
    UTE_NAME_LOOKUP_FAILED	- failure to resolve name
****************************************************/
int CUT_WSThread::GetClientAddress(LPSTR address,int maxlen){

    char *buf = inet_ntoa(m_clientSocketAddr.sin_addr);
    
    if(buf == NULL)
        return OnError(UTE_NAME_LOOKUP_FAILED);

    if(strlen(buf) > (unsigned int) (maxlen-1) )
        return OnError(UTE_BUFFER_TOO_SHORT);

    strcpy(address, buf);

    return OnError(UTE_SUCCESS);
}

/***************************************************
GetClientAddress
    Returns the addess of the connected client
Params
    none
Return
    returns the address string or NULL for an error
****************************************************/
LPCSTR CUT_WSThread::GetClientAddress(){
    return (LPCSTR) inet_ntoa(m_clientSocketAddr.sin_addr);
}

/***************************************************
IsDataWaiting
    Returns TRUE if data is waiting to be read from 
    the current connection
Params
    none
Return
    TRUE - if data is waiting
    FALSE - if no data is waiting
****************************************************/
BOOL CUT_WSThread::IsDataWaiting() const
{
    return SocketIsDataWaiting(m_clientSocket);
}

#ifdef CUT_SECURE_SOCKET
		
/***************************************************
HandleSecurityError
    Provide security errors description
Params
    lpszErrorDescription	- error description
Return
    none
****************************************************/
void CUT_WSThread::HandleSecurityError(char *lpszErrorDescription)
{	
	(CUT_WSServer *)(ULONG_PTR)m_winsockclass_this->OnStatus(lpszErrorDescription);	
}

#endif // #ifdef CUT_SECURE_SOCKET
/***************************************************
ClearReceiveBuffer
    Clears all waiting data from the receive buffer for
    the socket (current connection)
Params
    none
Return
    UTE_SUCCESS	- success
    UTE_ERROR	- error
****************************************************/
int CUT_WSThread::ClearReceiveBuffer(){

    char buf[WSS_BUFFER_SIZE];

    // Remove all other data
    while(IsDataWaiting()){
        if( Receive(buf,sizeof(buf)) <=0)
            break;
    }
    return UTE_SUCCESS;
}

/***************************************************
ReceiveToFile
    receives data directly to a file from the current
    connection.
Params
    name			- the name of the file to receive to
    fileType		- UTM_OM_WRITING:create/overwrite  
					  UTM_OM_APPEND:create/append
    bytesReceived	- number of bytes received in total
    [timeOut]		- timeOut in seconds
Return
    UTE_SUCCESS				- success
    UTE_FILE_TYPE_ERROR		- invalid file type
	UTE_DS_OPEN_FAILED		- unable to open specified data source
    UTE_SOCK_TIMEOUT		- timeout
    UTE_SOCK_RECEIVE_ERROR	- receive socket error
    UTE_DS_WRITE_FAILED		- data source write error
    UTE_ABORTED				- aborted
****************************************************/
int CUT_WSThread::ReceiveToFile(LPCTSTR name, OpenMsgType type, long* bytesReceived,int timeOut){
	CUT_FileDataSource ds(name);
	return Receive(ds, bytesReceived, type, timeOut);
}
/***************************************************
Receive
    receives data to the datasource from the current
    connection.
Params
    name			- the name of the file to receive to
    fileType		- UTM_OM_WRITING:create/overwrite  
					  UTM_OM_APPEND:create/append
    bytesReceived	- number of bytes received in total
    [timeOut]		- timeOut in seconds
Return
    UTE_SUCCESS				- success
    UTE_FILE_TYPE_ERROR		- invalid file type
	UTE_DS_OPEN_FAILED		- unable to open specified data source
    UTE_SOCK_TIMEOUT		- timeout
    UTE_SOCK_RECEIVE_ERROR	- receive socket error
    UTE_DS_WRITE_FAILED		- data source write error
    UTE_ABORTED				- aborted
****************************************************/
int CUT_WSThread::Receive(CUT_DataSource & dest, long* bytesReceived, OpenMsgType type, int timeOut) 
{
    char    data[WSS_BUFFER_SIZE];
    int     count;
    int     error = UTE_SUCCESS;

    *bytesReceived = 0;

    // Check data source open type 
    if(type != UTM_OM_APPEND && type != UTM_OM_WRITING)
        return OnError(UTE_FILE_TYPE_ERROR);

	// Open data source
	if(dest.Open(type) == -1)
		return OnError(UTE_DS_OPEN_FAILED);

    // Start reading in the data
    while(error == UTE_SUCCESS ) {
        if(timeOut > 0) 
		{
            if(WaitForReceive(timeOut, 0) != UTE_SUCCESS) 
			{
                error = UTE_SOCK_TIMEOUT;
                break;
			}
		}

        count = Receive(data,sizeof(data));

        // Check to see if the connection is closed
        if(count == 0)
            break;

        // If an error then break
        if(count == SOCKET_ERROR) {
            error = UTE_SOCK_RECEIVE_ERROR;
            break;
			}

        // Write the the data source
		if(dest.Write(data, count) != count) {
            error = UTE_DS_WRITE_FAILED;
            break;
			}

        // Count the bytes copied
        *bytesReceived += count;

        // Send notify
        if(ReceiveFileStatus(*bytesReceived) == FALSE) {
            error = UTE_ABORTED;
            break;
			}
		}

	// Close data source
	dest.Close();

    return OnError(error);
}

/***************************************************
GetMaxSend
    returns the max. number of bytes that can be
    sent at one time
Params
    none
Return
    max. number of bytes that can be sent
****************************************************/
int CUT_WSThread::GetMaxSend() const
{

    int length;
    int size = sizeof(int);
    if( getsockopt(m_clientSocket,SOL_SOCKET,SO_SNDBUF,(char *)&length,&size) ==0)
        return length;
    
    return 0;
}

/***************************************************
GetMaxReceive
    returns the max. number of bytes that can be
    received at one time
Params
    none
Return
    max. number of bytes that can be received
****************************************************/
int CUT_WSThread::GetMaxReceive() const
{

    int length;
    int size = sizeof(int);
    if( getsockopt(m_clientSocket,SOL_SOCKET,SO_RCVBUF,(char *)&length,&size) ==0)
        return length;
    
    return 0;
}

/***************************************************
SetMaxSend
    sets the max. number of bytes that can be
    sent at one time

    Note that the call to setsockopt may return 0 
    while actually setting the buf size smaller 
    than requested - double check with a call to 
    GetMaxSend

Params
    lenght - the max send size
Return
    UTE_SUCCESS - success
    any other value is an error
****************************************************/
int CUT_WSThread::SetMaxSend(int length){
    return setsockopt(m_clientSocket,SOL_SOCKET,SO_SNDBUF,(char *)&length,sizeof(int));
}

/***************************************************
SetMaxReceive
    sets the max. number of bytes that can be
    received at one time

    Note that the call to setsockopt may return 0 
    while actually setting the buf size smaller 
    than requested - double check with a call to 
    GetMaxReceive

Params
    lenght - the max receive size
Return
    0 - (UTE_SUCCESS) success
    any other value is an error
****************************************************/
int CUT_WSThread::SetMaxReceive(int length){
    return setsockopt(m_clientSocket,SOL_SOCKET,SO_RCVBUF,(char *)&length,sizeof(int));
}

/***************************************************
GetNameFromAddress
    Returns the name associated with a given address
Params
    address - address to lookup the name for
    name    - buffer for the name
    maxLen  - length of the buffer
Return
    UTE_SUCCESS				- success
    UTE_BUFFER_TOO_SHORT	- return string too short
    UTE_ERROR				- fail
****************************************************/
int CUT_WSThread::GetNameFromAddress(LPCSTR address,LPSTR name,int maxLen){

    unsigned long   addr;
    hostent FAR *   host;
    int             len;

    addr =  inet_addr (address);

    host = gethostbyaddr ((const char *)&addr,4,PF_INET);

    if(host == NULL)
        return OnError(UTE_ERROR);

    len = (int)strlen(host->h_name) +1;

    if(len > maxLen)
        return OnError(UTE_BUFFER_TOO_SHORT);

    strcpy(name,host->h_name);

    return OnError(UTE_SUCCESS);
}

/***************************************************
GetAddressFromName
    Returns the address associated with a given name
Params
    name    - name to lookup the address for
    address - buffer for the address
    maxLen  - length of the buffer
Return
    UTE_SUCCESS				- success
    UTE_BUFFER_TOO_SHORT	- return string too short
    UTE_NAME_LOOKUP_FAILED	- failure to resolve name
****************************************************/
int CUT_WSThread::GetAddressFromName(LPCSTR name,LPSTR address,int maxLen){

    in_addr         addr;
    hostent FAR *   host;
    int             len;
    char *          pChar;
    int             position;

    // Check for @ symbol
    len = (int)strlen(name);
    for(position = len; position >0;position --) {
        if(name[position]=='@') {
            position++;
            break;
			}
		}

    host = gethostbyname (&name[position]);

    if(host == NULL)
        return OnError(UTE_NAME_LOOKUP_FAILED);

    addr.S_un.S_un_b.s_b1 = host->h_addr_list[0][0];
    addr.S_un.S_un_b.s_b2 = host->h_addr_list[0][1];
    addr.S_un.S_un_b.s_b3 = host->h_addr_list[0][2];
    addr.S_un.S_un_b.s_b4 = host->h_addr_list[0][3];

    pChar = inet_ntoa (addr);

    if(pChar == NULL)
        return OnError(UTE_NAME_LOOKUP_FAILED);

    len = (int)strlen(pChar) +1;

    if(len > maxLen)
        return  OnError(UTE_BUFFER_TOO_SHORT);

    strcpy(address,pChar);

    return OnError(UTE_SUCCESS);
}


/***************************************************
WaitForReceive
    This function wiats up til the specified time 
    to see if there is data to be received.
Params
    secs - max. seconds to wait
    uSecs - max. micro seconds to wait
Return
    UTE_ERROR	- error
    UTE_SUCCESS	- success
****************************************************/
int CUT_WSThread::WaitForReceive(long secs,long uSecs)
{
	return SocketWaitForReceive(m_clientSocket, secs, uSecs);
}

/***************************************************
WaitForSend
    Waits up til the specified time to see if the
    connection is ready to send data if it is then
    it will return UTE_SUCCESS
Params
    secs  - max. seconds to wait
    uSecs - max. micro seconds to wait
Return
    UTE_ERROR	- error
    UTE_SUCCESS	- success
****************************************************/
int CUT_WSThread::WaitForSend(long secs,long uSecs){
    
    fd_set writeSet;
    struct timeval tv;

    tv.tv_sec = secs;
    tv.tv_usec =uSecs;

    FD_ZERO(&writeSet);
	
	//This will generate a warning 
	// this is internal to winsock (warning C4127: conditional expression is constant)
	// maybe you want to use 
	// #pragma warning (disable : 4127) 
	// but on the other hand maybe you don't
	
	#pragma warning( disable : 4127 )
	FD_SET(m_clientSocket,&writeSet);
	#pragma warning( default : 4127 )
	

    //wait up to the specified time to see if data is avail
    if( select(-1,NULL,&writeSet,NULL,&tv)!= 1)
        return OnError(UTE_ERROR);
    
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
int CUT_WSThread::ReceiveFileStatus(long /* BytesReceived */){
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
int CUT_WSThread::SendFileStatus(long /* BytesSent */){
    return TRUE;
}

/***************************************************
IsConnected
    This function checks to see if the client is
    still currently connected
Params
    none
Return
    TRUE - if the client is still connected
    FALSE - if the client has disconnected
****************************************************/
BOOL CUT_WSThread::IsConnected(){

    int rt1, rt2, error;

    fd_set readSet;
    struct timeval tv;

    tv.tv_sec = 0;      // do not block - for polling
    tv.tv_usec = 0;

    FD_ZERO(&readSet);  // always reinit
	
	//This will generate a warning 
	// this is internal to winsock (warning C4127: conditional expression is constant)
	// maybe you want to use 
	// #pragma warning (disable : 4127) 
	// but on the other hand maybe you don't
	#pragma warning( disable : 4127 )
    FD_SET(m_clientSocket,&readSet);
	#pragma warning( default : 4127 )
    rt1 = select(-1,&readSet,NULL,NULL,&tv);

    if(SOCKET_ERROR == rt1) {
        int err = WSAGetLastError();
        switch (err) {
        case WSANOTINITIALISED:
        case WSAENETDOWN:
        case WSAENOTSOCK:
            return FALSE;
            break;
        default:
            break;
        }                   // other possible errors:
    }                       // WSAEFAULT - WSAEINVAL - WSAEINTR(ws1.1)

    char data = 0;
    
    SetBlockingMode(CUT_NONBLOCKING);   //no blocking

	// the security version of Ultimate TCP/IP uses diffrent implemnation 
	// of Receive and in order to avoid calling it's error checking we will call the base
	// winsock recv 
	// if you use your own class or stream for using you can either 
	// change this code to call your own receive function 
	// or expose recv function of your base class that will fulfil this 
	// recv function requirment 
    rt2 = recv(m_clientSocket,(char *)&data,1, MSG_PEEK);
    
    if(rt2 == SOCKET_ERROR){
        error = WSAGetLastError();
        switch(error) {
        case WSAEWOULDBLOCK:
            SetBlockingMode(CUT_BLOCKING);  // back to blocking mode
            rt2 = 0;            // no data - set rt2 to 0 and continue
                                // Alternative - use a small
                                // timeout value in tv ....
            break;
        case WSAECONNRESET:
            SetBlockingMode(CUT_BLOCKING);  // back to blocking mode
            return  FALSE;
            break;
        case WSAEINPROGRESS:
            SetBlockingMode(CUT_BLOCKING);  // back to blocking mode
            return  TRUE;
            break;
        case WSAESHUTDOWN:
        case WSAENETDOWN:
        case WSAENETRESET:
        case WSAETIMEDOUT:          
            return  FALSE;
            break;
        case WSAEISCONN:
        case WSAENOTSOCK:
        case WSAEFAULT:
        case WSAEINTR:
        case WSAEINVAL:
        case WSAEOPNOTSUPP:
        case WSAEMSGSIZE:
            return FALSE;
            break;
        default:
            break;      // should never happen.
        }

    }

    SetBlockingMode(CUT_BLOCKING);  //blocking
    
    // final check - if we made it through the recv, we know the connection
    // was not reset (RST) - but no error will show if the remote has initiated
    // a graceful close (FIN).  

    // the select with readfds will return with 1 (rt1) if data is available or
    // the close is in progress.  The call to recv will be 0 if no data 
    // waiting, so we examine rt1 and rt2 together to determine closure.
    // For protocols using WSASendDisconnect() this behavior may need to be 
    // reexamined.

    if(0 == rt2 && 1 == rt1)
        return FALSE;

    return TRUE;

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
int CUT_WSThread::OnError(int error)
{
	return error;
}


/***********************************************
Connect
    Connects to a specified port
Params
    port		- port to connect to
    address		- address to connect to (ex."204.64.75.73")
	[timeout]	- time to wait for connection
    [family]	- protocol family: AF_INET, AF_AF_IPX, etc. Default AF_INET
    [sockType]	- SOCK_STREAM (TCP) or SOCK_DGRAM (UDP) Default is SOCK_STREAM
Return
    UTE_SOCK_ALREADY_OPEN	- socket already open or in use
    UTE_SOCK_CREATE_FAILED	- socket creation failed
    UTE_SOCK_CONNECT_FAILED - socket connection failed
	UTE_INVALID_ADDRESS		- invalid address
    UTE_SUCCESS				- success
	UTE_CONNECT_TIMEOUT		- connect time out
************************************************/
int CUT_WSThread::Connect(unsigned int port, LPCSTR address, long timeout, int family, int sockType) 
{
	int	nError = UTE_SUCCESS;

    if(m_clientSocket != INVALID_SOCKET)
        return OnError(UTE_SOCK_ALREADY_OPEN);

    //copy the params
    //check to see if the domain is a name or address

	if (inet_addr(address) == INADDR_NONE){
        if(GetAddressFromName(address, m_szAddress, sizeof(m_szAddress)) != UTE_SUCCESS)
            return OnError(UTE_INVALID_ADDRESS);
		}
    else 
		strncpy(m_szAddress, address, sizeof(m_szAddress));

	//m_nFamily    = family;
	m_nSockType  = sockType;

    //Set up the SockAddr structure
    memset(&m_clientSocketAddr, 0, sizeof(m_clientSocketAddr));				//clear all
    m_clientSocketAddr.sin_port         = (unsigned short) htons((u_short   )port);				//port
    m_clientSocketAddr.sin_family       = (short) family;					//family
    m_clientSocketAddr.sin_addr.s_addr  = inet_addr(m_szAddress);   //address

    //create a socket

	m_clientSocket = socket(family, sockType, 0);
    if(m_clientSocket == INVALID_SOCKET)
		return OnError(UTE_SOCK_CREATE_FAILED);

    
	// switch to nonblocking mode to support timeout
	if(timeout >= 0) 
		SetBlockingMode(CUT_NONBLOCKING);

    if( connect(m_clientSocket,(LPSOCKADDR)&m_clientSocketAddr,sizeof(m_clientSocketAddr))==SOCKET_ERROR){
		if(WSAGetLastError() == WSAEWOULDBLOCK ) {
			if(timeout >= 0) {
				if(WaitForSend(timeout, 0) != CUT_SUCCESS) {
					SocketClose(m_clientSocket);
					m_clientSocket = INVALID_SOCKET;
					return OnError(UTE_CONNECT_TIMEOUT);
					}

				SetBlockingMode(CUT_BLOCKING);
				}
			
			//set up the default send are receive time-outs
//			SetReceiveTimeOut(m_lRecvTimeOut);
//			SetSendTimeOut(m_lSendTimeOut);

            // save the remote port
     //       m_nRemotePort = ntohs(m_clientSocketAddr.sin_port);

            // save the local port
            SOCKADDR_IN sa;
            int len = sizeof(SOCKADDR_IN);
            getsockname(m_clientSocket, (SOCKADDR*) &sa, &len);
 //           m_nLocalPort = ntohs(sa.sin_port);

			// Call socket connection notification
			if((nError = SocketOnConnected(m_clientSocket, address)) != UTE_SUCCESS)
			{
				SocketClose(m_clientSocket);
				m_clientSocket = INVALID_SOCKET;
			}

			return OnError(nError);
			}
		else {
			// did not have these two lines prior to Feb 11 1999 
			SocketClose(m_clientSocket);
			m_clientSocket = INVALID_SOCKET;
			}

        return OnError(UTE_SOCK_CONNECT_FAILED);		// ERROR: connect unsuccessful 
    }

    // set up the default send are receive time-outs
//    SetReceiveTimeOut(m_lRecvTimeOut);
    SetSendTimeOut(m_lSendTimeOut);

	// Call socket connection notification
	if((nError = SocketOnConnected(m_clientSocket, address)) != UTE_SUCCESS)
	{
		SocketClose(m_clientSocket);
		m_clientSocket = INVALID_SOCKET;
	}


	return OnError(nError);
}


/***********************************************
CloseConnection
    Call this function to close any used and/or
    open port. 
Params
    none
Return (bitwise)
    UTE_SUCCESS		- success
	UTE_ERROR		- failed

    1 - client socket shutdown failed
    2 - client socket close failed
    3 - server socket shutdown failed
    4 - server socket close failed
************************************************/
int CUT_WSThread::CloseConnection(){

    int rt = UTE_SUCCESS;
    //if the socket is not open then just return
    if(m_clientSocket != INVALID_SOCKET){
        if(m_nSockType == SOCK_STREAM){
            if(SocketShutDown(m_clientSocket, 2) == SOCKET_ERROR){
                rt = UTE_ERROR;	//1
            }
        }
        if(SocketClose(m_clientSocket) == SOCKET_ERROR){
            rt = UTE_ERROR;		//2
        }
    }
    //clear the socket variables
    m_clientSocket        = INVALID_SOCKET;
    strcpy(m_szAddress, "");

    return OnError(rt);
}
/***********************************************
CloseConnection
    Ihrit the security attribute of the server
	to the instance of the therad 
Params
    wss  - A pointer to the server class
	wst  - a pointer to the thread class

Return 
	void
************************************************/
void CUT_WSThread::SetSecurityAttrib(CUT_WSServer *wss, CUT_WSThread *wst )
{
#ifdef CUT_SECURE_SOCKET
	wst->m_winsockclass_this = wss;
	// Set security properties
	wst->SetSecurityEnabled(wss->m_bSecureConnection);
	wst->SetCertSubjStr(wss->m_szCertSubjStr);
	wst->SetCertStoreName(wss->m_szCertStoreName);
	wst->SetSecurityProtocol(wss->m_dwProtocol);
	wst->SetMinCipherStrength(wss->m_dwMinCipherStrength);
	wst->SetMaxCipherStrength(wss->m_dwMaxCipherStrength);
	wst->SetCertValidation(wss->m_CertValidation);
	wst->SetMachineStore(wss->m_bMachineStore);
	wst->SetClientAuth(wss->m_bClientAuth);
	wst->SetCertificate(wss->m_Certificate);
#else
	UNREFERENCED_PARAMETER(wss);
	UNREFERENCED_PARAMETER(wst);
#endif // #ifdef CUT_SECURE_SOCKET


}

#pragma warning ( pop )
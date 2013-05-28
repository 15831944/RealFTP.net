//=================================================================
//  class: CUT_WSServer
//  File:  UT_SrvrS.cpp
//
//  Purpose:
//
//      CUT_WSServer - server abstract base class.  Derived classes
//      used in conjunction with derivations of CUT_WSThread to
//      implement server frameworks.
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

// these do not exist in core VC6 
#if _MSC_VER < 1400
#if !defined ULONG_PTR
#define ULONG_PTR DWORD
#define LONG_PTR DWORD
#endif
#endif

/***************************************************
CUT_WSServer
    Description
        Initialized the classes variables
    Parameters
        none
    Return
        none
****************************************************/
CUT_WSServer::CUT_WSServer() : 
	    m_nShutDownAccept(FALSE),			// Initialize shut down accept flag
	    m_nAcceptFlag(FALSE),				// Accept in progress flag
	    m_serverSocket(INVALID_SOCKET),		// ServerSocket ID
	    m_ptrThreadList(NULL),				// Empty thread list
	    m_nMaxConnections(20),				// Maximum connections at a time
	    m_nNumConnections(0),				// Current number of connections
	    m_nProtocol(0),						// Defult protocol type 0 - not specified
	    m_nPort(0xFFFF),					// Initialize port
		m_dwAcceptThread((DWORD)-1),		// Initialize thread handle
	    m_bShutDown(FALSE),					// Initialize shut down flag
		m_bCloseSocket(TRUE)				// Initialize close socket when thread is done

{
    // Setup the critical section variable
    InitializeCriticalSection(&m_criticalSection);

    // TD - allow one reference for CTOR/DTOR.
    WSAStartup(WINSOCKVER, &m_wsaData);
}

/***************************************************
~CUT_WSServer
    Description
        Shuts down all outstanding connections

        NOTE: Your derived class should provide code
                to set m_bShutDown = TRUE and wait 
                until m_nNumConnections = 0, either 
                thru its destructor or an Abort method.
                Custom CUT_WSThread::OnConnect methods
                should monitor this flag as an exit 
                condition. (m_winsockclass_this->m_bShutDown)
                See the discussion of the server framework 
                in the Server Help.

        Closes the WinSock DLL
        Deletes any allocated memory
    Parameters
        none
    Return
        none
****************************************************/
CUT_WSServer::~CUT_WSServer(){

    m_bShutDown = TRUE;

    // Shut down the accept thread
    StopAccept();

    // Close any open sockets
    if(m_serverSocket != INVALID_SOCKET) {
        int result = shutdown(m_serverSocket, 2);
        if(SOCKET_ERROR == result) {
            LPCTSTR temp = CUT_ERR::GetSocketError(&m_serverSocket); 
            OnStatus(temp);                                         // debug...
        }
        closesocket(m_serverSocket);
	}

    // Release the critical section
    DeleteCriticalSection(&m_criticalSection);

    // Shut down the socket DLL
    WSACleanup();
}

/***************************************************
    Description
    Parameters
    Return
****************************************************/
int CUT_WSServer::SetProtocol(int Protocol){
    m_nProtocol = Protocol;
    return m_nProtocol;
}

/***************************************************
    Description
    Parameters
    Return
****************************************************/
int CUT_WSServer::GetProtocol() const
{
    return m_nProtocol;
}

/***************************************************
SetShutDownFlag
    Description
		Sets shutdown flag
    Parameters
		newVal	- new flag value
    Return
		none
****************************************************/
void CUT_WSServer::SetShutDownFlag(BOOL newVal){
    m_bShutDown = newVal;
}

/***************************************************
ShutDown
    Description
		Gets shutdown flag
    Parameters
		none
    Return
		flag value
****************************************************/
BOOL CUT_WSServer::GetShutDownFlag() const
{
    return m_bShutDown;
}

/***************************************************
ConnectToPort
    Description
        Connects this class to a port and starts listening
        for incomming connections
    Parameters
        Port        Port to connect to
        QueSize Number of waiting connections that can be cached
    Return
        UTE_SUCCESS				- successful
        UTE_ERROR				- startup failed -winsock init failed
        UTE_SOCK_CREATE_FAILED	- initial socket creation failed
        UTE_SOCK_BIND_FAILED	- binding failed
        UTE_SOCK_LISTEN_ERROR	- listen failed
****************************************************/
int CUT_WSServer::ConnectToPort(unsigned short Port,int QueSize,short family,
        unsigned long address){

    // Initialize the Windows Socket DLL
    if( WSAStartup(WINSOCKVER, &m_wsaData) !=0)
        return OnError(UTE_ERROR);						//  WSAStartup unsuccessful 

    // Set up the serverSockAddr structure
    memset(&m_serverSockAddr, 0, sizeof(m_serverSockAddr)); // Clear all
    m_serverSockAddr.sin_port			= htons(Port);	// Port
    m_serverSockAddr.sin_family			= family;		// Internet family
    m_serverSockAddr.sin_addr.s_addr	= address;		// Address (any)

    // Create a socket                 
    if((m_serverSocket=socket(AF_INET,SOCK_STREAM, m_nProtocol)) == INVALID_SOCKET)
        return OnError(UTE_SOCK_CREATE_FAILED);			// ERROR: socket unsuccessful 

   	

    // Associate the socket with the address
    if(bind(m_serverSocket,(LPSOCKADDR)&m_serverSockAddr,sizeof(m_serverSockAddr))
					== SOCKET_ERROR)
        return OnError(UTE_SOCK_BIND_FAILED);			// ERROR: bind unsuccessful 

    // Allow the socket to take connections
    if( listen(m_serverSocket, QueSize ) == SOCKET_ERROR)
        return OnError(UTE_SOCK_LISTEN_ERROR);			// ERROR: listen unsuccessful 

    m_nPort = Port;

    return OnError(UTE_SUCCESS);
}

/***************************************************
StartAccept
    Description
        Allows that class to start accepting incomming
        connections. The accept routine is in it's own
        thread, so this function returns immediatly.
    Parameters
        none
    Return
        UTE_SUCCESS	- success -accept thread created and started
        UTE_ERROR	- accept thread failed
****************************************************/
int CUT_WSServer::StartAccept(){

    if(m_nAcceptFlag == FALSE) {
        m_nShutDownAccept = FALSE;

        m_dwAcceptThread = (DWORD)_beginthread(AcceptConnections,0,(VOID *)this);
        
        if (m_dwAcceptThread == (DWORD)-1)
            return OnError(UTE_ERROR);

        m_nAcceptFlag = TRUE;
        return OnError(UTE_SUCCESS);
		}

	return OnError(UTE_ERROR);
}

/***************************************************
StopAccept
    Description
        Shuts down the accept thread. It does not shutdown
        any existing connections.
    Parameters
        none
    Return
        UTE_SUCCESS	- success
        UTE_ERROR	- fail
****************************************************/
int CUT_WSServer::StopAccept(){

    if(m_nAcceptFlag == TRUE) {

        m_nAcceptFlag		= FALSE;
        m_nShutDownAccept	= TRUE;

        // Kick accept thread loop so that it
        // can test for the exit condition
        int res = TempConnect(m_nPort, inet_ntoa (m_serverSockAddr.sin_addr));
        if(UTE_SUCCESS != res) {
            // trouble - our temp connect failed.
            // accept thread still running...
			if(!TerminateThread((HANDLE)(LONG_PTR)m_dwAcceptThread, 1)) {
                // int err = GetLastError(); // For Debugging only 
			}
            m_dwAcceptThread = (DWORD)-1; 
        }

        // Wait for accept loop to terminate
        if(m_dwAcceptThread != -1) 
        { 
            WaitForSingleObject((HANDLE)(ULONG_PTR)m_dwAcceptThread,30000); 
        }

        if(m_serverSocket != INVALID_SOCKET) {
            shutdown(m_serverSocket, 2);
            closesocket(m_serverSocket);
            m_serverSocket = INVALID_SOCKET;
	    }
    }

    return UTE_SUCCESS;
}

/***************************************************
AcceptConnections
    Description
        Accept connections thread function. This thread
        creates a new thread for every incomming connection.
    Parameters
        _this       a pointer to the winsock class
    Return
        none
****************************************************/
void CUT_WSServer::AcceptConnections(void * _this){

    SOCKET          clientSocket;
    SOCKADDR_IN     clientSockAddr;
    int             addrLen = sizeof(SOCKADDR_IN);
    BOOL            bCanaccept;
    CUT_WSServer	*c = (CUT_WSServer *) _this;
    UT_THREADINFO * volatile ti;
    unsigned long   threadID;
	bool			bFlag = true;  // this flag is just to avoid warning C4127: 
									// conditional expression is constant

    do
	{
        // Accept the connection request when one is received
        if(c->m_nShutDownAccept == FALSE) 
		{
            if(INVALID_SOCKET == c->m_serverSocket) 
			{
                c->OnStatus(_T("ServerSocket invalid"));
				break;
            }
			else
			{
                clientSocket = accept(c->m_serverSocket,(LPSOCKADDR)&clientSockAddr,&addrLen);
            }
        }
        else 
		{
            // Reset flags
            c->m_nAcceptFlag=FALSE;
            //c->m_nShutDownAccept = FALSE;
          
            // Clear thread handle
            c->m_dwAcceptThread = (DWORD)-1;
            return;
        }

        // note double check for m_nShutDownAccept - see TempConnect
        if(clientSocket == INVALID_SOCKET || c->m_nShutDownAccept == TRUE) {
            // Clear thread handle
            shutdown(clientSocket, 2);
            closesocket(clientSocket);
            c->m_dwAcceptThread = (DWORD)-1;
            return;
        }

		// Call virtual function
	    bCanaccept = c->OnCanAccept((LPCSTR)inet_ntoa(clientSockAddr.sin_addr));

        // Is connection accepted?
        if(bCanaccept == FALSE )
		{
            
            // Close connection
            if(clientSocket != INVALID_SOCKET)
			{
                shutdown(clientSocket, 2);
                closesocket(clientSocket);
            }
			// Coninue accepting connections
            
			 continue;
		}
			// check that the peer IP address is not on the blocked list
		if (!c->m_aclObj.IsAddressAllowed (clientSockAddr.sin_addr))
		{
				// if the peer is on our block list then reject the connection and close the socket
				shutdown(clientSocket, 2);
                closesocket(clientSocket);
				continue;
		}
			// Coninue accepting connections
              
        

        ////////////////////////////////////////////////////////////
        // ready to create new thread to service the new connection.
        ////////////////////////////////////////////////////////////
        // Create a new thread info structure
        ti = new UT_THREADINFO;
        ti->cs = clientSocket;
        ti->ss = c->m_serverSocket;
        memcpy(&ti->csAddr,&clientSockAddr,addrLen);
        ti->winsockclass_this =(CUT_WSServer*)_this;

        // Set the max connections flag
        if(c->m_nMaxConnections <= c->m_nNumConnections)
            ti->MaxConnectionFlag = TRUE;
        else
            ti->MaxConnectionFlag = FALSE;

        // Create the thread
        threadID = (DWORD)_beginthread(ThreadEntry,0,(VOID *)ti);

        // If the thread failed then close the connection
        if(threadID <= (DWORD)0) {

            // Close the connection
            shutdown(clientSocket, 2);
            closesocket(clientSocket);

            // Delete the thread info
            delete ti;
			}
		}while (bFlag == true);
}

/***************************************************
ThreadEntry
    Description
        This function sets up a new thread class for the
        given connection. Then adds itself to the winsock
        class thread list. Next it call the thread classes
        OnConnect member. Once this member returns the socket
        is shut down, the class is removed from the thread list
        and is deleted.
    Parameters
        ptr     Pointer to a UT_THREADINFO structure
    Return
        none
****************************************************/
void CUT_WSServer::ThreadEntry(void *ptr) {
    CUT_WSThread	*CThread;
    UT_THREADINFO		*ti			= (UT_THREADINFO*)ptr;
    CUT_WSServer	*cwinsock	= (CUT_WSServer*)ti->winsockclass_this;

    // Get an instance of the CUT_WSThread class or one derived from it
    CThread = (CUT_WSThread *)cwinsock->CreateInstance();

    // Set up the params in the class
    if(CThread != NULL) {

        // Set up the class variables
        CThread->m_clientSocket = ti->cs;
        CThread->m_serverSocket = ti->ss;
        memcpy(&CThread->m_clientSocketAddr,&ti->csAddr,sizeof(SOCKADDR_IN));
        CThread->m_winsockclass_this = ti->winsockclass_this;

        // Add this thread to the thread list
        cwinsock->ThreadListAdd(CThread,GetCurrentThreadId(),ti->cs);

        // Default time-outs
        CThread->SetReceiveTimeOut(30000);
        CThread->SetSendTimeOut(30000);

#ifdef CUT_SECURE_SOCKET
		
		// Set security properties
		CThread->SetSecurityEnabled(cwinsock->m_bSecureConnection);
		CThread->SetCertSubjStr(cwinsock->m_szCertSubjStr);
		CThread->SetCertStoreName(cwinsock->m_szCertStoreName);
		CThread->SetSecurityProtocol(cwinsock->m_dwProtocol);
		CThread->SetMinCipherStrength(cwinsock->m_dwMinCipherStrength);
		CThread->SetMaxCipherStrength(cwinsock->m_dwMaxCipherStrength);
		CThread->SetCertValidation(cwinsock->m_CertValidation);
		CThread->SetMachineStore(cwinsock->m_bMachineStore);
		CThread->SetClientAuth(cwinsock->m_bClientAuth);
		CThread->SetCertificate(cwinsock->m_Certificate);

#endif // #ifdef CUT_SECURE_SOCKET

        // Call the entry function for the thread class
		if(FALSE == cwinsock->GetShutDownFlag()) {
	        if(ti->MaxConnectionFlag == FALSE)
			{
				if(CThread->SocketOnConnected(ti->cs, "") == UTE_SUCCESS)
					CThread->OnConnect();
			}
	        else
			{
		        CThread->OnMaxConnect();
			}
		}

		// Close the socket
		if(cwinsock->m_bCloseSocket || cwinsock->GetShutDownFlag() || ti->MaxConnectionFlag)
		{
			CThread->SocketShutDown(ti->cs, 2);
			CThread->SocketClose(ti->cs);
		}

        // Remove this thread from the thread list
        cwinsock->ThreadListRemove(GetCurrentThreadId());

        // Delete the thread class
        cwinsock->ReleaseInstance(CThread);
	}
    
    // Delete the thread info structure
    delete ti;

    // End the thread
    _endthread();
}

/***************************************************
Pause
    Description
    Parameters
        none
    Return
****************************************************/
int CUT_WSServer::Pause(){
    return UTE_SUCCESS;
}

/***************************************************
Resume
    Description
    Parameters
        none
    Return
        TRUE if successful
        FALSE if it failed
****************************************************/
int CUT_WSServer::Resume(){
    return UTE_SUCCESS;
}

/***************************************************
****************************************************/
int CUT_WSServer::GetNumConnections() const
{
    return m_nNumConnections;
}

/***************************************************
****************************************************/
long CUT_WSServer::OnCanAccept(LPCSTR  /* address */ ){
    return TRUE;
}

/***************************************************
    return
        new number of max connections
        -1 if failed
****************************************************/
int CUT_WSServer::SetMaxConnections(int max){
    if(max < 1 || max > 32000)
        return -1;
    
    m_nMaxConnections = max;
    return m_nMaxConnections;
}

/***************************************************
    return
        number of max connections
****************************************************/
int CUT_WSServer::GetMaxConnections() const
{
    return m_nMaxConnections;
}

/***************************************************
ThreadListAdd
    Description
        Adds a new threads information to the thread list
    Parameters
        WSThread        pointer to an instance of a CUT_WSThread class
        ThreadID        ID of the thread
        cs              client socket handle
    Return
        TRUE if successful
        FALSE if it failed
****************************************************/
int CUT_WSServer::ThreadListAdd(CUT_WSThread * WSThread, DWORD ThreadID,SOCKET cs){

    UT_THREADLIST *list;
    UT_THREADLIST *item;

    // Enter into a critical section
    EnterCriticalSection(&m_criticalSection);

    list = m_ptrThreadList;

    // Check to see if the list is empty
    if(list == NULL) {
        // Create an item for the thread list
        item			= new UT_THREADLIST;
        item->WSThread	= WSThread;
        item->ThreadID	= ThreadID;
        item->cs		= cs;
        item->prev		= NULL;
        item->next		= NULL;

        // Set the thread list pointer to the only item
        m_ptrThreadList = item;
		}
    else {
        // Create an item for the thread list
        item			= new UT_THREADLIST;
        item->WSThread	= WSThread;
        item->ThreadID	= ThreadID;
        item->cs		= cs;
        item->prev		= NULL;

        // Put this new item at the beginning of the thread list
        item->next		= list;		//point to the first item in the list
        list->prev		= item;		//point to the new item
        m_ptrThreadList = item;		//set the pointer to the new first item
		}

	// Increment the counter
    m_nNumConnections++;        

    // Exit the critical section
    LeaveCriticalSection(&m_criticalSection);

    return TRUE;
}

/***************************************************
ThreadListRemove
    Description
        Removes an item from the thread list
    Parameters
        WSThread        pointer to a CUT_WSThread class
    Return
        TRUE if successful
        FALSE if it failed
****************************************************/
int CUT_WSServer::ThreadListRemove(DWORD ThreadID){

    UT_THREADLIST *list;
    UT_THREADLIST *item;

    // Enter into a critical section
    EnterCriticalSection(&m_criticalSection);

    list = m_ptrThreadList;

    // Go through the list until the item is found or the end is reached
    while(list != NULL) {
        // Check to see if the items match
        if(list->ThreadID == ThreadID){
            // Update the links then remove the item
            item = list;
            if(item->prev != NULL) {
                list = item->prev;
                list->next = item->next;
				}

            if(item->next != NULL) {
                list = item->next;
                list->prev = item->prev;
				}

            // Check to see if this is the first item in the list
            if(item == m_ptrThreadList)
                m_ptrThreadList = item->next;

			// Release the memory
            delete item;         

			// Decrement the counter
			m_nNumConnections--;    

            // Exit the search loop
            break;
			}

        // Get the next item in the list
        list = list->next;
		}

    // Exit the critical section
    LeaveCriticalSection(&m_criticalSection);

    return TRUE;
}

/***************************************************
ThreadListSuspend
    Description
        Suspends all threads that are in the thread list.
        Ideal if running as service and responding to 
        SERVICE_PAUSE
    Parameters
        none
    Return
        TRUE if successful
        FALSE if it failed
****************************************************/
int CUT_WSServer::ThreadListSuspend(){

    UT_THREADLIST	*list;
    int				numitems =0;

    // Enter into a critical section
    EnterCriticalSection(&m_criticalSection);
    list = m_ptrThreadList;

    //go through the list and suspend them all
    while(list != NULL) {

        // Suspend the thread
        SuspendThread((HANDLE)(ULONG_PTR)list->ThreadID);

        numitems++;

        // Get the next item
        list = list->next;
		}

    // Exit the critical section
    LeaveCriticalSection(&m_criticalSection);

    return numitems;
}

/***************************************************
ThreadListResume
    Description
        Resumes all paused threads in the thread list
    Parameters
        none
    Return
        TRUE if successful
        FALSE if it failed
****************************************************/
int CUT_WSServer::ThreadListResume(){
    UT_THREADLIST	*list;
    int				numitems = 0;

    // Enter into a critical section
    EnterCriticalSection(&m_criticalSection);
    list = m_ptrThreadList;

    // Go through the list and suspend them all
    while(list != NULL) {
        
		// Resume the thread
        ResumeThread((HANDLE)(ULONG_PTR)list->ThreadID);
        numitems++;
        
		// Get the next item
        list = list->next;
		}

    // Exit the critical section
    LeaveCriticalSection(&m_criticalSection);

    return numitems;
}

/***************************************************
ThreadListKill
    Description
        Kills all the threads in the thread list
    Parameters
        none
    Return
        TRUE if successful
        FALSE if it failed
****************************************************/
int CUT_WSServer::ThreadListKill(){
    UT_THREADLIST	*list;
    UT_THREADLIST	*next;
    int				numitems = 0;

    // Enter into a critical section
    EnterCriticalSection(&m_criticalSection);
    list = m_ptrThreadList;

    // Go through the list and kill them all
    while(list != NULL) {
        next = list->next;

        // Call the thread classes StartShutdown member
        list->WSThread->StartShutdown();
        numitems++;

        // Put in a wait for thread routine, then if the thread doesn't
        // terminate within nnn seconds then automatically terminate it

        // Delete the item from the list
        delete list;

        // Get the next item
        list = next;
		}

    m_ptrThreadList		= NULL;
    m_nNumConnections	= 0;

    // Exit the critical section
    LeaveCriticalSection(&m_criticalSection);

    return numitems;
}

/***************************************************
ThreadListGetLength
    Description
        Returns the number of items in the threadlist
    Parameters
        none
    Return
        number of items
****************************************************/
int CUT_WSServer::ThreadListGetLength(){
    UT_THREADLIST	*list;
    int				numberitems = 0;

    // Enter into a critical section
    EnterCriticalSection(&m_criticalSection);

    list = m_ptrThreadList;

    // Go through the list and suspend them all
    while(list != NULL) {

        // Increment the counter
        numberitems++;

        // Get the next item
        list = list->next;
		}

    // Exit the critical section
    LeaveCriticalSection(&m_criticalSection);

    return numberitems;
}

/***********************************************
TempConnect
    Description
        Makes a temporary connection to itself
        so that it can easily shut down the accept loop
	Return
        UTE_SUCCESS				- successful
        UTE_ERROR				- startup failed -winsock init failed
        UTE_SOCK_CREATE_FAILED	- initial socket creation failed
		UTE_SOCK_CONNECT_FAILED	- connection failed
************************************************/
int CUT_WSServer::TempConnect(unsigned int Port, LPSTR Address){

    char			*c;
    unsigned long	ConnectAddr;
    SOCKADDR_IN     sockAddr;
    SOCKET          sock;
    int             retval = UTE_SUCCESS;

    // Convert the address string to a number
    ConnectAddr = inet_addr(Address);

    // Set up the SockAddr structure
    // Clear all bytes
    c = (char *)&sockAddr;
    memset(c, 0, sizeof(sockaddr));

    sockAddr.sin_port			= htons((unsigned short)Port);	// Port
    sockAddr.sin_family			= AF_INET;						// Internet family
    sockAddr.sin_addr.s_addr	= ConnectAddr;					// Address
    
    // Create a socket
    if((sock = socket(AF_INET, SOCK_STREAM, 0))== INVALID_SOCKET)
        return OnError(UTE_SOCK_CREATE_FAILED);					// ERROR: socket unsuccessful 

    if( connect(sock,(LPSOCKADDR)&sockAddr,sizeof(sockAddr)) ==SOCKET_ERROR) {
        LPCTSTR temp = CUT_ERR::GetSocketError(&sock); 
        OnStatus(temp);                                 // debug...
        retval = UTE_SOCK_CONNECT_FAILED;				// ERROR: connect unsuccessful 
    }

    // Close the socket
    int result=0;

    result = shutdown(sock, 2);
    if (SOCKET_ERROR == result) {
        LPCTSTR temp = CUT_ERR::GetSocketError(&sock); 
        OnStatus(temp);                                         // debug...
   }    
    result = closesocket(sock);
    if (SOCKET_ERROR == result) {
        LPCTSTR temp = CUT_ERR::GetSocketError(&sock); 
        OnStatus(temp);                                         // debug...
    }
    
    return OnError(retval);
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
int CUT_WSServer::OnError(int error)
{
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
int CUT_WSServer::OnStatus(LPCTSTR  /* StatusText */)
{
	return UTE_SUCCESS;
}
// overload for char * calls (e.g. e.g. client data)
#if defined _UNICODE
int CUT_WSServer::OnStatus(LPCSTR  /* StatusText */)
{
	return UTE_SUCCESS;
}
#endif

/*********************************************
SetPort
    Sets the port number to connect to
Params
    newPort		- connect port
Return
    UTE_SUCCESS - success
**********************************************/
int CUT_WSServer::SetPort(unsigned short newPort) {
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
unsigned int  CUT_WSServer::GetPort() const
{
    return m_nPort;
}

/****************************************
 Inet_Addr
 Convert string representation of dotted
 ip address to unsigned long equivalent.

 RETURN
	unsigned long equivalent of address 
	or INADDR_NONE
*****************************************/
unsigned long CUT_WSServer::Inet_Addr(LPCSTR string) {
	return inet_addr (string); 
}
#if defined _UNICODE
unsigned long CUT_WSServer::Inet_Addr(LPCWSTR string) {
	if(string != NULL) {
		size_t len = wcslen(string);
		char * str = new char[len+1];
		CUT_Str::cvtcpy(str, len+1, string);
		return inet_addr(str);
	}
	else {
		return INADDR_NONE;
	}
}
#endif // _UNICODE
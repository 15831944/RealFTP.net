// =================================================================
//  class: CUT_WSThread
//  class: CUT_WSServer
//  struct:UT_THREADLIST
//  struct:UT_THREADINFO
//  File:  Ut_Srvr.h
//
//  Purpose:
//
//  Class declarations of the server and thread base classes 
//  for the server framework.
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

#ifndef IncludeCUT_WSServer //make sure this class is only included once
#define IncludeCUT_WSServer
#pragma warning(disable:4245) /* the extern before template is a non-standard extension */
#include "UTDataSource.h"
#include "UTErr.h"
#include "ut_clnt.h"


     
//#pragma warning (disable :4239)
// =========================================================
//          WINSOCK LIBRARY & HEADER
#ifndef _WINSOCK_2_0_
    // WINSOCK 1.1
    // When you you want to include winsock 1,1
    // the you need to link or load winsock.lib 
    // (for 16 bit apps)  or wsock32.lib ( 32 bit apps)
    #pragma comment(lib, "wsock32.lib")
    #include <winsock.h>        //use for Winsock v1.x
    #define WINSOCKVER  MAKEWORD(1,1)
#else
    // WINSOCK 2.0
    // When you you want to include winsock 2.0
    // the you need to link or load ws2_32.lib 
    // (There is only a 32 bit version of it )
    #pragma comment(lib, "ws2_32.lib")
    #include <winsock2.h>     //use for Winsock v2.x
    #define WINSOCKVER    MAKEWORD(2,0)
#endif

//blocking defines
#ifndef CUT_BLOCKING
#define CUT_BLOCKING        0
#define CUT_NONBLOCKING     1
#endif
/*
//success/error  define
#define CUT_SUCCESS 0
#define CUT_ERROR   1
*/
#define	WSS_BUFFER_SIZE				256
#define	WSS_LINE_BUFFER_SIZE		1000

//classes
class CUT_WSServer;
class CUT_WSThread;

// Define base class for the server
#ifndef CUT_SERVER_BASE_CLASS
	#ifdef CUT_SECURE_SOCKET
		#include "..\Security\Include\UTSecureSocket.h"
		#define CUT_SERVER_BASE_CLASS	CUT_SecureSocketServer
	#else
		#define CUT_SERVER_BASE_CLASS	CUT_Socket
	#endif
#else
	#include "CustomSocket.h"
#endif


	// in order to allow the developer access to overiding the implementation 
	// of the Access control object 
	// we will check for a define MY_UT_ACCESS_CONTROL_CLASS
	// if this macro is not defined 
	// then we will inherit from the default access control class
#ifndef CUT_ACCESSCONTROL_BASE_CLASS
	#include "UT_AccessControl.h"
	#define	CUT_ACCESSCONTROL_BASE_CLASS 	CUT_AccessControl
#else
	// if the user developer elected to provide his/her own implementation of the access control object
	// we must include the defenition of the new class
	// this is done by 
	//first defining a constant that will contain the name of the header file	
	#include MYACCESS_CONTROL_HEADER
	// second we will inherit use the identifier of the class provided by the developer
	// YOU MUST define MY_UT_ACCESS_CONTROL_CLASS_BASE as the identifier of the new class
#endif //MY_UT_ACCESS_CONTROL_CLASS


/***************************************
CUT_WSThread
    Thread class, this class is created
    by the server class when a new connection
    comes in
****************************************/
class CUT_WSThread : public CUT_SERVER_BASE_CLASS
{

friend CUT_WSServer;

    public:

        SOCKET          m_clientSocket;			// Client socket
        SOCKADDR_IN     m_clientSocketAddr;		// Client socket address
        SOCKET          m_serverSocket;			// Server socket
        CUT_WSServer *  m_winsockclass_this;	// Pointer to the WinSock server class
        BOOLEAN         m_bIsUDP;				// Is UDP flag
		BOOLEAN         m_bBlockingMode;		// Blocking mode flag
        long            m_lReceiveTimeOut;		// Receive time-out
        long            m_lSendTimeOut;			// Send tie-out
        int             m_nShutDownFlag;		// Shut down flag
		int				m_nSockType;

		char			m_szAddress[32];

    public:
	    void SetSecurityAttrib(CUT_WSServer * wss, CUT_WSThread  *wst);
        
		// Contructors/Destructors
        CUT_WSThread();
        virtual ~CUT_WSThread();

	
		// Starts shutdown
        int		StartShutdown();
        
        // Get/Set blocking mode
        int		SetBlockingMode(int mode);
		BOOL	GetBlockingMode() const;

        // Set/Get Send time out
        int		SetSendTimeOut(long miliSecs);
		long	GetSendTimeOut() const;

		// Set/Get Receive time out
        int		SetReceiveTimeOut(long miliSecs);
		long	GetReceiveTimeOut() const;
        
		// Set/Get Socket options
        int		SetSocketOption(int option,void *optionValue, int iBufferSize = 4); // NOTE: no implementation for this function exists 
        int		GetSocketOption(int option,void *optionValue); // NOTE: no implementation for this function exists 

        // Address functions
        int     GetClientName(LPSTR name,int maxLen);
        int     GetClientAddress(LPSTR address,int maxLen);
        LPCSTR  GetClientAddress();
        int     GetNameFromAddress(LPCSTR address,LPSTR name,int maxLen);
        int     GetAddressFromName(LPCSTR name,LPSTR address,int maxLen);

        // Send functions
        virtual int     Send(CUT_DataSource & source, long *bytesSent);
        virtual int     Send(LPCSTR data, int len = -1,int flags = 0);
        virtual int     SendBlob(LPBYTE data,int dataLen,int flags = 0);
        virtual long    SendAsLine(LPCSTR data, long len, int lineLen);
        virtual int     SendFile(LPCTSTR path, long *bytesSent);
        int     GetMaxSend() const;
        int     SetMaxSend(int length);
        int     WaitForSend(long secs,long uSecs);

        // Receive functions
		virtual int		Receive(CUT_DataSource & dest, long* bytesReceived, OpenMsgType type, int timeOut = 0);
        virtual int     Receive(LPSTR data,int maxDataLen,int timeOut = 0);
        virtual int     ReceiveBlob(LPBYTE data,int dataLen,int timeOut = 0);
        virtual int     ReceiveToFile(LPCTSTR name, OpenMsgType type,long* bytesReceived,int timeOut = 0);
        virtual int     ReceiveLine(LPSTR data,int maxDataLen,int timeOut = 0);
		int     WaitForReceive(long secs,long uSecs);
        BOOL    IsDataWaiting() const;
        int     ClearReceiveBuffer();
        int     GetMaxReceive() const;
        int     SetMaxReceive(int length);
        
        // Error functions
        DWORD   GetLastError();        // NOTE: no implementation for this function exists 
        LPCSTR  GetLastErrorString();  // NOTE: no implementation for this function exists 

        // Connection
        BOOL	IsConnected();


		// Added to support connection in thread class
		// Connect. default type is TCP Stream, and family AF_INET
		virtual int Connect(unsigned int port, LPCSTR address, long timeout = -1, 
        int family = AF_INET, int sockType = SOCK_STREAM);


		// Terminate current connection 
		virtual int CloseConnection();
  


protected:        

		/////////////////////////////////////////////////////////////
        // Overridables
        /////////////////////////////////////////////////////////////
        
        virtual void OnConnect();
        virtual void OnMaxConnect();
        virtual void OnUnAuthorizedConnect();

        // Send and receive file status
        virtual int ReceiveFileStatus(long BytesReceived);
        virtual int SendFileStatus(long BytesSent);

		// This virtual function is called each time we return a value
		virtual int OnError(int error);

#ifdef CUT_SECURE_SOCKET
		
		// Function can be overwritten to handle security errors
		virtual void HandleSecurityError(char *lpszErrorDescription);

#endif // #ifdef CUT_SECURE_SOCKET

// v4.2 removed these to get rid of warnings for derived classes (VC6)
//private:
//	CUT_WSThread& operator=(const CUT_WSThread&); // no implementation
//	CUT_WSThread(const CUT_WSThread&); // no implementation
};


/**********************************
CUT_WSServer
    Main server class, this class 
    listens for connections then spins
    off threads for each connection
***********************************/
class CUT_WSServer : public CUT_SERVER_BASE_CLASS
{

    /**********************************
    UT_THREADLIST
        Structure for storing the list of
        open connections
    ***********************************/
    typedef struct UT_THREADLISTtag{
        CUT_WSThread		*WSThread;				// Pointer to the thread class
        DWORD				ThreadID;				// Thread ID
        SOCKET				cs;						// Thread socket
        UT_THREADLISTtag	*prev;					// Pointer to the previous item
        UT_THREADLISTtag	*next;					// Pointer to the next item
    }UT_THREADLIST;

    /**********************************
    UT_THREADINFO
        structure for passing informtion
        from the server to the connect 
        thread
    ***********************************/
    typedef struct UT_THREADINFOTag{
        SOCKET				cs;						// Client socket
        SOCKET				ss;						// Server socket
        SOCKADDR_IN			csAddr;					// Client socket address
        int					MaxConnectionFlag;		// Max connection flag
        CUT_WSServer		*winsockclass_this;		// Pointer to the Server class
    }UT_THREADINFO;


friend CUT_WSThread;

    protected:

        WSADATA			m_wsaData;				// Server socket information
        SOCKADDR_IN		m_serverSockAddr;		// Server socket address
        SOCKET			m_serverSocket;			// Server socket

        UT_THREADLIST	*m_ptrThreadList;		// Pointer to the start of the thread list

        DWORD			m_dwAcceptThread;		// Thread ID of accept function
        int				m_nAcceptFlag;			// Accept in progress flag
        int				m_nShutDownAccept;		// Shutdown accept flag

        int				m_nMaxConnections;		// Max. connections number
        int				m_nNumConnections;		// Connection counters

        int				m_nProtocol;			// Protocol type 
        unsigned short	m_nPort;				// Port connected to
		BOOL			m_bShutDown;			// Server shutdown flag
		BOOL			m_bCloseSocket;			// Close socket when thread is done

		CRITICAL_SECTION	 m_criticalSection;		// Critical section variables
	


	protected:
    
		///////////////////////////////
		// Thread list functions
		///////////////////////////////

		// Add thread to the list
        int		ThreadListAdd(CUT_WSThread* WSThread,DWORD ThreadID,SOCKET cs);

		// Remove thread from list
        int		ThreadListRemove(DWORD ThreadID);

		// Get num threads in list
        int		ThreadListGetLength();

		// Suspend threads
        int		ThreadListSuspend();                        

		// Resume suspended threads
        int		ThreadListResume();                     

		// Kill all and remove
        int		ThreadListKill();                           


        // Accept connection thread
        static void AcceptConnections(void *_this);

        static void ThreadEntry(void *ptr);

    public:
	  
        // Contructors/Destructors
        CUT_WSServer();
        virtual ~CUT_WSServer();

        // Set up the port to connect to
        virtual int		ConnectToPort(unsigned short port, int queSize = 5, short family = AF_INET, unsigned long address = htonl(INADDR_ANY));
        int		TempConnect(unsigned int Port, LPSTR Address);

		// Set/Get connect port
		int				SetPort(unsigned short newPort);
		unsigned int	GetPort() const;

		// Set/Get protocol
        int		SetProtocol(int protocol);
		int		GetProtocol() const;

		// Set/Get maximum connection number
        int		SetMaxConnections(int max);
        int		GetMaxConnections() const;

        // Start and stop accepting connections
        int		StartAccept();
        int		StopAccept();

        // Retrieves the current number of connections
        int		GetNumConnections() const;

		// Set/Get shutdown flag
        void	SetShutDownFlag(BOOL newVal);
		BOOL	GetShutDownFlag() const;
		CUT_ACCESSCONTROL_BASE_CLASS	 m_aclObj;			// Access Control object to manage blocked IP list 

		// added for v4.2 - winsock equivalent takes only char
		static unsigned long Inet_Addr(LPCSTR string);
#if defined _UNICODE
		static unsigned long Inet_Addr(LPCWSTR string);
#endif // _UNICODE

   

protected:        
        /////////////////////////////////////////////////////////////
        // Overridables
        /////////////////////////////////////////////////////////////

        // Pause and resume
        virtual int Pause();        //pause all threads, including the accept thread
        virtual int Resume();       //resume all threads,     "      "   "      "

        // Create class instance callback, this is where a C_WSTHREAD clss is created
        virtual CUT_WSThread* CreateInstance() = 0;
        
        // This function is called so that the instance created above can be released
        virtual void ReleaseInstance(CUT_WSThread * ptr) = 0;

        // Called for every socket trying to connect
        virtual long OnCanAccept(LPCSTR address);

		// This virtual function is called each time we have any
		// status information to display.
		virtual int OnStatus(LPCTSTR StatusText);
#if defined _UNICODE
		virtual int OnStatus(LPCSTR StatusText);
#endif

		// This virtual function is called each time we return a value
		virtual int OnError(int error);

// v4.2 removed these to get rid of warnings for derived classes (VC6)")
//private:
//	CUT_WSServer& operator=(const CUT_WSServer&); // no implementation
//	CUT_WSServer(const CUT_WSServer&); // no implementation
};
#pragma warning(default:4245) /* the extern before template is a non-standard extension */
#endif
//
//

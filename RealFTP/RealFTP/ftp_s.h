// =================================================================
//  class: CUT_FTPServer 
//  class: CUT_FTPThread
//  File:  ftp_s.h
//  
//  Purpose:
// 
//  FTP server and thread class declarations.
//
//  The primary function  of FTP is to transfer files efficiently and 
//  reliably among Hosts and to allow the convenient use of remote 
//  file storage capabilities.
//    The objectives of FTP are 
//      1) to promote sharing of files (computer
//         programs and/or data),
//      2) to encourage indirect or implicit (via
//         programs) use of remote computers, 
//      3) to shield a user from
//         variations in file storage systems among Hosts, and 
//      4) to transfer
//         data reliably and efficiently.
// 
// INFORMATION: see RFC 959 and it's refrences
// ===================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// ===================================================================

#ifndef FTP_SERVER_H
#define FTP_SERVER_H

// Definition of command IDs
typedef enum CommandIDTag {
    CMD_UNKNOWN = 0,
    USER,       PASS,       ACCT,       CWD,        CDUP,
    SMNT,       QUIT,       REIN,       PORT,       PASV,
    TYPE,       STRU,       MODE,       RETR,       STOR,
    STOU,       APPE,       ALLO,       REST,       RNFR,
    RNTO,       ABOR,       DELE,       RMD,        MKD,
    PWD,        LIST,       NLST,       SITE,       SYST,
    STAT,       HELP,       AUTH,		FEAT,		NOOP ,
	ABORT

} CommandID;

#ifndef __midl

#include <stdio.h>
#include <time.h>

#ifdef UT_DISPLAYSTATUS
    #include "uh_ctrl.h"
#endif

#ifdef __BORLANDC__
    #include <dir.h>
#else
    #include <direct.h>
#endif

// Server  and connection threads base class
#include "ut_srvr.h"
// Winsock connection class
#include "ut_clnt.h"
#ifdef CUT_SECURE_SOCKET
		typedef enum enumFtpSSLConnectionType {
			FTP_SSL_EXPLICIT,		//Explicit = SSL After we do the nogotiation
				FTP_SSL_IMPLICIT	//SSL All the way from the minute we are connecting
		} enumFtpSSLConnectionType;
#endif //



class CUT_FTPThread;

//=================================================================
//  class: CUT_FTPServer 
//=================================================================
 
class CUT_FTPServer:public CUT_WSServer{

friend CUT_FTPThread; 

private:
        char		m_szPath[MAX_PATH + 1];        // Client personal directory
        int			m_MaxTimeOut;                   // Maximum time out to wait fro client response or request
		BOOL		m_bDataPortProtection ;			// A flag to impose Data port protection to prevent bounce attacks

		
public:

        // Define added to be compatible with the examples provided with Ultimate TCP/IP 2.0
#ifdef UT_DISPLAYSTATUS
        #define			UH_THREADSAFE
        CUH_Control     *ctrlHistory;           // History control pointer
#endif

public:

        // Constructor/Destructor
						CUT_FTPServer();
        virtual			~CUT_FTPServer();

		void SetDataPortProtection(BOOL enable = TRUE);


        // Get/Set the maximum timeout to wait for a client response or request
        int				GetMaxTimeOut() const;            
        void			SetMaxTimeOut(int miliSec);            

        // Set/Get the root path 
        int				SetPath(LPCSTR);
#if defined _UNICODE
        int				SetPath(LPCWSTR);
#endif

        LPCSTR			GetPath() const;
		// v4.2 Refactored - added interface with LPSTR and LPWSTR params
		int GetPath(LPSTR path, size_t maxSize, size_t *size);
#if defined _UNICODE
		int GetPath(LPWSTR path, size_t maxSize, size_t *size);
#endif

        // Create an instance of the connections thread 
        virtual	CUT_WSThread    *CreateInstance(); // changed to virtual

        // Delete the instance of the connections thread 
        virtual void    ReleaseInstance(CUT_WSThread *ptr);


#ifdef CUT_SECURE_SOCKET
public:

		// Sets the FTP Connection to use SSL from the start or to wait for negotiation
		void	SetFtpSslConnectionType(enumFtpSSLConnectionType type = FTP_SSL_EXPLICIT){
			m_FtpSSLConnectionType = type;
		}

	
protected:
		//  IMPLICIT = SSL All the way from the minute we are connecting
		//  Explicit = SSL After we do the nogotiation
		enumFtpSSLConnectionType  m_FtpSSLConnectionType;

#endif //


protected:

        // On display status
        virtual int     OnStatus(LPCSTR StatusText);

        // Helper function. Gets name of the command by ID.
        LPCSTR          GetCommandName(CommandID ID);


};
class CUT_FTPThread ;
//=================================================================
//  class:class CUT_FTPThreadData 
// Added to allow for PASSIVE mode for Secure and none secure versions
//=================================================================
class CUT_FTPThreadData: public CUT_WSThread{

// ctor
public:
		CUT_FTPThreadData();
		~CUT_FTPThreadData();

		CUT_FTPThread *m_cutParent ;	


public:
		// port number
	     int				m_nDataPort;                    // Data winsock port
		 // prepare incoming connection
		 int				AcceptConnection();
		 //  Accept incomming connections
		 int				WaitForAccept(long secs);

		 // wait for connection to be made
		 virtual int		WaitForConnect(unsigned short port, int queSize = 1,short family = AF_INET, 
			 unsigned long address = htonl(INADDR_ANY));

		 // release current connection
		 void				ReleaseDataConnection();
protected:
	      virtual int ReceiveFileStatus(long BytesReceived);
		  virtual int SendFileStatus(long BytesSent);

};


//=================================================================
//  class:class CUT_FTPThread
//=================================================================
class CUT_FTPThread:public CUT_WSThread{

    protected: 

        char			m_szData[WSS_BUFFER_SIZE + 1];  // General purpose data buffer
        char			m_szParam[WSS_BUFFER_SIZE + 1]; // Buffer for extracting command line params
        int				m_nLen;                         // General purpose string length var
        CommandID		m_nCommand;                     // Command number

        int				m_nExit;                        // Main loop exit flag
        int				m_nLoginState;                  // FTP progress state
                                                    // 0 - user needed 1-pass needed 2 -logged in
        char			m_szPath[MAX_PATH + 1];     // Current path
        char			m_szRootPath[MAX_PATH + 1]; // Pointer to the root path string
        char			m_szUser[MAX_PATH + 1];     // User name
        char			m_szPass[MAX_PATH + 1];     // Password
         char			m_szDataIPAddress[26];          // IP address for data

        char			m_szFrom[MAX_PATH + 1];     // Used by the rename commands

        int				m_nSuccess;                     // Success flag

		BOOL			m_bHandshakeDone ;			// Check if the client had gone through a secure handshake 

		BOOL			m_bPASV; // Passive Mode

        char			m_szDataPASVAddress[26];        // Full PASV address for data

		int				m_nHiByte;                      // Hi Byte for winsock port
	    int				m_nLoByte;                      // Lo Byte for winsock port






public:
	     int			m_nDataPort;                    // Data winsock port

  
		CUT_FTPThread()	{m_bHandshakeDone = FALSE;}


protected:
        
        // Get the command ID for the passed string
        virtual CommandID GetCommand(LPSTR data);

        // Adjust the root path 
        virtual int       SetDir(LPSTR adjustPath);

        // Delete file from the server 
        virtual int       DeleteFile(LPSTR file);

        // Rename file on the server
        virtual int       RenameFile(LPSTR oldfile,LPSTR newfile);
        
        // Create a new directory on the server
        virtual int       MakeDirectory(LPSTR dir);
        
        // Remove directory from the server
        virtual int       RemoveDirectory(LPSTR dir);

        // List the directory structure 
        virtual int       ListDirectory();
        virtual int       NListDirectory();

        // Set the data port of FTP 
        virtual int       SetDataPort(LPSTR param);

        // Send a file to the client 
        virtual int       RetrFile(LPSTR filename);
        
        // Receive a file from the client
        virtual int       StorFile(LPSTR filename);
        
        virtual int       MakePath(LPSTR path, LPSTR filename, int nBufferSize);

		int				  SocketOnConnected(SOCKET s, const char *lpszName);


		virtual int		  SetDataPortPASV();

		virtual int		  CreateDataConnection();
	
		

protected:

        // By overriding this function you can set personal directory access for each user
        virtual void	  OnSetUserPath( LPSTR userPath);        

        // Invoked when the maximum connections to the server has been reached
        virtual void      OnMaxConnect();            
        
        // The entry point of the client comunication with the server
        virtual void      OnConnect();                       

        // Set the root path for the client
        int               SetRootPath(LPCSTR rootPath);


		CUT_FTPThreadData		*m_wscDataConnection;		
	

    protected:
        // Override this function to implement your own server login message
        virtual void      OnWelcome();  
        
        // Override this function to implement your own password checking routines
        virtual int      OnCheckPassword(LPSTR user,LPSTR pass);  

        // Inform us of the client attepting to connects IP address
        virtual int      OnConnectNotify(LPCSTR ipAddress);             
        
        // Inform us when the client is disconnecting
        virtual void     OnDisconnect();                             

        // Override this function to enforce level of access on
        // each command for each client, or to provide alternate processing.
        virtual int     OnCommandStart(CommandID command,LPCSTR data); 

        // Inform us when the command has been processed 
        virtual void    OnCommandFinished(CommandID command,LPCSTR data,int success); 



#ifdef CUT_SECURE_SOCKET
		BOOL			OnNonSecureConnection(LPCSTR ipAddress);
		void HandleSecurityError(char * /* lpszErrorDescription */)
		{	
		// v4.2 OnStatus inaccessible - (protected) review this
		//	(CUT_WSServer *)m_winsockclass_this->OnStatus(lpszErrorDescription);	
		}
#endif //



};

#endif  //#ifndef __midl
#endif  

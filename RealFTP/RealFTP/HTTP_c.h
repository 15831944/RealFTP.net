// =================================================================
//  class: CUT_HTTPClient
//  File:  HTTP_C.h
//
//  Purpose:
//
//  HTTP Client class declaration
//
//  Hypertext Transfer Protocol -- HTTP/1.0
//
//  RFC  1945 
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

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H


#include "ut_clnt.h"        // Ultimate TCP/IP TCP& UDP client class
#include "utstrlst.h"       // String list header
#include "uttaglst.h"       // Tag list header file
#define REALM_BUFFER_SIZE	30
#define SCHEME_BUFFER_SIZE	30
#define URL_RESOURCE_LENGTH		256
#define USERR_NAME_BUFFER_SIZE 30
#define PASSWORD_BUFFER_SIZE 30

class CUT_HTTPClient: public CUT_WSClient
{

protected:
	
	// Proxy authorization information
	BOOL m_bEnableTunnelling;
    BOOL            m_bUseProxy;                // Use proxy flag
	char*			m_szProxyPassword;
	char*			m_szProxyUserName;
	char            m_szProxyAddr[MAX_PATH+1];  // the proxy information
	int             m_nProxyPort;				// ProxyPortNumber
	int				RequestTunnel();	
	virtual int  CreateProxyBasicAuthorizationHeader(LPCSTR ProxyUserName, LPCSTR ProxyPassword,LPSTR ResultHeader );

	// connection information
    int             m_nPort;                    // the port we are comunicating with the server on
    char            m_szServerAddr[32];         // the server informations
    char            m_szServerName[MAX_PATH+1]; // server name
	int				HTTPConnect();				// perform the connection prior to sending the request
	    
    // String manipulation routine
    int				SplitURL(LPSTR url,LPSTR fileName,int fileLen);
    
	// Response functions and variables
	long            m_lMaxBodyLines;
    int             m_nTagCount;
    CUT_StringList  m_listReceiveHeader;        // Linked list of the headers recieved
    CUT_StringList  m_listReceiveBody;          // Linked list of the lines received
	long            m_lLastStatusCode;			// the last status code received from the server
    int				SetServer(LPSTR name);
	long			ParseResponseCode(LPCSTR data);
	int				ReceiveResponseBody();
	int				ReceiveResponseHeaders();
	int				GetAuthenticationParams(LPSTR scheme ,LPSTR	realm );

	// request functions and variables
	int             m_nConnectTimeout;          // the wait for connect time out 
    CUT_TagList     m_listTags;                 // member to manipulate and extract the header tags with
	CUT_StringList  m_listSendHeader;           // Linked list of the headers to send
    int				SendRequestHeaders();
	int				SendPutCommand(LPSTR targetURL, CUT_DataSource &sourceFile, BOOL bPost);
	int				SendGetCommand(LPSTR url);
	int				SendHeadCommand(LPSTR url);
	int				SendDeleteCommand(LPSTR url);
	BOOL			GetMovedLocation(LPSTR newUrl );
	

public: 
	virtual BOOL OnRedirect(LPCSTR szUrl);
	virtual BOOL OnRedirect(LPCWSTR szUrl);

	
	virtual void HandleSecurityError(char * /* lpszErrorDescription */) { /*	printf(lpszErrorDescription);*/}
	virtual void HandleSecurityError(wchar_t * /* lpszErrorDescription */) { /*	_tprintf(lpszErrorDescription);*/}

	// when a proxy is used, we must establish a tunnel if this flag is set to true.
	void			EnableTunneling(BOOL flag);


    CUT_HTTPClient();
    virtual ~CUT_HTTPClient();

    // Set/Get connect time out
    int				SetConnectTimeout(int secs);
    int				GetConnectTimeout() const;

    // Sets the connection to be through a proxy server
    void			UseProxyServer(BOOL flag);
    
    // Sets the proxy server address or name to be used when UseProxy is set to TRUE
	// v4.2 Refactored - added interface LPWSTR param
    int				SetProxy( LPCSTR proxyNameOrIP);
#if defined _UNICODE
    int				SetProxy( LPCWSTR proxyNameOrIP);
#endif 
    
	LPCSTR			GetProxy() const;
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetProxy( LPSTR proxy, size_t maxSize, size_t *size);
#if defined _UNICODE
	int GetProxy( LPWSTR proxy, size_t maxSize, size_t *size);
#endif

    // SetPort changes the port of communication between the client and the HTTP Server
    // Although HTTP uses a well known port number (80), proxy servers 
    // could be set to communicate with the client on different ports
    int				SetPort(int port);
    int				GetPort() const;
    
    // Perform the get command 
	// v4.2 Refactored - added interfaces with LPWSTR param
    virtual int     GET(LPSTR url, CUT_DataSource * dest); 
    virtual int     GET(LPSTR url, LPCTSTR desFile = NULL); 
#if defined _UNICODE
    virtual int     GET(LPWSTR url, CUT_DataSource * dest); 
    virtual int     GET(LPWSTR url, LPCTSTR desFile = NULL); 
#endif

    // Issues a head command
	// v4.2 Refactored - added interface with LPWSTR param
    virtual int     HEAD(LPSTR url);
#if defined _UNICODE
    virtual int     HEAD(LPWSTR url);
#endif

    // Delete a resource on the server side
	// v4.2 Refactored - added interface with LPWSTR param
    virtual int     DeleteFile(LPSTR url);
#if defined _UNICODE
    virtual int     DeleteFile(LPWSTR url);
#endif

    // Send a file to be created on the server or to replace a resource on the server
	// v4.2 Refactored - added interfaces with LPWSTR params
    virtual int     PUT( LPSTR url, CUT_DataSource &dest, BOOL bpost = FALSE); 
    virtual int     PUT( LPSTR targetURL, LPCTSTR sourceFile ,BOOL bpost = FALSE);
#if defined _UNICODE
    virtual int     PUT( LPWSTR url, CUT_DataSource &dest, BOOL bpost = FALSE); 
    virtual int     PUT( LPWSTR targetURL, LPCTSTR sourceFile ,BOOL bpost = FALSE);
#endif

	// send a POST command to the server
	// v4.2 Refactored - added interfaces with LPWSTR params
	virtual int     POST(LPSTR url, CUT_DataSource &dest); 
    virtual int     POST(LPSTR targetURL, LPCTSTR sourceFile);
#if defined _UNICODE
	virtual int     POST(LPWSTR url, CUT_DataSource &dest); 
    virtual int     POST(LPWSTR targetURL, LPCTSTR sourceFile);
#endif

    // Issues a custom command
	// v4.2 Refactored - added interface with LPWSTR params
    virtual int     CommandLine(LPSTR command,LPSTR url, LPCSTR data = NULL);
#if defined _UNICODE
    virtual int     CommandLine(LPWSTR command,LPWSTR url, LPCWSTR data = NULL);
#endif
    
    // Send header mime tags
    // Headers can be used for setting the user name, passwords, cookies, and User-Agent fields 
	// v4.2 Refactored - added interface with LPWSTR param
    int     AddSendHeaderTag(LPCSTR tag);
#if defined _UNICODE
    int     AddSendHeaderTag(LPCWSTR tag);
#endif   
    // Remove all added headers for the next command
    int     ClearSendHeaderTags();

    // Received data functions
    int     MaxLinesToStore(long count);
    long    GetMaxLinesToStore() const;

    // Retrive the number of lines in the server answer 
    long    GetBodyLineCount() const;
    
    // Retrieve the servers answer line by line
    LPCSTR  GetBodyLine(long index) const;
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetBodyLine(LPSTR body, size_t maxSize, int index, size_t *size);
#if defined _UNICODE
	int GetBodyLine(LPWSTR body, size_t maxSize, int index, size_t *size);
#endif

    // Saves the retrieved data to a disk file 
    virtual int     SaveToFile(LPCTSTR name);

    // Returns the number of reference tags on the page 
    int     GetLinkTagCount();

    // Retrieves the reference link tag specified by the index
    int     GetLinkTag(int index,LPSTR tag,int maxLen);
#if defined _UNICODE
    int     GetLinkTag(int index,LPWSTR tag,int maxLen);
#endif
    
    // Header information from the server response
    int		GetHeaderLineCount() const;

	LPCSTR  GetHeaderLine(int index) const;
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetHeaderLine(LPSTR header, size_t maxSize, int index, size_t *size);
#if defined _UNICODE
	int GetHeaderLine(LPWSTR header, size_t maxSize, int index, size_t *size);
#endif

    LPCSTR  GetHeaderLine(LPCSTR subString, int *pos = NULL) const;
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetHeaderLine(LPSTR header, size_t maxSize, LPCSTR subString, size_t *size, int *pos = NULL);
#if defined _UNICODE
	int GetHeaderLine(LPWSTR header, size_t maxSize, LPCSTR subString, size_t *size, int *pos = NULL);
#endif

    // Returns the Last Modified date header of the page
    LPCSTR  GetModifiedDateFromHeader();
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetModifiedDateFromHeader( LPSTR date, size_t maxSize, size_t *size);
#if defined _UNICODE
	int GetModifiedDateFromHeader( LPWSTR date, size_t maxSize, size_t *size);
#endif

    // Returns the length of the page or file from the header response
    LPCSTR  GetLengthFromHeader();
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetLengthFromHeader( LPSTR length, size_t maxSize, size_t *size);
#if defined _UNICODE
	int GetLengthFromHeader( LPWSTR length, size_t maxSize, size_t *size);
#endif

    // The content type of the required URL, file, or page 
    LPCSTR  GetContentType();
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetContentType( LPSTR contentType, size_t maxSize, size_t *size);
#if defined _UNICODE
	int GetContentType( LPWSTR contentType, size_t maxSize, size_t *size);
#endif

	// Removes the last response from memory
    int     ClearReceivedData();

	// returns the status code of the last call
	long		 GetStatusCode();
	virtual BOOL OnAccessDenied (LPCSTR realm, LPSTR userName, LPSTR password);
	virtual BOOL OnAccessDenied (LPCWSTR realm, LPWSTR userName, LPWSTR password);
	virtual int  AddBasicAuthorizationHeader(LPCSTR userName, LPCSTR password);
	virtual int  AddBasicAuthorizationHeader(LPCWSTR userName, LPCWSTR password);
	virtual void SetProxyPortNumber(int newPort);
	
	int			SetProxyPassword(LPCSTR szPass);
#if defined _UNICODE
	int			SetProxyPassword(LPCWSTR szPass);
#endif
	int			SetProxyUserName(LPCSTR szUserName);
#if defined _UNICODE
	int			SetProxyUserName(LPCWSTR szUserName);
#endif
	

};
#endif

// =================================================================
//  class: CUT_HTTPServer 
//  class: CUT_HTTPThread
//  File:  HTTP_s.h
//
//  Purpose:
//
//  HTTP Server and thread class declarations.
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

#ifndef HTTP_SEREVER_H
#define HTTP_SEREVER_H


// Command enumeration
typedef enum CommandTag {
    CUT_NA,
    CUT_GET,
    CUT_HEAD,
    CUT_POST,
    CUT_PUT,
    CUT_PLACE,
    CUT_DELETE
} Command;

#ifndef CUT_START
    #define CUT_START       1
    #define CUT_FINISH      2
#endif

#ifndef __midl

#include "ut_srvr.h"
#include <direct.h>
#include <stdio.h>
#include <time.h>

#ifdef UT_DISPLAYSTATUS 
    // since some examples uses the history control where
    // other applications you will be developing may not need the functionality of the history 
    // control window  this statement was left in a define
    // to enable it from the project setting define  UT_DISPLAYSTATUS as a preeprocessor defenition 
    // in the preprocessor category of the C++ tab (that if you are using VC5 <g>)
    #include "uh_ctrl.h"
#endif              // #ifdef UT_DISPLAYSTATUS 

class CUT_HTTPThread;

//=================================================================
//  class: CUT_HTTPClient
//=================================================================

class CUT_HTTPServer:public CUT_WSServer {

friend CUT_HTTPThread;

private:
    char            m_szPath[MAX_PATH + 1];     // Root path

protected:

    // Create class instance callback, this is where a C_WSTHREAD clss is created
    CUT_WSThread    *CreateInstance();

    // This function is called so that the instance created above can be released
    void            ReleaseInstance(CUT_WSThread *ptr);

    // On display status
    virtual int     OnStatus(LPCSTR StatusText);

public:

    // Constructor/ Destructor
    CUT_HTTPServer();
    virtual ~CUT_HTTPServer();
    
    // Set/Get the root path
    int             SetPath(LPCSTR);
#if defined _UNICODE
    int             SetPath(LPCWSTR);
#endif

	LPCSTR          GetPath() const;
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetPath(LPSTR path, size_t maxSize, size_t *size);
#if defined _UNICODE
	int GetPath(LPWSTR path, size_t maxSize, size_t *size);
#endif

    #ifdef UT_DISPLAYSTATUS 
        // The examples of a finger server in UT2.0 uses the history control.
        // Applications you will be developing may not need the functionality of the history 
        // control window.  This statement was left in as a define - to enable it from the 
        // project settings define  UT_DISPLAYSTATUS as a preprocessor defenition 
        // in the preprocessor category of the C++ tab (VC)
        CUH_Control *ctrlHistory;
    #endif      // #ifdef UT_DISPLAYSTATUS 

};

//=================================================================
//  class: CUT_HTTPThread
//=================================================================

class CUT_HTTPThread:public CUT_WSThread {

protected: 
    char            m_szData[(WSS_BUFFER_SIZE+1) * 2];          // Data
    char            m_szFileName[MAX_PATH+1];                   // File name
    char            m_szBuf[WSS_BUFFER_SIZE + 1];               // Temporary buffer
    char            m_szGMTStamp[WSS_BUFFER_SIZE + 1];          // GMT Stamp buffer
    char            m_szFileModifiedDate[WSS_BUFFER_SIZE + 1];  // File modified date buffer

    // Returns a MIME content tag based off of the given filename
    LPCSTR          GetContentType(LPSTR filename);

    // Returns a GMT time stamp string, including the date
    LPCSTR          GetGMTStamp();

    // Returns the current hour and minute as a string
    LPCSTR          GetTimeString();

    // Returns a string containing the last modified time and date of the given file
    LPCSTR          GetFileModifiedTime(HANDLE hFile);

    // Returns a number which represents the command that was in the given command line
    virtual int     GetCommandID(LPSTR command) const;

    // Makes an absolute path using the root path member variable and the given filename
    int             MakePath(LPSTR path,LPSTR file);

protected:
    
    virtual void    OnMaxConnect();

    virtual void    OnConnect(); 

    virtual void    OnSendStatus(int commandID,LPSTR fileName,int success,long tickCount);
    virtual int     OnNotify(int startFinish,int commandID,LPSTR param, int maxParamLen,int success);
	virtual	void	OnCustomCommand(int commandID, LPSTR szData, int maxDataLen);

};

#endif  // #ifndef __midl
#endif

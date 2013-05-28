// ================================================================
//  class: CUT_FingerServer
//  class: CUT_FingerThread
//  File:  Finger_S.cpp
//
//  Purpose:
//
//  Finger server and thread classes
//  Implementation of Finger protocol server
//
//  SEE ALSO RFC  1288
// ================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// ================================================================

#ifdef _WINSOCK_2_0_
    #define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
                            /* Remove this line if you are using WINSOCK 1.1 */
#endif

        
#include "stdafx.h"
#include "ut_strop.h"
#include <process.h>

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)


#ifdef UT_DISPLAYSTATUS 
            // since this examples uses the history control where
            // other applications you will be developing may not need the functionality of the history 
            // control window  this statement was left in a define
            // to enable it from the project setting define  UT_DISPLAYSTATUS as a preeprocessor defenition 
            // in the preprocessor category of the C++ tab (that if you are using VC5 <g>)
    #include "uh_ctrl.h"
#endif              // #ifdef UT_DISPLAYSTATUS 

#include "finger_s.h"


//=================================================================
//  class: CUT_FingerServer
//=================================================================

/********************************
Constructor
*********************************/
CUT_FingerServer::CUT_FingerServer() 
{
    // Initialize root path
    m_szPath[0] = 0;
    _tgetcwd(m_szPath, MAX_PATH);
    if(m_szPath[_tcslen(m_szPath) - 1] != '\\')
        _tcscat(m_szPath,_T("\\"));

}

/********************************
Destructor
*********************************/
CUT_FingerServer::~CUT_FingerServer(){

    // Shut down the accept thread
    StopAccept();

	// Wait for threads to shutdown
    m_bShutDown = TRUE;
    while(GetNumConnections())
	    Sleep(50);
}
/********************************
CreateInstance
    Returns a pointer to an instance
    of a class derived from CUT_WSThread
PARAM
    NONE
RETURN
    CUT_WSThread *  - the new created instance 
        of the thread class
*********************************/
CUT_WSThread * CUT_FingerServer::CreateInstance(){
    return new CUT_FingerThread;
}
/********************************
ReleaseInstance
    Deletes the given class which
    was derived from CUT_WSThread
PARAM
    CUT_WSThread *ptr - pointer to the class instance to be deleted
RETURN
    VOID
*********************************/
void CUT_FingerServer::ReleaseInstance(CUT_WSThread *ptr){
    if(ptr != NULL)
        delete ptr;
}

/********************************
SetPath
    Stores the path where the finger
    information is stored
PARAM
    LPCSTR path - the full path name
RETURN
    UTE_SUCCESS                 - success
    UTE_PARAMETER_INVALID_VALUE - invalid parameter
*********************************/
int CUT_FingerServer::SetPath(LPCTSTR path) {

    if(path == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);    
    
    _tcsncpy(m_szPath, path, MAX_PATH);

    if(m_szPath[_tcslen(m_szPath) - 1] != _T('\\'))
        _tcscat(m_szPath,_T("\\"));
    
    return OnError(UTE_SUCCESS);
}

/********************************
GetPath
    Gets the path where the finger
    information is stored
PARAM
    none
RETURN
    LPCSTR path - the full path name
*********************************/
LPCTSTR CUT_FingerServer::GetPath() const
{
    return m_szPath;
}

/***************************************************
OnStatus
    This virtual function is called each time we have any
    status information to display.
Params
    StatusText  - status text
Return
    UTE_SUCCESS - success   
****************************************************/
int CUT_FingerServer::OnStatus(LPCTSTR StatusText)
{
    #ifdef UT_DISPLAYSTATUS 
        ctrlHistory->AddLine(StatusText);
    #endif

    return UTE_SUCCESS;
}
// overload for char * requests 
#if defined _UNICODE
int CUT_FingerServer::OnStatus(LPCSTR StatusText)
{
    #ifdef UT_DISPLAYSTATUS 
        ctrlHistory->AddLine(StatusText);	
    #endif

    return UTE_SUCCESS;
}
#endif
//=================================================================
//  class: CUT_FingerThread
//=================================================================

/***************************************************
OnMaxConnect
    This member is called when the maximum number of
    connections allowed has already been reached
PARAM
    NONE
RETURN
    VOID    
****************************************************/
void CUT_FingerThread::OnMaxConnect(){

    char data[WSS_BUFFER_SIZE + 1];

   Send("Too Many Connections, Please Try Later.",0);
      
    _snprintf(data,sizeof(data)-1,"(%d) Max Connections Reached",
        m_winsockclass_this->GetNumConnections());
    
    ((CUT_FingerServer *)(m_winsockclass_this))->OnStatus(data);
    return;
}
/***************************************************
OnConnect
    This member is called when a connection is made and
    accepted. Do all processing here, since the connection
    is automatically terminated when this funtion returns.
    Plus the thread that this function runs in is terminated
    as well.
PARAM
    NONE
RETURN
    VOID
****************************************************/
void CUT_FingerThread::OnConnect(){

    CUT_FingerServer    *ns     = (CUT_FingerServer *)m_winsockclass_this;
    char                data[WSS_BUFFER_SIZE];
    long                len     = WSS_BUFFER_SIZE;
    _TCHAR              filename[WSS_BUFFER_SIZE];
    long                bytes;
    
    // Send as status the address of connected client
    LPSTR   szClientAddress = inet_ntoa(m_clientSocketAddr.sin_addr);
    if(szClientAddress != NULL) {
        _snprintf(data,sizeof(data)-1, "%s connected", szClientAddress);
        ns->OnStatus(data);
        }

    // Get the first line
    len = ReceiveLine(data,len);
    CUT_StrMethods::RemoveCRLF(data);

    if(len > 2)
        ns->OnStatus(data);

    if(strlen(data) > 0) {

        // Remove all outstanding data
        ClearReceiveBuffer();

        // Make sure that there are not two . in a row
        if(strstr(data,"..") != NULL) {
            Send("Invalid Request\r\n");
            ns->OnStatus(_T("Invalid Request"));
            }
        else {
            // Send the data back
            _tcscpy(filename, ns->m_szPath);

			// v4.2 using WC() macro here...
            _tcscat(filename, WC(data));

            ns->OnStatus(filename);

            if(SendFile(filename,&bytes) != UTE_SUCCESS) {
                Send("No Information Found\r\n");
                ns->OnStatus(_T("No Information Found"));
                }
            else 
                ns->OnStatus(_T("Information Sent"));
            }
        }
    else {
        // Send the default file
        _tcscpy(filename,ns->m_szPath);
        _tcscat(filename,_T("default"));
        if(SendFile(filename,&bytes) != UTE_SUCCESS) {
            Send("Information unavailable\r\n");
            ns->OnStatus(_T("Information unavailable"));
            }
        else
            ns->OnStatus(_T("Default Information Sent"));
        }

    return;
}


#pragma warning ( pop )
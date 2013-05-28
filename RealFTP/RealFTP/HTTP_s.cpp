// =================================================================
//  class: CUT_HTTPServer
//  class: CUT_HTTPThread
//  File:  HTTP_s.cpp
//
//  Purpose:
//
//  HTTP server and thread class implementations.
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

#ifdef _WINSOCK_2_0_
    #define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
                            /* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include "HTTP_s.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)


//=================================================================
//  class: CUT_HTTPServer
//=================================================================

/********************************
Constructor
*********************************/
CUT_HTTPServer::CUT_HTTPServer(){

    // Initialize root path
    m_szPath[0] = 0;
    _getcwd(m_szPath, MAX_PATH);
    if(m_szPath[strlen(m_szPath) - 1] != '\\')
        strcat(m_szPath,"\\");

}

/********************************
Destructor
*********************************/
CUT_HTTPServer::~CUT_HTTPServer(){

    // Shut down the accept thread
    StopAccept();

	// Wait for threads to shutdown
    m_bShutDown = TRUE;
    while(GetNumConnections())
	    Sleep(50);
}

/********************************
CreateInstance
*********************************/
CUT_WSThread * CUT_HTTPServer::CreateInstance(){
    return new CUT_HTTPThread;
}

/********************************
ReleaseInstance
*********************************/
void CUT_HTTPServer::ReleaseInstance(CUT_WSThread *ptr){
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
#if defined _UNICODE
int CUT_HTTPServer::SetPath(LPCWSTR path) {
	return SetPath(AC(path));}
#endif
int CUT_HTTPServer::SetPath(LPCSTR path) {

    if(path == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);    
    
    strncpy(m_szPath, path, MAX_PATH);

    if(m_szPath[strlen(m_szPath) - 1] != '\\')
        strcat(m_szPath,"\\");
    
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
LPCSTR CUT_HTTPServer::GetPath() const
{
    return m_szPath;
}

/*************************************************
GetPath()
Retrieves root server path -

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
int CUT_HTTPServer::GetPath(LPSTR path, size_t maxSize, size_t *size)
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
int CUT_HTTPServer::GetPath(LPWSTR path, size_t maxSize, size_t *size)
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
int CUT_HTTPServer::OnStatus(LPCSTR StatusText)
{
    #ifdef UT_DISPLAYSTATUS 
        ctrlHistory->AddLine(StatusText);
	 #endif

    return UTE_SUCCESS;
}

//=================================================================
//  class: CUT_HTTPThread
//=================================================================

/***************************************************
OnMaxConnect
    This member is called when the maximum number of
    connections allowed has already been reached

****************************************************/
void CUT_HTTPThread::OnMaxConnect(){

   Send("Too Many Connections, Please Try Later.",0);

    _snprintf(m_szData,sizeof(m_szData)-1,"(%d) Max Connections Reached",
        m_winsockclass_this->GetNumConnections());
    
    ((CUT_HTTPServer *)m_winsockclass_this)->OnStatus(m_szData);

    ClearReceiveBuffer();

    return;
}

/***************************************************
OnConnect
    This member is called when a connection is made and
    accepted. Do all processing here, since the connection
    is automatically terminated when this function returns.
    Plus the thread that this function runs in is terminated
    as well.
    This function is the main engine that process all clients 
    commands.
PARAM
    NONE
RETURN
    VOID
****************************************************/
void CUT_HTTPThread::OnConnect(){

    char    command[30];
    int     success = TRUE;
    
    // Clear the data variables
    m_szFileName[0] = 0;
    m_szBuf[0]      = 0;
    m_szData[0]     = 0;

    // Get the current tick count
    long tickCount = GetTickCount();

    // Get the first line
    int len = ReceiveLine(m_szData, WSS_BUFFER_SIZE*2);

    // Break on error or disconnect
    if(len <= 0)
        return;

    // Remove the CRLF pair from the string
    CUT_StrMethods::RemoveCRLF(m_szData);

    // Parse the line
    if(CUT_StrMethods::ParseString(m_szData," ",0,command,30) != UTE_SUCCESS) {

        // Command not reconized
        Send("Command Not Reconized\r\n");
        return;
        }

    // Get the command ID
    int commandID = GetCommandID(command);

    if(commandID == CUT_NA) {
        Send("Command Not Reconized\r\n");
        return;
        }

    //**********************************
    // Check for GET
    //**********************************
    if(commandID == CUT_GET || commandID == CUT_HEAD ){

        // Get the filename
        if(CUT_StrMethods::ParseString(m_szData," ?",1,m_szBuf, WSS_BUFFER_SIZE) != UTE_SUCCESS) {
            // Command not reconized
            Send("Command Not Reconized\r\n");
            return;
            }

        // Get the absolute path
        if(MakePath(m_szFileName,m_szBuf) != UTE_SUCCESS) {
            Send("HTTP/1.0 404 Requested file not found\r\n");
            return;
            }

        // Send a start notification
        if(OnNotify(CUT_START,commandID,m_szFileName,sizeof(m_szFileName),success) == FALSE) {
            // If FALSE is returned then do not continue
            ClearReceiveBuffer();
            return;
            }

        // Open the file
		// v4.2 using CreateFileA here
        HANDLE hFile = CreateFileA(m_szFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        if(hFile ==  INVALID_HANDLE_VALUE) {
            Send("HTTP/1.0 404 Requested file not found\r\n");
            success = FALSE;
            }
        else {
            // Send the MIME header
            Send("HTTP/1.0 200 OK\r\n");
            _snprintf(m_szBuf,sizeof(m_szBuf)-1,"Date:%s\r\n",GetGMTStamp());
            Send(m_szBuf);
            Send("Server: The Ultimate Toolbox WWW Server\r\n");
            Send("MIME-verion: 1.0\r\n");
            _snprintf(m_szBuf,sizeof(m_szBuf)-1,"Content-type: %s\r\n",GetContentType(m_szFileName));
            Send(m_szBuf);
            _snprintf(m_szBuf,sizeof(m_szBuf)-1,"Last-modified: %s\r\n",GetFileModifiedTime(hFile));
            Send(m_szBuf);
            DWORD hiVal;
            long OrigFileSize = GetFileSize(hFile,&hiVal);
            _snprintf(m_szBuf,sizeof(m_szBuf)-1,"Content-length: %ld\r\n",OrigFileSize);
            Send(m_szBuf);
            Send("\r\n");

            // Send the file only for a GET
            if(commandID == CUT_GET) {

                // Get the max send size to send at once
                len = GetMaxSend();
                if(len > WSS_BUFFER_SIZE*2)
                    len = WSS_BUFFER_SIZE*2;       

                // Send the file
                DWORD   readLen;
                int     rt;
                long    totalLen = 0;

				// v4.2 change - was while(1) - gave conditional expression is constant warning
				for (;;) {
                    rt = ReadFile(hFile,m_szData,len,&readLen,NULL);
                    if(rt == FALSE || readLen ==0)
                        break;
                    if(Send(m_szData,readLen) == SOCKET_ERROR)
						break;
                    totalLen+= readLen;
                    }

                if(totalLen != OrigFileSize)
                    success = FALSE;
                }
            CloseHandle(hFile);
            ClearReceiveBuffer();
            }

        // Send a finish notification
        OnNotify(CUT_FINISH,commandID,m_szFileName,sizeof(m_szFileName),success);
        }

    //**********************************
    // Check for POST
    //**********************************
    else if(commandID == CUT_POST) {
        
        while(IsDataWaiting()) {
            len = ReceiveLine(m_szData, WSS_BUFFER_SIZE*2);
            CUT_StrMethods::RemoveCRLF(m_szData);
            ((CUT_HTTPServer *)m_winsockclass_this)->OnStatus(m_szData);
            }
        }

    //**********************************
    // Check for PUT
    //**********************************
    else if(commandID == CUT_PUT){
		Send("HTTP/1.0 501 Not Implemented\r\n\r\n");
	    Send("Server: The Ultimate Toolbox WWW Server\r\n");
        Send("MIME-verion: 1.0\r\n");
		return;

        }

    //**********************************
    // Check for PLACE
    //**********************************
    else if(commandID == CUT_PLACE) {
		Send("HTTP/1.0 501 Not Implemented\r\n\r\n");
		return;

        }

    //**********************************
    // Check for DELETE
    //**********************************
    else if(commandID == CUT_DELETE) {

        // Get the filename
        if(CUT_StrMethods::ParseString(m_szData," ",1,m_szBuf, WSS_BUFFER_SIZE) != UTE_SUCCESS) {
            // Command not reconized
            Send("HTTP/1.0 400 Bad Request\r\n\r\n");
            return;
            }

        // Get the absolute path
        if(MakePath(m_szFileName,m_szBuf) != UTE_SUCCESS) {
            Send("HTTP/1.0 404 Requested file not found\r\n\r\n");
            return;
            }

        // Send a start notification
        if(OnNotify(CUT_START,commandID,m_szFileName,sizeof(m_szFileName),success) == FALSE) {
            // If FALSE is returned then do not continue
			Send("HTTP/1.0 405 Method Not Allowed\r\n");
			Send("Content-Type: text/html\r\n\r\n");			
			Send("<html><body>DELETE Method Not Allowed</body></html>");			
            ClearReceiveBuffer();
            return;
            }

        // Send the finish notify
		// v4.2 using DeleteFileA here
        if(DeleteFileA(m_szFileName) == FALSE)
		{
			Send("HTTP/1.0 500 Internal Server Error\r\n");
		    success = FALSE;
			return ;
		}

        // Send a start notification
        OnNotify(CUT_FINISH,commandID,m_szFileName,sizeof(m_szFileName),success);
        }

    //**********************************
    // Allow handling for custom commands
    //**********************************
	else
	{
		// Get the rest of the data after the command
		char* pData = strstr(m_szData, " ");
		if (pData)
			strncpy(m_szBuf, pData+1, sizeof(m_szBuf));

		if(OnNotify(CUT_START,commandID,m_szBuf,sizeof(m_szBuf),success) == FALSE) {
			// If FALSE is returned then do not continue
			ClearReceiveBuffer();
			return;
		}

		OnCustomCommand(commandID,m_szBuf,WSS_BUFFER_SIZE);

        // Send a finish notification
        OnNotify(CUT_FINISH,commandID,m_szBuf,sizeof(m_szBuf),success);

		//Returns so the default OnSendStatus() implementation is not called.
		// If needed OnStatus() can be called in the OnNotify(CUT_FINISH)!
		return;
	}


    // Send a status line to the server history window
    OnSendStatus(commandID,m_szFileName,success,tickCount);
}

/***************************************
OnCustomCommand
    Allows for handling of custom HTTP commands
Params
    commandID	- command ID as returned by GetCommandID
	szData		- the entire command line
	maxDataLen	- length of the data string
	szFileName	- file name requested
	maxFileNameLen
Return
	VOID
****************************************/
void CUT_HTTPThread::OnCustomCommand( int /* commandID */, LPSTR /* szData */, int /* maxDataLen */)
{
}

/***************************************
GetCommandID
    Returns a number which represents
    the command that was in the given
    command line
Params
    command - command line string
Return
    command number
        CUT_GET,CUT_HEAD,CUT_POST,CUT_PLACE
        CUT_PUT,CUT_DELETE
        or for an unreconized command CUT_NA
****************************************/
int CUT_HTTPThread::GetCommandID(LPSTR command) const
{
    
    _strupr(command);
    
    if(strstr(command,"GET") != NULL)
        return CUT_GET;
    else if(strstr(command,"HEAD") != NULL)
        return CUT_HEAD;
    else if(strstr(command,"POST") != NULL)
        return CUT_POST;
    else if(strstr(command,"PUT") != NULL)
        return CUT_PUT;
    else if(strstr(command,"PLACE") != NULL)
        return CUT_PLACE;
    else if(strstr(command,"DELETE") != NULL)
        return CUT_DELETE;
    return CUT_NA;
}

/***************************************
GetContentType
    Returns a MIME content tag based off
    of the given filename
Params
    filename - filename to check
Return
    MIME tag
****************************************/
LPCSTR CUT_HTTPThread::GetContentType(LPSTR filename){
    int len,loop;

    // Find the last period
    len = (int)strlen(filename);
    for(loop = len; loop > 0; loop--) {
        if(filename[loop]=='.')
            break;
        }

    if((len - loop ) > 60)
        m_szBuf[0]=0;
    else
        strcpy(m_szBuf,&filename[loop+1]);
    
    _strupr(m_szBuf);

    if(strcmp(m_szBuf,"HTM")==0)
        return "text/html";
    else if(strcmp(m_szBuf,"HTML")==0)
        return "text/html";
    else if(strcmp(m_szBuf,"TXT")==0)
        return "text/text";

    else if(strcmp(m_szBuf,"GIF")==0)
        return "image/gif";
    else if(strcmp(m_szBuf,"JPG")==0)
        return "image/jpeg";
    else if(strcmp(m_szBuf,"JPEG")==0)
        return "image/jpeg";
    else if(strcmp(m_szBuf,"BMP")==0)
        return "image/xbitmap";

    else if(strcmp(m_szBuf,"ZIP")==0)
        return "application/octet-stream";
    else if(strcmp(m_szBuf,"EXE")==0)
        return "application/octet-stream";
    else if(strcmp(m_szBuf,"DOC")==0)
        return "application/msword";

    else if(strcmp(m_szBuf,"AU")==0)
        return "audio/au";
    else if(strcmp(m_szBuf,"WAV")==0)
        return "audio/wav";

    else if(strcmp(m_szBuf,"MPEG")==0)
        return "vidio/mpeg";
    else if(strcmp(m_szBuf,"MPG")==0)
        return "vidio/mpeg";
    else if(strcmp(m_szBuf,"AVI")==0)
        return "vidio/avi";

    return "application/octet-stream";
}

/**********************************************
GetGMTStamp
    Returns a GMT time stamp string, including
    the date
Params
    none
Return
    GMT time stamp string
***********************************************/
LPCSTR CUT_HTTPThread::GetGMTStamp(){

    time_t      t;
    struct tm   *systime;
    static char     *days[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static char     *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
                            "Sep","Oct","Nov","Dec"};
    _tzset();
 
    // Get the time/date
    t = time(NULL);
    systime = gmtime(&t);   // Fails after 2037 - t will be NULL (0)
  
    _snprintf(m_szGMTStamp,sizeof(m_szGMTStamp)-1,"%s, %2.2d %s %d %2.2d:%2.2d:%2.2d GMT",
        days[systime->tm_wday],
        systime->tm_mday, months[systime->tm_mon], systime->tm_year+1900,
        systime->tm_hour, systime->tm_min, systime->tm_sec);
    
    return (LPCSTR) m_szGMTStamp;
}

/**********************************************
GetTimeString
    Returns the current hour and minute as a
    string
Params 
    none
Return
    current hour and minute string
***********************************************/
LPCSTR CUT_HTTPThread::GetTimeString(){

    // Get the time/date
    time_t t = time(NULL);
    struct tm * systime = localtime(&t);

    strftime(m_szGMTStamp,64,"%H:%M",systime);

    return (LPCSTR) m_szGMTStamp;
}

/**********************************************
GetFileModifiedTime
    Returns a string containing the last modified
    time and date of the given file
Params
    hFile - handle to an open file
Return
    Time and date string
***********************************************/
LPCSTR CUT_HTTPThread::GetFileModifiedTime(HANDLE hFile){

    FILETIME ct;
    FILETIME lat;
    FILETIME lwt;
        
    static char *months[]={"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
                    "Sep","Oct","Nov","Dec"};
    static char *wkDays[] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
    
    if(GetFileTime(hFile,&ct,&lat,&lwt)== FALSE)
        return "";

    SYSTEMTIME st;
    FileTimeToSystemTime(&lwt,&st);

    _snprintf(m_szFileModifiedDate,sizeof(m_szFileModifiedDate)-1,"%s, %d %s %d %d:%d:%d %s",
        wkDays[st.wDayOfWeek],
        st.wDay,
        months[st.wMonth],
        st.wYear,
        st.wHour,
        st.wMinute,
        st.wSecond,
        "GMT");

    return (LPCSTR)m_szFileModifiedDate;
}

/**********************************************
MakePath
    Makes an absolute path using the root path
    member variable and the given filename
Params
    path - buffer for the absolute path
    file - file to add into the path
Return
    UTE_SUCCESS - success
    UTE_ERROR   - error
***********************************************/
int CUT_HTTPThread::MakePath(LPSTR path,LPSTR file){
    
    strcpy(path,((CUT_HTTPServer *)m_winsockclass_this)->GetPath());
    
    if(file[0] == '\\' || file[0] == '/') {
        if(file[1] == 0)
            strcat(path,"default.htm");
        else
            strcat(path,&file[1]);
        }
    else
        strcat(path,file);

    if(strstr(file,"..") != NULL)
        return OnError(UTE_ERROR);

    return OnError(UTE_SUCCESS);
}

/**********************************************
OnSendStatus
    This function is called after a command is
    processed. The command that was processed is
    passed in as well as the name of the file that
    the command was working on, the success state
    of the command and the time it took to perform
    the task
Params
    commandID  - command that was processed
    fileName   - the file that the command processed
    success    - the success state of the command (TRUE/FALSE)
    tickCount  - time it took to perform the command (ms)
Return 
    none
***********************************************/
void CUT_HTTPThread::OnSendStatus(int commandID,LPSTR fileName,int success,long tickCount){
    static char *szCommands[] = {"N/A","GET","HEAD","POST","PUT","PLACE","DELETE"};
    
    tickCount = GetTickCount() - tickCount;
    
    _snprintf(m_szBuf,sizeof(m_szBuf)-1,"%d Time=%-6.6s IP=%-15.15s Duration=%-7.7ld %s=%s",
        success,GetTimeString(),GetClientAddress(),tickCount,
        szCommands[commandID],fileName);
    
    ((CUT_HTTPServer *)m_winsockclass_this)->OnStatus(m_szBuf);
}

/**********************************************
OnNotify
    This function is called before and after a command
    is processed. When a command is just starting a return
    value of TRUE will allow the command to continue, FALSE
    will deny the command. The 'param' parameter contains 
    the rest of the command line following the command itself;
    This parameter can be modified before returning. This is 
    very useful in redirecting GET commands.
Params
    startFinish - TRUE if the command is just starting
                  otherwise FALSE
    commandID   - command starting or ending
    param       - the rest of the command line
    maxParamLen - the max. length of the param buffer
    success     - success flag (used when finishing only)
Return 
    TRUE    - continue
    FALSE   - abort command
***********************************************/
int CUT_HTTPThread::OnNotify(int /* startFinish */, int commandID, LPSTR /* param */,int /* maxParamLen */,
                             int /* success */){

    if(commandID == CUT_DELETE)
        return FALSE;
    return TRUE;
}


#pragma warning ( pop )
//=================================================================
//  class: CUT_HTTPClient
//  File:  HTTP_c.cpp
//
//  Purpose:
//
//  HTTP Client Class 
//  Implementation of Hypertext Transfer Protocol -- HTTP/1.0
//
//  RFC  1945
//	Updated to support RFC2817 May 2000
//      
//=================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
//=================================================================

#ifdef _WINSOCK_2_0_
    #define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
                            /* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include <stdio.h>

#include    "HTTP_c.h"      // HTTP client header file
#include	"base64.h"

#include    "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

/*********************************************
Constructor
**********************************************/
CUT_HTTPClient::CUT_HTTPClient() :
        m_nPort(80),                // Set default connecting port
        m_lMaxBodyLines(16000),     // Set default body lines
        m_nTagCount(-1),            // Initialize tag count
        m_bUseProxy(FALSE),         // Set by default - not using proxy
        m_nConnectTimeout(5),        // Set default connection time out to 5 sec.
		m_lLastStatusCode(0)	,// nothing received from the server yet
		m_nProxyPort(80),
		m_bEnableTunnelling(FALSE)
{
    m_szServerName[0]   = 0;
    m_szServerAddr[0]   = 0;
    m_szProxyAddr[0]    = 0;
	m_szProxyUserName = NULL;
	m_szProxyPassword = NULL;


}

/*********************************************
Destructor
**********************************************/
CUT_HTTPClient::~CUT_HTTPClient()
{
    m_listSendHeader.ClearList();       // Clear send header list
    m_listReceiveHeader.ClearList();    // Clear receive header list
    m_listReceiveBody.ClearList();      // Clear receive body list
    m_listTags.ClearList();             // Clear tags list
	if(m_szProxyPassword != NULL)
		delete m_szProxyPassword ;
	if(m_szProxyUserName != NULL)
		delete m_szProxyUserName;


}

/*********************************************
SplitURL:
    (Internal Function )
    parses the url passed to find out the  target server and
    the requested file absolute path.  HTTP URL format:
	"http:" "//" host [ ":" port ] [ abs_path [ "?" query ]] 
PARAM:
    LPSTR url      - The origional url buffer
    LPSTR fileName - requested file name 
    int fileLen    - length of the file name    
Side Effects;
    - sets the server address
	- Sets the Port number if found
Return:
    UTE_SUCCESS                 - the operation was successfull
    UTE_SERVER_SET_NAME_FAILED  - Unable to set the server name 
**********************************************/
int CUT_HTTPClient::SplitURL(LPSTR url,LPSTR fileName,int fileLen){

    int     len = (int)strlen(url);
    int     index = 0;
    int     error = UTE_SUCCESS;
    char    server[MAX_PATH+1] ="";
	BOOL	bPortIncluded = FALSE;

    //put all the slashes in the same direction
    for(int loop = 0; loop < len;loop++){
        if(url[loop] == '\\')
            url[loop] = '/';
    }

    //check to see if there is a HTTP://
    if(strstr(url,"://") != NULL)
        index = 1;

    //parse the string to get the server name
    if(CUT_StrMethods::ParseString(url,"/?%%",index,server,sizeof(server)) != UTE_SUCCESS)
        error = UTE_SERVER_SET_NAME_FAILED;     // Unable to find the server in the string

	// At this point the "server" variable contains information on the server
	// as well as the port number that the client chooses to connect on (if specified).
	// Find the port number:
	if ( CUT_StrMethods::GetParseStringPieces (server,":") > 1 )
	{
		char temp[MAX_PATH];
		long port = m_nPort;
		// extract the port number from the server name
		if(CUT_StrMethods::ParseString(server,":",1,temp,MAX_PATH) == UTE_SUCCESS)
		{
			if(*temp != '/')
			{
				if (CUT_StrMethods::ParseString(temp,"/",0,&port)  == UTE_SUCCESS)
				{
					SetPort(port);
					bPortIncluded = TRUE;
					// Remove the port information from the server name
					CUT_StrMethods::ParseString(server,":",0,temp,MAX_PATH);
					strcpy( server, temp );
				}
			}
		}
	}
    
    //set the server name
    if( SetServer(server) != UTE_SUCCESS)
        error = UTE_SERVER_SET_NAME_FAILED;     // Unable to set the server name 

    //parse the string to get the filename name
    LPSTR buf = strstr(url,server) + strlen(server);
	
	// if the port number is included then lets move forward the pointer for the 
	// file name as many bytes as the port number digits + : 
	if (bPortIncluded )
	{		
		char portString[20];
		_itoa( m_nPort,portString, 10);
		buf = buf + strlen(portString) +1; // the port number number plus the : 
	}


    if(CUT_StrMethods::ParseString(buf,"#",0,fileName,fileLen) != UTE_SUCCESS)
        fileName[0] = 0;
    
    return OnError(error);
}

/*********************************************
SetServer
    Sets the name or address of the web server
    to use.
Params
    name - the name or address of the web server
            to use
Return
    UTE_SUCCESS             - success
    UTE_PARAMETER_TOO_LONG  - param too long
    UTE_ERROR               - failure
**********************************************/
int CUT_HTTPClient::SetServer(LPSTR name){

    //check to see if the same name was already set
    if(strcmp(name, m_szServerName) == 0)
        return UTE_SUCCESS;
        
    //check to see if the given name is too long
    if( strlen(name) > 255)
        return OnError(UTE_PARAMETER_TOO_LONG);
    
    //copy the given name
    strncpy(m_szServerName, name, sizeof(m_szServerName));

    //clear the address variable
    m_szServerAddr[0] = 0;

    //check to see if the name given is an IP address
    if(IsIPAddress(name) == TRUE){
        if(strlen(name) < sizeof(m_szServerAddr))
		{
			strcpy(m_szServerAddr,name);
			return OnError(UTE_SUCCESS);
		}
        else
            return OnError(UTE_PARAMETER_TOO_LONG);
    }

    if(GetAddressFromName(name, m_szServerAddr, sizeof(m_szServerAddr)) != UTE_SUCCESS)
        return OnError(UTE_ERROR);

    return OnError(UTE_SUCCESS);
}

/*********************************************
SetPort
    Sets a new communication port
Params
    port        - new port number (default 80)
Return
    UTE_SUCCESS - success
    UTE_ERROR   - failure
**********************************************/
int CUT_HTTPClient::SetPort(int port){
    
    if(port < 0)
        return UTE_ERROR;

    m_nPort = port;
    return UTE_SUCCESS;
}

/*********************************************
GetPort
    Sets a new communication port
Params
    none
Return
    communication port
**********************************************/
int CUT_HTTPClient::GetPort() const
{
    return m_nPort;
}

/*********************************************
AddSendHeaderTag
    Adds a tag to the list of MIME tags to send
    as part of any command sent
Params
    tag - tage string to add to the list
Return
    TRUE - SUCCESS
    FALSE - FAiled
**********************************************/
#if defined _UNICODE
int CUT_HTTPClient::AddSendHeaderTag(LPCWSTR tag){
	return AddSendHeaderTag(AC(tag));}
#endif
int CUT_HTTPClient::AddSendHeaderTag(LPCSTR tag){
    int ret = FALSE;

	//check for a CRLF
    int len = (int)strlen(tag);
    if(tag[len-1]!= '\n' && tag[len-2] !='\r'){
        LPSTR buf = new char[len+3];
        strcpy(buf,tag);
        strcat(buf,"\r\n");
        ret = m_listSendHeader.AddString(buf);
		delete buf;
		return ret;
    }
    else
        return m_listSendHeader.AddString(tag);
}

/*********************************************
ClearSendHeaderTags
    Clears all MIME tags from the tag list
Params
    none
Return
    TRUE    - success
    FALSE   - error
**********************************************/
int CUT_HTTPClient::ClearSendHeaderTags(){
    return m_listSendHeader.ClearList();
}

/*********************************************
MaxLinesToMemStore
    Sets the maximum number of lines to store
    in memory when retrieving information from
    a server. If more lines are sent then then
    are truncated.
Params
    count - max. lines to store
Return
    UTE_SUCCESS - success
    UTE_ERROR   - error   
**********************************************/
int CUT_HTTPClient::MaxLinesToStore(long count){
    
    if(count <=0)
        return UTE_ERROR;

    m_lMaxBodyLines = count;
    return UTE_SUCCESS;
}
/*********************************************
GetMaxLinesToMemStore
    Sets the maximum number of lines to store
    in memory when retrieving information from
    a server. If more lines are sent then then
    are truncated.
Params
    count - max. lines to store
Return
    UTE_SUCCESS - success
    UTE_ERROR   - error   
**********************************************/
long CUT_HTTPClient::GetMaxLinesToStore() const
{
    return m_lMaxBodyLines;
}
/*********************************************
Params
    none
Return
    UTE_SUCCESS - success
    UTE_ERROR   - error   
**********************************************/
int CUT_HTTPClient::ClearReceivedData(){
    
    int rt = UTE_SUCCESS;
    
    if(m_listReceiveBody.ClearList() == FALSE)
        rt = UTE_ERROR;
    if(m_listReceiveHeader.ClearList() == FALSE)
        rt = UTE_ERROR;

    m_nTagCount = -1;
    return rt;
}

/*********************************************
GetBodyLineCount
    Returns the number of lines in memory storage
Params
    none
Return
    the number of lines in memory storage
**********************************************/
long CUT_HTTPClient::GetBodyLineCount() const
{
    return m_listReceiveBody.GetCount();
}

/*********************************************
GetBodyLine
    Retrieve the servers answer line by line
Params
    index - 0 based index of line to get
Return
    pointer to the string or NULL
**********************************************/
LPCSTR CUT_HTTPClient::GetBodyLine(long index) const
{
    return m_listReceiveBody.GetString(index);
}
/*************************************************
GetBodyLine()
Retrieve the servers answer line by line
PARAM
header	- [out] pointer to buffer to receive title
maxSize - length of buffer
index	- index of the line
size    - [out] length of line

  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- body and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - body not found
  UTE_BUFFER_TOO_SHORT  - space in body buffer indicated by maxSize insufficient, realloc 
  based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
int CUT_HTTPClient::GetBodyLine(LPSTR body, size_t maxSize, int index, size_t *size) {
	
	int retval = UTE_SUCCESS;
	
	if(body == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetBodyLine(index);
		
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
				strcpy(body, str);
			}
		}
	}
	return retval;	
}
#if defined _UNICODE
int CUT_HTTPClient::GetBodyLine(LPWSTR body, size_t maxSize, int index, size_t *size) {
	
	int retval;
	
	if(maxSize > 0) {
		char * bodyA = new char [maxSize];
		
		if(bodyA != NULL) {
			retval = GetBodyLine( bodyA, maxSize, index, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(body, maxSize, bodyA);
			}
			delete [] bodyA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetBodyLine(index);
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

/*********************************************
SaveToFile
    Saves memory storage to a file
Params
    name - name of the file to create
Return
    UTE_SUCCESS             - success
    UTE_FILE_OPEN_ERROR     - file open error
    UTE_FILE_WRITE_ERROR    - file write error
**********************************************/
int CUT_HTTPClient::SaveToFile(LPCTSTR name)
{
    CUT_File fileData;
    int error = UTE_SUCCESS;
    
    if (fileData.Open(name, GENERIC_WRITE, CREATE_ALWAYS) != 0)
        return OnError(UTE_FILE_OPEN_ERROR);

    long count, loop;
    LPCSTR buf;
    DWORD len;

    count = GetBodyLineCount();

    // write the file
    for (loop = 0; loop < count ; loop++) {
        
        // Check if procedure is aborted
        if (IsAborted()) {  
            CloseConnection();
            return OnError(UTE_ABORTED);        // Abort
        }

        buf = GetBodyLine(loop);                
        len = (int)strlen(buf);
        
        // write to the file
        if (fileData.Write(buf, (UINT) len) < len) {
            error = UTE_FILE_WRITE_ERROR;
            break;
        }
        
        if (!CUT_StrMethods::IsWithCRLF(buf)) {
            if(fileData.Write("\r\n", 2) < 2) {
                error = UTE_FILE_WRITE_ERROR;
                break;
            }
        }
    }
    
    return OnError(error);
}

/*********************************************
GetLinkTagCount
    Returns the total number of link tags found
    in memory storage
Params
    none
Return
    number of links
**********************************************/
int CUT_HTTPClient::GetLinkTagCount(){

    //if the tag count was already calculated for this data then
    //just return the number, otherwise calc. the data
    if(m_nTagCount != -1)
        return m_nTagCount;
    else
        m_nTagCount = 0;

    m_listTags.ClearList();

    long count = 0;
	long loop = 0;
    int loop2 = 0;
	int len = 0;
    LPSTR tagString = {"<a href"};
    LPSTR tagUString = {"<A HREF"};
    LPCSTR buf;
    int comparePos = 0;
    int compareLen = (int)strlen(tagString);
        
    count = GetBodyLineCount();

    for(loop=0;loop<count;loop++){
    
        buf = GetBodyLine(loop);
        len = (int)strlen(buf);

        for(loop2=0;loop2<len;loop2++){
            if(tagString[comparePos] == buf[loop2] || tagUString[comparePos] == buf[loop2]){
                comparePos++;
                if(comparePos == compareLen){
                    m_nTagCount ++;
                    m_listTags.AddTagPos(loop,loop2);
                    comparePos = 0;
                }
            }
            else
                comparePos = 0;
        }
    }
        

    return m_nTagCount;
}

/*********************************************
GetLinkTag
    returns a link tag from memory storage
Params
    index - 0 based index of the tag
    tag - buffer where the tage is copied into
    maxLen - length of the buffer
Return
    UTE_SUCCESS             - success
    UTE_INDEX_OUTOFRANGE    - invalid index
    UTE_BUFFER_TOO_SHORT    - buffer too short
**********************************************/
#if defined _UNICODE
int CUT_HTTPClient::GetLinkTag(int index,LPWSTR tag,int maxLen){
	char * str = (char*) alloca(maxLen+1);
	int result = GetLinkTag(index, str, maxLen);
	if(result == UTE_SUCCESS) {
		CUT_Str::cvtcpy((_TCHAR*)tag, maxLen, str);
	}
	return result;
}
#endif
int CUT_HTTPClient::GetLinkTag(int index,LPSTR tag,int maxLen){

    if(m_nTagCount == -1)
        GetLinkTagCount();

    long line =0 ;
    int pos = 0;
    
    if(m_listTags.GetTagPos(index,&line,&pos) != TRUE)
        return OnError(UTE_INDEX_OUTOFRANGE);
    
    LPCSTR buf = GetBodyLine(line);
    if(CUT_StrMethods::ParseString(&buf[pos],">",0,tag,maxLen) != UTE_SUCCESS){
		return OnError(UTE_BUFFER_TOO_SHORT);
	}
	
	// now that wa have known where is the maximum of the link 
	// we need to mak sure that it is not including the = sign of the SRC tag
	// however we have to make sure that we are not removing any other equal signs the link may include
	
	// create a work buffer
	char *temp = new char[maxLen];
	
	// let's populate it 
	strcpy(temp,tag);

	// it must include an equal sign
	if(CUT_StrMethods::ParseString(temp,"=",0,tag,maxLen) != UTE_SUCCESS){
		delete []temp;
		return OnError(UTE_BUFFER_TOO_SHORT);
	}
	// we need the working one be starting from the equal sign since we may hav spaces before the 
	// first " 
	strcpy(temp,&temp[strlen(tag)]);

	// but just incase there is no " sign and no spaces we must make a copy of it 
	// which will be returned if the next if statement did not pass
	strcpy(tag,temp+1);

	if (CUT_StrMethods::GetParseStringPieces (temp,"\"'") > 1)
	{
		if(CUT_StrMethods::ParseString(temp,"\"'",1,tag,maxLen) != UTE_SUCCESS){
			delete []temp;
			return OnError(UTE_BUFFER_TOO_SHORT);
		}
	}

	// remove th working buffer
	delete []temp;
	       
    return OnError(UTE_SUCCESS);
}

/*********************************************
GetHeaderLineCount
    Returns the number of lines in the header
    of the data that was last received from 
    the server
Params
    none
Return
    number of header lines
**********************************************/
int CUT_HTTPClient::GetHeaderLineCount() const
{
    return m_listReceiveHeader.GetCount();
}

/*********************************************
GetHeaderLine
    Returns a header line from the header
    of the data that was last received from
    the server
Params
    index - 0 based index of a line in the header
Return
    pointer to the string or NULL
**********************************************/
LPCSTR CUT_HTTPClient::GetHeaderLine(int index) const
{
    return m_listReceiveHeader.GetString(index);
}
/*************************************************
GetHeaderLine()
Returns a header line from the header
    of the data that was last received from
    the server
PARAM
header	- [out] pointer to buffer to receive title
maxSize - length of buffer
index	- index of the header
size    - [out] length of title

  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- header and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - header not found
  UTE_BUFFER_TOO_SHORT  - space in title buffer indicated by maxSize insufficient, realloc 
  based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
int CUT_HTTPClient::GetHeaderLine(LPSTR header, size_t maxSize, int index, size_t *size) {
	
	int retval = UTE_SUCCESS;
	
	if(header == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetHeaderLine(index);
		
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
				strcpy(header, str);
			}
		}
	}
	return retval;	
}
#if defined _UNICODE
int CUT_HTTPClient::GetHeaderLine(LPWSTR header, size_t maxSize, int index, size_t *size) {
	
	int retval;
	
	if(maxSize > 0) {
		char * headerA = new char [maxSize];
		
		if(headerA != NULL) {
			retval = GetHeaderLine( headerA, maxSize, index, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(header, maxSize, headerA);
			}
			delete [] headerA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetHeaderLine(index);
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

/*********************************************
GetHeaderLine
    Returns a first header line from the header
    of the data that was last received from
    the server that contains specified substring
Params
    subString   - substring to serch for
    [pos]       - position of substring if found
Return
    pointer to the string or NULL
**********************************************/
LPCSTR CUT_HTTPClient::GetHeaderLine(LPCSTR subString, int *pos) const
{
    return m_listReceiveHeader.GetString(subString, pos);
}
/*************************************************
GetHeaderLine()
    Returns a first header line from the header
    of the data that was last received from
    the server that contains specified substring
PARAM
header	- [out] pointer to buffer to receive title
maxSize - length of buffer
subString   - substring to serch for
pos       - [out] position of substring if found {may be NULL if not wanted)
index	- index of the header
size    - [out] length of header

  RETURN				
  UTE_SUCCESS			- ok - 
  UTE_NULL_PARAM		- header and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - header not found
  UTE_BUFFER_TOO_SHORT  - space in header buffer indicated by maxSize insufficient, realloc 
						  based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
int CUT_HTTPClient::GetHeaderLine(LPSTR header, size_t maxSize, LPCSTR subString, size_t *size, int *pos) {
	
	int retval = UTE_SUCCESS;
	
	if(header == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetHeaderLine(subString, pos);
		
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
				strcpy(header, str);
			}
		}
	}
	return retval;	
}
#if defined _UNICODE
int CUT_HTTPClient::GetHeaderLine(LPWSTR header, size_t maxSize, LPCSTR subString, size_t *size, int *pos) {
	
	int retval;
	
	if(maxSize > 0) {
		char * headerA = new char [maxSize];
		
		if(headerA != NULL) {
			retval = GetHeaderLine( headerA, maxSize, subString, size, pos);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(header, maxSize, headerA);
			}
			delete [] headerA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetHeaderLine(subString,pos);
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

/*********************************************
GetModifiedDateFromHeader
    Return the "Last-modified" header received from the server
PARAM:
    NONE
RETURN:
    LPCSTR the whole "Last-modified: date " header.
    otherwise NULL
**********************************************/
LPCSTR CUT_HTTPClient::GetModifiedDateFromHeader(){
    //mime tage: Last-modified:
    
    int     count,loop;
    char    buf[64];

    count = GetHeaderLineCount();
    for(loop = 0; loop < count ; loop++){
        CUT_StrMethods::ParseString(GetHeaderLine(loop),": ",0,buf,sizeof(buf));
        if(_stricmp(buf,"Last-modified") == 0){
            return GetHeaderLine(loop);
        }
    }
    return NULL;
}
/***********************************************
GetModifiedDateFromHeader
	Return the "Last-modified:" header received from the server
PARAM:
	date		- [out] buffer to receive proxy address
	maxSize		- [in]  length of buffer in chars
	size		- [out] actual length of string returned or required.
RETURN:
  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- headerText and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - not found
  UTE_BUFFER_TOO_SHORT  - space in length buffer indicated by maxSize insufficient, realloc 
							based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload

************************************************/
int CUT_HTTPClient::GetModifiedDateFromHeader( LPSTR date, size_t maxSize, size_t *size) {
	
	int retval = UTE_SUCCESS;
	
	if(date == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetModifiedDateFromHeader();
		
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
				strcpy(date, str);
			}
		}
	}
	return retval;	
}
#if defined _UNICODE
int CUT_HTTPClient::GetModifiedDateFromHeader( LPWSTR date, size_t maxSize, size_t *size) {

	int retval;
	
	if(maxSize > 0) {
		char * dateA = new char [maxSize];
		
		if(dateA != NULL) {
			retval = GetModifiedDateFromHeader( dateA, maxSize, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(date, maxSize, dateA);
			}
			delete [] dateA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetModifiedDateFromHeader();
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

/*********************************************
GetLengthFromHeader
	Return the "Content-length:" header received from the server
PARAM:
    NONE
RETURN:
    LPCSTR the whole "Content-length:" header as string
    otherwise NULL

**********************************************/
LPCSTR CUT_HTTPClient::GetLengthFromHeader(){
    //mime tage: Content-length:
    
    int     count,loop;
    char    buf[64];

    count = GetHeaderLineCount();
    for(loop = 0; loop < count ; loop++){
        CUT_StrMethods::ParseString(GetHeaderLine(loop),": ",0,buf,sizeof(buf));
        if(_stricmp(buf,"Content-length") == 0 ){
            return GetHeaderLine(loop);
        }
    }

    return NULL;
}
/***********************************************
GetLengthFromHeader
	Return the "Content-length:" header received from the server
PARAM:
	length		- [out] buffer to receive proxy address
	maxSize		- [in]  length of buffer in chars
	size		- [out] actual length of string returned or required.
RETURN:
  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- headerText and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - not found
  UTE_BUFFER_TOO_SHORT  - space in length buffer indicated by maxSize insufficient, realloc 
							based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload

************************************************/
int CUT_HTTPClient::GetLengthFromHeader( LPSTR length, size_t maxSize, size_t *size) {
	
	int retval = UTE_SUCCESS;
	
	if(length == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetLengthFromHeader();
		
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
				strcpy(length, str);
			}
		}
	}
	return retval;	
}
#if defined _UNICODE
int CUT_HTTPClient::GetLengthFromHeader( LPWSTR length, size_t maxSize, size_t *size) {

	int retval;
	
	if(maxSize > 0) {
		char * lengthA = new char [maxSize];
		
		if(lengthA != NULL) {
			retval = GetLengthFromHeader( lengthA, maxSize, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(length, maxSize, lengthA);
			}
			delete [] lengthA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetLengthFromHeader();
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
/*********************************************
GetContentType:
Serach for the "Content-Type:" in thereceived headers 
    and retrurns it if found
PARAM:
    NONE
RETURN:
    LPCSTR  - the whole "Content-Type:...." header as string
    otherwise NULL

**********************************************/
LPCSTR CUT_HTTPClient::GetContentType(){
    //mime tage: Content-Type:
    
    int     count,loop;
    char    buf[64];

    count = GetHeaderLineCount();
    for(loop = 0; loop < count ; loop++){
        CUT_StrMethods::ParseString(GetHeaderLine(loop),": ",0,buf,sizeof(buf));
        if(_stricmp(buf,"Content-Type") == 0 ){
            return GetHeaderLine(loop);
        }
    }

    return NULL;
}
/***********************************************
GetContentType
	Retrieves the "Content-Type:...." header received from the server 
PARAM:
	contentType	- [out] buffer to receive proxy address
	maxSize		- [in]  length of buffer in chars
	size		- [out] actual length of string returned or required.
RETURN:
  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- headerText and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - not found
  UTE_BUFFER_TOO_SHORT  - space in title buffer indicated by maxSize insufficient, realloc 
							based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload

************************************************/
int CUT_HTTPClient::GetContentType( LPSTR contentType, size_t maxSize, size_t *size) {
	
	int retval = UTE_SUCCESS;
	
	if(contentType == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetContentType();
		
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
				strcpy(contentType, str);
			}
		}
	}
	return retval;	
}
#if defined _UNICODE
int CUT_HTTPClient::GetContentType( LPWSTR contentType, size_t maxSize, size_t *size) {

	int retval;
	
	if(maxSize > 0) {
		char * contentTypeA = new char [maxSize];
		
		if(contentTypeA != NULL) {
			retval = GetContentType( contentTypeA, maxSize, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(contentType, maxSize, contentTypeA);
			}
			delete [] contentTypeA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetContentType();
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

/*********************************************
    Sets communication that follow to be through 
    proxy
RET:
    VOID
******************************************/
void CUT_HTTPClient::UseProxyServer(BOOL flag){
    m_bUseProxy = flag;
}

/*************************************************
SetProxy
    Sets the proxy server address or name to be used
    when UseProxy is set to TRUE
Param:
    proxyIP     - new proxy address
Return:
    UTE_ERROR   - Empty string;
    UTE_SUCCESS - SUCESS
****************************************************/
#if defined _UNICODE
int CUT_HTTPClient::SetProxy( LPCWSTR proxyIP ){
	return SetProxy(AC(proxyIP));}
#endif
int CUT_HTTPClient::SetProxy( LPCSTR proxyIP ){
    
    if (proxyIP == NULL)
        return UTE_ERROR;

    strncpy(m_szProxyAddr, proxyIP, sizeof(m_szProxyAddr));
    return UTE_SUCCESS;
}

/*************************************************
GetProxy
    Gets the proxy server address or name to be used
    when UseProxy is set to TRUE
Param:
    none
Return:
    proxy address
****************************************************/
LPCSTR CUT_HTTPClient::GetProxy() const
{
    return m_szProxyAddr;
}

/***********************************************
GetProxy
	Get the proxy server previously assigned
PARAM:
	proxy		- [out] buffer to receive proxy address
	maxSize		- [in]  length of buffer in chars
	size		- [out] actual length of string returned or required.
RETURN:
  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- headerText and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - not found
  UTE_BUFFER_TOO_SHORT  - space in title buffer indicated by maxSize insufficient, realloc 
							based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload

************************************************/
int CUT_HTTPClient::GetProxy( LPSTR proxy, size_t maxSize, size_t *size) {
	
	int retval = UTE_SUCCESS;
	
	if(proxy == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetProxy();
		
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
				strcpy(proxy, str);
			}
		}
	}
	return retval;	
}
#if defined _UNICODE
int CUT_HTTPClient::GetProxy( LPWSTR proxy, size_t maxSize, size_t *size) {

	int retval;
	
	if(maxSize > 0) {
		char * proxyA = new char [maxSize];
		
		if(proxyA != NULL) {
			retval = GetProxy( proxyA, maxSize, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(proxy, maxSize, proxyA);
			}
			delete [] proxyA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetProxy();
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


/**************************************************
GET
    Sends a GET statement to the currently 
    connected server or proxy then retrieves the 
    results into the list or file.
  
 Route the Get command through a proxy

 Retrieves the results and seave it to a file if 
 destFile parameter set.

    
     The proxy may forward the request 
     on to another proxy or directly to the server specified
     In Order to avoid request loops, a proxy must be able to recognize all of its 
     server names, including any aliases, local variations 
     and numeric IP addreses
     SEE Section 5.1.2 RFC 1945

  "A proxy must interpret and, if necessary, rewrite a
 request message before forwarding it. Proxies are often 
 used as client-side portals through network firewalls 
 and as helper applications for handling requests via
 protocols not implemented by the user agent.
 "     See 1.2  (Terminology) RFC 1945.Berners-Lee et al

 Param:  
    url         - a pointer to the URL to be Requested
    [destFile]  - filename 
 Return:
    UTE_SUCCESS             - success
    UTE_CONNECT_FAILED      - unable to connect to server
    UTE_CONNECT_REJECTED    - connection rejected
    UTE_BAD_URL             - bad url
    UTE_CONNECT_TERMINATED  - connection terminated before the command
                                was completed   
    UTE_FILE_TYPE_ERROR     - invalid file type
    UTE_DS_OPEN_FAILED      - unable to open specified data source
    UTE_SOCK_TIMEOUT        - timeout
    UTE_SOCK_RECEIVE_ERROR  - receive socket error
    UTE_DS_WRITE_FAILED     - data source write error
    UTE_ABORTED             - aborted
**************************************************/
#if defined _UNICODE
int CUT_HTTPClient::GET(LPWSTR url, LPCTSTR destFile) {
	return GET(AC(url), destFile);}
#endif
int CUT_HTTPClient::GET(LPSTR url, LPCTSTR destFile) {
	int retval;

	if(destFile != NULL && _tcslen(destFile) > 0) 
	{
		CUT_FileDataSource ds(destFile);
		retval = GET(url,&ds);
	}
	else
	{	
		retval = GET(url, (CUT_FileDataSource*)NULL);
	}

    return retval;
}
/**************************************************
GET
    Sends a GET statement to the currently 
    connected server or proxy then retrieves the 
    results into the list or file.
  
 Route the Get command through a proxy

 Retrieves the results and seave it to a file if 
 destFile parameter set.

    
     The proxy may forward the request 
     on to another proxy or directly to the server specified
     In Order to avoid request loops, a proxy must be able to recognize all of its 
     server names, including any aliases, local variations 
     and numeric IP addreses
     SEE Section 5.1.2 RFC 1945

  "A proxy must interpret and, if necessary, rewrite a
 request message before forwarding it. Proxies are often 
 used as client-side portals through network firewalls 
 and as helper applications for handling requests via
 protocols not implemented by the user agent.
 "     See 1.2  (Terminology) RFC 1945.Berners-Lee et al

 Param:  
    url         - a pointer to the URL to be Requested
    [destFile]  - filename 
 Return:
    UTE_SUCCESS             - success
    UTE_CONNECT_FAILED      - unable to connect to server
    UTE_CONNECT_REJECTED    - connection rejected
    UTE_BAD_URL             - bad url
    UTE_CONNECT_TERMINATED  - connection terminated before the command
                                was completed   
    UTE_FILE_TYPE_ERROR     - invalid file type
    UTE_DS_OPEN_FAILED      - unable to open specified data source
    UTE_SOCK_TIMEOUT        - timeout
    UTE_SOCK_RECEIVE_ERROR  - receive socket error
    UTE_DS_WRITE_FAILED     - data source write error
    UTE_ABORTED             - aborted
**************************************************/
#if defined _UNICODE
int CUT_HTTPClient::GET(LPWSTR url, CUT_DataSource * dest) {
	return GET(AC(url), dest);}
#endif
int CUT_HTTPClient::GET(LPSTR url, CUT_DataSource * dest)
{
	int error = UTE_SUCCESS;

	if ((error = SendGetCommand(url)) != UTE_SUCCESS)
	{
		m_listSendHeader.ClearList();
		return OnError(error); //  for now 
	}
	
     //send the rest of the header info
		   //send the rest of the header info
	if ((error = SendRequestHeaders()) != UTE_SUCCESS)
	{
	   m_listSendHeader.ClearList();
       return OnError(error); //  for now 

	}


    Send("\r\n");
	// receive the response headers
	if ((error = ReceiveResponseHeaders()) == UTE_SUCCESS)
	{

		if (m_lLastStatusCode == 401)
		{
			char scheme[SCHEME_BUFFER_SIZE];
			char realm[REALM_BUFFER_SIZE];
			char password[PASSWORD_BUFFER_SIZE];
			char userName[USERR_NAME_BUFFER_SIZE];
			
			GetAuthenticationParams(scheme, realm);
			if (OnAccessDenied (realm,userName,password))
			{
				// no if the scheme is basic let call add basic Authorization
				if (_stricmp(scheme, "BASIC") == 0 )
				{
					CloseConnection();

					AddBasicAuthorizationHeader(userName, password);
					if ((error = SendGetCommand(url)) != UTE_SUCCESS)
					{
						m_listSendHeader.ClearList();
						return OnError(error); //  for now 
					}
					//send the rest of the header info
					if ((error = SendRequestHeaders()) != UTE_SUCCESS)
					{
						m_listSendHeader.ClearList();
						return OnError(error); //  for now 
					}
					
					Send("\r\n");
					// receive the response headers
					if(dest != NULL) 
					{
						error = Receive( *dest, UTM_OM_WRITING, GetReceiveTimeOut ()/1000);
					}
					else
					{
						// receive the response body
						error = ReceiveResponseBody();
					}
				}
			}
			
		}
		else if (
			m_lLastStatusCode ==  301  //Moved Permanently
			|| m_lLastStatusCode == 302 // Found
			|| m_lLastStatusCode == 303 //See Other
			|| m_lLastStatusCode ==  307 ) //Temporary Redirect
		{
			// get the redirection header
			//find the location header
			char newUrl[URL_RESOURCE_LENGTH];
			if (GetMovedLocation( newUrl ) )
			{
				if (OnRedirect(newUrl))
				{
					 CloseConnection();
					return GET (newUrl ,  dest);
				}	
				else if(dest != NULL)// Receive data in the file
				{
					error = Receive( *dest, UTM_OM_WRITING,GetReceiveTimeOut ()/1000);
				}
				else
				{
					// receive the response body
					error = ReceiveResponseBody();
				}
			}
			// pass it to the OnRedirect page
			// then see if we should go ahead and redirect.		

		}
		else if(dest != NULL)// Receive data in the file
		 
		{
			error = Receive( *dest, UTM_OM_WRITING,GetReceiveTimeOut ()/1000);
		}
		else
		{
			// receive the response body
			error = ReceiveResponseBody();
		}

	}
    CloseConnection();

    return OnError(error);
}
/*********************************************
HEADThroughProxy
    Sends a HEAD statement to the currently 
    connected server or to the proxy then 
    retrieves the results
Params
    url - URL to get the HEAD of
Return
    UTE_SUCCESS             - success
    UTE_CONNECT_FAILED      - unable to connect to server
    UTE_CONNECT_REJECTED    - connection rejected
    UTE_BAD_URL             - bad url
    UTE_CONNECT_TERMINATED  - connection terminated before the command
                                was completed   
    UTE_FILE_TYPE_ERROR     - invalid file type
    UTE_FILE_OPEN_ERROR     - unable to open or create file
    UTE_SOCK_TIMEOUT        - timeout
    UTE_SOCK_RECEIVE_ERROR  - receive socket error
    UTE_FILE_WRITE_ERROR    - file write error
    UTE_ABORTED             - aborted
**********************************************/
#if defined _UNICODE
int CUT_HTTPClient::HEAD(LPWSTR url) {
	return HEAD(AC(url));}
#endif
int CUT_HTTPClient::HEAD(LPSTR url) {

    int     error;
	
	if ((error = SendHeadCommand(url)) != UTE_SUCCESS)
	{
	   m_listSendHeader.ClearList();
       return OnError(error); //  for now 

	}
  //send the rest of the header info
	if ((error = SendRequestHeaders()) != UTE_SUCCESS)
	{
	   m_listSendHeader.ClearList();
       return OnError(error); //  for now 

	}
    Send("\r\n");
    //retrieve the header data
	// Get Response 
   	error = ReceiveResponseHeaders();
	if (m_lLastStatusCode == 401)
	{
			char scheme[SCHEME_BUFFER_SIZE];
			char realm[REALM_BUFFER_SIZE];
			char password[PASSWORD_BUFFER_SIZE];
			char userName[USERR_NAME_BUFFER_SIZE];
			
			GetAuthenticationParams(scheme, realm);
			if (OnAccessDenied (realm,userName,password))
			{
				// no if the scheme is basic let call add basic Authorization
				if (_stricmp(scheme, "BASIC") == 0 )
				{
					AddBasicAuthorizationHeader(userName, password);
					CloseConnection();

					if ((error = SendHeadCommand(url)) != UTE_SUCCESS)
					{
						m_listSendHeader.ClearList();
						return OnError(error); //  for now 
					}

					//send the rest of the header info
					if ((error = SendRequestHeaders()) != UTE_SUCCESS)
					{
						m_listSendHeader.ClearList();
						return OnError(error); //  for now 
					}
					Send("\r\n");
					error = ReceiveResponseHeaders();
				}
			}
	}
	ClearReceiveBuffer();
    CloseConnection();

    return OnError(error);
}
/*********************************************
CommandLine
    // issues a custom command through a currently 
    // connected server or proxy.
    // NOTE: ---->that the proxy may forward the request 
    // on to another proxy or directly to the server specified
    // In Order to avoid request loops, a proxy must be able to recognize all of its 
    // server names, including any aliases, local variations 
    // and numeric IP addreses
    // SEE Section 5.1.2 RFC 1945   
Params
    command - command to execute
    url     - URL to get the HEAD of
Return
    UTE_SUCCESS             - success
    UTE_CONNECT_FAILED      - unable to connect to server
    UTE_CONNECT_REJECTED    - connection rejected
    UTE_BAD_URL             - bad url
    UTE_CONNECT_TERMINATED  - connection terminated before the command
                                was completed   
    UTE_FILE_TYPE_ERROR     - invalid file type
    UTE_FILE_OPEN_ERROR     - unable to open or create file
    UTE_SOCK_TIMEOUT        - timeout
    UTE_SOCK_RECEIVE_ERROR  - receive socket error
    UTE_FILE_WRITE_ERROR    - file write error
    UTE_ABORTED             - aborted
********************************************/
#if defined _UNICODE
int CUT_HTTPClient::CommandLine(LPWSTR command,LPWSTR url, LPCWSTR data){
	return CommandLine(AC(command), AC(url), AC(data));}
#endif
int CUT_HTTPClient::CommandLine(LPSTR command,LPSTR url, LPCSTR data){
        
    char    buf[MAX_PATH * 2];
    char    file[MAX_PATH + 1];
    int     error;

    //if the url string is empty then return
    if (url == NULL)
        return OnError(UTE_BAD_URL);
    
    //clear any prev data
    ClearReceivedData();

    // if not using proxy
    if(!m_bUseProxy)
        //get the server Address and file to retrieve
        if(SplitURL(url, file, sizeof(file)) != UTE_SUCCESS)
            return OnError(UTE_BAD_URL);
    
     //connect
   if ((error = HTTPConnect()) !=  UTE_SUCCESS)
	    return OnError(error);

    
    // if using proxy
    if(m_bUseProxy) {
        // if the url includes http then it is already an absolute URI
        // so lets just format our command string
        if (_strnicmp (url,"http://", strlen("http://"))==0)
            _snprintf(buf,sizeof(buf)-1,"%s %s HTTP/1.0\r\n",command,url);
        else
            _snprintf(buf,sizeof(buf)-1,"%s http://%s HTTP/1.0\r\n",command,url);
        }
    else
        _snprintf(buf,sizeof(buf)-1,"%s %s HTTP/1.0\r\n", command, file);


    //send the command
    if(Send(buf) <= 0) {
        CloseConnection();
        return OnError(UTE_CONNECT_TERMINATED);
        }

    if(IsAborted()) {                       // Check if procedure is aborted
        CloseConnection();
        return OnError(UTE_ABORTED);        // Abort
    }

    //send the rest of the header info
	if ((error = SendRequestHeaders()) != UTE_SUCCESS)
	{
	   m_listSendHeader.ClearList();
       return OnError(error); //  for now 

	}	
     // now there is the blank line betwwen the data and the headers
    Send("\r\n");
    // send the data for the post function if any 
    if (data != NULL && strlen(data) > 0) {
        if (strlen(data))
        {
            Send(data);
            Send("\r\n");
        }
    }

    //retrieve the header data   
	// Get Response 
   	if ((error = ReceiveResponseHeaders()) == UTE_SUCCESS)
	{
    	// receive the response body
		error = ReceiveResponseBody();
	}


    CloseConnection();

    return OnError(UTE_SUCCESS);
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
int CUT_HTTPClient::SetConnectTimeout(int secs){
    
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
int CUT_HTTPClient::GetConnectTimeout() const
{
    return m_nConnectTimeout;
}

/***************************************************
PUT
    Create a resource at the target URL
    "
   The PUT method requests that the enclosed entity be stored under the
   supplied Request-URI. If the Request-URI refers to an already
   existing resource, the enclosed entity should be considered as a
   modified version of the one residing on the origin server. If the
   Request-URI does not point to an existing resource, and that URI is
   capable of being defined as a new resource by the requesting user
   agent, the origin server can create the resource with that URI.

   The fundamental difference between the POST and PUT requests is
   reflected in the different meaning of the Request-URI. The URI in a
   POST request identifies the resource that will handle the enclosed
   entity as data to be processed. That resource may be a data-accepting
   process, a gateway to some other protocol, or a separate entity that
   accepts annotations. In contrast, the URI in a PUT request identifies
   the entity enclosed with the request -- the user agent knows what URI
   is intended and the server should not apply the request to some other
   resource.
   "  - RFC 1945

PARAM
    LPSTR targetURL     -  the full name and path of the target file 
    LPCSTR sourceFile   -  full path of the source file
	BOOL	bPost		- if the command is post = TRUE or PUT =FALSE
Return
    UTE_SUCCESS             - success
    UTE_CONNECT_FAILED      - unable to connect to server
    UTE_CONNECT_REJECTED    - connection rejected
    UTE_BAD_URL             - bad url
    UTE_CONNECT_TERMINATED  - connection terminated before the command
                                was completed   
    UTE_FILE_TYPE_ERROR     - invalid file type
    UTE_FILE_OPEN_ERROR     - unable to open or create file
    UTE_SOCK_TIMEOUT        - timeout
    UTE_SOCK_RECEIVE_ERROR  - receive socket error
    UTE_FILE_WRITE_ERROR    - file write error
    UTE_ABORTED             - aborted
****************************************************/
#if defined _UNICODE
int CUT_HTTPClient::PUT(LPWSTR url, LPCTSTR sourceFile ,BOOL bPost ){
	return PUT(AC(url), sourceFile, bPost);}
#endif
int CUT_HTTPClient::PUT(LPSTR url, LPCTSTR sourceFile ,BOOL bPost ){
	CUT_FileDataSource ds(sourceFile);
    return  PUT(url,ds,bPost);
}

/***************************************************
PUT
    Create a resource at the target URL
    "
   The PUT method requests that the enclosed entity be stored under the
   supplied Request-URI. If the Request-URI refers to an already
   existing resource, the enclosed entity should be considered as a
   modified version of the one residing on the origin server. If the
   Request-URI does not point to an existing resource, and that URI is
   capable of being defined as a new resource by the requesting user
   agent, the origin server can create the resource with that URI.

   The fundamental difference between the POST and PUT requests is
   reflected in the different meaning of the Request-URI. The URI in a
   POST request identifies the resource that will handle the enclosed
   entity as data to be processed. That resource may be a data-accepting
   process, a gateway to some other protocol, or a separate entity that
   accepts annotations. In contrast, the URI in a PUT request identifies
   the entity enclosed with the request -- the user agent knows what URI
   is intended and the server should not apply the request to some other
   resource.
   "  - RFC 1945

PARAM
    LPSTR targetURL             -  the full name and path of the target file 
    CUT_DataSource sourceFile   -  the data source to be sent
Return
    UTE_SUCCESS                 - success
    UTE_CONNECT_FAILED          - unable to connect to server
    UTE_CONNECT_REJECTED        - connection rejected
    UTE_BAD_URL                 - bad url
    UTE_CONNECT_TERMINATED      - connection terminated before the command
                                was completed   
    UTE_FILE_TYPE_ERROR         - invalid file type
    UTE_FILE_OPEN_ERROR         - unable to open or create file
    UTE_SOCK_TIMEOUT            - timeout
    UTE_SOCK_RECEIVE_ERROR      - receive socket error
    UTE_FILE_WRITE_ERROR        - file write error
    UTE_ABORTED                 - aborted
****************************************************/
#if defined _UNICODE
int CUT_HTTPClient::PUT(LPWSTR targetURL, CUT_DataSource &sourceFile, BOOL bPost ){
	return PUT(AC(targetURL), sourceFile, bPost);}
#endif
int CUT_HTTPClient::PUT(LPSTR targetURL, CUT_DataSource &sourceFile, BOOL bPost )
{
    int      error = UTE_SUCCESS;

	if ((error = SendPutCommand(targetURL, sourceFile,  bPost)) != UTE_SUCCESS)
	{
	   m_listSendHeader.ClearList();
       return OnError(error); //  for now 

	}

	Send("\r\n");

	// Get Response 
   	if ((error = ReceiveResponseHeaders()) == UTE_SUCCESS)
	{
    	// receive the response body
		error = ReceiveResponseBody();
		if (m_lLastStatusCode == 401)
		{
			char scheme[SCHEME_BUFFER_SIZE];
			char realm[REALM_BUFFER_SIZE];
			char password[PASSWORD_BUFFER_SIZE];
			char userName[USERR_NAME_BUFFER_SIZE];
			
			GetAuthenticationParams(scheme, realm);
			if (OnAccessDenied (realm,userName,password))
			{
				// no if the scheme is basic let call add basic Authorization
				if (_stricmp(scheme, "BASIC") == 0 )
				{
					AddBasicAuthorizationHeader(userName, password);
					CloseConnection();
					if ((error = SendPutCommand(targetURL, sourceFile,  bPost)) != UTE_SUCCESS)
					{
						m_listSendHeader.ClearList();
						return OnError(error); //  for now 
					}
					
					Send("\r\n");
					// Get Response 
					error = ReceiveResponseHeaders();
				}
			}
		}
	}

    CloseConnection();
    m_listSendHeader.ClearList();

    return OnError(error);
}
/****************************************************
DeleteFile:
  The DELETE method requests that the origin server delete the resource
  identified by the Request-URI.
PARAM:
    LPSTR url   - The full URI of the resource to be deleted
RETURN:
    UTE_SUCCESS             - SUCCESS
    UTE_BAD_URL             - Bad domain name 
    UTE_CONNECT_TERMINATED  - Connection terminated prematurely
    UTE_ABORTED             - Operation aborted by user
****************************************************/
#if defined _UNICODE
int CUT_HTTPClient::DeleteFile(LPWSTR url){
	return DeleteFile(AC(url));}
#endif
int CUT_HTTPClient::DeleteFile(LPSTR url)
{
     int     error = UTE_SUCCESS;


	 if ((error = SendDeleteCommand(url)) != UTE_SUCCESS)
	{
		m_listSendHeader.ClearList();
		return OnError(error); //  for now 
	}

    //send the rest of the header info
	if ((error = SendRequestHeaders()) != UTE_SUCCESS)
	{
	   m_listSendHeader.ClearList();
       return OnError(error); //  for now 

	}

     // now there is the blank line between the data and the headers
    Send("\r\n");
  	// receive the response headers
	if ((error = ReceiveResponseHeaders()) == UTE_SUCCESS)
	{
    	// receive the response body
		error = ReceiveResponseBody();
		if (m_lLastStatusCode == 401)
		{
			char scheme[SCHEME_BUFFER_SIZE];
			char realm[REALM_BUFFER_SIZE];
			char password[PASSWORD_BUFFER_SIZE];
			char userName[USERR_NAME_BUFFER_SIZE];
			
			GetAuthenticationParams(scheme, realm);
			if (OnAccessDenied (realm,userName,password))
			{
				// no if the scheme is basic let call add basic Authorization
				if (_stricmp(scheme, "BASIC") == 0 )
				{
					AddBasicAuthorizationHeader(userName, password);
					CloseConnection();
					if ((error = SendDeleteCommand(url)) != UTE_SUCCESS)
					{
						m_listSendHeader.ClearList();
						return OnError(error); //  for now 
					}

					Send("\r\n");
					// Get Response 
					error = ReceiveResponseHeaders();
				}
			}
		}
	}

    CloseConnection();
    return OnError(error);
}
/************************************************************
	The POST method is used to request that the origin server accept the
	entity enclosed in the request as a new subordinate of the resource
	identified by the Request-URI in the Request-Line
PARAM
    LPSTR targetURL             -  the full name and path of the target file 
    CUT_DataSource sourceFile   -  the data source to be sent
Return
    UTE_SUCCESS                 - success
    UTE_CONNECT_FAILED          - unable to connect to server
    UTE_CONNECT_REJECTED        - connection rejected
    UTE_BAD_URL                 - bad url
    UTE_CONNECT_TERMINATED      - connection terminated before the command
                                was completed   
    UTE_FILE_TYPE_ERROR         - invalid file type
    UTE_FILE_OPEN_ERROR         - unable to open or create file
    UTE_SOCK_TIMEOUT            - timeout
    UTE_SOCK_RECEIVE_ERROR      - receive socket error
    UTE_FILE_WRITE_ERROR        - file write error
    UTE_ABORTED                 - aborted
************************************************************/
#if defined _UNICODE
int     CUT_HTTPClient::POST(LPWSTR url, CUT_DataSource &dest){
	return POST(AC(url), dest);}
#endif
int     CUT_HTTPClient::POST(LPSTR url, CUT_DataSource &dest)
{
	return PUT(url,dest, TRUE);
}
/************************************************************
	The POST method is used to request that the origin server accept the
	entity enclosed in the request as a new subordinate of the resource
	identified by the Request-URI in the Request-Line
 
PARAM
	LPSTR targetURL     -  the full name and path of the target file 
    LPCSTR sourceFile   -  full path of the source file
Return
    UTE_SUCCESS                 - success
    UTE_CONNECT_FAILED          - unable to connect to server
    UTE_CONNECT_REJECTED        - connection rejected
    UTE_BAD_URL                 - bad url
    UTE_CONNECT_TERMINATED      - connection terminated before the command
                                was completed   
    UTE_FILE_TYPE_ERROR         - invalid file type
    UTE_FILE_OPEN_ERROR         - unable to open or create file
    UTE_SOCK_TIMEOUT            - timeout
    UTE_SOCK_RECEIVE_ERROR      - receive socket error
    UTE_FILE_WRITE_ERROR        - file write error
    UTE_ABORTED                 - aborted
************************************************************/
#if defined _UNICODE
int   CUT_HTTPClient::POST( LPWSTR targetURL, LPCTSTR sourceFile){
	return POST(AC(targetURL), sourceFile);}
#endif
int   CUT_HTTPClient::POST( LPSTR targetURL, LPCTSTR sourceFile)
{
		return PUT(targetURL,sourceFile, TRUE);
}
/********************************************************
This function will Parses the server response 
to receive the status code as received from the server.

the status line is   
       Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
       
PARAM
	data	- The first line received from the server
RET
	the server response code.
                         
*********************************************************/
long	CUT_HTTPClient::ParseResponseCode(LPCSTR data)
{
	long code = 0;
	if (data != NULL)
	{

		// get the status code as number
		CUT_StrMethods::ParseString (data ," ",1,&code);
		// get the status reason
		// uncomment for debugging
		// char test[MAX_BUF];
		//CUT_StrMethods::ParseString (data ," ",1,test,MAX_PATH-1);	
		// assert(test);
	}
	return code;
}
/**********************************************************
This function will return the server response
the status line is   
       Status-Code    = "100"   ; Continue
                         | "101"   ; Switching Protocols
                         | "200"   ; OK
                         | "201"   ; Created
                         | "202"   ; Accepted
                         | "203"   ; Non-Authoritative Information
                         | "204"   ; No Content
                         | "205"   ; Reset Content
                         | "206"   ; Partial Content
                         | "300"   ; Multiple Choices
                         | "301"   ; Moved Permanently
                         | "302"   ; Moved Temporarily
                         | "303"   ; See Other
                         | "304"   ; Not Modified
                         | "305"   ; Use Proxy
                         | "400"   ; Bad Request
                         | "401"   ; Unauthorized
                         | "402"   ; Payment Required
                         | "403"   ; Forbidden
                         | "404"   ; Not Found
                         | "405"   ; Method Not Allowed
                         | "406"   ; Not Acceptable
                         | "407"   ; Proxy Authorization Required
                         | "408"   ; Request Time-out
                         | "409"   ; Conflict
                         | "410"   ; Gone
                         | "411"   ; Length Required
                         | "412"   ; Precondition Failed
                         | "413"   ; Request Entity Too Large
                         | "414"   ; Request-URI Too Large
                         | "415"   ; Unsupported Media Type
                         | "500"   ; Internal Server Error
                         | "501"   ; Not Implemented
                         | "502"   ; Bad Gateway
                         | "503"   ; Service Unavailable
                         | "504"   ; Gateway Time-out
                         | "505"   ; HTTP Version not supported
RET:
	long	- the status code as received from the server
**********************************************************/
long	CUT_HTTPClient::GetStatusCode()
{
	return m_lLastStatusCode;

}
/****************************************************
Receive the response headers

*****************************************************/
int CUT_HTTPClient::ReceiveResponseHeaders()
{

	char    buf[MAX_PATH * 2 + 1];
	bool once = true;
	int rt = 0;


    //retrieve the header data
	// v4.2 change to eliminate C4127: conditional expression is constant
	for(;;)
	{
        if(IsAborted())
		{                       // Check if procedure is aborted
            CloseConnection();
            m_listReceiveHeader.ClearList();
            return OnError(UTE_ABORTED);        // Abort
		}

        rt = ReceiveLine(buf,sizeof(buf),GetReceiveTimeOut ()/1000);
		if(rt <=0)
            break;
        if(rt <3)
            break;
        CUT_StrMethods::RemoveCRLF(buf);
		// lets just read the first line for the request status
		if (once )
		{
			m_lLastStatusCode = ParseResponseCode( buf);
			once = false;
		}
        m_listReceiveHeader.AddString(buf);
	}
	return UTE_SUCCESS;
}
/****************************************************
Receive the response body
*****************************************************/
int CUT_HTTPClient::ReceiveResponseBody()
{
	char    buf[MAX_PATH * 2 + 1];

	BOOL	bUppendToPrevious = FALSE;
	int rt = UTE_SUCCESS;


	//retrieve the body
	// v4.2 change to eliminate C4127: conditional expression is constant
	for(;;)
	{
		if(IsAborted())
		{                       // Check if procedure is aborted
			CloseConnection();
			m_listReceiveBody.ClearList();
			return OnError(UTE_ABORTED);        // Abort
		}

		rt = ReceiveLine(buf,sizeof(buf),GetReceiveTimeOut ()/1000);
		if(rt <=0)
			break;

		BOOL bNewFlag = CUT_StrMethods::IsWithCRLF(buf);
		CUT_StrMethods::RemoveCRLF(buf);
		if(bUppendToPrevious) 
		{
			// Add the string to the last string in the list
			unsigned int	nBufferSize;
			unsigned int    nLastItem = m_listReceiveBody.GetCount() - 1;
			char			*lpszBuffer;

			// Calculate the new string size
			nBufferSize = (int)strlen(buf);
			nBufferSize += (int)strlen(m_listReceiveBody.GetString(nLastItem)) + 10;

			// Allocate new buffer
			lpszBuffer = new char[nBufferSize];

			// Copy the data to the new string
			strcpy(lpszBuffer, m_listReceiveBody.GetString(nLastItem));
			strcat(lpszBuffer, buf);

			// Delete the last string in the list
			m_listReceiveBody.DeleteString(nLastItem);

			// Add new line (comined) to the list
			m_listReceiveBody.AddString(lpszBuffer);
		
			// Delete the buffer
			delete [] lpszBuffer;
		}
		else 
		{
			m_listReceiveBody.AddString(buf);
		}

		bUppendToPrevious = ! bNewFlag;
	}   
	return UTE_SUCCESS;
}
/**********************************************
Send the headers of the request to the server
PARAM:
	NONE
RET:
	UTE_CONNECT_TERMINATED
	UTE_SUCCESS
**********************************************/
int CUT_HTTPClient::SendRequestHeaders()
{
	 char buf[MAX_PATH * 2];

	BOOL hostIncluded = FALSE;


	// Send proxy authorization if any
	// notice we only send it if there is a proxy 
	// and it is not a tunneling process.
	char *ResultHeader = NULL;

	if (m_bUseProxy && !m_bEnableTunnelling) 
	if (CreateProxyBasicAuthorizationHeader(m_szProxyUserName,m_szProxyPassword,ResultHeader) == UTE_SUCCESS)
	{
		// now we send the proxy authorization 
		Send(ResultHeader);
		if (ResultHeader != NULL)
		delete ResultHeader;
	}

	int count = m_listSendHeader.GetCount();
    for(int loop =0;loop < count;loop++) 
	{
		// this send will return the number of bytes sent!!
        if(Send(m_listSendHeader.GetString(loop)) <= 0 )
		{
            CloseConnection();
            return OnError(UTE_CONNECT_TERMINATED);
		}
        if (strstr(m_listSendHeader.GetString(loop),"Host: "))
                hostIncluded = TRUE;
	}

     if (!hostIncluded || !m_bUseProxy) 
	 {
        // addede for the host command maybe we should leave it for the client to add the host header
        // add the host header 
        _snprintf(buf,sizeof(buf)-1,"Host: %s",m_szServerName);
        AddSendHeaderTag(buf);
        Send(buf);
        // note that we have sent a header string without the CRLF so lets send the CRLF
        Send("\r\n");
	 }

	 return UTE_SUCCESS;

}
/***************************************************
When an access denied response is received we will
 ask the user for password for the specified realm
 
   If the user wante to cancel then just return 0
REMARK
	maximum length of the userName will be 20, password is also 20
PARAM
	realm -  the realm the user name and password are for
	userName - The user name for the specified realm
	passWord - The password for the specified realm	
RET

	TRUE - the user has specified the user name and 
		password and would like us to continue
	FALSE - The user wishes to cancel the connection process
********************************************************/
BOOL CUT_HTTPClient::OnAccessDenied (LPCSTR /* realm */, LPSTR /* userName */, LPSTR /* password */)
{
	return FALSE;
}
// v4.2 this notification overload could be reviewed
BOOL CUT_HTTPClient::OnAccessDenied (LPCWSTR /* realm */, LPWSTR /* userName */, LPWSTR /* password */)
{
	return FALSE;
}
/**************************************************
Find the realm and Authorization mechanism

PARAM
	realm
		A string to be displayed to users so they know which username and
		password to use. This string should contain at least the name of
		the host performing the Authorization and might additionally
		indicate the collection of users who might have access.
	scheme	
		Encryption scheme used by the server to authenticate our request

RET
	UTE_SUCCESS
**************************************************/
int CUT_HTTPClient::GetAuthenticationParams(LPSTR scheme ,LPSTR	realm )
{
      
    int     count,loop;
    char    buf[64];	

    count = GetHeaderLineCount();
    for(loop = 0; loop < count ; loop++){
        CUT_StrMethods::ParseString(GetHeaderLine(loop),": ",0,buf,sizeof(buf));
        if(_stricmp(buf,"WWW-Authenticate") == 0 ){
			// get the schems
			CUT_StrMethods::ParseString(GetHeaderLine(loop)," ",1,scheme,SCHEME_BUFFER_SIZE-1);
			// get the realm
			CUT_StrMethods::ParseString(GetHeaderLine(loop),"=\"",1,realm,REALM_BUFFER_SIZE-1);            
        }
    }

    return UTE_SUCCESS;
}
/***************************************************
internal SendPutCommand
 Create a resource at the target URL
    "
   The PUT method requests that the enclosed entity be stored under the
   supplied Request-URI. If the Request-URI refers to an already
   existing resource, the enclosed entity should be considered as a
   modified version of the one residing on the origin server. If the
   Request-URI does not point to an existing resource, and that URI is
   capable of being defined as a new resource by the requesting user
   agent, the origin server can create the resource with that URI.

   The fundamental difference between the POST and PUT requests is
   reflected in the different meaning of the Request-URI. The URI in a
   POST request identifies the resource that will handle the enclosed
   entity as data to be processed. That resource may be a data-accepting
   process, a gateway to some other protocol, or a separate entity that
   accepts annotations. In contrast, the URI in a PUT request identifies
   the entity enclosed with the request -- the user agent knows what URI
   is intended and the server should not apply the request to some other
   resource.
   "  - RFC 1945

PARAM
	targetURL - Target URL as string
	sourceFile - Data source to be sent
	bPost		- should we use POST or PUT command

***************************************************/
int CUT_HTTPClient::SendPutCommand(LPSTR targetURL, CUT_DataSource &sourceFile, BOOL bPost){
   
	
	char    buf[MAX_PATH * 2 + 1];
    char    file[MAX_PATH + 1];
    int     rt, error;

    //if the url string is empty then return
    if (targetURL == NULL)
        return OnError(UTE_BAD_URL);
    
    //clear any prev data
    ClearReceivedData();

    // if not using proxy
    if(!m_bUseProxy)
        //get the server Address and file to retrieve
        if(SplitURL(targetURL, file, sizeof(file)) != UTE_SUCCESS)
            return OnError(UTE_BAD_URL);


    // before we attempt to connect 
    // lets make sure the file exists
    rt = sourceFile.Open (UTM_OM_READING);
    if (rt != UTE_SUCCESS)
         return OnError(UTE_DS_OPEN_FAILED);
    // now we will add the length of the bytes to be sent
    // from the file size
    _snprintf(buf,sizeof(buf)-1,"Content-Length: %d",sourceFile.Seek (0,SEEK_END));
    sourceFile.Close ();
    AddSendHeaderTag(buf);
	// if we are posting this header is required
	if (bPost)
		AddSendHeaderTag("Content-Type: application/x-www-form-urlencoded");
    //connect to proxy

   if ((error = HTTPConnect()) !=  UTE_SUCCESS)
		return OnError(error);

    // if using proxy
    if(m_bUseProxy) {
        // if the url includes http then it is already an absolute URI
        // so lets just format our command string
        if (_strnicmp (targetURL,"http://", strlen("http://"))==0)
			if (bPost)
				_snprintf(buf,sizeof(buf)-1,"POST %s HTTP/1.0\r\n",targetURL);
				else
            _snprintf(buf,sizeof(buf)-1,"PUT %s HTTP/1.0\r\n",targetURL);
        else
		{
			if (bPost)
				_snprintf(buf,sizeof(buf)-1,"POST http://%s HTTP/1.0\r\n",targetURL);
			else
            _snprintf(buf,sizeof(buf)-1,"PUT http://%s HTTP/1.0\r\n",targetURL);
		}
        }
    else
	{
		if (bPost)
			_snprintf(buf,sizeof(buf)-1, "POST %s HTTP/1.0\r\n",  file);
		else
			_snprintf(buf,sizeof(buf)-1,"PUT %s HTTP/1.0\r\n",  file);


	}


    //send the command
    if(Send(buf) <= 0) {
        CloseConnection();
        m_listSendHeader.ClearList();
        return OnError(UTE_CONNECT_TERMINATED);
        }

    if(IsAborted()) {                       // Check if procedure is aborted
        CloseConnection();
        m_listSendHeader.ClearList();
        return OnError(UTE_ABORTED);        // Abort
    }

	   //send the rest of the header info
	if ((rt = SendRequestHeaders()) != UTE_SUCCESS)
	{
	   m_listSendHeader.ClearList();
       return OnError(rt); //  for now 

	}

     // now there is the blank line betwwen the data and the headers
    Send("\r\n");
    
	// send the data for the post function if any 
    rt = Send(sourceFile);
    if ( rt != UTE_SUCCESS)
    {
        CloseConnection();
        m_listSendHeader.ClearList();
        return OnError(rt); //  for now 
    }
	return UTE_SUCCESS;

}
/***************************************************
	Sends a GET statement to the currently 
	connected server or proxy then retrieves the 
	results into the list or file.

	Route the Get command through a proxy

	Retrieves the results and seave it to a file if 
	destFile parameter set.


	 The proxy may forward the request 
	 on to another proxy or directly to the server specified
	 In Order to avoid request loops, a proxy must be able to recognize all of its 
	 server names, including any aliases, local variations 
	 and numeric IP addreses
	 SEE Section 5.1.2 RFC 1945

	"A proxy must interpret and, if necessary, rewrite a
	request message before forwarding it. Proxies are often 
	used as client-side portals through network firewalls 
	and as helper applications for handling requests via
	protocols not implemented by the user agent.
	"     See 1.2  (Terminology) RFC 1945

PARAM
	url - the usrl to retrieve
***************************************************/
int CUT_HTTPClient::SendGetCommand(LPSTR url)
{
    char    buf[MAX_PATH * 2 + 1];
    char    file[MAX_PATH+1];
    int     error = UTE_SUCCESS;

    //if the url string is empty then return
    if (url == NULL)
        return OnError(UTE_BAD_URL);

    //clear any prev data
    ClearReceivedData();

    // if not using proxy
    if(!m_bUseProxy || m_bEnableTunnelling)
        //get the server Address and file to retrieve
        if(SplitURL(url,file,sizeof(file)) != UTE_SUCCESS)
            return OnError(UTE_BAD_URL);


    //connect
	if ((error = HTTPConnect()) !=  UTE_SUCCESS)
			    return OnError(error);


    //prepare the GET command

    // if using proxy
    if(m_bUseProxy  && !m_bEnableTunnelling ) 
	{

        // NOTE:----> that when we add the proxy we don't add the slash after the 
        // GET command
        // if the url includes http then it is already an absolute URI
        // so lets just format our GET command string
        if (_strnicmp (url,"ftp://", strlen("ftp://"))==0)
            _snprintf(buf,sizeof(buf)-1,"GET %s HTTP/1.0\r\n",url);
        else if (_strnicmp (url,"ftp.", strlen("ftp."))==0)
            _snprintf(buf,sizeof(buf)-1,"GET ftp://%s HTTP/1.0\r\n",url);
        else if (_strnicmp (url,"http://", strlen("http://"))==0)
		{
            // does the target already contain http portion
            _snprintf(buf,sizeof(buf)-1,"GET %s HTTP/1.0\r\n",url);
		}
        else if (strncmp("//",url,2) == 0)
		{ // the user has input the whole url such as  
            //   { //user:passowrd@host:PortNumber/PathTarget }
            //    where the port number here is the port on the target not the proxy
            // then we will just send it to the proxy
            _snprintf(buf,sizeof(buf)-1,"GET %s HTTP/1.0\r\n",url);
		}
        else if (url[0] == '/')
		{
            // the user is targeting the Proxy as a server not a relay
            _snprintf(buf,sizeof(buf)-1,"GET http://%s:%d%s HTTP/1.0\r\n",m_szProxyAddr,m_nPort,url); 
		}   
        else // other wise prefix it with an http
            _snprintf(buf,sizeof(buf)-1,"GET http://%s HTTP/1.0\r\n",url); 
	}
    // if not using 
    else 
	{
        if (file[0] =='/' || (file[0] ==37))
            _snprintf(buf,sizeof(buf)-1,"GET %s HTTP/1.0\r\n",file);
        else
            _snprintf(buf,sizeof(buf)-1,"GET /%s HTTP/1.0\r\n",file);

    }

    //send the GET command
    if(Send(buf) <= 0)
	{
        CloseConnection();
        return OnError(UTE_CONNECT_TERMINATED);
	}

    if(IsAborted()) 
	{                       // Check if procedure is aborted
        CloseConnection();
        return OnError(UTE_ABORTED);        // Abort
	}
	return UTE_SUCCESS;



}
/***************************************************
	Sends a HEAD statement to the currently 
	connected server or to the proxy then 
	retrieves the results

	The head of the file will present the size date and other information of a resource
PARAM
	The resource to get the head for
***************************************************/
int		CUT_HTTPClient::SendHeadCommand(LPSTR url)
{
	char    buf[MAX_PATH * 2 + 1];
    char    file[MAX_PATH+1];
    int     error;
    
    //if the url string is empty then return
    if (url == NULL)
        return OnError(UTE_BAD_URL);

    //clear any prev data
    ClearReceivedData();

    // if not using proxy
    if(!m_bUseProxy)
        //get the server Address and file to retrieve
        if(SplitURL(url, file, sizeof(file)) != UTE_SUCCESS)
            return OnError(UTE_BAD_URL);

    //connect
   if ((error = HTTPConnect()) !=  UTE_SUCCESS)
	    return OnError(error);

    if(m_bUseProxy) {
        //send the HEAD command
        // NOTE:----> that when we add the proxy we don't add the slash after the 
        // HEAD command
        // if the url includes http then it is already an absolute URI
        // so lets just format our HEAD command string
        if (_strnicmp (url,"http://", strlen("http://"))==0)
            _snprintf(buf,sizeof(buf)-1,"HEAD %s HTTP/1.0\r\n",url);
        else // other wise prefix it with an http
            _snprintf(buf,sizeof(buf)-1,"HEAD http://%s HTTP/1.0\r\n", url);
        }
    else {
        if (file[0] =='/')
            _snprintf(buf,sizeof(buf)-1,"HEAD %s HTTP/1.0\r\n",file);
        else
            _snprintf(buf,sizeof(buf)-1,"HEAD /%s HTTP/1.0\r\n",file);
        }
    
    if(Send(buf) <= 0) {
        CloseConnection();
        return OnError(UTE_CONNECT_TERMINATED);
        }

    if(IsAborted()) {                       // Check if procedure is aborted
        CloseConnection();
        return OnError(UTE_ABORTED);        // Abort
        }
	return UTE_SUCCESS;

}
/***************************************************
  The DELETE method requests that the origin server delete the resource
  identified by the Request-URI.
PARAM:
    LPSTR url   - The full URI of the resource to be deleted
***************************************************/
int		CUT_HTTPClient::SendDeleteCommand(LPSTR url)
{
	char    buf[MAX_PATH * 2 + 1];
    char    file[MAX_PATH + 1];
    int     error;

    //if the url string is empty then return
    if (url == NULL)
        return OnError(UTE_BAD_URL);
    
    //clear any prev data
    ClearReceivedData();

    // if not using proxy
    if(!m_bUseProxy)
        //get the server Address and file to retrieve
        if(SplitURL(url, file, sizeof(file)) != UTE_SUCCESS)
            return OnError(UTE_BAD_URL);
    //connect to proxy
	if ((error = HTTPConnect()) !=  UTE_SUCCESS)
			return OnError(error);
    
    // if using proxy
    if(m_bUseProxy) {
        // if the url includes http then it is already an absolute URI
        // so lets just format our command string
        if (_strnicmp (url,"http://", strlen("http://"))==0)
            _snprintf(buf,sizeof(buf)-1,"DELETE %s HTTP/1.0\r\n",url);
        else
            _snprintf(buf,sizeof(buf)-1,"DELETE http://%s HTTP/1.0\r\n",url);
        }
    else
        _snprintf(buf,sizeof(buf)-1,"DELETE %s HTTP/1.0\r\n",  file);


    //send the command
    if(Send(buf) <= 0) {
        CloseConnection();
        return OnError(UTE_CONNECT_TERMINATED);
        }

    if(IsAborted()) {                       // Check if procedure is aborted
        CloseConnection();
        return OnError(UTE_ABORTED);        // Abort
    }
	return UTE_SUCCESS;

}
/**********************************************************

		The "basic" Authorization scheme is based on the model that the
		client must authenticate itself with a user-ID and a password for
		each realm.  The realm value should be considered an opaque string
		which can only be compared for equality with other realms on that
		server. The server will service the request only if it can validate
		the user-ID and password for the protection space of the Request-URI.
		There are no optional Authorization parameters

***********************************************************/
int	CUT_HTTPClient::AddBasicAuthorizationHeader(LPCWSTR userName, LPCWSTR password){
	return AddBasicAuthorizationHeader(AC(userName), AC(password));}
int	CUT_HTTPClient::AddBasicAuthorizationHeader(LPCSTR userName, LPCSTR password)
{
	if (userName == NULL || password == NULL)
	{
		return UTE_ERROR; // invalid parameters
	}
	int nNameLen = (int)strlen(userName);
	int nPassLen =  (int)strlen(password) ;

	if (  nNameLen <= 0  || nPassLen <= 0 )
		return UTE_ERROR; // invalid parameters
	
	char *pRawData = new char[nPassLen+nNameLen+2];
	_snprintf(pRawData,nPassLen+nNameLen+2, "%s:%s",userName,password);
	int nOutLen = (int)strlen(pRawData)*2;

	char *pResultData = new char[nOutLen];

	CBase64 base; 
	base.EncodeData ((BYTE*)pRawData,nPassLen+nNameLen+2,pResultData, nOutLen);
	delete pRawData;

	char *HeaderLine = new char [strlen(pResultData)+strlen("Authorization: Basic ")+2];

	_snprintf(HeaderLine,strlen(pResultData)+strlen("Authorization: Basic ")+2, "Authorization: Basic %s",pResultData);
	AddSendHeaderTag(HeaderLine);
	

	delete HeaderLine;
	delete pResultData;
	
	return 0;
	
}
/***********************************************************
		The "basic" Authorization scheme is based on the model that the
		client must authenticate itself with a user-ID and a password for
		each realm.  The realm value should be considered an opaque string
		which can only be compared for equality with other realms on that
		server. The server will service the request only if it can validate
		the user-ID and password for the protection space of the Request-URI.
		There are no optional Authorization parameters

REMARK:
	it is the responsibility of the caller to delete the resulting memory

***********************************************************/
int	CUT_HTTPClient::CreateProxyBasicAuthorizationHeader(LPCSTR ProxyUserName, LPCSTR ProxyPassword, LPSTR HeaderLine)
{
	if (ProxyUserName == NULL || ProxyPassword == NULL)
	{
		return UTE_ERROR; // invalid parameters
	}
	int nNameLen = (int)strlen(ProxyUserName);
	int nPassLen =  (int)strlen(ProxyPassword) ;

	if (  nNameLen <= 0  || nPassLen <= 0 )
		return UTE_ERROR; // invalid parameters
	
	char *pRawData = new char[nPassLen+nNameLen+2];
	_snprintf(pRawData,nPassLen+nNameLen+2,"%s:%s",ProxyUserName,ProxyPassword);
	int nOutLen = (int)strlen(pRawData)*2;

	char *pResultData = new char[nOutLen];

	CBase64 base; 
	base.EncodeData ((BYTE*)pRawData,nPassLen+nNameLen+2,pResultData, nOutLen);
	delete pRawData;

	HeaderLine = new char [strlen(pResultData)+strlen("Proxy-Authorization: Basic ")+2];

	_snprintf(HeaderLine,strlen(pResultData)+strlen("Proxy-Authorization: Basic ")+2, "Proxy-Authorization: Basic %s\r\n",pResultData);
	
	// the user will do call this line to add the authorization 
	// AddSendHeaderTag(HeaderLine);
	
	
	delete pResultData;
	
	return 0;
	
}
/*****************************************************
Set tne port number for the Proxy
Note that the proxy port number may differ from the target server 
port number.

PARAM
  newPort -  The new port number value
*****************************************************/
void CUT_HTTPClient::SetProxyPortNumber(int newPort)
{
	m_nProxyPort = newPort;
}

/****************************************************
Enable Tunneling:
	This call will tel the client that prior to issueing 
	the request you must request that the  proxy 
	establishes a tunnel for our communication.

	The differnet between a tunneled connection and 
	non-tunneled connection is that the proxy will not 
	attempt to modify our request by any means.
PARAM
	flag -  TRUE Tunneled is enabled 
*****************************************************/
void CUT_HTTPClient::EnableTunneling(BOOL flag)
{
	m_bEnableTunnelling = flag;

}
/*********************************************************
I have removed the connection part to be in a function by itself
because they were common to all methods. and I want to handle the connection 
separatly based on usage of proxy or not.

  If Tunneling is enabled we will call Request Tunnel
  Otherwise we will just connect to the proxy or the client
PARAM
	See connection errors
**********************************************************/
int CUT_HTTPClient::HTTPConnect()
{
	int error =  UTE_SUCCESS;
	//connect
	if (!m_bEnableTunnelling)
	{
		if((error = Connect(m_nPort, (m_bUseProxy) ? m_szProxyAddr : m_szServerName, m_nConnectTimeout)) != UTE_SUCCESS)
			return OnError(error);
	}
	else
	{
		return RequestTunnel();
	}
	return UTE_SUCCESS;
}
/********************************************************
Requesting a Tunnel with CONNECT

	A CONNECT method requests that a proxy establish a tunnel connection
	on its behalf. The Request-URI portion of the Request-Line is always
	an 'authority' as defined by URI Generic Syntax [2], which is to say
	the host name and port number destination of the requested connection
	separated by a colon:

	  CONNECT server.example.com:80 HTTP/1.1
	  Host: server.example.com:80

	Other HTTP mechanisms can be used normally with the CONNECT method --
	except end-to-end protocol Upgrade requests, of course, since the
	tunnel must be established first.

	For example, proxy authentication might be used to establish the
	authority to create a tunnel:

	  CONNECT server.example.com:80 HTTP/1.1
	  Host: server.example.com:80
	  Proxy-Authorization: basic aGVsbG86d29ybGQ=

	Like any other pipelined HTTP/1.1 request, data to be tunneled may be
	sent immediately after the blank line. The usual caveats also apply:
	data may be discarded if the eventual response is negative, and the
	connection may be reset with no response if more than one TCP segment
	is outstanding.

	For more info see RFC  2817 
PARAM:
	NONE
RET:
	Connect error -
	UTE_ERROR - The server rejected the request for tunneling

********************************************************/
int CUT_HTTPClient::RequestTunnel()
{
	
	int error = UTE_SUCCESS;
	//  since we are going to go through a tunnel
	// we need to make sure that the hand shake is 
	// handled by the client and the remote server not by the client and the proxy

	// so how do we do that.
	// first we disable the security setting and keep track of it
#ifdef CUT_SECURE_SOCKET
	BOOL currentSecuritySetting = GetSecurityEnabled();	
	SetSecurityEnabled(FALSE);
#endif
	// this will stop our client from trying to preform the handshake as soon as we connect.


	if((error = Connect(m_nProxyPort,  m_szProxyAddr, m_nConnectTimeout)) != UTE_SUCCESS)
			return OnError(error);

	// now we construct a string of which host to instruct the proxy to connect to
	char tempBuffer[500];

	_snprintf(tempBuffer,sizeof(tempBuffer)-1,"CONNECT %s:%d HTTP/1.1\r\n", m_szServerName,m_nPort);
	tempBuffer[499] = NULL;

	Send(tempBuffer);

	_snprintf(tempBuffer,sizeof(tempBuffer)-1,"Host: %s:%d \r\n", m_szServerName,m_nPort);
	tempBuffer[499] = NULL;
	Send(tempBuffer);


	char *ResultHeader = NULL;
	
	// create the proxy authorization header.
	if (CreateProxyBasicAuthorizationHeader(m_szProxyUserName,m_szProxyPassword,ResultHeader) == UTE_SUCCESS)
	{
		// now we send the proxy authorization 
		Send(ResultHeader);
		delete ResultHeader;
	}


	Send("\r\n");

	CUT_StringList ProxyResponseHeaders;
	
	// now we receive the proxy server response
	do
	{
			ReceiveLine(tempBuffer,sizeof(tempBuffer)-1,m_nConnectTimeout);
			ProxyResponseHeaders.AddString (tempBuffer);
	}
	while (IsDataWaiting());

	// now right here check for the server 
	// response and make sure that the second peice is 200
	// The server response will be the first line we receive 
	long nServerResponse = 0;
	CUT_StrMethods::ParseString ( ProxyResponseHeaders.GetString ((long)0)," ",1,&nServerResponse);
	if (nServerResponse > 199 && nServerResponse < 300)
	{
		// call the base class to continue the hand shake
		// notice that we are sending the name of the server
		// not the proxy as this function has received
		// 
#ifdef CUT_SECURE_SOCKET
	// Set back the security setting to it's origional status
	SetSecurityEnabled(currentSecuritySetting);
#endif

			return CUT_WSClient::SocketOnConnected(GetSocket (),m_szServerName);
	}
	// notice that we are setting the error code to be passed as the server response
	// in the case of error
	// the m_lLastStatusCode is not  set to the proxy response if the 
	// operation succeded
	m_lLastStatusCode = nServerResponse;


#ifdef CUT_SECURE_SOCKET
	// Set back the security setting to it's origional status
	SetSecurityEnabled(currentSecuritySetting);
#endif

	// tunneling failed
	return  UTE_ERROR;
}

/*************************************************
SetProxyPassword
	Set the password to be used in occuring a proxy
	authorization 
PARAM
	szPass - The password to be used in occuring 
	a proxy authorization
REMARK
	If the string passed is NULL or empty, the	proxy
	password will be set To NULL
**************************************************/
#if defined _UNICODE
int CUT_HTTPClient::SetProxyPassword(LPCWSTR szPass){
	return SetProxyPassword(AC(szPass));}
#endif
int CUT_HTTPClient::SetProxyPassword(LPCSTR szPass)
{
	if (m_szProxyPassword != NULL)
	{
		// for security reasons we want to zero the memory used so it can't be seen
		memset (m_szProxyPassword,0, strlen(m_szProxyPassword));
		delete m_szProxyPassword;
		m_szProxyPassword = NULL;
	}
	// is the password passed to us empty
	if ( szPass == NULL || szPass[0] == 0)
		return 0;	

	m_szProxyPassword = new char [strlen(szPass)+1];
	strcpy(m_szProxyPassword,szPass);		

	return UTE_SUCCESS;
}
/**************************************************************
	Set the user name for the proxy
	Notice that so far we are using a basic authentication with base 64 encoding
	the basic authentication is not highly recomended.
	We have added this functionality as requested by our clients.
REMARK
	A call to this function will replace the exsiting user name 
	with a new one or with a null if any of the parameter is wrong
PARAM
	szUserName - The proxy user name
RET
	UTE_SUCCESS
***************************************************************/
#if defined _UNICODE
int CUT_HTTPClient::SetProxyUserName(LPCWSTR szUserName){
	return SetProxyUserName(AC(szUserName));}
#endif
int CUT_HTTPClient::SetProxyUserName(LPCSTR szUserName)
{
	if (m_szProxyUserName != NULL)
	{
		// for security reason we want to 
		// mutate the memory used so it can't be recovered
		memset (m_szProxyUserName,0, strlen(m_szProxyPassword));
		delete m_szProxyUserName;
		m_szProxyUserName = NULL;
	}
	// is the password passed to us empty
  if ( szUserName == NULL || szUserName[0] == 0)
	  return 0;	

	  m_szProxyUserName = new char [strlen(szUserName)+1];
	  strcpy(m_szProxyUserName,szUserName);		

	return UTE_SUCCESS;
}
/*******************************************************
OnRedirect
	Ass the user for permition to continue following the redirection
	Some page request may be answered with a 3xx status code from the server.
	This class of status code indicates that further action needs to be
	taken by the user agent in order to fulfill the request.  The action
	required MAY be carried out by the user agent without interaction
	with the user if and only if the method used in the second request is
	GET or HEAD.
	
REMARK:
	A client SHOULD detect infinite redirection loops, since
	such loops generate network traffic for each redirection.
	Note: previous versions of this specification recommended a
	maximum of five redirections. Content developers should be aware
	that there might be clients that implement such a fixed
	limitation.

	Users of the CUT_HTTPClient can also take advantage of this function 
	to set up any cookies or additional request headers for state management 
	proccess.

PARAM:
	szUrl - a string describing the new URL we are being redirected to
RET
	TRUE - User accept to be redirected
	FALSE - DONT'REDIRECT.	
*******************************************************/
// v4.2 this notification overload could be reviewed
BOOL CUT_HTTPClient::OnRedirect(LPCWSTR /* szUrl */){
	return FALSE;
}

BOOL CUT_HTTPClient::OnRedirect(LPCSTR /* szUrl */){
	return FALSE;
}
/********************************************************
INTERNAL 
GetMovedLocation()
Get the redirect URL from the server response
PARAM
	LPSTR newUrl -  the location of the new url
RET
	TRUE - location was found
	FALSE - No url was found
********************************************************/
BOOL CUT_HTTPClient::GetMovedLocation(LPSTR newUrl )
{
      
    int     count,loop;
    char    buf[URL_RESOURCE_LENGTH+ 10];	
	char    location[URL_RESOURCE_LENGTH+ 10];
	BOOL	bFound = FALSE; 

    count = GetHeaderLineCount();
    for(loop = 0; loop < count ; loop++){
        CUT_StrMethods::ParseString(GetHeaderLine(loop),":",0,buf,sizeof(buf));
        if(_stricmp(buf,"Location") == 0 ){
			// get the schems
			if (CUT_StrMethods::ParseString(GetHeaderLine(loop)," ",1,location,URL_RESOURCE_LENGTH-1) ==  UTE_SUCCESS)
			{
				bFound = TRUE;
				// if the first charecter is / then lets make sure we add it to origional serevr address 
				if (location[0] == '/')
				{
					strcpy(newUrl,m_szServerName);
					strcat(newUrl,location);
				}	
				else
					strcpy(newUrl,location);
			}

			break;
        }
    }

    return bFound;
}

#pragma warning ( pop )
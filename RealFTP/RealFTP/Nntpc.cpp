// =================================================================
//  class: CUT_NNTPClient
//  File:  nntpc.cpp
//
//  Purpose:
//
//  Declaration of NNTP client class 
//
//  "Network News Transfer Protocol"
//  NNTP Specifies a protocol for the distribution, inquiry, 
//  retrieval, and posting of news articles using reliable 
//  stream-based transmission of news...
//
//  RFC 977
//  see the helper linked list classes CUT_NNTPGroupList & CUT_NNTParticleList
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


//MFC Users - include the following
#include "stdafx.h"
//OWL Users - include the following
//#include <owl\owlpch.h>
//SDK Users - include the following
//#include <windows.h>

#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>

#include "nntpc.h"

#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

/*************************************************
Constructor
**************************************************/
CUT_NNTPClient::CUT_NNTPClient() :
m_nConnectPort(119),                // Set default port to 119
m_nConnectTimeout(5),               // Set default connect time out to 5 sec.
m_nReceiveTimeout(20),              // Set default receive time out to 20 sec.
m_nPostingAllowed(FALSE),           // Clear posting allowed flag
m_bConnected(FALSE),                // Clear connection flag
m_szSelectedNewsGroup(NULL),        // Initialize pointer to the selected news group
m_nSelectedEstNum(0),               // Initialize number of articles available on the server upon establishing connection 
m_nSelectedFirst(0),                // Initialize id of the first article availabl on the srver 
m_nSelectedLast(0)                  // Initialize id of the last article availabl on the srver 
{
	m_OverviewFormat.ClearList ();
}
/*************************************************
Destructor
**************************************************/
CUT_NNTPClient::~CUT_NNTPClient(){
    m_listArticles.Empty();                 // Clear articles list
    m_listGroups.Empty();                   // Clear groups list
    EmptyNewArticlesList();                // Clear new articles list
	m_OverviewFormat.ClearList ();    
    if(m_szSelectedNewsGroup)               // Clear selected news group
        delete [] m_szSelectedNewsGroup;

	// cleanup _TCHAR article info proxy
	if(m_artW.lpszAuthor != NULL) delete [] m_artW.lpszAuthor;
	if(m_artW.lpszAuthorCharset != NULL) delete [] m_artW.lpszAuthorCharset;
	if(m_artW.lpszSubject != NULL) delete [] m_artW.lpszSubject;
	if(m_artW.lpszSubjectCharset != NULL) delete [] m_artW.lpszSubjectCharset;
	if(m_artW.lpszDate != NULL) delete [] m_artW.lpszDate;
	if(m_artW.lpszMessageId != NULL) delete [] m_artW.lpszMessageId;
	if(m_artW.lpszReferences != NULL) delete [] m_artW.lpszReferences;

}
/*************************************************
NConnect()
Connect to an NNTP server with a user name and passord
PARAM
HostName    - Host name or IP Address to connect to 
[User]      - The user name of the account to logon
to the server with (Default NULL)
[Pass]      - the password of the account 
to logon the server with (Default NULL)
RETURN:
UTE_SUCCESS         - success
UTE_CONNECT_FAILED  - connection failed
UTE_SVR_NO_RESPONSE - no response from host
UTE_CONNECT_REJECTED- connection not accepted
UTE_USER_FAILED     - USER failed
UTE_PASS_FAILED     - PASS failed
**************************************************/
#if defined _UNICODE
int CUT_NNTPClient::NConnect(LPCWSTR HostName, LPWSTR User, LPWSTR Pass){
	return NConnect(AC(HostName), AC(User), AC(Pass));}
#endif
int CUT_NNTPClient::NConnect(LPCSTR HostName, LPSTR User, LPSTR Pass)
{
    
    char    data[LINE_BUFFER_SIZE + 1];
    int     error = UTE_SUCCESS;
    
    GetAddressFromName(HostName,data, LINE_BUFFER_SIZE);
    
    // make sure we close the current connection if we are currently connected
    if (IsConnected())
        if (NClose () != UTE_SUCCESS)
            return OnError(UTE_SVR_NO_RESPONSE); // no response from the server
        
        //connect to the Relay Host
        if(Connect(m_nConnectPort, data, m_nConnectTimeout)!= UTE_SUCCESS) 
            return OnError(UTE_CONNECT_FAILED);       //connection failed
        
        //clear the recive  buffer
        ClearReceiveBuffer();
        
        //check for return value    200 means accepted and posting is allowed
        //                          201 means accepted and posting is not allowed
        //                          anything else, particularly 502 means connection not allowed
        do {
            if(ReceiveLine(data, LINE_BUFFER_SIZE) <= 0)
                return OnError(UTE_SVR_NO_RESPONSE);  //no response
            
            if(data[0] != '2') 
                return UTE_CONNECT_REJECTED;  // connection not accepted
            
        } while(data[0] != '2');
		
        long responseCode = GetResponseCode(data);
		
        switch(responseCode) {
        case 200:
            m_nPostingAllowed = CUT_POST_YES;
            break;
        case 201:
            m_nPostingAllowed = CUT_POST_NO;
            break;
        }
        
        m_bConnected = TRUE;
        
        if(User != NULL && strlen(User) > 0)
            if( (error = NAuthenticateUserSimple(User, Pass)) != UTE_SUCCESS)
                return OnError(error);       // authentication failed
            
            return OnError(UTE_SUCCESS);
}

/*************************************************
NClose() 
close the connection to an NNTP server
This function will attempt a graceful close if possible
but will force a close if necessary.  The only possible
result of this function is that the connection is terminated.
PARAM:
NONE
RETURN
UTE_SUCCESS         - success
UTE_SVR_NO_RESPONSE - no response from host
**************************************************/
int CUT_NNTPClient::NClose(){
    
    char    data[LINE_BUFFER_SIZE + 1];
    
    // setting the connection state to FALSE is acceptable here 
    // as this function will attempt a graceful close if possible
    // but will force a close if necessary.  The only possible
    // result of this function is that the connection is terminated.
    m_bConnected = FALSE;
    
    Send("QUIT\r\n");
    
    do {
        if(ReceiveLine(data, LINE_BUFFER_SIZE) <= 0) {
            CloseConnection();
            return OnError(UTE_SVR_NO_RESPONSE);  //no response
        }
        
        if(data[0]=='2') {
            CloseConnection();
            break;  // disconnection received
        }
        
    } while(data[0] != '2');  //205 indicates successful closing of NNTP session 
    
    CloseConnection();  // disconnect TCP/IP connection
    
    m_nPostingAllowed   = CUT_POST_NO;
    m_bConnected        = FALSE;
    
    return OnError(UTE_SUCCESS);
}


/*************************************************
NGetNewsGroupList()
Retrives the list of available News Groups on the connected server
issue a LIST command wich returns a list of valid newsgroups.
Each news group is sent as a line of text in the following format:
group last first posting
SEE ALSO NLoadNewsGroupList()
and RFC 977 section 3.6.1
PARAM
0   - will load all the newsgroups and clear our existing memory list
1   - will load only the newsgroups loaded since our last download
RETURN
UTE_SUCCESS             - success
UTE_CONNECT_TERMINATED  - connection terminated unexpectedly
UTE_INVALID_RESPONSE    - unexpected response
UTE_NOCONNECTION        - Not connected
UTE_ABORTED             - User canceled the Function call
**************************************************/
int CUT_NNTPClient::NGetNewsGroupList(int type)
{
    char    data[LINE_BUFFER_SIZE + 1];
    int     retval = UTE_SUCCESS;
    
    if (!IsConnected()) 
        return OnError(UTE_NOCONNECTION); 
    
    while(WaitForReceive(1,0) == UTE_SUCCESS)
        ClearReceiveBuffer();
    
    // issue a List command
    Send("LIST\r\n");
    
    if(ReceiveLine(data, LINE_BUFFER_SIZE) <= 0) 
        return OnError(UTE_CONNECT_TERMINATED); //connection terminted unexpectedly
	
   	long responseCode = GetResponseCode(data);
	
    // a response of 215 specifies that the list request was successful
    // so if we didn't get 215, then abort the process with an error
    if (responseCode != 215) 
        return OnError(UTE_INVALID_RESPONSE);   // unexpected response
    
    // 0 will load all the newsgroups and clear our existing memory list
    // 1 will load only the newsgroups loaded since our last download
    if (type == 0)
        m_listGroups.Empty();
    
    do {
        if(ReceiveLine(data, LINE_BUFFER_SIZE) <= 0) {
            retval = UTE_CONNECT_TERMINATED;    //connection terminted unexpectedly
            break;
        }
        
        if(data[0]=='.') {
            retval = UTE_SUCCESS;
            break;                              // end of list reached
        }
        
		char newsGroupName[MAX_PATH+1];
		//
		CUT_StrMethods::ParseString (data, " ",0, newsGroupName, sizeof(newsGroupName)-1 );
		m_listGroups.AddGroup(data);
		
        if (!OnNGetNewsGroupList(newsGroupName) || IsAborted() ) {  //check if the user wants to cancel
            ClearReceiveBuffer();
            retval = UTE_ABORTED; 
            break;
        }
        
    } while(data[0]!='.');
    
    return OnError(retval);  
}

/*************************************************
NSelectNewsGroup()
Select a news group from the available Groups on the Server 
By sending a GROUP command.
When a valid group is selected by means of this function
a server response  will contain the article numbers of the 
first and last articles in the group, and an estimate of 
aticles on file in group. 
See also CUT_NNTPGroupList::SetGroupInfo()    
and RFC 977 section 3.2.1
PARAM
newsGroup - name of the news group to be selected 
NOTE: The name of the news group is not case-dependent otherwise
match a newsgroup obtained from NGetNewsGroupList.
RETURN
CUT_SUCCCESS                    - Success
UTE_CONNECT_TERMINATED          - Connection terminated unexpectedly
UTE_INVALID_RESPONSE            - unexpected response
UTE_GROUP_INFO_UPDATE_FAILED    - unable to update group information, newsgroup name not in list.
UTE_NOCONNECTION                - Not connected
**************************************************/
#if defined _UNICODE
int CUT_NNTPClient::NSelectNewsGroup(LPCWSTR newsGroup){
	return NSelectNewsGroup(AC(newsGroup));}
#endif
int CUT_NNTPClient::NSelectNewsGroup(LPCSTR newsGroup)
{
    char    data[LINE_BUFFER_SIZE + 1];
    char    buf[100];
    
    if (!IsConnected()) 
        return OnError(UTE_NOCONNECTION); 
    
    _snprintf(buf,sizeof(buf)-1,"group %s \r\n",newsGroup);
    Send(buf);
    
    if(ReceiveLine(data,LINE_BUFFER_SIZE) <= 0)
        return OnError(UTE_CONNECT_TERMINATED);
    
    // take this opportunity to retrieve the current information
    // for this newsgroup, number of articles, starting and ending
    // article number and whether posting is permitted or not.
   	long responseCode = GetResponseCode(data);
	
	
    // a response of 211 specifies that the selection was successful
	if (responseCode != 211)
        return OnError(UTE_INVALID_RESPONSE);       // invalid response
    
	// we want to pass a long pointer 
	// so we need new  variables as the other ones are int
    long number, first, last;
    
	CUT_StrMethods::ParseString (data," ", 1, &number);
	CUT_StrMethods::ParseString (data," ", 2, &first);
	CUT_StrMethods::ParseString (data," ", 3, &last);
	
	
    if(m_szSelectedNewsGroup)
        delete [] m_szSelectedNewsGroup;
    
    m_szSelectedNewsGroup = new char[strlen(newsGroup) + 1];
    strcpy(m_szSelectedNewsGroup, newsGroup);
    
    
    m_nSelectedFirst    = first;
    m_nSelectedLast     = last;
    m_nSelectedEstNum   = number;
    
    if (m_listGroups.SetGroupInfo(newsGroup, number, first, last) <0)
        return OnError(UTE_GROUP_INFO_UPDATE_FAILED);  // unable to update group information, newsgroup name not in list.
    
    return OnError(UTE_SUCCESS);
}

/*************************************************
Save()
Loop through the list of available news groups available 
and save the summaru information for each group to a disk file
in the following format:
{"newsGroupName postingAllowed startId endId  postingAllowed"} 
PARAM :
filename    - the name of the target file name to save the list to
RETURN:
UTE_SUCCESS         - success
UTE_ERROR           - the Group list is empty
UTE_DS_OPEN_FAILED  - failed to open data source
UTE_DS_CLOSE_FAILED - failed to close data source
**************************************************/
int CUT_NNTPClient::NSaveNewsGroupList(CUT_DataSource &dest) {
    return OnError(m_listGroups.Save(dest));    
}

/**************************************************
Save()
Save the current list of articles headers to a disk file
The articles headers will be stored in the following format, this format is
the same format an XOVER command would be answered
"nArticleId[TAB]lpszSubject[TAB]Author[TAB]Date[TAB]Refrences[TAB]BytesCount[TAB]NumberOfLines[CRLF]"
We will save the articles list in the same format
PARAM
filename    - File name to save the articles list to
RETURN
UTE_SUCCESS         - success
UTE_ERROR           - the Group list is empty
UTE_DS_OPEN_FAILED  - failed to open data source
UTE_DS_CLOSE_FAILED - failed to close data source
****************************************************/
int CUT_NNTPClient::NSaveArticleHeaderList(CUT_DataSource &dest) {
    return OnError(m_listArticles.Save (dest));  
}

/**************************************************
NLoadArticleHeaderList
Load the current list of articles headers from a disk file
The articles headers will be retrived in the following format, this format is
the same format an XOVER command would be answered
"nArticleId[TAB]lpszSubject[TAB]Author[TAB]Date[TAB]Refrences[TAB]BytesCount[TAB]NumberOfLines[CRLF]"
We will then call the AddXOVERArticle() to parse the result
PARAM
filename    - File name to save the articles list to
RETURN
UTE_SUCCESS         - success
UTE_DS_OPEN_FAILED  - failed to open data source
UTE_DS_CLOSE_FAILED - failed to close data source
***************************************************/
int CUT_NNTPClient::NLoadArticleHeaderList(CUT_DataSource &source) {
    return OnError(m_listArticles.Load(source));  
}

/*************************************************
Load()
Read strings from the files and load them to the group list.
{"newsGroupName postingAllowed startId endId  postingAllowed"} 
This function was added to allow for caching News Groups and their
information to allow connecting and refrencing news group names 
without having to query the server for them each time you connect
PARAM :
LPCSTR file - the name of the source file name to read the list from
RETURN:
UTE_SUCCESS         - success
UTE_DS_OPEN_FAILED  - failed to open data source
UTE_DS_CLOSE_FAILED - failed to close data source
**************************************************/
int CUT_NNTPClient::NLoadNewsGroupList(CUT_DataSource &dest) {
    return OnError(m_listGroups.Load(dest));
}

/*************************************************
NGetNewsGroupCount()
Retrieve the number of news group in the list
PARAM
none
RETURN 
number of available news groups
**************************************************/
long CUT_NNTPClient::NGetNewsGroupCount() const
{
    return m_listGroups.GetCount();
}

/*************************************************
NGetArticleHeaderCount()
Retrieve the number of news articles in the articles list
PARAM
none
RETURN 
number of available news articles
**************************************************/
long CUT_NNTPClient::NGetArticleHeaderCount() const
{
    return m_listArticles.GetCount();
}

/*************************************************
NGetArticleInfo()
Retrieve the information for the indexed article
PARAM
int index - the index of the article requested
RETURN:
NULL    - error
pointer to UT_ARTICLEINFO structure
**************************************************/
UT_ARTICLEINFO *CUT_NNTPClient::NGetArticleInfo(int index) 
{
	UT_ARTICLEINFOA * temp = m_listArticles.GetArticleInfo(index);

	if(m_artW.lpszAuthor != NULL) delete [] m_artW.lpszAuthor;
	if(m_artW.lpszAuthorCharset != NULL) delete [] m_artW.lpszAuthorCharset;
	if(m_artW.lpszSubject != NULL) delete [] m_artW.lpszSubject;
	if(m_artW.lpszSubjectCharset != NULL) delete [] m_artW.lpszSubjectCharset;
	if(m_artW.lpszDate != NULL) delete [] m_artW.lpszDate;
	if(m_artW.lpszMessageId != NULL) delete [] m_artW.lpszMessageId;
	if(m_artW.lpszReferences != NULL) delete [] m_artW.lpszReferences;

	if(temp != NULL) {
		// copy members from ANSI based struct to _TCHAR 
		m_artW.nArticleId = temp->nArticleId;
		m_artW.nByteCount = temp->nByteCount;
		m_artW.next = NULL;
		m_artW.nLineCount = temp->nLineCount;
		m_artW.nStatus    = temp->nStatus;

		CUT_HeaderEncoding enc;
		size_t len = 0;
		LPCSTR lpszDecoded = NULL;
		char szCharSet[30];


		// use this copy operation to decode From and Subject strings for the user
		if(enc.IsEncoded(temp->lpszAuthor)) { // decode
			lpszDecoded = enc.Decode(temp->lpszAuthor, szCharSet);

			len = strlen(lpszDecoded);
			m_artW.lpszAuthor = new _TCHAR[len+1];
			CUT_Str::cvtcpy(m_artW.lpszAuthor, len+1, lpszDecoded);

			len = strlen(szCharSet);
			m_artW.lpszAuthorCharset = new _TCHAR[len+1];
			CUT_Str::cvtcpy(m_artW.lpszAuthorCharset, len+1, szCharSet);
		}
		else {
			// alloc for copy
			len = strlen(temp->lpszAuthor);
			m_artW.lpszAuthor = new _TCHAR[len+1];
			CUT_Str::cvtcpy(m_artW.lpszAuthor, len+1, temp->lpszAuthor);
			m_artW.lpszAuthorCharset = NULL;
		}

		if(enc.IsEncoded(temp->lpszSubject)) { // decode
			lpszDecoded = enc.Decode(temp->lpszSubject, szCharSet);
			
			len = strlen(lpszDecoded);
			m_artW.lpszSubject = new _TCHAR[len+1];
			CUT_Str::cvtcpy(m_artW.lpszSubject, len+1, lpszDecoded);

			len = strlen(szCharSet);
			m_artW.lpszSubjectCharset = new _TCHAR[len+1];
			CUT_Str::cvtcpy(m_artW.lpszSubjectCharset, len+1, szCharSet);
		}
		else {
			// alloc for copy
			len = strlen(temp->lpszSubject);
			m_artW.lpszSubject = new _TCHAR[len+1];
			CUT_Str::cvtcpy(m_artW.lpszSubject, len+1, temp->lpszSubject);
			m_artW.lpszSubjectCharset = NULL;
		}

		len = strlen(temp->lpszDate);
		m_artW.lpszDate = new _TCHAR[len+1];
		CUT_Str::cvtcpy(m_artW.lpszDate, len+1, temp->lpszDate);

		len = strlen(temp->lpszMessageId);
		m_artW.lpszMessageId = new _TCHAR[len+1];
		CUT_Str::cvtcpy(m_artW.lpszMessageId, len+1, temp->lpszMessageId);

		len = strlen(temp->lpszReferences);
		m_artW.lpszReferences = new _TCHAR[len+1];
		CUT_Str::cvtcpy(m_artW.lpszReferences, len+1, temp->lpszReferences);
		
		return &m_artW;
	}
	else {
		return NULL;
	}
}

/*************************************************
NGetNewsGroupTitle()
Get the title of the news group specified bby the index
PARAM
index   - index of the news group to be deleted
RETURN
NULL    - error
news group title
**************************************************/
LPCSTR CUT_NNTPClient::NGetNewsGroupTitle(int index) const
{
    return m_listGroups.GetGroupTitle(index);
}

/*************************************************
NGetNewsGroupTitle()
Get the title of the news group specified bby the index
PARAM
title	  - [out] pointer to buffer to receive title
maxSize - length of buffer
index   - index of the news group to be deleted
size    - [out] length of title

  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- title and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - title not found
  UTE_BUFFER_TOO_SHORT  - space in title buffer indicated by maxSize insufficient, realloc 
  based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
#if defined _UNICODE
int CUT_NNTPClient::NGetNewsGroupTitle(LPWSTR title, size_t maxSize, int index, size_t *size){
	
	int retval;
	
	if(maxSize > 0) {
		char * titleA = new char [maxSize];
		
		if(titleA != NULL) {
			retval = NGetNewsGroupTitle( titleA, maxSize, index, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(title, maxSize, titleA);
			}
			
			delete [] titleA;
			
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = NGetNewsGroupTitle(index);
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
/**************************************************/
/**************************************************/
int CUT_NNTPClient::NGetNewsGroupTitle(LPSTR title, size_t maxSize, int index, size_t *size){
	
	int retval = UTE_SUCCESS;
	
	if(title == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = m_listGroups.GetGroupTitle(index);
		
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
				strcpy(title, str);
			}
		}
	}
	return retval;
}

/*************************************************
NGetArticleList()
Attempt to get a list of the articles avilable on the selected 
news group. Add to existing list, replace existing list or merge, etc.
To make this work we will first try to use the XOVER command
which generates a continuous list of information about the 
articles in a news group.  Xover returns the article number, 
lpszSubject, MessageID, name of poster, and time of posting.
to make XOVER work, we need the starting and ending article numbers
to form a range.  This information is provided when we select the newsgroup
If Xover support is not available, we will iterate through
the list of articles by using a series of HEAD and then NEXT
commands.  HEAD returns the full header for the article, in the 
standard Internet EMAIL RFC822 format.
PARAM
0 will load all the articleList and clear our existing memory list
1 will load only the articleList loaded since our last download
RETURN
UTE_SUCCESS             - success
UTE_XOVER_FAILED        - xover command failed
UTE_CONNECT_TERMINATED  - connection terminated unexpectedly
UTE_INVALID_RESPONSE    - unexpected response
UTE_NOCONNECTION        - Not connected
UTE_ABORTED             - User canceled the Function call
**************************************************/
int CUT_NNTPClient::NGetArticleList(int type)
{
    
    char        *data;
    const int   datalen = 2047;
    int         retval = UTE_SUCCESS;
    char        buf[LINE_BUFFER_SIZE + 1];
    int         Xover = FALSE;
	//   char        *token;
    
    data = new char[datalen + 1];
	
	// v4.2 changed this...
	//    while(WaitForReceive(1,0) == UTE_SUCCESS)
    while(WaitForReceive(0,0) == UTE_SUCCESS)
        ClearReceiveBuffer();
	// I have previously hard coded the format I am expecting from the remote server
	// now I want to make sure I receive the format first
	
	retval  = GetOverviewFormat();
	if (retval == UTE_SUCCESS)
	{
		
		_snprintf(buf,sizeof(buf)-1, "xover %d-%d\r\n",m_nSelectedFirst, m_nSelectedLast);
		Send(buf);
		
		if(ReceiveLine(data, datalen) <= 0)
		{
			delete[] data;
			return OnError(UTE_CONNECT_TERMINATED);     //connection terminated unexpectedly
		}
		
		//2.8.1 Responses
		//	    224 Overview information follows
		//		412 No news group current selected
		//		420 No article(s) selected
		//		502 no permission
		
		/*
		Each line of output will be formatted with the article number,
		followed by each of the headers in the overview database or the
		article itself (when the data is not available in the overview
		database) for that article separated by a tab character.  The
		sequence of fields must be in this order: subject, author, date,
		message-id, references, byte count, and line count.  Other optional
		fields may follow line count.  Other optional fields may follow line
		count.  These fields are specified by examining the response to the
		LIST OVERVIEW.FMT command.  Where no data exists, a null field must
		be provided (i.e. the output will have two tab characters adjacent to
		each other).  Servers should not output fields for articles that have
		been removed since the XOVER database was created.
		*/
		long responseCode = GetResponseCode(data);
		if(responseCode == 224)
			Xover = TRUE;
		else
			Xover = FALSE;
		
		if (Xover)
		{
			// the xover command was successful so let's proceed with
			// xover downloading of article data.
			// the 211 return line contains other information about the article list to 
			// be returned
			if (type == 0 ) // do we want to replace or merge the article list
				m_listArticles.Empty();
			
			//based on the List format 
			// lets have the index of each column identified.
			int nId = 0;  // by default
			int nSubject  = 0;
			int nXref = 0;
			int nFrom = 0;
			int nDate = 0;
			int nByte = 0;
			int nMsgId = 0;
			int nNumberOfLines = 0;
			int nRefrence = 0;
			int nTopic = 0;
			
			for (int loop = 0; loop < m_OverviewFormat.GetCount (); loop++ )
			{
				if (nSubject == 0 && stricmp(m_OverviewFormat.GetString (loop),"Subject") == 0)
				{
					nSubject = loop+1;
					
				}
				else if (nFrom == 0 && stricmp(m_OverviewFormat.GetString (loop),"From") == 0)
				{
					nFrom = loop+1;
					
				}
				else if (nDate == 0 && stricmp(m_OverviewFormat.GetString (loop),"Date") == 0)
				{
					nDate = loop+1;
					
				}
				else if (nXref == 0 && stricmp(m_OverviewFormat.GetString (loop),"Xref") == 0)
				{
					nXref = loop+1;
					
				}
				else if (nMsgId == 0 && stricmp(m_OverviewFormat.GetString (loop),"Message-Id") == 0)
				{
					nMsgId = loop+1;
					
				}
				else if (nByte == 0 && stricmp(m_OverviewFormat.GetString (loop),"Bytes") == 0)
				{
					nByte = loop+1;
					
				}
				else if (nNumberOfLines == 0 && stricmp(m_OverviewFormat.GetString (loop),"Lines") == 0)
				{
					nNumberOfLines = loop+1;
					
				}
				else if (nRefrence == 0 && stricmp(m_OverviewFormat.GetString (loop),"References") == 0)
				{
					nRefrence = loop+1;
				}			
				else if (nTopic == 0 && stricmp(m_OverviewFormat.GetString (loop),"Thread-Topic") == 0)
				{
					nTopic = loop+1;
				}			
				
			}
			
			
			do{
				if(ReceiveLine(data,datalen) <= 0) 
				{
					retval = UTE_CONNECT_TERMINATED;    // connection terminted unexpectedly
					break;
				}
				
				if(data[0]=='.')
				{
					retval = UTE_SUCCESS;
					break;                              // end of list reached
				}
				// mdoified August 05 99
				// 1 ) nArticleId[TAB]
				char artId[32] ;
				artId[0] = 0;
				
				CUT_StrMethods::ParseString (data,"\t",nId,artId,32-1);
				
				// 2 ) lpszSubject[TAB]
				char subject[MAX_PATH+1] ;
				subject[0] = 0;
				
				if (nSubject)
					CUT_StrMethods::ParseString (data,"\t",nSubject,subject,MAX_PATH-1);
				
				// 3 ) Author[TAB]
				char author[MAX_PATH+1] ;
				author[0] = 0;
				if(nFrom)
					CUT_StrMethods::ParseString (data,"\t",nFrom,author,MAX_PATH-1);
				
				// 4 ) Date[TAB]
				char date[MAX_PATH+1];
				date[0] = 0;
				
				if(nDate)
					CUT_StrMethods::ParseString (data,"\t",nDate,date,MAX_PATH-1);
				
				// 5 ) Refrences[TAB]
				char refrence[MAX_PATH+1] ;
				refrence[0] = 0;
				
				if (nRefrence)
					CUT_StrMethods::ParseString (data,"\t",nRefrence,refrence,MAX_PATH-1);
				
				// 6) BytesCount[TAB]
				char BytesCount[32] ;
				BytesCount[0] = 0;
				
				if(nByte)
					CUT_StrMethods::ParseString (data,"\t",nByte,BytesCount,32-1);
				// 7) NumberOfLines[CRLF]
				char NumberOfLines[32] ;	
				NumberOfLines[0] = 0;
				if(nNumberOfLines)
					CUT_StrMethods::ParseString (data,"\t",nNumberOfLines,NumberOfLines,32-1);
				
				// added xrefs string to notification
				char xref[MAX_PATH];
				xref[0] = 0;
				if(nXref)
					CUT_StrMethods::ParseString (data,"\t",nXref,xref,MAX_PATH-1);
				
				// Note for next update
				// we are not informing the client of the XREF if it was found 
				// this is a limitattion by design 
				// it should be changed in next version.
				
				if (!OnNGetArticleList(artId, subject, author, date, refrence, BytesCount, NumberOfLines, xref ) || IsAborted() ) {
					ClearReceiveBuffer();
					retval = UTE_ABORTED;               // aborted
					break;
				}
				
				// let the article list know about the format of this group
				m_listArticles.SetOverViewFormat(m_OverviewFormat);
				m_listArticles.AddArticleXOVER(data);
				
			}while(data[0]!='.');
		}
		else 
		{
			m_listArticles.Empty();
			retval =  UTE_XOVER_COMMAND_FAILED;                             // xover command failed
		}                                           // end of HEAD and NEXT section
		
	} 
	else 
	{
		m_listArticles.Empty();
		retval =  UTE_XOVER_COMMAND_FAILED;                             // xover command failed
	}                                           // end of HEAD and NEXT section
	delete [] data; // reclaim memory
	return OnError(retval);  
}
/*************************************************
NGetNewArticlesCount
return number of new articles in the list
PARAM:
none
RETURN
long - number of new articles 
**************************************************/
long CUT_NNTPClient::NGetNewArticlesCount() const
{
    return m_listNewArticles.GetCount();
}


/*************************************************
NCanPost()
Check to see if posting is allowed on the 
selected server. The result is based on the connection attempt 
result when you call NConnect()
SEE ALSO NForceCheckCanPost()
RETURN
UTE_SUCCESS         - Posting is allowed
UTE_NNTP_NOPOSTING  - No posting allowd
**************************************************/
int CUT_NNTPClient::NCanPost(){
    if (m_nPostingAllowed)
        return UTE_SUCCESS;
    return UTE_ERROR;
}

/*************************************************
NForceCheckCanPost()
This routine forces the system to verify 
whether the client can post messages.  Normally
during connection, the system will indicate 
whether posting is accepted as part of the connect
response.  During the connect response, a class
member variable m_nPostingAllowed is set to true
or false to indicate this result.  
This function attempts to verify whether posting
is allowed currently.  NNTP does not provide a standard method of verifying 
whether posting is allowed, however the 'mode reader' 
command does respond with permission information (if
that command is supported). In the worst case, 
we can issue a 'POST' command and send a fake message
to see if the Post command is denied.  NNTP servers
will reject the malformed (empty) message generated by
this function and should not cause any difficulty.
Permission to post to a newsgroup can be allowed or
denied system wide, and can be allowed or denied for
specific newsgroups.
PARAM
NONE
RETURN
UTE_SUCCESS             - Popsting allowed
UTE_NNTP_NOPOSTING      - a response of 440 specifies that the command 
was recognized and that posting is not permitted
UTE_CONNECT_TERMINATED  - connection terminated unexpectedly
**************************************************/
int CUT_NNTPClient::NForceCheckCanPost(){
    
    char    data[LINE_BUFFER_SIZE + 1];
    int     retval = UTE_SUCCESS;
    
    //try the simpler mode reader test for posting capability
    Send("mode reader\r\n");
    
    if(ReceiveLine(data, LINE_BUFFER_SIZE) <= 0 ) 
        return OnError(UTE_CONNECT_TERMINATED);     //connection terminted unexpectedly
	long responseCode = GetResponseCode(data);   
    switch(responseCode) {
    case 200:
        // a response of 200 specifies that the selection was successful
        // and that posting is allowed
        retval = UTE_SUCCESS;
        m_nPostingAllowed = TRUE;
        break;
    case 201:
        // a response of 201 specifies that the selection was successful
        // and that posting is NOT allowed
        return OnError(UTE_NNTP_NOPOSTING);  
        break;
    case 205:
        // authentication required
        break;
    case 400:
        // service temporarily unavailable
        break;
    case 502:
        // service unavailable
        break;
    }
    
    // since we did not return yet then
    // The system has not recognized the command, so we should try a 
    // more forceful attempt to verify POST permission.
    Send("POST\r\n");
    
    if(ReceiveLine(data, LINE_BUFFER_SIZE) <= 0) 
        return OnError(UTE_CONNECT_TERMINATED);     //connection terminted unexpectedly
    
    // if this succeed we expect to receive a 300 series code asking us
    // to deliver the message.
    responseCode = GetResponseCode(data);   
    switch(responseCode){
    case 240:   // article posted OK
        retval = UTE_SUCCESS;
        break;
    case 340:  // posting permitted, please proceed.
        //the post command normally returns response 340 indicating that
        //the server is awaiting the message data.  If we receive this 
        //response, we know that posting is permitted.
        Send("\r\n.\r\n");  // terminate the blank message and ensure the server is left in an acceptable state.
        retval = UTE_SUCCESS;       
        break;
    case 381:  // 381 Posting allowed.
        //Microsoft's news server returns this code for some reason
        //the post command normally returns response 381 indicating that
        //the server is awaiting the message data.  If we receive this 
        //response, we know that posting is permitted.
        Send("\r\n.\r\n");  // terminate the blank message and ensure the server is left in an acceptable state.
        retval = UTE_SUCCESS;       // 381 Posting allowed
        break;
        
    case 440:
        // a response of 440 specifies that the command was recognized
        // and that posting is not permitted
        retval = UTE_NNTP_NOPOSTING;
        break;
        
    case 441:   // posting failed
        retval = UTE_NNTP_NOPOSTING;
        break;
    default:
        retval = UTE_NNTP_NOPOSTING; // we will default to no
    }
    
    return OnError(retval);
}

/***********************************
NPostArticle
This version of post article diffres from the NPostArticle in that it
does not expect the user to know the format of RFC822 message  & NNTP article 
Param:
PostGroup     The news group to which the article is to be posted
PostFrom      The author Idintification & or email as to apear on the article 
PostlpszSubject   The lpszSubject of the article
PostArticle   The text body of the message 
Return:
UTE_SUCCESS                 - success
UTE_ERROR                   - failed
UTE_PARAMETER_INVALID_VALUE - invalid parameter
UTE_NNTP_NOPOSTING          - a response of 440 specifies that the command 
was recognized and that posting is not permitted
UTE_NNTP_POST_FAILED        - posting rejected by server
UTE_NNTP_BAD_ARTICLE        - posting failed because of bad article
UTE_CONNECT_TERMINATED      - connection terminated unexpectedly
******************************/
#if defined _UNICODE
int CUT_NNTPClient::NPostArticle(LPCWSTR PostGroup, LPCWSTR PostFrom , LPCWSTR PostlpszSubject,LPCWSTR PostArticle) {
	return NPostArticle(AC(PostGroup), AC(PostFrom), AC(PostlpszSubject), AC(PostArticle));}
#endif
int CUT_NNTPClient::NPostArticle(LPCSTR PostGroup, LPCSTR PostFrom , LPCSTR PostlpszSubject,LPCSTR PostArticle) {
    if(PostGroup == NULL || PostFrom == NULL || PostlpszSubject == NULL || PostArticle == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);
    
    CUT_Msg     message;
    BOOL        bResult = TRUE;
    
    //*******************************
    //** Initialize message object **
    //*******************************
    
    // Set message headers fields
    bResult &= message.AddHeaderField(PostGroup, UTM_NEWSGROUPS);
    bResult &= message.AddHeaderField(PostFrom, UTM_FROM);
    bResult &= message.AddHeaderField(PostlpszSubject, UTM_SUBJECT);
    if(!bResult)
        return OnError(UTE_ERROR);
    
    // Set message body
    if(!message.SetMessageBody(PostArticle))
        return OnError(UTE_MSG_BODY_TOO_BIG);
    
    return NPostArticle(message);
}

/*************************************************
NPostArticle()
post a message as an article to an NNTP News Group
four fields are MUST:
Newsgroups:
From:
lpszSubject:
< (Optional headers of RFC822) >
'Blank Line'
" The body of the article
...........
.. "
The terminating period of RFC822 message will be added by this function 
PARAM
message  -  The message to be posted in an RFC822 fomat as follow
RETURN
UTE_SUCCESS                 - success
UTE_PARAMETER_INVALID_VALUE - invalid parameter
UTE_NNTP_NOPOSTING          - a response of 440 specifies that the command 
was recognized and that posting is not permitted
UTE_NNTP_POST_FAILED        - posting rejected by server
UTE_NNTP_BAD_ARTICLE        - posting failed because of bad article
UTE_CONNECT_TERMINATED      - connection terminated unexpectedly
**************************************************/
int CUT_NNTPClient::NPostArticle(CUT_Msg & message)
{ 
    char    data[LINE_BUFFER_SIZE + 1];
    int     retval = UTE_SUCCESS;
    
    Send("POST\r\n");
    
    if(ReceiveLine(data, LINE_BUFFER_SIZE) <= 0) 
        return OnError(UTE_CONNECT_TERMINATED);     //connection terminted unexpectedly    
    
    // if this succeed we expect to receive a 300 series code asking us
    // to deliver the message.
	long responseCode = GetResponseCode(data);   
    switch(responseCode) {
    case 240:   // article posted OK
        break;
        
    case 340:  // posting permitted, please proceed.
        {
            LPCSTR  ptrData = NULL, ptrFieldName = NULL;
            char    ptrPrevCustomName[MAX_FIELD_NAME_LENGTH];
            int     index           = 0;
            int     nAttachNumber   = message.GetAttachmentNumber();
            BOOL    bDateSent       = FALSE;
            
            ptrPrevCustomName[0] = 0;
            
            _TCHAR    *lpszTempFileName = _ttempnam(NULL, _T("ENCODE"));
            if(lpszTempFileName == NULL)
                return OnError(UTE_FILE_TMP_NAME_FAILED);
            
            LPSTR   msg_body = const_cast<LPSTR> (message.GetMessageBody());
            
            CUT_BufferDataSource    MsgBodySource(msg_body, strlen(msg_body));
            CUT_FileDataSource      AttachSource(lpszTempFileName);
			CUT_StringList			listGlbHeaders;
            
            //the post command normally returns response 340 indicating that
            //the server is awaiting the message data.  If we receive this 
            //response, we know that posting is permitted.
            
            
            // If we have an attachments
            if(nAttachNumber > 0) {
				// Get global header list
                index = 0;
                while(retval == UTE_SUCCESS && (ptrData = message.GetGlobalHeader(index++)) != NULL) 
					listGlbHeaders.AddString(ptrData);
				
                // Encode attachments
                if((retval = message.Encode(MsgBodySource, AttachSource)) != UTE_SUCCESS)
                    return OnError(retval);
            }
            
            
            // Open data sources
            if(MsgBodySource.Open(UTM_OM_READING) == -1)
                return OnError(UTE_DS_OPEN_FAILED);
            
            if(message.GetAttachmentNumber() > 0) 
                if(AttachSource.Open(UTM_OM_READING) == -1)
                    return OnError(UTE_DS_OPEN_FAILED);
                
                // calculate total send size for send status reporting.
                // for efficiency, ignore headers in calculation.
                long lengthOfMail = MsgBodySource.Seek(0,SEEK_END);
                MsgBodySource.Seek(0,SEEK_SET);
                
                if(nAttachNumber > 0) {
                    lengthOfMail += AttachSource.Seek(0,SEEK_END);
                    AttachSource.Seek(0,SEEK_SET);
                }
                
                
                // Send all fields
                int     nFieldType;
                BOOL    bFirstFieldLine = TRUE;
                for(nFieldType = UTM_MESSAGE_ID; retval == UTE_SUCCESS && nFieldType < UTM_MAX_FIELD; nFieldType++ ) {
                    
                    // Get known field name
                    if(nFieldType != UTM_CUSTOM_FIELD) 
                        ptrFieldName = message.GetFieldName((HeaderFieldID)nFieldType);
                    
                    index = 0;
                    bFirstFieldLine = TRUE;
                    while(retval == UTE_SUCCESS && (ptrData = message.GetHeaderByType((HeaderFieldID)nFieldType, index )) != NULL) {
                        
                        // Get custom field name
                        if(nFieldType == UTM_CUSTOM_FIELD) {
                            ptrFieldName = message.GetCustomFieldName(index);
                            if(stricmp(ptrPrevCustomName, ptrFieldName) != 0)
                                bFirstFieldLine = TRUE;
                            strncpy(ptrPrevCustomName, ptrFieldName, MAX_FIELD_NAME_LENGTH);
                        }
                        
                        // Add field name to the first line for every known field
                        if(bFirstFieldLine && ptrFieldName != NULL) 
                            if(Send(ptrFieldName, (int)strlen(ptrFieldName)) != (int)strlen(ptrFieldName)) 
                                retval = UTE_SOCK_SEND_ERROR;
                            
                            // Add a space if needed
                            if(ptrData[0] != ' ' && ptrData[0] != '\t' )
                                if(Send(" ", 1) != 1) 
                                    retval = UTE_SOCK_SEND_ERROR;
                                
                                bFirstFieldLine = FALSE;                    
                                
                                // Send field data
                                if(Send(ptrData, (int)strlen(ptrData)) != (int)strlen(ptrData)) 
                                    retval = UTE_SOCK_SEND_ERROR;
                                
                                // Add new line if needed
                                if(!CUT_StrMethods::IsWithCRLF(ptrData))
                                    if(Send("\r\n", 2) != 2) 
                                        retval = UTE_SOCK_SEND_ERROR;
                                    
                                    // We've just sent the date. Don't send current date.
                                    if(nFieldType == UTM_DATE)
                                        bDateSent = TRUE;
                                    
                                    ++index;
                    }
                }
                
                
                // If we have an attachments
                if(nAttachNumber > 0) {
                    // Add encoding headers fields
                    
                    for(index = 0; index < listGlbHeaders.GetCount(); index++) 
                        if(Send(listGlbHeaders.GetString(index), (int)strlen(listGlbHeaders.GetString(index))) != (int)strlen(listGlbHeaders.GetString(index))) 
                            retval = UTE_SOCK_SEND_ERROR;
                        if(!CUT_StrMethods::IsWithCRLF(listGlbHeaders.GetString(index)))
                            if(Send("\r\n", 2) != 2) 
                                retval = UTE_SOCK_SEND_ERROR;
                }               
				
                // Send Article body
                Send("\r\n");
                
                // If we don't have any attachments- just send the Article
                char    szMessageLine[MAX_LINE_LENGTH + 1];
                int     nLineSize, nMaxSend = GetMaxSend();
                int     nBytesSend, nSendPos = 0;
                long    nTotalSent = 0;
                if(nAttachNumber <= 0) {
                    
                    // Send message line by line
                    while(retval == UTE_SUCCESS && (nLineSize = MsgBodySource.ReadLine(szMessageLine, MAX_LINE_LENGTH)) > 0) {
                        
                        if(IsAborted())
                            retval = UTE_ABORTED;
                        
                        if(szMessageLine[0] == '.')
                            Send(".", 1);
                        
                        // If line is too big to send - split it
                        nSendPos = 0;
                        while(nLineSize > 0) {
                            
                            if((nBytesSend = Send((szMessageLine + nSendPos), min(nLineSize, nMaxSend))) <= 0)
                                retval = UTE_SOCK_SEND_ERROR;
                            
                            nSendPos += nBytesSend;
                            nLineSize -= nBytesSend;
                            nTotalSent += nBytesSend;
                            
                            if (OnSendArticleProgress(nTotalSent, lengthOfMail) == FALSE)
                                retval = UTE_ABORTED;
                        }
                    }
                }
                
                // Send attachments data
                // NOTE! The Article body is alredy added to the attachments data source
                else {
                    while( retval == UTE_SUCCESS && (nLineSize = AttachSource.Read(szMessageLine, min(nMaxSend, MAX_LINE_LENGTH-1))) > 0) {
                        
                        if(IsAborted())
                            retval = UTE_ABORTED;
                        
                        
                        if((nBytesSend = Send(szMessageLine, nLineSize)) != nLineSize)
                            retval = UTE_SOCK_SEND_ERROR;
                        
                        nTotalSent += nBytesSend;
                        
                        if (OnSendArticleProgress(nTotalSent, lengthOfMail) == FALSE)
                            retval = UTE_ABORTED;
                    }
                }
                
                
                
                Send("\r\n.\r\n");  // terminate the blank message and ensure the server is left in an acceptable state.
                
                ReceiveLine(data, LINE_BUFFER_SIZE);
                retval = GetResponseCode(data);	
                
                if (retval == 240)
                    retval = UTE_SUCCESS;
				else
					retval = UTE_NNTP_POST_FAILED;
                
                // Close attachment datasource before deleting file
                AttachSource.Close();
                MsgBodySource.Close();
                
                // Delete temp. file
                DeleteFile(lpszTempFileName);
                free(lpszTempFileName);
                
                return OnError(retval);
                break;
            }
        case 381:  // Microsoft's news server returns this code for some reason
            //the post command normally returns response 381 indicating that
            //the server is awaiting the message data.  If we receive this 
            //response, we know that posting is permitted.
            Send("\r\n.\r\n");          // terminate the blank message and ensure the server is left in an acceptable state.
            return OnError(UTE_SUCCESS);
            break;
            
        case 440:
            // a response of 440 specifies that the command was recognized
            // and that posting is not permitted
            retval = UTE_NNTP_POST_FAILED;
            return retval;
            break;
            
        case 441:   // posting rejected Bad Article
            retval = UTE_NNTP_BAD_ARTICLE;
            break;
        }
        
        return OnError(retval);
}

/*************************************************
NGetServerDate(LPSTR date, int nBytes)
get the NNTP server's date
PARAM
date    - buffer to hold the date string
nBytes  - length of maximum length allowed 
RETURN
UTE_SUCCESS             - success
UTE_SVR_NO_RESPONSE     - no response form the server
UTE_INVALID_RESPONSE    - invalid or negative response
UTE_BUFFER_TOO_SHORT    - buffer too short
**************************************************/
#if defined _UNICODE
// v4.2 changed 'nBytes' to 'nChars' - better for unicode context
int CUT_NNTPClient::NGetServerDate(LPWSTR date, int nChars) {
	
	int retval;
	
	if(nChars > 14) {
		char * dateA = new char [nChars];
		
		if(dateA != NULL) {
			retval = NGetServerDate( dateA, nChars);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(date, nChars, dateA);
			}
			
			delete [] dateA;
			
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
		
	}
	else {
		retval = UTE_BUFFER_TOO_SHORT;
	}
	
	return retval;
}

#endif
int CUT_NNTPClient::NGetServerDate(LPSTR date, int nChars) {
    char data[LINE_BUFFER_SIZE + 1];
    
    if (nChars < 15)
        return OnError(UTE_BUFFER_TOO_SHORT);       // need more space!
    
    Send("DATE\r\n");
    
    if(ReceiveLine(data, LINE_BUFFER_SIZE) <=0)
        return OnError(UTE_SVR_NO_RESPONSE);        // no response
    
    if ( GetResponseCode(data)	 != 111)         // success code
        return OnError(UTE_INVALID_RESPONSE);       // invalid or negative response
	
	return	CUT_StrMethods::ParseString (data, " ", 1,date,nChars);    
}

/**************************************************
NAuthenticateUserSimple
User authentication    
the NNTP standard supports multiple methods of authenticating
user id.  At the most basic level, you simple call authinfo twice,
once for the userid and after receiving a 300 series continue message
a second time with the password.

  more advanced implementations support the 'authinfo simple user pass' 
  construction that allows you to perform clear-text authentication in
  one step.  The most flexible and complicated method of user authentication
  is referred to as 'authinfo generic' to which the NNTP server responds with
  a list of acceptable authentication schemes.
  PARAM
  User    - user name or ID on the server
  Pass    - user's password
  RETURN
  UTE_SUCCESS                     - success
  UTE_NNTP_AUTHINFO_USER_FAILED   - server not responding to AUTHINFO USER command
  UTE_NNTP_AUTHINFO_PASS_FAILED   - server not responding to AUTHINFO PASS command
  UTE_PARAMETER_INVALID_VALUE     - invalid parameter
  UTE_SVR_NO_RESPONSE             - no response form the server
*************************************************/
#if defined _UNICODE
int CUT_NNTPClient::NAuthenticateUserSimple(LPWSTR User, LPWSTR Pass){
	return NAuthenticateUserSimple(AC(User), AC(Pass));}
#endif
int CUT_NNTPClient::NAuthenticateUserSimple(LPSTR User, LPSTR Pass){
    
    char    data[LINE_BUFFER_SIZE + 1];
    
    //clear the recive  buffer
    ClearReceiveBuffer();
    
    // User authentication  
    // the NNTP standard supports multiple methods of authenticating
    // user id.  At the most basic level, you simple call authinfo twice,
    // once for the userid and after receiving a 300 series continue message
    // a second time with the password.
    
    // more advanced implementations support the 'authinfo simple user pass' 
    // construction that allows you to perform clear-text authentication in
    // one step.  The most flexible and complicated method of user authentication
    // is referred to as 'authinfo generic' to which the NNTP server responds with
    // a list of acceptable authentication schemes.
    
    if (User != NULL) {
        //send the USER
        _snprintf(data,sizeof(data)-1,"authinfo user %s\r\n",User);
        Send(data);
        
        //check for a return of +OK
        do {
            if(ReceiveLine(data, LINE_BUFFER_SIZE) <=0)
                return OnError(UTE_SVR_NO_RESPONSE);            //no response
            if(data[0] == '5')
                return OnError(UTE_NNTP_AUTHINFO_USER_FAILED);  // connection not accepted or syntactical error
            
        } while(data[0] != '3');                            //  381 code provides an indication that more data is expected
        
        //clear the recive  buffer
        ClearReceiveBuffer();
        
        if (Pass != NULL) {
            //send the  PASSWORD
            _snprintf(data,sizeof(data)-1,"authinfo pass %s\r\n",Pass);
            Send(data);
            
            //check for a return of +OK
            do {
                if(ReceiveLine(data, LINE_BUFFER_SIZE) <=0)
                    return OnError(UTE_SVR_NO_RESPONSE);        //no response
                if(data[0] == '5')                              // 502 permission denied response
                    return OnError(UTE_NNTP_AUTHINFO_PASS_FAILED);// connection not accepted
                
            } while(data[0] != '2');                        // 2?? codes indicate success
        }
    } 
    else 
        return OnError(UTE_PARAMETER_INVALID_VALUE);            // username NULL
    
    
    return OnError(UTE_SUCCESS);
}

/**************************************************
OnNGetNewsGroupList()
A virtual function that is to be overridden to interrupt or report 
progress of group list update 
PARAM
NONE 
RETURN
TRUE    - Continue
FALSE   - CANCEL process
**************************************************/
int CUT_NNTPClient::OnNGetNewsGroupList(LPCSTR /* newsGroupTitle */ ) {
    return TRUE;
}

/**************************************************
OnNGetArticleList()
A virtual function that is to be overridden to interrupt or report 
progress of article list update 

  Note that the strings passed in are char*, not _TCHAR - and may not
  be decoded
  
	PARAM
	NONE 
	RETURN
	TRUE    - Continue
	FALSE   - CANCEL process
**************************************************/
//int CUT_NNTPClient::OnNGetArticleList(LPCTSTR /* artId */  , LPCTSTR  /* subject */ ,LPCTSTR  /* author */ ,LPCTSTR  /* date */ , LPCTSTR  /* refrence */ ,LPCTSTR  /* BytesCount */, LPCTSTR  /*  NumberOfLines */  ) {
int CUT_NNTPClient::OnNGetArticleList(LPCSTR /*artId*/ , LPCSTR  /* subject */ ,LPCSTR  /* author */ ,LPCSTR  /* date */ , LPCSTR  /* refrence */ ,LPCSTR  /*BytesCount*/, LPCSTR  /*  NumberOfLines */, LPCSTR /* xrefs */  ) {
    //TRACE(_T("\n%s, %s"), WC(artId), WC(BytesCount));
	return TRUE;
}

/****************************************************
ReceiveToDSUntil

  Receives data from the current connection directly
  to a file. This function will return when the untilSting
  is read from the stream.  This function leaves the connection
  open when it completes.
  
	PARAM
	name          - name of file to copy received data into
	fileType      - 1:create/overwrite   2:create/append
	bytesReceived - number of bytes received
	timeOut       - timeout in seconds
	untilString   - a series of characters identifying the end of 
	a block of data.  Usualy CRLF.
	RETURN
	UTE_SUCCESS         - success
	UTE_FILE_TYPE_ERROR - invalid data source type
	UTE_DS_OPEN_FAILED  - unable to open or create data source
	UTE_DS_CLOSE_FAILED - error closing data source
	UTE_SOCK_TIMEOUT    - timeout
	UTE_DS_WRITE_FAILED - data source write error
	UTE_ABORTED         - aborted
*****************************************************/
int CUT_NNTPClient::ReceiveToDSUntil(   CUT_DataSource &dest, 
                                     int fileType,
                                     long *bytesReceived,
                                     int timeOut, 
                                     LPCSTR untilString)
                                     
{
    char    data[LINE_BUFFER_SIZE + 1];
    int     count, error = UTE_SUCCESS;
    int     i, matches;
    int     untilStringLen = (int)strlen(untilString);
    
    *bytesReceived = 0;
    
    // Open the data source
    if(fileType == 1)
        error = dest.Open(UTM_OM_WRITING);
    else if(fileType ==2)
        error = dest.Open(UTM_OM_APPEND);
    else
        error = UTE_FILE_TYPE_ERROR;
	
    if(error < 0)
        error = UTE_DS_OPEN_FAILED;
    
    // Check to see if the data source opened properly
    if(error != UTE_SUCCESS)    
        return error;
    
    // Start reading in the data
    for(;;) {
        if(timeOut > 0) {
            if(WaitForReceive(timeOut, 0) != UTE_SUCCESS) {
                error = UTE_SOCK_TIMEOUT;
                break;
            }
        }
        
        count = ReceiveLineUntil(data, LINE_BUFFER_SIZE, timeOut);  // read in until CRLF or LINE_BUFFER_SIZE chars
        
        // Check to see if the connection is closed
        if(count == 0)
            break;
        
        // Write the data into the data source
        if(dest.Write(data, count) != count) {
            error = UTE_DS_WRITE_FAILED;
            break;
        }
        
        // compare the strings if there is anything to compare
        if(untilStringLen > 0) {
            for (i = 0, matches = 0; i < untilStringLen; i++) {
                if (untilString[i] == data[i])
                    matches++;
                else
                    break;      // something didn't match exit the loop
            }
            
            // check to see if all untilString characters were matched.
            if (matches == untilStringLen)
                break;
        }
        
        //count the bytes copied
        *bytesReceived += count;
        
        //send notify
        if(ReceiveFileStatus(*bytesReceived) == FALSE || IsAborted() ) {
            error = UTE_ABORTED;
            break;
        }
    }
    
    // Close the data source
    if(dest.Close() == -1)
        error = UTE_DS_CLOSE_FAILED;
    
    return error;
}

/***************************************************
ReceiveLine
Receives a line of data. This function will receive data
until the maxDataLen is reached or a '\r\n' is encountered.
This function will also return when the timeOut has expired
as well.
PARAM
data        - data buffer for receiving data
maxDataLen  - length of the data buffer
timeOut     - timeout in seconds
RETURN
the length of the data received
****************************************************/
int CUT_NNTPClient::ReceiveLineUntil(LPSTR data,int maxDataLen,int timeOut)
{
	
    return ReceiveLine(data, maxDataLen, timeOut);
}


/*********************************
NGetArticle
Prompt the server to send the article Identified by the message ID 
PARAM
articlemsgId    - pointer to a buffer containing the article Message ID
dest            - destination data source
RETURN
UTE_SUCCESS                 - success
UTE_FILE_TYPE_ERROR         - invalid data source type
UTE_DS_OPEN_FAILED          - unable to open or create data source
UTE_DS_CLOSE_FAILED         - error closing data source
UTE_SOCK_TIMEOUT            - timeout
UTE_DS_WRITE_FAILED         - data source write error
UTE_CONNECT_TERMINATED      - connection terminated unexpectedly
UTE_INVALID_RESPONSE        - invalid response
UTE_ABORTED                 - aborted
UTE_PARAMETER_INVALID_VALUE - invalid parameter
**************************************************/
#if defined _UNICODE
int CUT_NNTPClient::NGetArticle(LPCWSTR articlemsgId, CUT_DataSource &dest){
	return NGetArticle(AC(articlemsgId), dest);}
#endif
int CUT_NNTPClient::NGetArticle(LPCSTR articlemsgId, CUT_DataSource &dest)
{
    int     error = UTE_SUCCESS;
    char    data[LINE_BUFFER_SIZE + 1];
    
    if(articlemsgId == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);
    
    // Send ARTICLE command 
    _snprintf(data,sizeof(data)-1,"ARTICLE %s\r\n", articlemsgId);
    Send(data);
    
    if(ReceiveLine(data, LINE_BUFFER_SIZE) <= 0) 
        return OnError(UTE_CONNECT_TERMINATED);             //  connection terminted unexpectedly   
    
   	
    if ( GetResponseCode(data) != 220) 
        return OnError(UTE_INVALID_RESPONSE);               // unexpected response
    
    // Receive article to the data source
    long bytesReceived;
    error = ReceiveToDSUntil(dest, 1, &bytesReceived, m_nReceiveTimeout, ".\r\n");
	
    return OnError(error);
}

/***************************************
EmptyNewArticlesList()
Loop through the current list of new articles  
if it exist and delete all old ones 
then create a new list
populate it with the newly found articles
PARAM
NONE
RETURN
NONE
****************************************/
void CUT_NNTPClient::EmptyNewArticlesList(){
    m_listNewArticles.ClearList();
}

/**********************************************
NGetNNTPDateStamp
- return a date string in conformance
to RFC 822.  
- year as 4 digits (2 wld be acceptable)
- e.g. Dow, DD Mon YYYY HH:MM:SS -400
RETURN
UTE_SUCCESS             - success
UTE_BUFFER_TOO_SHORT    - the buffer is too small
***********************************************/
int CUT_NNTPClient::NGetNNTPDateStamp(LPSTR buf,int bufLen){
    
    time_t      t;
    struct tm   *systime;
    char        *days[]     = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    char        *months[]   = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
        "Sep","Oct","Nov","Dec"};
    if(bufLen < 32)
        return OnError(UTE_BUFFER_TOO_SHORT);
    
    tzset();
    
    //get the time/date
    t = time(NULL);
    systime = localtime(&t);
    
    // will assert 2037 January 18th 
    assert(systime);
    
    char    cSign = '-';
    int     nGmtHrDiff  = _timezone / 3600;
    int     nGmtMinDiff = abs((_timezone % 3600) / 60);
    if ( systime->tm_isdst > 0 )
        nGmtHrDiff -= 1;
    
    if ( nGmtHrDiff < 0 ) {
        // because VC++ doesn't do "%+2d"
        cSign = '+';
        nGmtHrDiff *= -1;
    }
    
    _snprintf(buf,bufLen,"%s, %2.2d %s %d %2.2d:%2.2d:%2.2d %c%2.2d%2.2d",
        days[systime->tm_wday],
        systime->tm_mday, 
        months[systime->tm_mon], 
        systime->tm_year+1900,
        systime->tm_hour, 
        systime->tm_min, 
        systime->tm_sec,
        cSign, 
        nGmtHrDiff, 
        nGmtMinDiff );
    
    return OnError(UTE_SUCCESS);
}

/***************************************
SetDataPort
Sets the default connection port
Params
port number (default is 119)
Return
UTE_SUCCESS - success
UTE_ERROR   - error
****************************************/
int CUT_NNTPClient::SetConnectPort(int port) {
    if(port <= 0)
        return OnError(UTE_ERROR);
    
    m_nConnectPort = port;
    
    return OnError(UTE_SUCCESS);
}
/***************************************
GetDataPort
Gets the connection port
Params
none
Return
port number (default is 119)
****************************************/
int CUT_NNTPClient::GetConnectPort() const
{
    return m_nConnectPort;
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
int CUT_NNTPClient::SetConnectTimeout(int secs){
    
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
int CUT_NNTPClient::GetConnectTimeout() const
{
    return m_nConnectTimeout;
}
/*********************************************
SetReceiveTimeout
Sets the time to wait for incomming data
packets in seconds
5 seconds is the default time
Params
secs - seconds to wait
Return
UTE_SUCCESS - success
UTE_ERROR   - invalid input value
**********************************************/
int CUT_NNTPClient::SetReceiveTimeout(int secs){
    
    if(secs <= 0)
        return OnError(UTE_ERROR);
    
    m_nReceiveTimeout = secs;
    
    return OnError(UTE_SUCCESS);
}
/*********************************************
GetReceiveTimeout
Gets the time to wait for incomming data
packets in seconds
Params
none
Return
current time out value in seconds
**********************************************/
int CUT_NNTPClient::GetReceiveTimeout() const
{
    return m_nReceiveTimeout;
}

/********************************
OnSendArticleProgress
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
BOOL CUT_NNTPClient::OnSendArticleProgress(long /* bytesSent */ ,long /* totalBytes */ ) {
    return !IsAborted();
}
/*********************************
Get The response code from the server
this function is may be simple however it is 
used all around so we put it in a function by it self
PARAM
data - Line received from server
RET
long - the response digits
*********************************/
long	CUT_NNTPClient::GetResponseCode(LPCSTR data)
{
	long code = 0;
	if (data != NULL)
	{
		// get the status code as number
		CUT_StrMethods::ParseString (data ," ",0,&code);
	}
	return code;
}
/*********************************************
Older versions of this class assumed that
the server will return a hard coded set of 
overview format. Since it is defined by RFC2980 
that any server 

  which implement XOevr must implement this command
  then we finally can rely on the server to identify
  the databas format.
  
	PARAM
	viod
	RET:
	UTE_SUCCESS -  we got the format
	UTE_ERROR	- error
	REMARK:
	this internal function will populate the m_OverviewFormat
	variable which also is used by 
	CUT_NNTPArticleList::SetOverViewFormat 
	
**********************************************/
int CUT_NNTPClient::GetOverviewFormat()
{
    char        data[LINE_BUFFER_SIZE];
    char        buf[LINE_BUFFER_SIZE + 1];
    
    m_OverviewFormat.ClearList ();
    while(WaitForReceive(1,0) == UTE_SUCCESS)
        ClearReceiveBuffer();
	
	// I have previously hard coded the format I am expecting from the remote server
	// now I want to make sure I receive the format first
	_snprintf(buf,sizeof(buf)-1,"LIST OVERVIEW.FMT\r\n");
    Send(buf);
	if (ReceiveLine(data, LINE_BUFFER_SIZE-1) > 0)
	{
		//2.1.7.1 Responses
		//      215 information follows
		//		503 program error, function not performed
		long code = GetResponseCode(data);
		if ( code == 215)
		{
			int count = 0;
			do
			{
				count= ReceiveLine(data, LINE_BUFFER_SIZE-1) ;
				if( count <= 0)
					// v4.2 not all servers seem to bother with the '.' line...
					return UTE_SUCCESS; UTE_ERROR;// error 
				if (data[0] != '.')
				{
					CUT_StrMethods::ParseString (data,":\r\n ",0,buf,LINE_BUFFER_SIZE-1);
					m_OverviewFormat.AddString (buf);
				}
				else
					break;
			}
			while (count >0 );
			return UTE_SUCCESS;	
		}	
		return UTE_ERROR; // 503 program error, function not performed
	}
	else
		return UTE_ERROR; // no server response
	
}

#pragma warning ( pop )
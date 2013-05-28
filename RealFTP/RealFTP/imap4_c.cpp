// =================================================================
//  class: CUT_IMAP4Client
//  File:  IMAP4_c.cpp
//
//  Purpose:
//
//  IMAP4rev1 Client Class 
//
//  RFC  1730, 2060
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
    #define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
                            /* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include <process.h> // required for _beginthread API

#include "IMAP4_c.h"

#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)


/********************************
CUT_IMAP4Client
    Constructor
Params
    none
Return
    none
*********************************/
CUT_IMAP4Client::CUT_IMAP4Client() : 
    m_ClientState(STATE_NON_AUTHENTICATED), // Start in non authenticated state
    m_bConnection(FALSE),                   // Clear connection flag
    m_nPort(143),                           // Set default connection port to 143
    m_nConnectTimeout(15),                  // Connection time out. Default 15 sec.
    m_nIMAP4TimeOut(15),                    // IMAP4 time out. Default 15 sec.
    m_bMailBoxWriteAccess(TRUE),            // Initialize write access
    m_lNewMailCheckInterval(15),            // Initialize new mail checking interval
    m_bGoingToDestroy(FALSE),               // Clear destroy flag
    m_hNewMailThread((HANDLE)-1),           // Initialize new mail thread
    m_bFireOnExists(FALSE),                 // Clear OnExists event flag
    m_bFireOnRecent(FALSE),                 // Clear OnRecent event flag
    m_bFireOnExpunge(FALSE),                // Clear OnExpunge event flag
    m_bFireOnFetch(FALSE),                  // Clear OnFetch event flag
    m_lOnExistsParam(0),           
    m_lOnRecentParam(0),
    m_lOnExpungeParam(0)
    
{
    // Initialize capability information
    m_szCapabiltyInfo[0] = NULL;

    // Initialize critical section
    InitializeCriticalSection(&m_CriticalSection);
}
/********************************
~CUT_IMAP4Client
    Destructor
Params
    none
Return
    none
*********************************/
CUT_IMAP4Client::~CUT_IMAP4Client()
{
    // Set destroy flag
    m_bGoingToDestroy = TRUE;

    // Stop new mail thread
    if(m_hNewMailThread != (HANDLE) -1)
        WaitForSingleObject(m_hNewMailThread, 30000);

    // Close connection
    IMAP4Close();

    // Clear FETCH result vector
    m_vectorMsgData.clear();

    // Clear messages data Data Sources
    vector<CUT_DataSource*>::iterator   Index;
    for(Index = m_vectorPtrDataSource.begin(); Index != m_vectorPtrDataSource.end(); Index++)
        delete *Index;
    m_vectorPtrDataSource.clear();

    // Delete critical section
    DeleteCriticalSection(&m_CriticalSection);
}

/*********************************************
SetPort
    Sets the port number to connect to
Params
    newPort     - connect port
Return
    UTE_SUCCESS - success
**********************************************/
int CUT_IMAP4Client::SetPort(unsigned int newPort) {
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
unsigned int  CUT_IMAP4Client::GetPort() const
{
    return m_nPort;
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
int CUT_IMAP4Client::SetConnectTimeout(int secs){
    
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
int CUT_IMAP4Client::GetConnectTimeout() const
{
    return m_nConnectTimeout;
}
/********************************
SetIMAP4TimeOut
    Sets IMAP4 time out value
PARAM:
    timeout 
RETURN:
    UTE_SUCCESS - success
    UTE_ERROR   - invalid input value
*********************************/
int CUT_IMAP4Client::SetIMAP4TimeOut(int timeout) {
    if(timeout <= 0)
        return OnError(UTE_ERROR);

    m_nIMAP4TimeOut = timeout;

    return OnError(UTE_SUCCESS);
}
/********************************
GetIMAP4TimeOut
    Gets IMAP4 time out value
PARAM:
    none
RETURN:
    timeout 
*********************************/
int CUT_IMAP4Client::GetIMAP4TimeOut() const
{
    return m_nIMAP4TimeOut;
}

/********************************
SetNewMailCheckInterval
    Sets new mail check interval in sec.
PARAM:
    nInterval   - new interval value
RETURN:
    UTE_SUCCESS - success
    UTE_ERROR   - invalid input value
*********************************/
int CUT_IMAP4Client::SetNewMailCheckInterval(long nInterval) {
    if(nInterval <= 0)
        return OnError(UTE_ERROR);

    m_lNewMailCheckInterval = nInterval;

    return OnError(UTE_SUCCESS);
}

/********************************
GetNewMailCheckInterval
    Gets new mail check interval
PARAM:
    none
RETURN:
    interval value in sec.
*********************************/
long CUT_IMAP4Client::GetNewMailCheckInterval() const
{
    return m_lNewMailCheckInterval;
}

/********************************
GetStringData
    Gets string data in quoted o literal format
    "12345..." or {15} CRLF 12345...
PARAM:
    lpszData    - data pointer
RETURN:
    pointer to the data source
*********************************/
CUT_DataSource *CUT_IMAP4Client::GetStringData(LPCSTR lpszData)
{
    char            buffer[MAX_LINE_BUFFER + 1];
    CUT_DataSource *ptrResult = NULL;
    BOOL            bError = FALSE;

    // Get string from the buffer ("12345....")
    if(*lpszData == '\"') {
        if(CUT_StrMethods::ParseString(lpszData + 1, "\"\r\n", 0, buffer, MAX_LINE_BUFFER) == UTE_SUCCESS) {
            ptrResult = new CUT_BufferDataSource(NULL, strlen(buffer) + 1);
            if(ptrResult->Open(UTM_OM_WRITING) == UTE_SUCCESS) {
                if(ptrResult->Write(buffer, strlen(buffer)) == -1)
                    bError = TRUE;
                ptrResult->Close();
                }
            else 
                bError = TRUE;
            }
        }

    // Get string from the socket ( {15}CRLF 12345....)
    else if(*lpszData == '{') {
        long    lDataSize = 0;
        if(CUT_StrMethods::ParseString(lpszData + 1, "}\r\n", 0, &lDataSize) == UTE_SUCCESS) {
            if(lDataSize > 30000)
                ptrResult = new CUT_MapFileDataSource(0, lDataSize + 1);
            else
                ptrResult = new CUT_BufferDataSource(NULL, lDataSize + 1);

            if(Receive(*ptrResult, UTM_OM_WRITING, m_nIMAP4TimeOut, lDataSize) != UTE_SUCCESS)
                bError = TRUE;
            }
        }

    // Test for NIL string
    else if(_strnicmp(lpszData, "NIL", 3) == 0) {
        ptrResult = new CUT_BufferDataSource(NULL, 1);
        }


    // Error have been detected
    if(bError && ptrResult != NULL) {
        delete ptrResult;
        ptrResult = NULL;
        }

    // Add pointer to the vector so we can clean it up later
    if(ptrResult != NULL)
        m_vectorPtrDataSource.push_back(ptrResult);

    return ptrResult;
}

/********************************
GetFlags
    Parse flags string (\Seen \Deleted).
    All unknown flags may be returned as a string.
PARAM:
    lpszFlags           - flags string to parse
    nFlagResult         - system flags result value
    [lpszUnknownFlags]  - buffer for the unknown flags
    [nBufferSize]       - buffer size
RETURN:
    none
*********************************/
void CUT_IMAP4Client::GetFlags(LPCSTR lpszFlags, long &nFlagResult, LPSTR lpszUnknownFlags, int nBufferSize)
{
    char    buffer[MAX_LINE_BUFFER + 1];
    int     nIndex = 0;

    nFlagResult = 0;

    // Initialize unknown flags buffer
    if(lpszUnknownFlags != NULL && nBufferSize > 0) 
        lpszUnknownFlags[0] = 0;

    // Parse flags string
    while(CUT_StrMethods::ParseString(lpszFlags, "\\ \r\n", nIndex++, buffer, MAX_LINE_BUFFER) == UTE_SUCCESS)
        if(_stricmp(buffer, "ANSWERED") == 0)
            nFlagResult |= SYS_FLAG_ANSWERED;
        else if(_stricmp(buffer, "FLAGGED") == 0)
            nFlagResult |= SYS_FLAG_FLAGGED;
        else if(_stricmp(buffer, "DELETED") == 0)
            nFlagResult |= SYS_FLAG_DELETED;
        else if(_stricmp(buffer, "SEEN") == 0)
            nFlagResult |= SYS_FLAG_SEEN;
        else if(_stricmp(buffer, "DRAFT") == 0)
            nFlagResult |= SYS_FLAG_DRAFT;
        else if(_stricmp(buffer, "RECENT") == 0)
            nFlagResult |= SYS_FLAG_RECENT;
        else if(_stricmp(buffer, "*") == 0)
            nFlagResult |= SYS_FLAG_SPECIAL;

        // Unknown flag
        else if(lpszUnknownFlags != NULL && nBufferSize > 0) 
            if((strlen(lpszUnknownFlags) + strlen(buffer) + 2) < (unsigned) nBufferSize) {
                strcat(lpszUnknownFlags, "\\");
                strcat(lpszUnknownFlags, buffer);
                strcat(lpszUnknownFlags, " ");
                }
}

/********************************
FlagsToString
    Converts status code to string
PARAM:
    nStatus     - status to convert
    lpszBuffer  - result buffer
    nBufferSize - buffer size
RETURN:
    none
*********************************/
#if defined _UNICODE
// v4.2 overload - this should still be updated to return error check - refactor next version 
void CUT_IMAP4Client::FlagsToString(int nStatus, LPWSTR lpszBuffer, int nBufferSize) {
	char * buffA = (char*) alloca(nBufferSize+1);
	*buffA = '\0';
	*lpszBuffer = _T('\0');
	FlagsToString(nStatus, buffA, nBufferSize);
	CUT_Str::cvtcpy(lpszBuffer, nBufferSize, buffA);
}
#endif
void CUT_IMAP4Client::FlagsToString(int nStatus, LPSTR lpszBuffer, int nBufferSize)
{

    // Buffer must be at least 70 characters
    if(nBufferSize < 70)
        return;

    // Clear buffer
    lpszBuffer[0] = NULL;

    // Generate status string
    BOOL    bFirst = TRUE;
    if(nStatus & SYS_FLAG_ANSWERED) {
        if(!bFirst)     strcat(lpszBuffer, " ");
        bFirst = FALSE;
        strcat(lpszBuffer, "\\Answered");
        }   
    if(nStatus & SYS_FLAG_FLAGGED) {
        if(!bFirst)     strcat(lpszBuffer, " ");
        bFirst = FALSE;
        strcat(lpszBuffer, "\\Flagged");
        }
    if(nStatus & SYS_FLAG_DELETED) {
        if(!bFirst)     strcat(lpszBuffer, " ");
        bFirst = FALSE;
        strcat(lpszBuffer, "\\Deleted");
        }
    if(nStatus & SYS_FLAG_SEEN) {
        if(!bFirst)     strcat(lpszBuffer, " ");
        bFirst = FALSE;
        strcat(lpszBuffer, "\\Seen");
        }
    if(nStatus & SYS_FLAG_DRAFT) {
        if(!bFirst)     strcat(lpszBuffer, " ");
        bFirst = FALSE;
        strcat(lpszBuffer, "\\Draft");
        }
    if(nStatus & SYS_FLAG_RECENT) {
        if(!bFirst)     strcat(lpszBuffer, " ");
        bFirst = FALSE;
        strcat(lpszBuffer, "\\Recent");
        }
    if(nStatus & SYS_FLAG_SPECIAL) {
        if(!bFirst)     strcat(lpszBuffer, " ");
        bFirst = FALSE;
        strcat(lpszBuffer, "\\*");
        }
}

/********************************
GetResponse
    If command tag is specified waits command 
    responce. All other responses are sent to 
    the ProcessResponse function.
    If command tag is not specified returns 
    first response line from server.
PARAM:
    lpszCmdTag          - command tag (can be NULL or "")
    lpszResponseText    - response text buffer
    nBufferSize         - response text buffer length
RETURN:
    UTE_SUCCESS         - Command completed successful
    UTE_ERROR           - Command failed
    UTE_UNKNOWN_COMMAND - Command unknown or arguments invalid
    UTE_UNKNOWN_RESPONSE- Unknown response
    UTE_SOCK_TIMEOUT    - time out error
    UTE_ABORTED         - aborted
*********************************/
int CUT_IMAP4Client::GetResponse(LPCSTR lpszCmdTag, LPSTR lpszResponseText, int nBufferSize)
{
    char    buffer[MAX_LINE_BUFFER + 1];

    for(;;) {

        // Receive line from the server
        if(ReceiveLine(buffer, MAX_LINE_BUFFER, m_nIMAP4TimeOut) <= 0) {
            
            // Exit the critical section
            LeaveCriticalSection(&m_CriticalSection);

            return UTE_SOCK_TIMEOUT;
            }

        // Process response
        ProcessResponse(buffer);

        // If commang tag was not specified - return first line
        if(lpszCmdTag == NULL || strlen(lpszCmdTag) == 0) {
            strncpy(lpszResponseText, buffer, nBufferSize - 1);
            lpszResponseText[nBufferSize - 1] = 0;

            strncpy(m_szLastResponse, buffer, MAX_LINE_BUFFER - 1);
            m_szLastResponse[MAX_LINE_BUFFER - 1] = 0;

            // Exit the critical section
            LeaveCriticalSection(&m_CriticalSection);

            return UTE_SUCCESS;
            }

        // If tag was found
        if(strstr(buffer, lpszCmdTag) == buffer) {
            int     result = UTE_UNKNOWN_RESPONSE;

            LPSTR   ptrResult = buffer + strlen(lpszCmdTag);
            LPSTR   ptrText = (strlen(ptrResult) > 3) ? ptrResult + 3 : ptrResult;

            if(*ptrText == ' ')
                ++ptrText;

            if(lpszResponseText != NULL && nBufferSize > 0) {
                strncpy(lpszResponseText, ptrText, nBufferSize - 1);
                lpszResponseText[nBufferSize - 1] = 0;
                }

            strncpy(m_szLastResponse, ptrText, MAX_LINE_BUFFER - 1);
            m_szLastResponse[MAX_LINE_BUFFER - 1] = 0;
            
                
            // Get result code
            _strupr(ptrResult);
            if(strstr(ptrResult, "OK ") == ptrResult) 
                result = UTE_SUCCESS;
            else if(strstr(ptrResult, "NO ") == ptrResult) 
                result = UTE_ERROR;
            else if(strstr(ptrResult, "BAD ") == ptrResult) 
                result = UTE_UNKNOWN_COMMAND;

            // Exit the critical section
            LeaveCriticalSection(&m_CriticalSection);

            // Fire OnExists event
            if(m_bFireOnExists) {
                m_bFireOnExists = FALSE;
                OnExists(m_lOnExistsParam);
                m_lMailBoxMsgNumber = m_lOnExistsParam;
                }

            // Fire OnRecent event
            if(m_bFireOnRecent) {
                m_bFireOnRecent = FALSE;
                OnRecent(m_lOnRecentParam);
                m_lMailBoxMsgRecent = m_lOnRecentParam;
                }

            // Fire OnExpunge event
            if(m_bFireOnExpunge) {
                m_bFireOnExpunge = FALSE;
                OnExpunge(m_lOnExpungeParam);
                }

            // Fire OnFetch event
            if(m_bFireOnFetch)
                OnFetch(); 

            return result;
            }

        // In case of object destruction don't wait for response
        if(m_bGoingToDestroy) {
            
            // Exit the critical section
            LeaveCriticalSection(&m_CriticalSection);

            return UTE_ABORTED;
            }
        }
}

/********************************
ProcessResponse
    Process untaged server responses
PARAM:
    lpszResponse    - response data
RETURN:
    none
*********************************/
void CUT_IMAP4Client::ProcessResponse(LPSTR lpszResponse) 
{
    char    buffer[MAX_LINE_BUFFER + 1];
    char    szToken1[MAX_LINE_BUFFER + 1] ,szToken2[MAX_LINE_BUFFER + 1];

    // Check input parameter
    if(lpszResponse == NULL)
        return;
    
    // Get first two tokens
    CUT_StrMethods::ParseString(lpszResponse, " \r\n", 1, szToken1, MAX_LINE_BUFFER);
    CUT_StrMethods::ParseString(lpszResponse, " \r\n", 2, szToken2, MAX_LINE_BUFFER);


    // BYE response (server disconnected)
    if(_stricmp(szToken1, "BYE") == 0) 
        if(m_bConnection) {
            m_bConnection = FALSE;
            m_ClientState = STATE_LOGOUT;
            IMAP4Close();
            }


    // CAPABILITY response
    if(_stricmp(szToken1, "CAPABILITY") == 0) 
        strcpy(m_szCapabiltyInfo, lpszResponse + 13);
    

    // EXISTS response (number of messages in the mailbox)
    else if(_stricmp(szToken2, "EXISTS") == 0) {
        m_bFireOnExists = TRUE;
        m_lOnExistsParam = atol(szToken1);
        }
    
    
    // RECENT response (number of messages that have arrived since 
    // the previous time a SELECT command was done on this mailbox)
    else if(_stricmp(szToken2, "RECENT") == 0) {
        m_bFireOnRecent = TRUE;
        m_lOnRecentParam = atol(szToken1);
        }

    // EXPUNGE response. The EXPUNGE response reports that the specified message sequence
    // number has been permanently removed from the mailbox.  
    else if(_stricmp(szToken2, "EXPUNGE") == 0) {
        m_bFireOnExpunge = TRUE;
        m_lOnExpungeParam = atol(szToken1);
        }


    // SEARCH response. The SEARCH command searches the mailbox for messages that match
    // the given searching criteria.  
    else if(_stricmp(szToken1, "SEARCH") == 0) {
        long    lMessageID;
        int     nIndex = 0;
        while(CUT_StrMethods::ParseString(lpszResponse + 9, " \r\n", nIndex++, &lMessageID) == UTE_SUCCESS) 
            m_vectorSearchResult.push_back(lMessageID);
        }


    // FETCH response. Returns data about a message to the client.
    // The data are pairs of data item names and their values in parentheses.
    else if(_stricmp(szToken2, "FETCH") == 0) {
        CUT_MsgDataA    MsgData;
        LPSTR           ptrData = strstr(lpszResponse, "FETCH (");

        MsgData.m_lMsgNumber = atol(szToken1);

        // Pointer to the data begining
        if(ptrData != NULL) {
            ptrData += 7;

            // Get the attribute name
            while(CUT_StrMethods::ParseString(ptrData, " \r\n", 0, szToken1, MAX_LINE_BUFFER) == UTE_SUCCESS) {
                // End of data found
                if(_stricmp(szToken1, ")") == 0)
                    break;

                // Process FLAGS attribute data
                else if(_stricmp(szToken1, "FLAGS") == 0) {
                    // Get FLAGS data
                    ptrData += strlen(szToken1) + 2;
                    if(*ptrData == ')')
                        ++ptrData;
                    else if(CUT_StrMethods::ParseString(ptrData, ")\r\n", 0, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) {
                        ptrData += strlen(szToken2) + 1;
                        GetFlags(szToken2, MsgData.m_lFlags, MsgData.m_szUnknownFlags, MAX_LINE_BUFFER);
                        }
                    }

                // Process INTERNALDATE attribute data
                else if(_stricmp(szToken1, "INTERNALDATE") == 0) {
                    // Get INTERNALDATE data
                    ptrData += strlen(szToken1) + 2;
                    if(CUT_StrMethods::ParseString(ptrData, "\"\r\n", 0, MsgData.m_szDateTime, MAX_DATE_TIME_SIZE) != UTE_SUCCESS) 
                        MsgData.m_szDateTime[0] = 0;
                    ptrData += strlen(MsgData.m_szDateTime) + 1;
                    }

                // Process UID attribute data
                else if(_stricmp(szToken1, "UID") == 0) {
                    // Get UID data
                    ptrData += strlen(szToken1) + 1;
                    MsgData.m_lUID = 0;
                    if(CUT_StrMethods::ParseString(ptrData, " )\r\n", 0, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) {
                        MsgData.m_lUID = atol(szToken2);
                        ptrData += strlen(szToken2);
                        }
                    }

                // Process RFC822.SIZE attribute data
                else if(_stricmp(szToken1, "RFC822.SIZE") == 0) {
                    // Get RFC822.SIZE data
                    ptrData += strlen(szToken1) + 1;
                    MsgData.m_lSize = 0;
                    if(CUT_StrMethods::ParseString(ptrData, " )\r\n", 0, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) {
                        MsgData.m_lSize = atol(szToken2);
                        ptrData += strlen(szToken2);
                        }
                    }

                // Process BODY[] attribute data
                else if(_stricmp(szToken1, "BODY[]") == 0) {
                    // Get BODY[] data
                    ptrData += strlen(szToken1) + 1;

                    // Get string data
                    MsgData.m_ptrData = GetStringData(ptrData);
                    MsgData.m_nDataType = MSG_BODY;
                    }

                // Process BODY[HEADER] attribute data
                else if(_stricmp(szToken1, "BODY[HEADER]") == 0) {
                    // Get BODY[HEADER] data
                    ptrData += strlen(szToken1) + 1;

                    // Get string data
                    MsgData.m_ptrData = GetStringData(ptrData);
                    MsgData.m_nDataType = MSG_BODY_HEADER;
                    }
                
                // Process BODY[TEXT] attribute data
                else if(_stricmp(szToken1, "BODY[TEXT]") == 0) {
                    // Get BODY[TEXT] data
                    ptrData += strlen(szToken1) + 1;

                    // Get string data
                    MsgData.m_ptrData = GetStringData(ptrData);
                    MsgData.m_nDataType = MSG_BODY_TEXT;
                    }

                // Unknown attribute (error)
                else 
                    ptrData += strlen(szToken1) + 2;



                // Receive line from the server
                if(strstr(lpszResponse, "}\r\n") != NULL && *ptrData == '{') {
                    if(ReceiveLine(buffer, MAX_LINE_BUFFER, m_nIMAP4TimeOut) <= 0)
                        break;
                    ptrData = buffer;
                    }
                

                // Skip leading spaces
                while(*ptrData == ' ') 
                    ++ptrData;

                }

            m_bFireOnFetch = TRUE;

            // Add new item to the vector
            m_vectorMsgData.push_back(MsgData);
            }
        }


    // STATUS response. The STATUS response occurs as a result of an STATUS command.  
    // It returns the mailbox name that matches the STATUS specification and
    // the requested mailbox status information.
    else if(_stricmp(szToken1, "STATUS") == 0) {

        // Clear result data
        m_LastUT_StatusData.m_strName   = "";
        m_LastUT_StatusData.m_lMsgNumber = m_LastUT_StatusData.m_lMsgRecent = 
        m_LastUT_StatusData.m_lMsgUnseen = m_LastUT_StatusData.m_lNextUID   = 
        m_LastUT_StatusData.m_lUIVV = 0;

        // Get MailBox Name
        if(CUT_StrMethods::ParseString(lpszResponse, " \r\n", 2, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) 
            m_LastUT_StatusData.m_strName = szToken2;

        // Get status data
        if(CUT_StrMethods::ParseString(lpszResponse, "()\r\n", 1, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) {
            int     nIndex = 0;
            while(CUT_StrMethods::ParseString(szToken2, "()\\ \r\n", nIndex++, szToken1, MAX_LINE_BUFFER) == UTE_SUCCESS) 
                if(_stricmp(szToken1, "MESSAGES") == 0)
                    CUT_StrMethods::ParseString(szToken2, "()\\ \r\n", nIndex++, &m_LastUT_StatusData.m_lMsgNumber);
                else if(_stricmp(szToken1, "RECENT") == 0)
                    CUT_StrMethods::ParseString(szToken2, "()\\ \r\n", nIndex++, &m_LastUT_StatusData.m_lMsgRecent);
                else if(_stricmp(szToken1, "UIDNEXT") == 0)
                    CUT_StrMethods::ParseString(szToken2, "()\\ \r\n", nIndex++, &m_LastUT_StatusData.m_lNextUID);
                else if(_stricmp(szToken1, "UIDVALIDITY") == 0)
                    CUT_StrMethods::ParseString(szToken2, "()\\ \r\n", nIndex++, &m_LastUT_StatusData.m_lUIVV);
                else if(_stricmp(szToken1, "UNSEEN") == 0)
                    CUT_StrMethods::ParseString(szToken2, "()\\ \r\n", nIndex++, &m_LastUT_StatusData.m_lMsgUnseen);
            }
        }


    // LIST response. The LIST response occurs as a result of a LIST command.  
    // It returns a single name that matches the LIST specification.  
    // There can be multiple LIST responses for a single LIST command.
    //                        OR
    // LSUB response. The LSUB response occurs as a result of an LSUB command.  
    // It returns a single name that matches the LSUB specification.  
    // There can be multiple LSUB responses for a single LSUB command.  
    // The data is identical in format to the LIST response.
    else if(    _stricmp(szToken1, "LIST") == 0 ||
                _stricmp(szToken1, "LSUB") == 0) {
        // New list item
        CUT_MailBoxListItem newItem;

        // Process the flags
        if(CUT_StrMethods::ParseString(lpszResponse, " \r\n", 2, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) {
            int     nIndex = 0;
            while(CUT_StrMethods::ParseString(szToken2, "()\\ \r\n", nIndex++, szToken1, MAX_LINE_BUFFER) == UTE_SUCCESS)
                if(_stricmp(szToken1, "NOINFERIORS") == 0)
                    newItem.m_nNameAttrib |= NAME_ATTRIB_NOINFERIORS;
                else if(_stricmp(szToken1, "NOSELECT") == 0)
                    newItem.m_nNameAttrib |= NAME_ATTRIB_NOSELECT;
                else if(_stricmp(szToken1, "MARKED") == 0)
                    newItem.m_nNameAttrib |= NAME_ATTRIB_MARKED;
                else if(_stricmp(szToken1, "UNMARKED") == 0)
                    newItem.m_nNameAttrib |= NAME_ATTRIB_UNMARKED;
            }

        // Get the delimiter
        if(CUT_StrMethods::ParseString(lpszResponse, " \r\n", 3, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) 
            if(CUT_StrMethods::ParseString(szToken2, "\"\r\n", 0, szToken1, MAX_LINE_BUFFER) == UTE_SUCCESS) 
                newItem.m_strDelimiter = szToken1;

        // Get name
        if(CUT_StrMethods::ParseString(lpszResponse, "\"\r\n", 2, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) 
            if(strcmp(szToken2, " ") == 0)
                CUT_StrMethods::ParseString(lpszResponse, "\"\r\n", 3, szToken2, MAX_LINE_BUFFER);
            if(CUT_StrMethods::ParseString(szToken2, "\"\r\n", 0, szToken1, MAX_LINE_BUFFER) == UTE_SUCCESS) {
                int nPos = 0;
                while(szToken1[nPos] == ' ')
                    ++nPos;
                newItem.m_strName = szToken1+nPos;
                }

        // Add new list item to the vector
        m_vectorListItems.push_back(newItem);
        }


    // FLAGS response . The flag parenthesized list identifies the flags 
    // (at a minimum, the system-defined flags) that are applicable for this mailbox.
    else if(_stricmp(szToken1, "FLAGS") == 0) 
        if(CUT_StrMethods::ParseString(lpszResponse, "()\r\n", 1, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) 
            GetFlags(szToken2, m_lMailBoxFlags, m_szMailBoxUnknownFlags, MAX_LINE_BUFFER);

    // Get response codes from untagged or tagged OK/NO/BAD response
    else if(    _stricmp(szToken1, "OK") == 0 || 
                _stricmp(szToken1, "NO") == 0 || 
                _stricmp(szToken1, "BAD") == 0 ) {

        // ALERT - the text contains a special alert that MUST be presented 
        // to the user 
        if(_stricmp(szToken2, "[ALERT]") == 0 ) 
            OnAlertMessage(lpszResponse + 12);

        // TRYCREATE - This is a hint to the client that the operation can 
        // succeed if the mailbox is first created by the CREATE command.
        else if(_stricmp(szToken2, "[TRYCREATE]") == 0 ) {
            }

        // NEWNAME - Followed by a mailbox name and a new mailbox name.
        // This is a hint to the client that the operation can succeed if the
        // SELECT or EXAMINE is reissued with the new mailbox name.
        else if(_stricmp(szToken2, "[NEWNAME") == 0 ) {
            }

        // PARSE - the text represents an error in parsing the [RFC-822] header 
        // or [MIME-IMB] headers of a message in the mailbox.
        else if(_stricmp(szToken2, "[PARSE]") == 0 ) {
            }

        // PERMANENTFLAGS - Followed by a parenthesized list of flags,
        // indicates which of the known flags that the client can change permanently.  
        else if(_stricmp(szToken2, "[PERMANENTFLAGS") == 0 ) {
            // Get parameters token
            if(CUT_StrMethods::ParseString(lpszResponse, "[]\r\n", 1, szToken1, MAX_LINE_BUFFER) == UTE_SUCCESS)
                if(CUT_StrMethods::ParseString(szToken1, "()\r\n", 1, szToken2, MAX_LINE_BUFFER) == UTE_SUCCESS) 
                    GetFlags(szToken2, m_lMailBoxPermanentFlags, m_szMailBoxPermanentUnknownFlags, MAX_LINE_BUFFER);
            }

        // READ-ONLY - The mailbox is selected read-only, or its access
        // while selected has changed from read-write to read-only.
        else if(_stricmp(szToken2, "[READ-ONLY]") == 0 ) 
            m_bMailBoxWriteAccess = FALSE;
            
        // READ-WRITE - The mailbox is selected read-write, or its access
        // while selected has changed from read-only to read-write.
        else if(_stricmp(szToken2, "[READ-WRITE]") == 0 ) 
            m_bMailBoxWriteAccess = TRUE;

    
        // UIDVALIDITY - Followed by a decimal number, indicates the unique
        // identifier validity value.
        else if(_stricmp(szToken2, "[UIDVALIDITY") == 0 ) {
            m_lMailBoxUIVV = 0;
            CUT_StrMethods::ParseString(lpszResponse, " \r\n", 3, &m_lMailBoxUIVV);
            }

        // UNSEEN - Followed by a decimal number, indicates the number
        // of the first message without the \Seen flag set.
        else if(_stricmp(szToken2, "[UNSEEN") == 0 ) {
            m_lMailBoxMsgUnSeen = 0;
            CUT_StrMethods::ParseString(lpszResponse, " \r\n", 3, &m_lMailBoxMsgUnSeen);
            }

        }

}

/********************************
GetNewTag
    Gets new tag string
PARAM:
    lpszTag - pointer to the buffer to store tag.
            (must be at least 6 charactesrs size)
RETURN:
    none
*********************************/
void CUT_IMAP4Client::GetNewTag(LPSTR lpszTag, int iBufferSize) {
    static  int     nCount = 1;

    if(nCount > 9999)
        nCount = 0;

	_snprintf(lpszTag,iBufferSize,"A%04d ", nCount++);
}

/********************************
IMAP4Connect()
    Connect to port 143 on a IMAP4 server informing it of the 
    user name and password. 
PARAM:
    mailHost    - server ip address or name.
    user        - user account id
    password    - account password
RETURN:
    UTE_SOCK_ALREADY_OPEN   - socket already open or in use
    UTE_SOCK_CREATE_FAILED  - socket creation failed
    UTE_SOCK_CONNECT_FAILED - socket connection failed
    UTE_INVALID_ADDRESS     - invalid address
    UTE_SUCCESS             - success
    UTE_CONNECT_TIMEOUT     - connect time out
    UTE_LOGIN_FAILED        - user login failed
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::IMAP4Connect(LPCWSTR mailHost, LPCWSTR user, LPCWSTR password) {
	return IMAP4Connect(AC(mailHost), AC(user), AC(password));}
#endif
int CUT_IMAP4Client::IMAP4Connect(LPCSTR mailHost, LPCSTR user, LPCSTR password) {

    int     error;
    char    buffer[MAX_LINE_BUFFER + 1];
    char    szTag[MAX_TAG_SIZE + 1];

    // Close any open connection
    IMAP4Close();

    // Connect to the IMAP4 server
    if((error = Connect(m_nPort, mailHost, m_nConnectTimeout)) != UTE_SUCCESS) 
        return OnError(error);

    // Enter into a critical section (will be leaved in GetResponse()) (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Login
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1, "%sLOGIN %s %s\r\n", szTag, user, password);

    Send(buffer);
    if(GetResponse(szTag, buffer, MAX_LINE_BUFFER) != UTE_SUCCESS)
        return OnError(UTE_LOGIN_FAILED);

    // Set connection flag
    m_bConnection = TRUE;

    // Set authenticated state
    m_ClientState = STATE_AUTHENTICATED;

    // Run new mail checking thread
    m_hNewMailThread = (HANDLE) _beginthread(NewMailThreadEntry, 0, (VOID *)this);
    if(m_hNewMailThread == (HANDLE)-1)
        OnError(UTE_ERROR);

    return OnError(UTE_SUCCESS);
}
/********************************
IMAP4Close() 
    Close connection to the IMAP4 server by
    issuing a QUIT command
PARAM:
    NONE
RETURN:
    UTE_SUCCESS     - success
    UTE_ERROR       - failed
*********************************/
int CUT_IMAP4Client::IMAP4Close()
{
    // Logout if we were connected
    if(m_bConnection) {
        char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];
        GetNewTag(szTag, sizeof(szTag));

        // Enter into a critical section (will be leaved in GetResponse())
        EnterCriticalSection(&m_CriticalSection);

        Send(szTag);    
        Send("LOGOUT\r\n"); 
        GetResponse(szTag, buffer, MAX_LINE_BUFFER);

        // Set logout state
        m_ClientState = STATE_LOGOUT;
        }

    // Clear connection flag
    m_bConnection = FALSE;

    return CloseConnection();
}

/********************************
GetCapability
    Returns IMAP4 capability information. 

    Can be called in any client state
PARAM:
    none
RETURN:
    pointer to the IMAP4 capability information. 
    string is emty in case of error
*********************************/
LPCSTR CUT_IMAP4Client::GetCapability() 
{
    // If we do it for the first time - execute CAPABILITY Command 
    if(m_szCapabiltyInfo[0] == NULL && m_bConnection) {
        char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

        // Enter into a critical section (will be leaved in GetResponse())
        EnterCriticalSection(&m_CriticalSection);

        GetNewTag(szTag, sizeof(szTag));
        Send(szTag);    
        Send("CAPABILITY\r\n");
        if(GetResponse(szTag, buffer, MAX_LINE_BUFFER) != UTE_SUCCESS)
            m_szCapabiltyInfo[0] = NULL;
        }

    return m_szCapabiltyInfo;
}

/********************************
Noop
    Execute NOOP commnad.

    Can be called in any client state.

    "Since any command can return a status 
    update as untagged data, the NOOP command 
    can be used as a periodic poll for new 
    messages or message status updates during 
    a period of inactivity. The NOOP command 
    can also be used to reset any inactivity 
    autologout timer on the server." RFC 2060
PARAM:
    none
RETURN:
    UTE_SUCCESS         - success
    UTE_SOCK_TIMEOUT    - time out
    UTE_ERROR           - command failed
    UTE_NOCONNECTION    - no connection
*********************************/
int CUT_IMAP4Client::Noop() 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);
        
    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute NOOP Command 
    GetNewTag(szTag, sizeof(szTag));

    Send(szTag);    
    Send("NOOP\r\n");

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxSelect
    Selects a mailbox so that messages 
    in the mailbox can be accessed.

    Can be called only in the authenticated 
    and selected client states.
PARAM:
    lpszMailBoxName - mailbox name to select
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxSelect(LPCWSTR lpszMailBoxName){
	return MailBoxSelect(AC(lpszMailBoxName));}
#endif 
int CUT_IMAP4Client::MailBoxSelect(LPCSTR lpszMailBoxName) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];
    int     error;

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Initialize MailBox data
    m_bMailBoxWriteAccess       = TRUE;
    m_lMailBoxMsgNumber         = 0;
    m_lMailBoxMsgRecent         = 0;
    m_lMailBoxFlags         = 0;
    m_lMailBoxPermanentFlags    = 0;
    m_lMailBoxUIVV              = 0;
    m_lMailBoxMsgUnSeen         = 0;

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute SELECT Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sSELECT \"%s\"\r\n", szTag, lpszMailBoxName);
    Send(buffer);    

    // Get response
    if((error=GetResponse(szTag, buffer, MAX_LINE_BUFFER)) != UTE_SUCCESS)
        // If SELECT failed - we are in the authenticated state
        m_ClientState = STATE_AUTHENTICATED;
    else
        m_ClientState = STATE_SELECTED;

    return OnError(error);
}

/********************************
MailBoxExamine
    "The EXAMINE command is identical to SelectMailBox
    and returns the same output; however, the 
    selected mailbox is identified as read-only.
    No changes to the permanent state of the 
    mailbox, including per-user state, are permitted.

    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszMailBoxName - mailbox name to examine
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxExamine(LPCWSTR lpszMailBoxName){
	return MailBoxExamine(AC(lpszMailBoxName));}
#endif
int CUT_IMAP4Client::MailBoxExamine(LPCSTR lpszMailBoxName) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];
    int     error;

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Initialize MailBox data
    m_bMailBoxWriteAccess       = TRUE;
    m_lMailBoxMsgNumber         = 0;
    m_lMailBoxMsgRecent         = 0;
    m_lMailBoxFlags         = 0;
    m_lMailBoxPermanentFlags    = 0;
    m_lMailBoxUIVV              = 0;
    m_lMailBoxMsgUnSeen         = 0;

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute EXAMINE Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sEXAMINE \"%s\"\r\n", szTag, lpszMailBoxName);
    Send(buffer);    

    // Get response
    if((error=GetResponse(szTag, buffer, MAX_LINE_BUFFER)) != UTE_SUCCESS)
        // If EXAMINE failed - we are in the authenticated state
        m_ClientState = STATE_AUTHENTICATED;
    else
        m_ClientState = STATE_SELECTED;

    return OnError(error);
}

/********************************
MailBoxCreate
    "The CREATE command creates a mailbox with the given name.  

    If the mailbox name is suffixed with the server's hierarchy
    separator character (as returned from the server by a LIST
    command), this is a declaration that the client intends to create
    mailbox names under this name in the hierarchy.  Server
    implementations that do not require this declaration MUST ignore
    it.

    If the server's hierarchy separator character appears elsewhere in
    the name, the server SHOULD create any superior hierarchical names
    that are needed for the CREATE command to complete successfully.
    In other words, an attempt to create "foo/bar/zap" on a server in
    which "/" is the hierarchy separator character SHOULD create foo/
    and foo/bar/ if they do not already exist.

    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszMailBoxName - mailbox name to create
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxCreate(LPCWSTR lpszMailBoxName){
	return MailBoxCreate(AC(lpszMailBoxName));}
#endif
int CUT_IMAP4Client::MailBoxCreate(LPCSTR lpszMailBoxName) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute CREATE Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sCREATE \"%s\"\r\n", szTag, lpszMailBoxName);
    Send(buffer);    

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxDelete
    "The DELETE command permanently removes the mailbox 
    with the given name.  

    The DELETE command MUST NOT remove inferior hierarchical names.
    For example, if a mailbox "foo" has an inferior "foo.bar"
    (assuming "." is the hierarchy delimiter character), removing
    "foo" MUST NOT remove "foo.bar".  It is an error to attempt to
    delete a name that has inferior hierarchical names and also has
    the \Noselect mailbox name attribute (see the description of the
    LIST response for more details).

    It is permitted to delete a name that has inferior hierarchical
    names and does not have the \Noselect mailbox name attribute.  In
    this case, all messages in that mailbox are removed, and the name
    will acquire the \Noselect mailbox name attribute.

    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszMailBoxName - mailbox name to delete
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxDelete(LPCWSTR lpszMailBoxName){
	return MailBoxDelete(AC(lpszMailBoxName));}
#endif
int CUT_IMAP4Client::MailBoxDelete(LPCSTR lpszMailBoxName) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute DELETE Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sDELETE \"%s\"\r\n", szTag, lpszMailBoxName);
    Send(buffer);    

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxRename
    "The RENAME command changes the name of a mailbox.

    If the name has inferior hierarchical names, then the inferior
    hierarchical names MUST also be renamed.  For example, a rename of
    "foo" to "zap" will rename "foo/bar" (assuming "/" is the
    hierarchy delimiter character) to "zap/bar".

    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszMailBoxName     - mailbox name to rename
    lpszNewMailBoxName  - new mailbox name
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxRename(LPCWSTR lpszMailBoxName, LPCWSTR lpszNewMailBoxName){
	return MailBoxRename(AC(lpszMailBoxName), AC(lpszNewMailBoxName));}
#endif
int CUT_IMAP4Client::MailBoxRename(LPCSTR lpszMailBoxName, LPCSTR lpszNewMailBoxName) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL || lpszNewMailBoxName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute RENAME Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sRENAME \"%s\" \"%s\"\r\n", szTag, lpszMailBoxName, lpszNewMailBoxName);
    Send(buffer);    

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxSubscribe
    "The SUBSCRIBE command adds the specified mailbox name to the
    server's set of "active" or "subscribed" mailboxes as returned by
    the LSUB command.  

    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszMailBoxName - mailbox name to subscribe
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxSubscribe(LPCWSTR lpszMailBoxName){
	return MailBoxSubscribe(AC(lpszMailBoxName));}
#endif
int CUT_IMAP4Client::MailBoxSubscribe(LPCSTR lpszMailBoxName) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute SUBSCRIBE Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sSUBSCRIBE \"%s\"\r\n", szTag, lpszMailBoxName);
    Send(buffer);    

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxUnSubscribe
    "The UNSUBSCRIBE command removes the specified mailbox name from
    the server's set of "active" or "subscribed" mailboxes as returned
    by the LSUB command.

    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszMailBoxName - mailbox name to unsubscribe
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxUnSubscribe(LPCWSTR lpszMailBoxName){
	return MailBoxUnSubscribe(AC(lpszMailBoxName));}
#endif
int CUT_IMAP4Client::MailBoxUnSubscribe(LPCSTR lpszMailBoxName) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute UNSUBSCRIBE Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sUNSUBSCRIBE \"%s\"\r\n", szTag, lpszMailBoxName);
    Send(buffer);    

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxList
    "The LIST command returns a subset of names from the complete set
    of all names available to the client.  

    An empty ("" string) reference name argument indicates that the
    mailbox name is interpreted as by SELECT. The returned mailbox
    names MUST match the supplied mailbox name pattern.  A non-empty
    reference name argument is the name of a mailbox or a level of
    mailbox hierarchy, and indicates a context in which the mailbox
    name is interpreted in an implementation-defined manner.

    An empty ("" string) mailbox name argument is a special request to
    return the hierarchy delimiter and the root name of the name given
    in the reference.  The value returned as the root MAY be null if
    the reference is non-rooted or is null.  In all cases, the
    hierarchy delimiter is returned.  This permits a client to get the
    hierarchy delimiter even when no mailboxes by that name currently
    exist.

    The character "*" is a wildcard, and matches zero or more
    characters at this position.  The character "%" is similar to "*",
    but it does not match a hierarchy delimiter.  If the "%" wildcard
    is the last character of a mailbox name argument, matching levels
    of hierarchy are also returned.  If these levels of hierarchy are
    not also selectable mailboxes, they are returned with the
    \Noselect mailbox name attribute 

    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszReferenceName   - reference name
    lpszMailBoxName     - mailbox name with possible wildcards
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxList(LPCWSTR lpszReferenceName, LPCWSTR lpszMailBoxName){
	return MailBoxList(AC(lpszReferenceName), AC(lpszMailBoxName));}
#endif
int CUT_IMAP4Client::MailBoxList(LPCSTR lpszReferenceName, LPCSTR lpszMailBoxName) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);
    
    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL || lpszReferenceName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Clear items vector
    m_vectorListItems.clear();

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute LIST Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sLIST \"%s\" \"%s\"\r\n", szTag, lpszReferenceName, lpszMailBoxName);
    Send(buffer);    

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxLSub
    "The LSUB command returns a subset of names from the set of names
    that the user has declared as being "active" or "subscribed".
    Zero or more untagged LSUB replies are returned.  The arguments to
    LSUB are in the same form as those for LIST.

    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszReferenceName   - reference name
    lpszMailBoxName     - mailbox name with possible wildcards
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxLSub(LPCWSTR lpszReferenceName, LPCWSTR lpszMailBoxName){
	return MailBoxLSub(AC(lpszReferenceName), AC(lpszMailBoxName));}
#endif
int CUT_IMAP4Client::MailBoxLSub(LPCSTR lpszReferenceName, LPCSTR lpszMailBoxName) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL || lpszReferenceName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Clear items vector
    m_vectorListItems.clear();

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute LSUB Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sLSUB \"%s\" \"%s\"\r\n", szTag, lpszReferenceName, lpszMailBoxName);
    Send(buffer);    

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxStatus
    "The STATUS command requests the status of the indicated mailbox.
    It does not change the currently selected mailbox, nor does it
    affect the state of any messages in the queried mailbox.

    The STATUS command provides an alternative to opening a second
    IMAP4rev1 connection and doing an EXAMINE command on a mailbox to
    query that mailbox's status without deselecting the current
    mailbox in the first IMAP4rev1 connection.

    Unlike the LIST command, the STATUS command is not guaranteed to
    be fast in its response.  In some implementations, the server is
    obliged to open the mailbox read-only internally to obtain certain
    status information.  Also unlike the LIST command, the STATUS
    command does not accept wildcards.

    The currently defined status data items that can be requested are:

        MESSAGES       The number of messages in the mailbox.

        RECENT         The number of messages with the \Recent flag set.

        UIDNEXT        The next UID value that will be assigned to a new
                     message in the mailbox.  It is guaranteed that this
                     value will not change unless new messages are added
                     to the mailbox; and that it will change when new
                     messages are added even if those new messages are
                     subsequently expunged.

        UIDVALIDITY    The unique identifier validity value of the
                     mailbox.

        UNSEEN         The number of messages which do not have the \Seen
                     flag set.


    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszMailBoxName     - mailbox name 
    lpszStatusName      - space separated status names 
RETURN:
    UTE_SUCCESS                         - success
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxStatus(LPCWSTR lpszMailBoxName, LPCWSTR lpszStatusName, UT_StatusData &Result){
	return MailBoxStatus(AC(lpszMailBoxName), AC(lpszStatusName), Result);}
#endif
int CUT_IMAP4Client::MailBoxStatus(LPCSTR lpszMailBoxName, LPCSTR lpszStatusName, UT_StatusData &Result) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];
    int     error;

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL || lpszStatusName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute STATUS Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sSTATUS \"%s\" (%s)\r\n", szTag, lpszMailBoxName, lpszStatusName);
    Send(buffer);    

    // Get response
    if((error=GetResponse(szTag, buffer, MAX_LINE_BUFFER)) == UTE_SUCCESS)
        Result = m_LastUT_StatusData;

    return OnError(error);
}

/********************************
MailBoxAppend
    "The APPEND command appends the literal argument as a new message
    to the end of the specified destination mailbox.  This argument
    SHOULD be in the format of an [RFC-822] message.  

    If a flag parenthesized list is specified, the flags SHOULD be set in
    the resulting message; otherwise, the flag list of the resulting
    message is set empty by default.


    If a date_time is specified, the internal date SHOULD be set in the
    resulting message; otherwise, the internal date of the resulting
    message is set to the current date and time by default.

    Can be called only in the authenticated 
    and selected client states." RFC 2060
PARAM:
    lpszMailBoxName     - mailbox name to append (must exist)
    source              - message data source
    [lpszFlags]         - message flags string
    [lpszDateTime]      - date and time in format: 01-Jan-1999 05:54:01 +0004
RETURN:
    UTE_SUCCESS                         - success
    UTE_DS_OPEN_FAILED                  - failed to open data source
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxAppend(LPCWSTR lpszMailBoxName, CUT_DataSource &source, LPCWSTR lpszFlags,  LPCWSTR lpszDateTime){
	return MailBoxAppend(AC(lpszMailBoxName), source, AC(lpszFlags), AC(lpszDateTime));}
#endif
int CUT_IMAP4Client::MailBoxAppend(LPCSTR lpszMailBoxName, CUT_DataSource &source, LPCSTR lpszFlags,  LPCSTR lpszDateTime) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];
    int     error;

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);

    // Check client state
    if( m_ClientState != STATE_AUTHENTICATED && 
        m_ClientState != STATE_SELECTED)
        return OnError(UTE_AUTH_OR_SELECTED_STATE_REQUIRED);

    // Test input parameter
    if(lpszMailBoxName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Try to open data source and get it's size
    if(source.Open(UTM_OM_READING) != UTE_SUCCESS)
        return OnError(UTE_DS_OPEN_FAILED);
    int nSourceSize = source.Seek(0, SEEK_END);
    source.Close();

    // Execute APPEND Command 
    GetNewTag(szTag, sizeof(szTag));
    _snprintf(buffer,sizeof(buffer)-1,"%sAPPEND \"%s\" ", szTag, lpszMailBoxName);
    Send(buffer);    

    // Send status string
    if(lpszFlags != NULL && strlen(lpszFlags) > 0) {
        _snprintf(buffer,sizeof(buffer)-1,"(%s) ", lpszFlags);
        Send(buffer);    
        }

    // Send date and time
    if(lpszDateTime != NULL && strlen(lpszDateTime) != 0) {
        _snprintf(buffer,sizeof(buffer)-1,"\"%s\" ", lpszDateTime);
        Send(buffer);    
        }

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Send data size 
    _snprintf(buffer,sizeof(buffer)-1,"{%d}\r\n", nSourceSize);
    Send(buffer);    

    // Wait for continue response
    GetResponse(NULL, buffer, MAX_LINE_BUFFER);
    if(buffer[0] != '+')
        return OnError(UTE_ERROR);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Send data
    if((error = Send(source)) != UTE_SUCCESS) {
        // Exit the critical section
        LeaveCriticalSection(&m_CriticalSection);

        return OnError(error);
        }

    // Send end of data
    Send("\r\n");

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxAppend
    See previous function
PARAM:
    lpszMailBoxName     - mailbox name to append (must exist)
    source              - message data source
    [nFlags]            - message flags (see SystemFlags enum)
    [lpszDateTime]      - date and time in format: 01-Jan-1999 05:54:01 +0004
RETURN:
    UTE_SUCCESS                         - success
    UTE_DS_OPEN_FAILED                  - failed to open data source
    UTE_AUTH_OR_SELECTED_STATE_REQUIRED - wrong client state
    UTE_PARAMETER_INVALID_VALUE         - invalid parameter
    UTE_ERROR                           - command failed
    UTE_NOCONNECTION                    - no connection
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MailBoxAppend(LPCWSTR lpszMailBoxName, CUT_DataSource &source, int nFlags,  LPCWSTR lpszDateTime){
	return MailBoxAppend(AC(lpszMailBoxName), source, nFlags, AC(lpszDateTime));}
#endif
int CUT_IMAP4Client::MailBoxAppend(LPCSTR lpszMailBoxName, CUT_DataSource &source, int nFlags,  LPCSTR lpszDateTime) 
{
    char    szFlags[MAX_LINE_BUFFER + 1];

    // Convert status to string
    FlagsToString(nFlags, szFlags, MAX_LINE_BUFFER);

    return MailBoxAppend(lpszMailBoxName, source, szFlags,  lpszDateTime);
}
/********************************
MailBoxCheck
    "The CHECK command requests a checkpoint of the currently selected
    mailbox.  A checkpoint refers to any implementation-dependent
    housekeeping associated with the mailbox (e.g. resolving the
    server's in-memory state of the mailbox with the state on its
    disk) that is not normally executed as part of each command.  A
    checkpoint MAY take a non-instantaneous amount of real time to
    complete.  If a server implementation has no such housekeeping
    considerations, CHECK is equivalent to NOOP.

    Can be called only in selected state." RFC 2060
PARAM:
    none
RETURN:
    UTE_SUCCESS                 - success
    UTE_SOCK_TIMEOUT            - time out
    UTE_ERROR                   - command failed
    UTE_NOCONNECTION            - no connection
    UTE_SELECTED_STATE_REQUIRED - invalid state
*********************************/
int CUT_IMAP4Client::MailBoxCheck() 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);
        
    // Check client state
    if(m_ClientState != STATE_SELECTED)
        return OnError(UTE_SELECTED_STATE_REQUIRED);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute CHECK Command 
    GetNewTag(szTag, sizeof(szTag));
    Send(szTag);    
    Send("CHECK\r\n");

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MailBoxClose
    "The CLOSE command permanently removes from the currently selected
    mailbox all messages that have the \Deleted flag set, and returns
    to authenticated state from selected state.  No untagged EXPUNGE
    responses are sent.

    No messages are removed, and no error is given, if the mailbox is
    selected by an EXAMINE command or is otherwise selected read-only.

    Even if a mailbox is selected, a SELECT, EXAMINE, or LOGOUT
    command MAY be issued without previously issuing a CLOSE command.
    The SELECT, EXAMINE, and LOGOUT commands implicitly close the
    currently selected mailbox without doing an expunge.  However,
    when many messages are deleted, a CLOSE-LOGOUT or CLOSE-SELECT
    sequence is considerably faster than an EXPUNGE-LOGOUT or
    EXPUNGE-SELECT because no untagged EXPUNGE responses (which the
    client would probably ignore) are sent.

    Can be called only in selected state." RFC 2060
PARAM:
    none
RETURN:
    UTE_SUCCESS                 - success
    UTE_SOCK_TIMEOUT            - time out
    UTE_ERROR                   - command failed
    UTE_NOCONNECTION            - no connection
    UTE_SELECTED_STATE_REQUIRED - invalid state
*********************************/
int CUT_IMAP4Client::MailBoxClose() 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);
        
    // Check client state
    if(m_ClientState != STATE_SELECTED)
        return OnError(UTE_SELECTED_STATE_REQUIRED);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute CLOSE Command 
    GetNewTag(szTag, sizeof(szTag));
    Send(szTag);    
    Send("CLOSE\r\n");

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MessageExpunge
    "The EXPUNGE command permanently removes from the currently
    selected mailbox all messages that have the \Deleted flag set.
    Before returning an OK to the client, an untagged EXPUNGE response
    is sent for each message that is removed.

    Virtual function OnExpunge will be called with deleted message 
    number as a parameter for each EXPUNGE response.

    The EXPUNGE response reports that the specified message sequence
    number has been permanently removed from the mailbox.  The message
    sequence number for each successive message in the mailbox is
    immediately decremented by 1, and this decrement is reflected in
    message sequence numbers in subsequent responses (including other
    untagged EXPUNGE responses).

    As a result of the immediate decrement rule, message sequence
    numbers that appear in a set of successive EXPUNGE responses
    depend upon whether the messages are removed starting from lower
    numbers to higher numbers, or from higher numbers to lower
    numbers.  For example, if the last 5 messages in a 9-message
    mailbox are expunged; a "lower to higher" server will send five
    untagged EXPUNGE responses for message sequence number 5, whereas
    a "higher to lower server" will send successive untagged EXPUNGE
    responses for message sequence numbers 9, 8, 7, 6, and 5.

    Can be called only in selected state." RFC 2060
PARAM:
    none
RETURN:
    UTE_SUCCESS                 - success
    UTE_SOCK_TIMEOUT            - time out
    UTE_ERROR                   - command failed
    UTE_NOCONNECTION            - no connection
    UTE_SELECTED_STATE_REQUIRED - invalid state
*********************************/
int CUT_IMAP4Client::MessageExpunge() 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);
        
    // Check client state
    if(m_ClientState != STATE_SELECTED)
        return OnError(UTE_SELECTED_STATE_REQUIRED);

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute EXPUNGE Command 
    GetNewTag(szTag, sizeof(szTag));
    Send(szTag);    
    Send("EXPUNGE\r\n");

    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MessageSearch
        "The SEARCH command searches the mailbox for messages that match
        the given searching criteria.  Searching criteria consist of one
        or more search keys.  The untagged SEARCH response from the server
        contains a listing of message sequence numbers corresponding to
        those messages that match the searching criteria.

        When multiple keys are specified, the result is the intersection
        (AND function) of all the messages that match those keys.  For
        example, the criteria DELETED FROM "SMITH" SINCE 1-Feb-1994 refers
        to all deleted messages from Smith that were placed in the mailbox
        since February 1, 1994.  A search key can also be a parenthesized
        list of one or more search keys (e.g. for use with the OR and NOT
        keys).

        Server implementations MAY exclude [MIME-IMB] body parts with
        terminal content media types other than TEXT and MESSAGE from
        consideration in SEARCH matching.

        The OPTIONAL [CHARSET] specification consists of the word
        "CHARSET" followed by a registered [CHARSET].  It indicates the
        [CHARSET] of the strings that appear in the search criteria.
        [MIME-IMB] content transfer encodings, and [MIME-HDRS] strings in
        [RFC-822]/[MIME-IMB] headers, MUST be decoded before comparing
        text in a [CHARSET] other than US-ASCII.  US-ASCII MUST be
        supported; other [CHARSET]s MAY be supported.  If the server does
        not support the specified [CHARSET], it MUST return a tagged NO
        response (not a BAD).

        In all search keys that use strings, a message matches the key if
        the string is a substring of the field.  The matching is case-
        insensitive.

        The defined search keys are as follows.  Refer to the Formal
        Syntax section for the precise syntactic definitions of the
        arguments.

      <message set>  Messages with message sequence numbers
                     corresponding to the specified message sequence
                     number set

      ALL            All messages in the mailbox; the default initial
                     key for ANDing.

      ANSWERED       Messages with the \Answered flag set.

      BCC <string>   Messages that contain the specified string in the
                     envelope structure's BCC field.

      BEFORE <date>  Messages whose internal date is earlier than the
                     specified date.

      BODY <string>  Messages that contain the specified string in the
                     body of the message.

      CC <string>    Messages that contain the specified string in the
                     envelope structure's CC field.

      DELETED        Messages with the \Deleted flag set.

      DRAFT          Messages with the \Draft flag set.

      FLAGGED        Messages with the \Flagged flag set.

      FROM <string>  Messages that contain the specified string in the
                     envelope structure's FROM field.

      HEADER <field-name> <string>
                     Messages that have a header with the specified
                     field-name (as defined in [RFC-822]) and that
                     contains the specified string in the [RFC-822]
                     field-body.

      KEYWORD <flag> Messages with the specified keyword set.

      LARGER <n>     Messages with an [RFC-822] size larger than the
                     specified number of octets.

      NEW            Messages that have the \Recent flag set but not the
                     \Seen flag.  This is functionally equivalent to
                     "(RECENT UNSEEN)".

      NOT <search-key>
                     Messages that do not match the specified search
                     key.

      OLD            Messages that do not have the \Recent flag set.
                     This is functionally equivalent to "NOT RECENT" (as
                     opposed to "NOT NEW").

      ON <date>      Messages whose internal date is within the
                     specified date.

      OR <search-key1> <search-key2>
                     Messages that match either search key.

      RECENT         Messages that have the \Recent flag set.

      SEEN           Messages that have the \Seen flag set.

      SENTBEFORE <date>
                     Messages whose [RFC-822] Date: header is earlier
                     than the specified date.

      SENTON <date>  Messages whose [RFC-822] Date: header is within the
                     specified date.

      SENTSINCE <date>
                     Messages whose [RFC-822] Date: header is within or
                     later than the specified date.

      SINCE <date>   Messages whose internal date is within or later
                     than the specified date.

      SMALLER <n>    Messages with an [RFC-822] size smaller than the
                     specified number of octets.

      SUBJECT <string>
                     Messages that contain the specified string in the
                     envelope structure's SUBJECT field.

      TEXT <string>  Messages that contain the specified string in the
                     header or body of the message.

      TO <string>    Messages that contain the specified string in the
                     envelope structure's TO field.

      UID <message set>
                     Messages with unique identifiers corresponding to
                     the specified unique identifier set.

      UNANSWERED     Messages that do not have the \Answered flag set.

      UNDELETED      Messages that do not have the \Deleted flag set.

      UNDRAFT        Messages that do not have the \Draft flag set.

      UNFLAGGED      Messages that do not have the \Flagged flag set.

      UNKEYWORD <flag>
                     Messages that do not have the specified keyword
                     set.

      UNSEEN         Messages that do not have the \Seen flag set.


    Can be called only in selected state." RFC 2060
PARAM:
    lpszSerchString - serch string
    [lpszCharSet]   - charset
    [bUseMsgUID]    - use UID message numbers instead of sequence numbers
RETURN:
    UTE_SUCCESS                 - success
    UTE_SOCK_TIMEOUT            - time out
    UTE_ERROR                   - command failed
    UTE_NOCONNECTION            - no connection
    UTE_SELECTED_STATE_REQUIRED - invalid state
    UTE_PARAMETER_INVALID_VALUE - invalid parameter
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MessageSearch(LPCWSTR lpszSerchString, LPCWSTR lpszCharSet, BOOL bUseMsgUID){
	return MessageSearch(AC(lpszSerchString), AC(lpszCharSet), bUseMsgUID);}
#endif
int CUT_IMAP4Client::MessageSearch(LPCSTR lpszSerchString, LPCSTR lpszCharSet, BOOL bUseMsgUID) 
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);
        
    // Test input parameter
    if(lpszSerchString == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Check client state
    if(m_ClientState != STATE_SELECTED)
        return OnError(UTE_SELECTED_STATE_REQUIRED);

    // Clear search result vector
    m_vectorSearchResult.clear();

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute SEARCH Command 
    GetNewTag(szTag, sizeof(szTag));
    Send(szTag);

    // Use UID message numbers instead of sequence numbers
    if(bUseMsgUID)
        Send("UID ");

    Send("SEARCH ");

    // Send charset
    if(lpszCharSet != NULL && strlen(lpszCharSet) > 0) {
        _snprintf(buffer,sizeof(buffer)-1,"[CHARSET %s ] ", lpszCharSet);
        Send(buffer);    
        }

    // Send search string
    Send(lpszSerchString);
    Send("\r\n");
    
    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MessageCopy
    "The COPY command copies the specified message(s) to the end of the
    specified destination mailbox.  The flags and internal date of the
    message(s) SHOULD be preserved in the copy.

    Can be called only in selected state." RFC 2060
PARAM:
    lpszMessageSet  - message set (Example: "2,4:7,9,12:*" )
    lpszMailBoxName - mailbox name to copy to
    [bUseMsgUID]    - use UID message numbers instead of sequence numbers
RETURN:
    UTE_SUCCESS                 - success
    UTE_SOCK_TIMEOUT            - time out
    UTE_ERROR                   - command failed
    UTE_NOCONNECTION            - no connection
    UTE_SELECTED_STATE_REQUIRED - invalid state
    UTE_PARAMETER_INVALID_VALUE - invalid parameter
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MessageCopy(LPCWSTR lpszMessageSet, LPCWSTR lpszMailBoxName, BOOL bUseMsgUID){
	return MessageCopy(AC(lpszMessageSet), AC(lpszMailBoxName), bUseMsgUID);}
#endif
int CUT_IMAP4Client::MessageCopy(LPCSTR lpszMessageSet, LPCSTR lpszMailBoxName, BOOL bUseMsgUID)
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);
        
    // Test input parameter
    if(lpszMessageSet == NULL || lpszMailBoxName == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Check client state
    if(m_ClientState != STATE_SELECTED)
        return OnError(UTE_SELECTED_STATE_REQUIRED);
    
    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute COPY Command 
    GetNewTag(szTag, sizeof(szTag));
    Send(szTag);

    // Use UID message numbers instead of sequence numbers
    if(bUseMsgUID)
        Send("UID ");

    _snprintf(buffer,sizeof(buffer)-1,"COPY %s \"%s\"\r\n", lpszMessageSet, lpszMailBoxName);
    Send(buffer);    
    
    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MessageFetch
    "The FETCH command retrieves data associated with a message in the
    mailbox.  The data items to be fetched can be either a single atom
    or a parenthesized list.

    Can be called only in selected state." RFC 2060
PARAM:
    lpszMessageSet  - message set (Example: "2,4:7,9,12:*" )
    Item            - message data items (BODY, BODY TEXT or BODY HEADER)
    [bUseMsgUID]    - use UID message numbers instead of sequence numbers
RETURN:
    UTE_SUCCESS                 - success
    UTE_SOCK_TIMEOUT            - time out
    UTE_ERROR                   - command failed
    UTE_NOCONNECTION            - no connection
    UTE_SELECTED_STATE_REQUIRED - invalid state
    UTE_PARAMETER_INVALID_VALUE - invalid parameter
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MessageFetch(LPCWSTR lpszMessageSet, FetchItem Item, BOOL bSetSeenFlag, BOOL bUseMsgUID){
	return MessageFetch(AC(lpszMessageSet), Item, bSetSeenFlag, bUseMsgUID);}
#endif
int CUT_IMAP4Client::MessageFetch(LPCSTR lpszMessageSet, FetchItem Item, BOOL bSetSeenFlag, BOOL bUseMsgUID)
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];
    char    *szItem[] = {"BODY", "BODY.PEEK"};
    char    *szSubItem[] = {"[]", "[TEXT]", "[HEADER]"};

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);
        
    // Test input parameter
    if(lpszMessageSet == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Check client state
    if(m_ClientState != STATE_SELECTED)
        return OnError(UTE_SELECTED_STATE_REQUIRED);

    // Clear result vector
    m_vectorMsgData.clear();

    // Clear result Data Sources
    vector<CUT_DataSource*>::iterator   Index;
    for(Index = m_vectorPtrDataSource.begin(); Index != m_vectorPtrDataSource.end(); Index++) 
        delete *Index;
    m_vectorPtrDataSource.clear();

    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute FETCH Command 
    GetNewTag(szTag, sizeof(szTag));
    Send(szTag);

    // Use UID message numbers instead of sequence numbers
    if(bUseMsgUID)
        Send("UID ");

    _snprintf(buffer,sizeof(buffer)-1,"FETCH %s (RFC822.SIZE FLAGS UID INTERNALDATE %s%s)\r\n", lpszMessageSet, (bSetSeenFlag) ? szItem[0] : szItem[1], szSubItem[(int)Item]);
    Send(buffer);    
    
    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MessageStore
      "The STORE command alters data associated with a message in the
      mailbox.  Normally, STORE will return the updated value of the
      data with an untagged FETCH response.  A suffix of ".SILENT" in
      the data item name prevents the untagged FETCH, and the server
      SHOULD assume that the client has determined the updated value
      itself or does not care about the updated value.

         Note: regardless of whether or not the ".SILENT" suffix was
         used, the server SHOULD send an untagged FETCH response if a
         change to a message's flags from an external source is
         observed.  The intent is that the status of the flags is
         determinate without a race condition.

      The currently defined data items that can be stored are:

      FLAGS <flag list>
                     Replace the flags for the message with the
                     argument.  The new value of the flags are returned
                     as if a FETCH of those flags was done.

      FLAGS.SILENT <flag list>
                     Equivalent to FLAGS, but without returning a new
                     value.

      +FLAGS <flag list>
                     Add the argument to the flags for the message.  The
                     new value of the flags are returned as if a FETCH
                     of those flags was done.

      +FLAGS.SILENT <flag list>
                     Equivalent to +FLAGS, but without returning a new
                     value.

      -FLAGS <flag list>
                     Remove the argument from the flags for the message.
                     The new value of the flags are returned as if a
                     FETCH of those flags was done.

      -FLAGS.SILENT <flag list>
                     Equivalent to -FLAGS, but without returning a new
                     value.

    Can be called only in selected state." RFC 2060
PARAM:
    lpszMessageSet  - message set (Example: "2,4:7,9,12:*" )
    lpszStoreType   - store type
    lpszFlags       - store flags
    [bUseMsgUID]    - use UID message numbers instead of sequence numbers
RETURN:
    UTE_SUCCESS                 - success
    UTE_SOCK_TIMEOUT            - time out
    UTE_ERROR                   - command failed
    UTE_NOCONNECTION            - no connection
    UTE_SELECTED_STATE_REQUIRED - invalid state
    UTE_PARAMETER_INVALID_VALUE - invalid parameter
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MessageStore(LPCWSTR lpszMessageSet, LPCWSTR lpszStoreType, LPCWSTR lpszFlags, BOOL bUseMsgUID){
	return MessageStore(AC(lpszMessageSet), AC(lpszStoreType), AC(lpszFlags), bUseMsgUID);}
#endif
int CUT_IMAP4Client::MessageStore(LPCSTR lpszMessageSet, LPCSTR lpszStoreType, LPCSTR lpszFlags, BOOL bUseMsgUID)
{
    char    buffer[MAX_LINE_BUFFER + 1], szTag[MAX_TAG_SIZE + 1];

    // Check connection
    if(!m_bConnection)
        return OnError(UTE_NOCONNECTION);
        
    // Test input parameter
    if(lpszMessageSet == NULL || lpszStoreType == NULL || lpszFlags == NULL)
        return OnError(UTE_PARAMETER_INVALID_VALUE);

    // Check client state
    if(m_ClientState != STATE_SELECTED)
        return OnError(UTE_SELECTED_STATE_REQUIRED);
    
    // Enter into a critical section (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Execute STORE Command 
    GetNewTag(szTag, sizeof(szTag));
    Send(szTag);

    // Use UID message numbers instead of sequence numbers
    if(bUseMsgUID)
        Send("UID ");

    _snprintf(buffer,sizeof(buffer)-1,"STORE %s %s (%s)\r\n", lpszMessageSet, lpszStoreType, lpszFlags);
    Send(buffer);    
    
    // Get response
    return OnError(GetResponse(szTag, buffer, MAX_LINE_BUFFER));
}

/********************************
MessageStore
    See previous function
PARAM:
    lpszMessageSet  - message set (Example: "2,4:7,9,12:*" )
    nStoreType      - store type 
    nFlags          - flags
    [bUseMsgUID]    - use UID message numbers instead of sequence numbers
RETURN:
    UTE_SUCCESS                 - success
    UTE_SOCK_TIMEOUT            - time out
    UTE_ERROR                   - command failed
    UTE_NOCONNECTION            - no connection
    UTE_SELECTED_STATE_REQUIRED - invalid state
    UTE_PARAMETER_INVALID_VALUE - invalid parameter
*********************************/
#if defined _UNICODE
int CUT_IMAP4Client::MessageStore(LPCWSTR lpszMessageSet, StoreType nStoreType, int nFlags, BOOL bUseMsgUID){
	return MessageStore(AC(lpszMessageSet), nStoreType, nFlags, bUseMsgUID);}
#endif
int CUT_IMAP4Client::MessageStore(LPCSTR lpszMessageSet, StoreType nStoreType, int nFlags, BOOL bUseMsgUID)
{
    char    szFlags[MAX_LINE_BUFFER + 1];
    char    *szStoreType[] = {  "FLAGS",
                                "FLAGS.SILENT",
                                "+FLAGS",
                                "+FLAGS.SILENT",
                                "-FLAGS",
                                "-FLAGS.SILENT"};

    // Convert status to string
    FlagsToString(nFlags, szFlags, MAX_LINE_BUFFER);

    return MessageStore(lpszMessageSet, szStoreType[(int)nStoreType], szFlags, bUseMsgUID);
}

/********************************
OnAlertMessage
    This virtual function called each time we 
    receive an alert message.

PARAM:
    lpszAlertMsg    - alert message
RETURN:
    none
*********************************/
void CUT_IMAP4Client::OnAlertMessage(LPCSTR  /* lpszAlertMsg */ ) 
{
}

/********************************
OnExpunge
    Virtual function OnExpunge will be called with deleted message 
    number as a parameter for each EXPUNGE response.

    "The EXPUNGE response reports that the specified message sequence
    number has been permanently removed from the mailbox.  The message
    sequence number for each successive message in the mailbox is
    immediately decremented by 1, and this decrement is reflected in
    message sequence numbers in subsequent responses (including other
    untagged EXPUNGE responses).

    As a result of the immediate decrement rule, message sequence
    numbers that appear in a set of successive EXPUNGE responses
    depend upon whether the messages are removed starting from lower
    numbers to higher numbers, or from higher numbers to lower
    numbers.  For example, if the last 5 messages in a 9-message
    mailbox are expunged; a "lower to higher" server will send five
    untagged EXPUNGE responses for message sequence number 5, whereas
    a "higher to lower server" will send successive untagged EXPUNGE
    responses for message sequence numbers 9, 8, 7, 6, and 5." RFC 2060
PARAM:
    nMsgNumber  - message number
RETURN:
    none
*********************************/
void CUT_IMAP4Client::OnExpunge(long /*  nMsgNumber */ ) 
{
}

/********************************
OnFetch
    Virtual function OnFetch() called if 
    we receive a FETCH response from the server.
PARAM:
    none
RETURN:
    none
*********************************/
void CUT_IMAP4Client::OnFetch()
{
    
}

/********************************
OnExists
    Virtual function called each time we receive 
    an EXISTS response
PARAM:
    nMsgNumber  - number of messages in the mail box
RETURN:
    none
*********************************/
void CUT_IMAP4Client::OnExists(long  /* nMsgNumber */ ) 
{
}

/********************************
OnRecent
    Virtual function called each time we receive 
    an RECENT response
PARAM:
    nMsgNumber  - number of messages in the mail box with \Recent flag
RETURN:
    none
*********************************/
void CUT_IMAP4Client::OnRecent(long /*  nMsgNumber */ ) 
{
}

/********************************
NewMailThreadStarted
    Virtual function called when the new mail
    thread is started
PARAM:
    none
RETURN:
    FALSE   - if the thread should terminate
*********************************/
BOOL CUT_IMAP4Client::NewMailThreadStarted() {
    return TRUE;
}

/********************************
NewMailThreadTerminated
    Virtual function called when the new mail
    thread is terminated
PARAM:
    none
RETURN:
    none
*********************************/
void CUT_IMAP4Client::NewMailThreadTerminated() {
}

/***************************************************
NewMailThreadEntry
    This thread checks new mail by executing NOOP command.
PARAM:
    ptr - pointer to the CUT_IMAP4Client class
RETURN:
    none
****************************************************/
void CUT_IMAP4Client::NewMailThreadEntry(void *ptr) 
{
    CUT_IMAP4Client *ptrIMAP4Client = (CUT_IMAP4Client *) ptr;
    long            lTime = 0;

    if(ptrIMAP4Client->NewMailThreadStarted())
        while(!ptrIMAP4Client->m_bGoingToDestroy) {
            Sleep(1000);
            ++lTime;
        
            if(lTime >= ptrIMAP4Client->m_lNewMailCheckInterval) {
                lTime = 0;

                // Execute the Noop command to get new mail
                if(ptrIMAP4Client->m_ClientState == STATE_SELECTED && !ptrIMAP4Client->m_bGoingToDestroy)
                    ptrIMAP4Client->Noop();
                }
            }

    ptrIMAP4Client->NewMailThreadTerminated();

    // End the thread
    ptrIMAP4Client->m_hNewMailThread = (HANDLE)-1;
    _endthread();
}

/********************************
GetMailBoxListSize
    Returns the size of the mailbox list.
    Mailbox list is filled during the 
    MailBoxList() or MailBoxLSub() functions.
PARAM:
    none
RETURN:
    mailbox list size
*********************************/
long CUT_IMAP4Client::GetMailBoxListSize() {
    return (long)m_vectorListItems.size();
}

/********************************
GetMailBoxListItem
    Returns the mailbox list item.
    Mailbox list is filled during the 
    MailBoxList() or MailBoxLSub() functions.
PARAM:
    lIndex          - index of item
    lpszName        - pointer to the Name
    lpszDelimiter   - pointer to the Delimiter
    nAttrib         - attributes
RETURN:
    UTE_SUCCESS                 - success
    UTE_INDEX_OUTOFRANGE        - index out of range
    UTE_PARAMETER_INVALID_VALUE - invalid parameters
*********************************/
int CUT_IMAP4Client::GetMailBoxListItem(long lIndex, LPCSTR &lpszName, LPCSTR &lpszDelimiter, int &nAttrib) 
{
    if((unsigned) lIndex >= m_vectorListItems.size())
        return(OnError(UTE_INDEX_OUTOFRANGE));

    // Copy attribute
    nAttrib = m_vectorListItems[lIndex].m_nNameAttrib;

    // Copy name pointer 
    lpszName = m_vectorListItems[lIndex].m_strName.c_str();

    // Copy delimiter  pointer 
    lpszDelimiter = m_vectorListItems[lIndex].m_strDelimiter.c_str();

    return(OnError(UTE_SUCCESS));
}
// v4.2 overloaded implementation to allow for _TCHAR retrieval
int CUT_IMAP4Client::GetMailBoxListItem(long lIndex, CUT_MailBoxListItemT *item) 
{
	if(item == NULL) 
		return (OnError(UTE_PARAMETER_INVALID_VALUE));

    if((unsigned) lIndex >= m_vectorListItems.size())
        return(OnError(UTE_INDEX_OUTOFRANGE));

	if(item->m_strDelimiter != NULL) delete [] item->m_strDelimiter;
	if(item->m_strName != NULL) delete [] item->m_strName;

	size_t size = m_vectorListItems[lIndex].m_strName.length();
	item->m_strName = new _TCHAR[size+1];
	CUT_Str::cvtcpy(item->m_strName, size+1, m_vectorListItems[lIndex].m_strName.c_str());

	size = m_vectorListItems[lIndex].m_strDelimiter.length();
	item->m_strDelimiter = new _TCHAR[size+1];
	CUT_Str::cvtcpy(item->m_strDelimiter, size+1, m_vectorListItems[lIndex].m_strDelimiter.c_str());

	item->m_nNameAttrib = m_vectorListItems[lIndex].m_nNameAttrib;

    return(OnError(UTE_SUCCESS));
}

/********************************
GetMsgDataListSize
    Returns the size of the message data list.
    Message data list is filled during the 
    MessageFetch() function call.
PARAM:
    none
RETURN:
    message data list size
*********************************/
long CUT_IMAP4Client::GetMsgDataListSize() {
    return (long)m_vectorMsgData.size();
}

/********************************
GetMsgDataListItem
    Returns the message data list item.
    Message data list is filled during the 
    MessageFetch() function call.
PARAM:
    lIndex  - index of item
RETURN:
    UTE_SUCCESS                 - success
    UTE_INDEX_OUTOFRANGE        - index out of range
    UTE_PARAMETER_INVALID_VALUE - invalid parameters
*********************************/
int CUT_IMAP4Client::GetMsgDataListItem(long lIndex, CUT_MsgData *&ptrMsgData) 
{
    if((unsigned) lIndex >= m_vectorMsgData.size())
        return(OnError(UTE_INDEX_OUTOFRANGE));

    CUT_MsgDataA *mda = &m_vectorMsgData[lIndex];

	// v4.2 conversion from internal ANSI to UI wide char 
	m_msgData.m_lFlags = mda->m_lFlags;
	m_msgData.m_lMsgNumber = mda->m_lMsgNumber;
	m_msgData.m_lSize = mda->m_lSize;
	m_msgData.m_lUID = mda->m_lUID;
	m_msgData.m_nDataType = mda->m_nDataType;
	m_msgData.m_ptrData = mda->m_ptrData;
	CUT_Str::cvtcpy(m_msgData.m_szDateTime, MAX_DATE_TIME_SIZE+1, mda->m_szDateTime);
	CUT_Str::cvtcpy(m_msgData.m_szUnknownFlags, MAX_LINE_BUFFER+1, mda->m_szUnknownFlags);

	ptrMsgData = &m_msgData;

    return(OnError(UTE_SUCCESS));
}

/********************************
GetSearchResultSize
    Returns the size of the last search result.
PARAM:
    none
RETURN:
    search result size
*********************************/
long CUT_IMAP4Client::GetSearchResultSize() {
    return (long)m_vectorSearchResult.size();
}

/********************************
GetSearchResultItem
    Returns the item of the last search result.
PARAM:
    lIndex  - index of item
    lItem   - search item
RETURN:
    UTE_SUCCESS                 - success
    UTE_INDEX_OUTOFRANGE        - index out of range
    UTE_PARAMETER_INVALID_VALUE - invalid parameters
*********************************/
int CUT_IMAP4Client::GetSearchResultItem(long lIndex, long &lItem) 
{
    if((unsigned) lIndex >= m_vectorSearchResult.size())
        return(OnError(UTE_INDEX_OUTOFRANGE));

    lItem = m_vectorSearchResult[lIndex];

    return(OnError(UTE_SUCCESS));
}

/********************************
GetClientState
    Returns current client state.
PARAM:
    none
RETURN:
    client state:
        STATE_NON_AUTHENTICATED
        STATE_AUTHENTICATED
        STATE_SELECTED
        STATE_LOGOUT
*********************************/
ClientState CUT_IMAP4Client::GetClientState() {
    return m_ClientState;
}

/********************************
GetLastCommandResponseText
    Returns last command response text
PARAM:
    none
RETURN:
    last command response text
*********************************/
LPCSTR CUT_IMAP4Client::GetLastCommandResponseText() {
    return m_szLastResponse;
}


/**************************************************************
SocketOnConnected(SOCKET s, const char *lpszName)
	
		If the security is enabled then perform te SSL neogotiation
		otherwise just return a success and let the plain text FTP handles
		the comunication

		To let the server know that we are looking for SSL or TLS we need to
		send the following command
		FEAT  
		The Feature negotiation mechanism for the File Transfer Protocol 
		
		To ask the server for SSL or TLS negotiation 
		we will send the AUTH command.

		A parameter for the AUTH command to indicate that TLS is
		required.  It is recommended that 'TLS', 'TLS-C', 'SSL' and 'TLS-P'
		are acceptable, and mean the following :-

		 'TLS' or 'TLS-C' - the TLS protocol or the SSL protocol will be
			negotiated on the control connection.  The default protection
			setting for the Data connection is 'Clear'.

			'SSL' or 'TLS-P' - the TLS protocol or the SSL protocol will be
			negotiated on the control connection.  The default protection
			setting for the Data connection is 'Private'.  This is primarily
			for backward compatibility.

	Notice that we will first send a TLS P to select The highest implementation.
	the server might response with the response
	503 unknown security mechanism 

	if this happened we will issue an Auth SSL command.

Param:
	SOCKET s		- The newly created socket 
	lpszName		- apointer to the host name we are attempting to connect to


Return:
	UTE_NO_RESPONSE - Server did not response to our command
	UTE_CONNECT_FAIL_NO_SSL_SUPPORT - Server does not support SSL.
	UTE_CONNECT_FAILED	-	 The connection have failed
	security  errors   - This function may fail with other error
			UTE_LOAD_SECURITY_LIBRARIES_FAILED
			UTE_OUT_OF_MEMORY
			UTE_FAILED_TO_GET_SECURITY_STREAM_SIZE
			UTE_OUT_OF_MEMORY
			UTE_FAILED_TO_QUERY_CERTIFICATE
			UTE_NULL_PARAM
			UTE_PARAMETER_INVALID_VALUE
			UTE_FAILED_TO_GET_CERTIFICATE_CHAIN
			UTE_FAILED_TO_VERIFY_CERTIFICATE_CHAIN
			UTE_FAILED_TO_VERIFY_CERTIFICATE_TRUST
	
**************************************************************/
int CUT_IMAP4Client::SocketOnConnected(SOCKET s, const char *lpszName){

	char    buffer[MAX_LINE_BUFFER + 1];
#ifdef CUT_SECURE_SOCKET
	
		int		rt = UTE_SUCCESS;	
		char    szTag[MAX_TAG_SIZE + 1];
		BOOL bSecFlag = GetSecurityEnabled();

		if(bSecFlag)
		{
				// set connection flag
				m_bConnection = TRUE;

				// disable the secure comunication so we can send the plain data 
				SetSecurityEnabled(FALSE);

				// enter into a critical section (will be leaved in GetResponse()) (will be leaved in GetResponse())
				EnterCriticalSection(&m_CriticalSection);

				// get server response
				GetResponse(NULL, buffer, MAX_LINE_BUFFER);

				// get server capability
				GetCapability();
				
				// Check if server support TLS
				strupr(m_szCapabiltyInfo);
				if(strstr(m_szCapabiltyInfo, "STARTTLS") == NULL)
					return UTE_IMAP4_TLS_NOT_SUPPORTED;
				*m_szCapabiltyInfo = NULL;

				// Enter into a critical section (will be leaved in GetResponse()) (will be leaved in GetResponse())
				EnterCriticalSection(&m_CriticalSection);

				// Execute STARTTLS Command 
				GetNewTag(szTag, sizeof(szTag));
				Send(szTag);    
				Send("STARTTLS\r\n");

				// get response
				rt = GetResponse(szTag, buffer, MAX_LINE_BUFFER);

				if(rt == UTE_SUCCESS)
				{
					// reset back the security so we can proceed with negotiation
					SetSecurityEnabled(bSecFlag);
					rt =  CUT_SecureSocketClient::SocketOnConnected(s, lpszName);
				}
				
				return rt;
		}
		else
		{
			// Enter into a critical section (will be leaved in GetResponse()) (will be leaved in GetResponse())
			EnterCriticalSection(&m_CriticalSection);

			// Get server response
			GetResponse(NULL, buffer,MAX_LINE_BUFFER);

			return UTE_SUCCESS;
		}

#else		

	UNREFERENCED_PARAMETER(s);
	UNREFERENCED_PARAMETER(lpszName);

	// Enter into a critical section (will be leaved in GetResponse()) (will be leaved in GetResponse())
    EnterCriticalSection(&m_CriticalSection);

    // Get server response
    GetResponse(NULL, buffer,MAX_LINE_BUFFER);

	// v4.2 Unreachable code in secure builds - moved endif
	return OnError(UTE_SUCCESS);
#endif
}

#pragma warning ( pop )
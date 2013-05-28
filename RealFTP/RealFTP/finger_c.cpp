//=================================================================
//  class: CUT_FingerClient
//  File:  Finger_C.cpp
//
//  Purpose:
//
//  Finger client class
//  Implementation of Finger protocol
//  RFC  1288
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

#include "ut_clnt.h"
#include "uterr.h"      /* Errors enumeration & string constants        */
#include "finger_c.h"
#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)


/*********************************************
Constructor
**********************************************/
CUT_FingerClient::CUT_FingerClient() :
        m_nPort(79),                        // Set default port 79
        m_nConnectTimeout(5),               // Set default connect time out to 5 sec.
        m_nReceiveTimeout(5)                // Set default receive time out to 5 sec.
{
}
/*********************************************
Destructor
**********************************************/
CUT_FingerClient::~CUT_FingerClient()
{
    m_listReturnLines.ClearList();          // Clear return strings list
}

/*********************************************
SetPort
    Sets the port number to connect to
Params
    newPort     - connect port
Return
    UTE_SUCCESS - success
**********************************************/
int CUT_FingerClient::SetPort(unsigned int newPort) {
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
unsigned int CUT_FingerClient::GetPort() const
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
int CUT_FingerClient::SetConnectTimeout(int secs){
    
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
int CUT_FingerClient::GetConnectTimeout() const
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
int CUT_FingerClient::SetReceiveTimeout(int secs){

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
int CUT_FingerClient::GetReceiveTimeout() const
{
    return m_nReceiveTimeout;
}




/*********************************************
Finger
    Performs the finger for the specified address
    the information that is returned can be 
    retrieved with the GetReturnLine function.

    If filename parameter was specified all the 
    received data is saved in the file. In this 
    case you can't access data with GetReturnLine 
    function.

Params
    address     - address to perform the finger on. 
                  address format is: @domain.com or 
                  xxx@domain.com

    [filename]  -   filename where to save received data
    [fileType]  -   UTM_OM_WRITING  open writing
                    UTM_OM_APPEND   open for appending 

Return
    UTE_SUCCESS                 - success
    UTE_INVALID_ADDRESS_FORMAT  - invalid address format
    UTE_INVALID_ADDRESS         - invalid address
    UTE_CONNECT_FAILED          - connection failed
    UTE_NO_RESPONSE             - no response
    UTE_ABORTED                 - aborted
    UTE_CONNECT_TIMEOUT         - time out
    UTE_DS_OPEN_FAILED          - unable to open specified data source
    UTE_DS_WRITE_FAILED         - data source write error
**********************************************/
int CUT_FingerClient::Finger(LPTSTR address, LPTSTR filename, OpenMsgType fileType) {
	CUT_FileDataSource ds(filename);
    return CUT_FingerClient::Finger(address, ds, (filename == NULL) ? (OpenMsgType)-1 : fileType);
}
/*********************************************
Finger
    Performs the finger for the specified address
    the information that is returned can be 
    retrieved with the GetReturnLine function.

    If filename parameter was specified all the 
    received data is saved in the file. In this 
    case you can't access data with GetReturnLine 
    function.

Params
    address     - address to perform the finger on. 
                  address format is: @domain.com or 
                  xxx@domain.com

    dest        - data source for saving received data
    [fileType]  -   -1              - don't use
                    UTM_OM_WRITING  - open writing
                    UTM_OM_APPEND   - open for appending 


Return
    UTE_SUCCESS                 - success
    UTE_INVALID_ADDRESS_FORMAT  - invalid address format
    UTE_INVALID_ADDRESS         - invalid address
    UTE_CONNECT_FAILED          - connection failed
    UTE_NO_RESPONSE             - no response
    UTE_ABORTED                 - aborted
    UTE_CONNECT_TIMEOUT         - time out
**********************************************/
int CUT_FingerClient::Finger(LPTSTR address, CUT_DataSource & dest, OpenMsgType fileType) 
{
    
    int     error = UTE_SUCCESS;
    int     len;
    char    buf[MAX_PATH+1];
    char    domain[MAX_PATH+1];

    //delete prev finger information
    m_listReturnLines.ClearList();          

    //split the address apart
	// v4.2 address splitting into name and domain allows AC macro, hence LPTSTR in interface.
    if(SplitAddress(buf,domain,AC(address)) != UTE_SUCCESS)
        return OnError(UTE_INVALID_ADDRESS_FORMAT);

    //check to see if the domain is a name or address
    if(IsIPAddress(domain) != TRUE) {
        //get the name from the address
        if(GetAddressFromName(domain,domain,sizeof(domain)) != UTE_SUCCESS)
            return OnError(UTE_INVALID_ADDRESS);
        }

    //connect using a timeout
    if((error=Connect(m_nPort, domain, m_nConnectTimeout)) != UTE_SUCCESS)
        return OnError(error);

    if(IsAborted())                         // Test abort flag
        error = UTE_ABORTED;                // Aborted

    if(error == UTE_SUCCESS) {

        //send the name to finger
        Send(buf);
        Send("\r\n");

        // Read data into the file
        if(fileType != -1) {
            error = Receive(dest, fileType, m_nReceiveTimeout);
            }

        // Read data line by line & save it in the string list
        else {
            //read in the return lines

			// v4.2 change to eliminate C4127:conditional expression is constant
			for(;;) {
				if(IsAborted()) {                       // Test abort flag
                    error = UTE_ABORTED;                // Aborted
                    break;
                    }

                //wait for a receive
                if(WaitForReceive(m_nReceiveTimeout, 0)!= UTE_SUCCESS)
                    break;
        
                //get the information
                len = ReceiveLine(buf, sizeof(buf)-1);
                if(len <= 0)
                    break;
        
                buf[len]=0;
                CUT_StrMethods::RemoveCRLF(buf);

                //store the information
                m_listReturnLines.AddString(buf);
                }

            if (error == UTE_SUCCESS && m_listReturnLines.GetCount() == 0)
                error = UTE_NO_RESPONSE;                // No response
            }
        }

    // Close connection
    CloseConnection();

    return OnError(error);
}
/*********************************************
SplitAddress
    Internal function
    Splits the given address into its two parts,
    domain name and user name
Params
    name    - user name buffer
    domain  - domain name buffer
    address - address to split apart
Return
    UTE_SUCCESS - success
    UTE_ERROR   - error bad address format
**********************************************/
int CUT_FingerClient::SplitAddress(LPSTR name,LPSTR domain,LPCSTR address){

    int t;
    int len         = (int)strlen(address);
    int position    = -1;

    //search for the @ symbol
    for(t=0; t<len; t++) {
        if(address[t] == '@'){
            position = t;
            break;
        }
    }
    //if no @ was found or no domain then return an error
    if(position == -1 || position == (len-1))
        return OnError(UTE_ERROR);

    //copy the name
    strcpy(name, address);
    name[position] = 0;

    //copy the domain
    strcpy(domain, &address[position+1]);

    return OnError(UTE_SUCCESS);
}
/*********************************************
GetNumberReturnLines
    Returns the number of text lines that were
    returned during the last finger
Params
    none
Return
    Number of lines returned.
**********************************************/
int CUT_FingerClient::GetNumberReturnLines() const
{
    return m_listReturnLines.GetCount();
}
/*********************************************
GetReturnLine
    Returns the text of a line retrieved during
    the last finger operation
Params
    index - 0 based index of the line to return
Return
    constant pointer to the string at the given
    line
**********************************************/
LPCSTR CUT_FingerClient::GetReturnLine(int index) const
{
    return m_listReturnLines.GetString(index);
}

/*************************************************
GetReturnLine()
Gets the one line of a multiline response 
PARAM
line     - [out] pointer to buffer to receive response
maxSize  - length of buffer
index    - index response 
size     - [out] length of response

  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- name and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - name not found
  UTE_BUFFER_TOO_SHORT  - space in line buffer indicated by maxSize insufficient, realloc 
  based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
int	CUT_FingerClient::GetReturnLine(LPSTR line, size_t maxSize, int index, size_t *size) {
	
	int retval = UTE_SUCCESS;
	
	if(line == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetReturnLine(index);
		
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
				strcpy(line, str);
			}
		}
	}
	return retval;
}
#if defined _UNICODE
int	CUT_FingerClient::GetReturnLine(LPWSTR line, size_t maxSize, int index, size_t *size) {
	
	int retval;
	
	if(maxSize > 0) {
		char * lineA = new char [maxSize];
		
		if(lineA != NULL) {
			retval = GetReturnLine( lineA, maxSize, index, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(line, maxSize, lineA);
			}
			delete [] lineA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetReturnLine(index);
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
ReceiveFileStatus
    This virtual function is called during a 
    ReceiveToFile function.
Params
    bytesReceived - number of bytes received so far
Return
    TRUE - allow the receive to continue
    FALSE - abort the receive
****************************************************/
BOOL CUT_FingerClient::ReceiveFileStatus(long bytesReceived){
    if(IsAborted())     
        return FALSE;
    return CUT_WSClient::ReceiveFileStatus(bytesReceived);
}

#pragma warning ( pop )
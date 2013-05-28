//=================================================================
//  class: CUT_DNSClient
//  File:  DNS_c.cpp
//
//  Purpose:
//
//      Retrieves the Domain Name Service entries for a specific domain by
//      sending a TCP query to the NS servers on port 53
//
//  RFC 1035
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

#include "DNS_c.h"

#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

/*********************************************
Constructor
**********************************************/
CUT_DNSClient::CUT_DNSClient() :
        m_nPort(53),
        m_bIsAuth(FALSE),
        m_lpszDNSdata(NULL),
        m_nDNSdataLength(0),
        m_nDNSEntryCount(0),
        m_DNSEntries(NULL),
        m_nEnumIndex(0),
        m_nTimeOut(10),
        m_bIncludeDefaultMX(FALSE),
        m_nQueryLength(0),
        m_bUseUDP (FALSE)
{
    m_szRequestedDomainName[0]    = 0;
}
/*********************************************
Destructor
**********************************************/
CUT_DNSClient::~CUT_DNSClient(){

    //clean up any allocated data
    if(m_lpszDNSdata != NULL)
        delete[] m_lpszDNSdata;
    
    if(m_DNSEntries != NULL)
        delete[] m_DNSEntries;
}
/*********************************************
LookupName
    Performs a basic DNS lookup of the given
    name. The results are held internally and
    can be retrieved with the Enum... functions
Params
    nameServer  - the name of the server to send the
                    request to
    domain      - domain name to look up
Return
    UTE_SUCCESS                 - success
    UTE_PARAMETER_INVALID_VALUE - invalid parameter
    UTE_INVALID_ADDRESS_FORMAT  - invalid address format
    UTE_INVALID_ADDRESS         - invalid address
    UTE_CONNECT_FAILED          - connection failed
    UTE_NO_RESPONSE             - no response
    UTE_ABORTED                 - aborted
    UTE_CONNECT_TIMEOUT         - time out
**********************************************/
#if defined _UNICODE
int CUT_DNSClient::LookupName(LPCWSTR nameServer,LPCWSTR domain, int qtype){
	return LookupName( AC(nameServer), AC(domain), qtype);}
#endif
int CUT_DNSClient::LookupName(LPCSTR nameServer,LPCSTR domain, int qtype)
{
    int             error = UTE_SUCCESS, t,x;
    int             rt;
    int             len;
    char            buf[DNS_BUFFER_SIZE];
    // char            address[30];
    unsigned char   datalen[5];
        
    // Check input parameters
    if (domain == NULL || nameServer == NULL) 
        return OnError(UTE_PARAMETER_INVALID_VALUE);
    
    if((len = (int)strlen(domain)) < 1)
        return OnError(UTE_PARAMETER_INVALID_VALUE);
    
    if(strlen(nameServer) < 1)
        return OnError(UTE_PARAMETER_INVALID_VALUE);
    
    // Clear the authoritative flag
    m_bIsAuth = FALSE;
    
    //***** create the query *****
    BuildQuery(buf, domain,  qtype);
    m_nQueryLength = 18 + len;
    if (IsAborted())
	{
		return OnError(UTE_ABORTED);
	}
    // Connect - with timeout
    if (m_bUseUDP){
        if((error = Connect(53,nameServer,m_nTimeOut,AF_INET,SOCK_DGRAM)) != UTE_SUCCESS)
            return OnError(error);
    }
    else
    {
        if((error=Connect(m_nPort, nameServer, m_nTimeOut)) != UTE_SUCCESS)
            return OnError(error);

		  if (IsAborted())
		  {
			  return OnError(UTE_ABORTED);
		  }
		  
        
    }   
    
    // Send the query
    Send(buf, m_nQueryLength+2);
    
    if (m_bUseUDP) {
        if(m_lpszDNSdata != NULL)
            delete[] m_lpszDNSdata; 
        m_lpszDNSdata = new unsigned char[1024];
		memset( m_lpszDNSdata, 0, 1024 );
        m_nDNSdataLength = Receive((LPSTR)m_lpszDNSdata, 540,m_nTimeOut);
        }

    else{
      // Set the time-out
      SetReceiveTimeOut(m_nTimeOut * 1000);
    
        // Read in the first two bytes for the length
      if( Receive((LPSTR)datalen,2) <= 0) {
            CloseConnection();
            return OnError(UTE_CONNECT_FAILED);
            }

        len = (datalen[0]*DNS_BUFFER_SIZE) + datalen[1];
        
        // Alloc the buffer
        if(m_lpszDNSdata != NULL) 
            delete[] m_lpszDNSdata;
        
        m_lpszDNSdata = new unsigned char[len+10];
		memset( m_lpszDNSdata, 0, len+10 );
        m_nDNSdataLength = len+10;
        
        //read in the rest of the data
        t = 0;      //data position
        x = 0;      //receive length

		for (;;){
            x = len -t;
            if(x <=0)
                break;
			if (IsAborted())
			{
				CloseConnection();
				return OnError(UTE_ABORTED);
			}
            rt = Receive((LPSTR)m_lpszDNSdata+t, x);
            if(rt <= 0)
                break;
            t += rt;
        }
    }
    
    CloseConnection();
    
    //check to see if the answer is authoritative
    if( m_lpszDNSdata[2] & 4)
        m_bIsAuth = TRUE;
    
    strcpy(m_szRequestedDomainName, domain);
    if (m_nDNSdataLength > 0)
        ExtractDNSEntries();
    else
    {
        return OnError(UTE_NO_RESPONSE);
    }

    m_nEnumIndex = 0;
    
    return OnError(UTE_SUCCESS);
}

/*********************************************
AuthorizedLookup
    Performs an authorized DNS lookup of the given
    name. If the given name server is not
    authorized for the given domain, then this 
    routine will try and search for an authorized
    name server. The results are held internally and
    can be retrieved with the Enum... functions
Params
    nameServer  - the name of the server to send the
                    request to
    domain      - domain name to look up
Return
    UTE_SUCCESS                 - success
    UTE_PARAMETER_INVALID_VALUE - invalid parameter
    UTE_INVALID_ADDRESS_FORMAT  - invalid address format
    UTE_INVALID_ADDRESS         - invalid address
    UTE_CONNECT_FAILED          - connection failed
    UTE_NO_RESPONSE             - no response
    UTE_ABORTED                 - aborted
    UTE_CONNECT_TIMEOUT         - time out
**********************************************/
#if defined _UNICODE
int CUT_DNSClient::AuthoritativeLookup(LPCWSTR nameServer,LPCWSTR domain, int qtype ){
	return AuthoritativeLookup( AC(nameServer),AC(domain), qtype );}
#endif
int CUT_DNSClient::AuthoritativeLookup(LPCSTR nameServer,LPCSTR domain, int qtype )
{
    int     rt;
    int     lookUpCount = 1;
    char    name[DNS_BUFFER_SIZE];
    char    address[DNS_BUFFER_SIZE];


	if (IsAborted())
	{
		return OnError(UTE_ABORTED);
	}
  

    rt = LookupName(nameServer,domain); // the first one will be for all records 

    while(rt == UTE_SUCCESS && lookUpCount < 10 ) {

        if(IsAborted())
            return OnError(UTE_ABORTED);

        // If auth then exit
        if(m_bIsAuth)
            return OnError(UTE_SUCCESS);

        //else check the given name servers
        else {
            //find the first working name server
            ResetEnumerations();

            do {
                if((rt = EnumDNSServers(name,sizeof(name),address,sizeof(address))) != UTE_SUCCESS)
                   return OnError(rt);
				if (IsAborted())
				{
					CloseConnection();
					return OnError(UTE_ABORTED);
				}
                
                if(address[0] != 0) // now we ask for the question
                    rt = LookupName(address,domain, qtype);
                else
                    rt = LookupName(name,domain, qtype);

                lookUpCount++;
                } while(rt != UTE_SUCCESS && lookUpCount < 10 );
            }
        }

    return OnError(UTE_INVALID_ADDRESS);
}
/*********************************************
IsAuthoritative
    Returns if the last lookup was autoritative.
Params
    none
Return
    TRUE - autoritative
    FALSE - not autoritative
**********************************************/
int CUT_DNSClient::IsAuthoritative() const
{
    return m_bIsAuth;
}

/*********************************************
SetLookupTimeOut
    Sets the lookup time-out in secs.
Param
    secs - timeout value
Return
    UTE_SUCCESS - success
    UTE_ERROR   - failure
**********************************************/
int CUT_DNSClient::SetLookupTimeOut(int secs) {
    if(secs < 0)
        return UTE_ERROR;

    m_nTimeOut = secs;
    return UTE_SUCCESS;
}

/*********************************************
GetLookupTimeOut
    Sets the lookup time-out in secs.
Param
    none
Return
    int - timeout value
**********************************************/
int CUT_DNSClient::GetLookupTimeOut() const
{
    return m_nTimeOut;
}

/*********************************************
ResetEnumerations
    Resets the enumeration routines so that
    the next time one is called it will start
    from the beginning.
Params
    none
Return
    UTE_SUCCESS - success
    UTE_ERROR   - failure
**********************************************/
int CUT_DNSClient::ResetEnumerations(){
    m_nEnumIndex = 0;
    return UTE_SUCCESS;
}
/*********************************************
EnumDNSServers
    Enumerates the available name servers
    that were returned during the last lookup.
    If an address is available as well as the
    name then it is also returned
Params
    name        - buffer to store the name in
    maxNameLen  - length of the name buffer
    address     - buffer to store the address in
    maxAddrLen  - lenght of the address buffer
Return
    UTE_SUCCESS - success
    UTE_ERROR   - failure
**********************************************/
#if defined _UNICODE
int CUT_DNSClient::EnumDNSServers(LPWSTR name,int maxNameLen,LPWSTR address,int maxAddrLen){

	char * nameA = (char*) alloca(maxNameLen);
	char *addressA = NULL;

	*nameA = '\0';
	if(address != NULL) {
		addressA = (char*) alloca(maxAddrLen);
		*addressA = '\0';
	}

	int result = EnumDNSServers(nameA, maxNameLen, addressA, maxAddrLen);
	
	if(result == UTE_SUCCESS) {
		CUT_Str::cvtcpy(name, maxNameLen, nameA);
		if(addressA != NULL) {
			CUT_Str::cvtcpy(address, maxAddrLen, addressA);
		}
	}
	return result;
}
#endif
int CUT_DNSClient::EnumDNSServers(LPSTR name,int maxNameLen,LPSTR address,int maxAddrLen){
    return EnumDNSEntry(name, maxNameLen, address, maxAddrLen, CUT_DNS_NS);    
}
/*********************************************
EnumMXRecords
    Enumerates the available mail servers
    that were returned during the last lookup.
    If an address is available as well as the
    name then it is also returned
Params
    name - buffer to store the name in
    maxNameLen - length of the name buffer
    address - buffer to store the address in
    maxAddrLen - lenght of the address buffer
Return
    UTE_SUCCESS - success
    UTE_ERROR   - failure
**********************************************/
#if defined _UNICODE
int CUT_DNSClient::EnumMXRecords(LPWSTR name,int maxNameLen,LPWSTR address,int maxAddrLen){
	char * nameA = (char*) alloca(maxNameLen);
	char *addressA = NULL;

	*nameA = '\0';
	if(address != NULL) {
		addressA = (char*) alloca(maxAddrLen);
		*addressA = '\0';
	}

	int result = EnumMXRecords(nameA, maxNameLen, addressA, maxAddrLen);
	
	if(result == UTE_SUCCESS) {
		CUT_Str::cvtcpy(name, maxNameLen, nameA);
		if(addressA != NULL) {
			CUT_Str::cvtcpy(address, maxAddrLen, addressA);
		}
	}
	return result;
}
#endif
int CUT_DNSClient::EnumMXRecords(LPSTR name,int maxNameLen,LPSTR address,int maxAddrLen){
    return EnumDNSEntry(name, maxNameLen, address, maxAddrLen, CUT_DNS_MX);
}
/*********************************************
EnumMXRecords
    Enumerates the available DNS entries
    that were returned during the last lookup.
    If an address is available as well as the
    name then it is also returned
Params
    name        - buffer to store the name in
    maxNameLen  - length of the name buffer
    address     - buffer to store the address in
    maxAddrLen  - lenght of the address buffer
    type        - the type of DNS entry to return
                    (eg. CUT_DNS_A, CUT_DNS_NS, etc)
                    see the header file for a full list
Return
    UTE_SUCCESS - success
    UTE_ERROR   - failure
**********************************************/
#if defined _UNICODE
int CUT_DNSClient::EnumDNSEntry(LPWSTR name,int maxNameLen,LPWSTR address,int maxAddrLen,int type){
	char * nameA = (char*) alloca(maxNameLen);
	char *addressA = NULL;

	*nameA = '\0';
	if(address != NULL) {
		addressA = (char*) alloca(maxAddrLen);
		*addressA = '\0';
	}

	int result = EnumDNSEntry(nameA, maxNameLen, addressA, maxAddrLen, type);
	
	if(result == UTE_SUCCESS) {
		CUT_Str::cvtcpy(name, maxNameLen, nameA);
		if(addressA != NULL) {
			CUT_Str::cvtcpy(address, maxAddrLen, addressA);
		}
	}
	return result;
}
#endif
int CUT_DNSClient::EnumDNSEntry(LPSTR name,int maxNameLen,LPSTR address,int maxAddrLen,int type){
    
    int loop;

    while(m_nEnumIndex < m_nDNSEntryCount) {

        //check to see if a NS enrty is found
        if( m_DNSEntries[m_nEnumIndex].nType == type) {
      
            //if an entry is found then return the name
            if(maxNameLen > 0)
                strncpy(name,m_DNSEntries[m_nEnumIndex].szData,maxNameLen);
            
            //get the address for the name
            if(maxAddrLen >0) {
                for(loop=0;loop< m_nDNSEntryCount;loop++) {
                    if(m_DNSEntries[loop].nType == CUT_DNS_A) {
                        if(strcmp(name,m_DNSEntries[loop].szHost)== 0) {
                            strncpy(address,m_DNSEntries[loop].szData,maxAddrLen);
                            break;
                            }
                        }
                    }           

                if( loop == m_nDNSEntryCount )
                    address[0] = 0;
                }

            m_nEnumIndex++;
            return OnError(UTE_SUCCESS);
            }
        //if an entry is not found then continue searching
        m_nEnumIndex++;
        }

    return OnError(UTE_ERROR);
}
/*********************************************
EnumDNSEntry
    Enumerates the available DNS entries
    that were returned during the last lookup.
    If an address is available as well as the
    name then it is also returned
Params
    entry - returns a structure containing all
        the information for an entry.
        CUT_DNSEntry - see the header file
        for a list of params
Return
    UTE_SUCCESS - success
    UTE_ERROR   - failure
**********************************************/
int CUT_DNSClient::EnumDNSEntry(CUT_DNSEntry * entry){
    
    if(m_nEnumIndex >= m_nDNSEntryCount)
        return OnError(UTE_ERROR);

	// v4.2 These cvtcpy's allow us to return _TCHAR entries.
	CUT_Str::cvtcpy(entry->szHost,sizeof(entry->szHost)/sizeof(TCHAR),m_DNSEntries[m_nEnumIndex].szHost);
    CUT_Str::cvtcpy(entry->szData,sizeof(entry->szData)/sizeof(TCHAR),m_DNSEntries[m_nEnumIndex].szData);
    CUT_Str::cvtcpy(entry->szData2,sizeof(entry->szData2)/sizeof(TCHAR),m_DNSEntries[m_nEnumIndex].szData2);

    entry->nType            = m_DNSEntries[m_nEnumIndex].nType;
    entry->lTTL             = m_DNSEntries[m_nEnumIndex].lTTL;
    entry->nMX_Preference   = m_DNSEntries[m_nEnumIndex].nMX_Preference;
    entry->lSOA_Serial      = m_DNSEntries[m_nEnumIndex].lSOA_Serial;
    entry->lSOA_Refresh     = m_DNSEntries[m_nEnumIndex].lSOA_Refresh;
    entry->lSOA_Retry       = m_DNSEntries[m_nEnumIndex].lSOA_Retry;
    entry->lSOA_Expire      = m_DNSEntries[m_nEnumIndex].lSOA_Expire;
    entry->lSOA_Minimum     = m_DNSEntries[m_nEnumIndex].lSOA_Minimum;

    m_nEnumIndex++;

    return OnError(UTE_SUCCESS);
}
/*********************************************
GetDNSEntryCount
    Returns the number of DNS entries found
    during the last lookup
Params
    none
Return
    number of entries
**********************************************/
int CUT_DNSClient::GetDNSEntryCount() const
{
    
    int count = 0;

    if(m_lpszDNSdata == NULL)
        return 0;

    count =     m_lpszDNSdata[6]*DNS_BUFFER_SIZE + m_lpszDNSdata[7]+
                m_lpszDNSdata[8]*DNS_BUFFER_SIZE + m_lpszDNSdata[9] +
                m_lpszDNSdata[10]*DNS_BUFFER_SIZE+ m_lpszDNSdata[11];

    if (m_bIncludeDefaultMX)
        count++;

    return count;
}
/*********************************************
ExtractDNSEntries
    Internal function
    This function extracts the DNS entries 
    from the raw information that was received
    from the DNS server
**********************************************/
int CUT_DNSClient::ExtractDNSEntries()
{
   
    in_addr     addr;
    int         type;
    int         currentpos;
    int         nextrecord = m_nQueryLength;
    int         endpos;
    int         resultcount = GetDNSEntryCount();
    int         rdatalength;
    int         loop;

    m_nDNSEntryCount = GetDNSEntryCount();

    //delete any existing m_DNSEntries data
    if(m_DNSEntries != NULL)
        delete[] m_DNSEntries;
    m_DNSEntries = new CUT_DNSEntryA[ resultcount ];
	memset( m_DNSEntries, 0, resultcount );

    // if we're inserting the default MX record
    // don't attempt to retrieve it from the server
    // supplied DNS record.
    if (m_bIncludeDefaultMX)
        resultcount--;

    //main loop
    for(loop = 0; loop < resultcount; loop++) {
        m_DNSEntries[loop].lSOA_Serial  = 0;
        m_DNSEntries[loop].lSOA_Refresh = 0;
        m_DNSEntries[loop].lSOA_Retry   = 0;
        m_DNSEntries[loop].lSOA_Expire  = 0;
        m_DNSEntries[loop].lSOA_Minimum = 0;
        
        //get the position for the next record
        currentpos = nextrecord;

        //exand the host name
        m_DNSEntries[loop].szHost[0] = 0;
        ExpandName(currentpos,&endpos,m_DNSEntries[loop].szHost,sizeof(m_DNSEntries[loop].szHost));
        currentpos = endpos;

        //get the record type
        m_DNSEntries[loop].nType = m_lpszDNSdata[currentpos]*DNS_BUFFER_SIZE + m_lpszDNSdata[currentpos+1];
        currentpos+=2;

        //class is the next two bytes
        currentpos+=2;

        //Time to live
        m_DNSEntries[loop].lTTL = MakeLong(m_lpszDNSdata[currentpos],m_lpszDNSdata[currentpos+1],
                                        m_lpszDNSdata[currentpos+2],m_lpszDNSdata[currentpos+3]);
        currentpos+=4;

        //get rdata length
        rdatalength = m_lpszDNSdata[currentpos]*DNS_BUFFER_SIZE + m_lpszDNSdata[currentpos+1];
        currentpos+=2;

        //store the position for the next record
        nextrecord = currentpos+rdatalength;

        //get the rdata
        m_DNSEntries[loop].szData[0]    = 0;
        m_DNSEntries[loop].szData2[0]   = 0;
        type = m_DNSEntries[loop].nType;

        if( type == CUT_DNS_MX) {
            m_DNSEntries[loop].nMX_Preference = m_lpszDNSdata[currentpos]*DNS_BUFFER_SIZE + m_lpszDNSdata[currentpos+1];
            currentpos+=2;
            ExpandName(currentpos,&endpos,m_DNSEntries[loop].szData,sizeof(m_DNSEntries[loop].szData));
            }
        else if(type == CUT_DNS_HINFO) {
            GetBString(currentpos,&endpos,m_DNSEntries[loop].szData,sizeof(m_DNSEntries[loop].szData));
            currentpos=endpos;
            GetBString(currentpos,&endpos,m_DNSEntries[loop].szData,sizeof(m_DNSEntries[loop].szData));
            m_DNSEntries[loop].nMX_Preference = 0;
            }
        else if(type == CUT_DNS_MINFO) {
            GetBString(currentpos,&endpos,m_DNSEntries[loop].szData,sizeof(m_DNSEntries[loop].szData));
            currentpos=endpos;
            GetBString(currentpos,&endpos,m_DNSEntries[loop].szData,sizeof(m_DNSEntries[loop].szData));
            m_DNSEntries[loop].nMX_Preference = 0;
            }
        else if(type == CUT_DNS_SOA) {
            ExpandName(currentpos,&endpos,m_DNSEntries[loop].szData,sizeof(m_DNSEntries[loop].szData));
            currentpos=endpos;
            ExpandName(currentpos,&endpos,m_DNSEntries[loop].szData2,sizeof(m_DNSEntries[loop].szData2));
            currentpos=endpos;
            m_DNSEntries[loop].nMX_Preference = 0;
            m_DNSEntries[loop].lSOA_Serial   = MakeLong(m_lpszDNSdata[currentpos],m_lpszDNSdata[currentpos+1],
                                                        m_lpszDNSdata[currentpos+2],m_lpszDNSdata[currentpos+3]);
            currentpos+=4;
            m_DNSEntries[loop].lSOA_Refresh  = MakeLong(m_lpszDNSdata[currentpos],m_lpszDNSdata[currentpos+1],
                                                        m_lpszDNSdata[currentpos+2],m_lpszDNSdata[currentpos+3]);
            currentpos+=4;
            m_DNSEntries[loop].lSOA_Retry    = MakeLong(m_lpszDNSdata[currentpos],m_lpszDNSdata[currentpos+1],
                                                        m_lpszDNSdata[currentpos+2],m_lpszDNSdata[currentpos+3]);
            currentpos+=4;
            m_DNSEntries[loop].lSOA_Expire   = MakeLong(m_lpszDNSdata[currentpos],m_lpszDNSdata[currentpos+1],
                                                        m_lpszDNSdata[currentpos+2],m_lpszDNSdata[currentpos+3]);
            currentpos+=4;
            m_DNSEntries[loop].lSOA_Minimum  = MakeLong(m_lpszDNSdata[currentpos],m_lpszDNSdata[currentpos+1],
                                                        m_lpszDNSdata[currentpos+2],m_lpszDNSdata[currentpos+3]);
            }
        else if(m_DNSEntries[loop].nType == CUT_DNS_A) {
            addr.S_un.S_un_b.s_b1 = m_lpszDNSdata[currentpos];
            addr.S_un.S_un_b.s_b2 = m_lpszDNSdata[currentpos+1];
            addr.S_un.S_un_b.s_b3 = m_lpszDNSdata[currentpos+2];
            addr.S_un.S_un_b.s_b4 = m_lpszDNSdata[currentpos+3];
            strcpy(m_DNSEntries[loop].szData,inet_ntoa (addr));
            m_DNSEntries[loop].nMX_Preference = 0;
            }
        else {
            ExpandName(currentpos,&endpos,m_DNSEntries[loop].szData,sizeof(m_DNSEntries[loop].szData));
            m_DNSEntries[loop].nMX_Preference = 0;
            }
        }

    if (m_bIncludeDefaultMX) {
        strcpy(m_DNSEntries[resultcount].szHost, m_szRequestedDomainName);
        m_DNSEntries[resultcount].nMX_Preference = 65535;
        strcpy(m_DNSEntries[resultcount].szData, m_szRequestedDomainName);
        m_DNSEntries[resultcount].szData2[0]    = 0;
        m_DNSEntries[resultcount].nType         = CUT_DNS_MX;
        m_DNSEntries[resultcount].lTTL          = 60;  // short cache time so record isn't stored anywhere
        }

    return UTE_SUCCESS;
}
/*********************************************
ExpandName
    Internal function
    This function expands a name from the 
    compressed format that the DNS server
    returns it in
Param
    int startPos - starting position in the buffer
    int *endPos  - the 
    LPSTR buf    - the string bufffer to hold the result
    int maxLen   - maximum length of the buffer
**********************************************/
int CUT_DNSClient::ExpandName(int startPos,int *endPos,LPSTR buf,int maxLen){

    int     t;
    int     len;
    int     bufPos      = 0;
    int     currentPos  = startPos;
    int     refFlag     = FALSE;
	int		nCount		= m_nDNSdataLength;
	*endPos = 0;

    while(currentPos < m_nDNSdataLength && nCount > 0) {
		
		nCount --;

        if(m_lpszDNSdata[currentPos] & 192) {       //reference
            if(!refFlag)
                *endPos = currentPos+2;
            currentPos = ((UCHAR)m_lpszDNSdata[currentPos]&63)*DNS_BUFFER_SIZE +(UCHAR)m_lpszDNSdata[currentPos+1];
            refFlag = TRUE;
        }

        else if(m_lpszDNSdata[currentPos] == 0) {
            if(!refFlag)
                *endPos = currentPos+1;
            buf[bufPos]=0;
            break;
        }
        else {                                      //label
            if(bufPos >0) {
                buf[bufPos]='.';
                bufPos++;
            }

            len = (unsigned char)m_lpszDNSdata[currentPos];
            for(t=0;t<len;t++) {
                buf[bufPos] = m_lpszDNSdata[currentPos+t+1];
                bufPos++;
            }
            currentPos +=(len+1);
        }

        if(bufPos >= maxLen){
            bufPos = maxLen;
            return FALSE;
        }
    }

    return TRUE;
}
/*********************************************
GetBString
    Internal function   
**********************************************/
int CUT_DNSClient::GetBString(int currentPos,int *endPos,LPSTR buf,int maxLen) const
{
    int     t;
    int     len = (unsigned char)m_lpszDNSdata[currentPos];

    if(len >= maxLen)
        len = maxLen-1;

    for(t=0;t<len;t++) {
        currentPos++;
        buf[t] = m_lpszDNSdata[currentPos];
    }

    buf[t] = 0;
    *endPos = currentPos+1;
    return 1;
}
/*********************************************
GetShortName
    Internal function
**********************************************/
LPCTSTR CUT_DNSClient::GetShortName(int index) const
{
    static LPCTSTR DNSNames_Short[] = 
    {
        _T(""),_T("A"),_T("NS"),_T("MD"),_T("MF"),_T("CName"),_T("SOA"),_T("MB"),_T("MG"),_T("MR"),
			_T("NULL"),_T("WKS"),_T("PTR"),_T("HINFO"),_T("MINFO"),_T("MX"),_T("TXT")
    };
    
    if(index < 0 || index > 16)
        return _T("");
    else
        return DNSNames_Short[index];
}
/*********************************************
GetLongName
    Internal function
**********************************************/
LPCTSTR CUT_DNSClient::GetLongName(int index) const
{
    static LPCTSTR DNSNames_Long[] =
    {
        _T(""),_T("Host Address"),_T("Authoritative Name Server"),_T("Mail Destination"),
            _T("Mail Forwarder"),_T("Canonical Name"),_T("Start Of Zone Of Authority"),
            _T("Mailbox Domain"),_T("Mail Group Member"),_T("Mail Rename Domain"),_T("NULL"),
            _T("WKS"),_T("Domain Name Pointer"),_T("Host Information"),
            _T("Mail List Information"),_T("Mail Exchange"),_T("Text Strings")
    };
    
    if(index < 0 || index > 16)
        return _T("");
    else
        return DNSNames_Long[index];
}
/*********************************************
MakeLong    
      Internal function
**********************************************/
long CUT_DNSClient::MakeLong(BYTE high,BYTE lower, BYTE evenLower, BYTE lowest){

    long l = ((long)(high) << 24) + ((long)(lower) << 16) +
                ((long)(evenLower) << 8) + ((long)(lowest));
    return l;
}
/*********************************************
    IncludeDefaultMX

    When set to true, the system will return
    the requested domain name as a valid MX record
    as if it were added by the DNS system. 

    Some improperly configured domains do not 
    include MX records, and in that case we
    have found that trying the requested domain
    will often allow the mail to be sent

**********************************************/
void CUT_DNSClient::IncludeDefaultMX(BOOL bInclude){
    m_bIncludeDefaultMX = bInclude;
}

/*********************************************
    Returns IncludeDefaultMX
**********************************************/
BOOL CUT_DNSClient::GetIncludeDefaultMX() const
{
    return m_bIncludeDefaultMX;
}

/*********************************************
SetPort
    Sets the port number to connect to
Params
    newPort     - connect port
Return
    UTE_SUCCESS - success
**********************************************/
int CUT_DNSClient::SetPort(unsigned int newPort) {
    m_nPort = newPort;
    return OnError(UTE_SUCCESS);
}
/*********************************************
GetPort
    retrieves the port number to connect to
Params
    none
Return
    connect port
**********************************************/
unsigned int  CUT_DNSClient::GetPort() const
{
    return m_nPort;
}
/**********************************************************
// Set the socket type to be used to UDP = TRUE , TCP = FALSE 
**********************************************************/
void CUT_DNSClient::SetUseUDP(BOOL flag){
    m_bUseUDP = flag;
}
/**********************************************************
// Is the socket using UDP or a TCP query
Param
    NONE
Return
    TRUE - UDP is used
    FALSE- TCP is used
**********************************************************/
BOOL CUT_DNSClient::GetUseUDP() const
{
    return m_bUseUDP;
}
/***********************************************************
Build a question to be sent to the server
the question will be based on the socket choice either udp or TCP
and the question type
RET:
    VOID
PARAM:
    IN, OUT char *lpszQuery - buffer to hold the query
    IN LPCSTR domain - the domain name for the question
    IN int QueryType - question type
************************************************************/
void CUT_DNSClient::BuildQuery(char *lpszQuery, LPCSTR domain,  int QueryType){
    
    char             t,x,y;

	// v4.2 using size_t
	size_t           len;
    
    len = strlen(domain);
    
    if (m_bUseUDP){ 

        lpszQuery[0]=6;
        lpszQuery[1]=57;   // two bytes specifying a unique id
        
        lpszQuery[2]=1;   //
        lpszQuery[3]=0;   // operation flags (QR, opcode, AA, TC, RD, RA, Z, Rcode)
        
        lpszQuery[4]=0;
        lpszQuery[5]=1;   // two bytes # question entries
        
        lpszQuery[6]=0;
        lpszQuery[7]=0; // two bytes # answer entries
        
        lpszQuery[8]=0;
        lpszQuery[9]=0; // two bytes # ns entries
        
        lpszQuery[10]=0;
        lpszQuery[11]=0;    // two bytes # additional entries
        
        y=0;        //input string pos
        x = 12;     //length position
        for(t=13;t < (int)(len+13);t++){
            
            if(domain[y]!='.')
                lpszQuery[t] = domain[y];
            else{
                lpszQuery[x] = (char)(t - x -1);
                x = t;
            }
            y++;
        }
        lpszQuery[x] = (char)(t-x-1);
        lpszQuery[t]=0;
        
        t++;
        lpszQuery[t]=0;
        lpszQuery[t+1]=(char)QueryType; //qtype     question type
        
        lpszQuery[t+2]=0;
        lpszQuery[t+3]=1;           //qclass    internet class -- END OF Question Entry --
    }
    else
    {
        lpszQuery[0]    = 0;
        lpszQuery[1]    = (unsigned char)(18 + len);    // two bytes specifying message length

        lpszQuery[2]    = 6;
        lpszQuery[3]    = 57;           // two bytes specifying a unique id
        
        lpszQuery[4]    = 1;   
        lpszQuery[5]    = 0;            // operation flags (QR, opcode, AA, TC, RD, RA, Z, Rcode)
        
        lpszQuery[6]    = 0;
        lpszQuery[7]    = 1;            // two bytes # question entries
        
        lpszQuery[8]    = 0;
        lpszQuery[9]    = 0;            // two bytes # answer entries
        
        lpszQuery[10]   = 0;
        lpszQuery[11]   = 0;            // two bytes # ns entries
        
        lpszQuery[12]   = 0;
        lpszQuery[13]   = 0;            // two bytes # additional entries
        
        y   = 0;                //input string pos
        x   = 14;               //length position
        
        for(t = 15; t < (int)(len+15); t++) {
            if(domain[y]!='.')
                lpszQuery[t] = domain[y];
            else {
                lpszQuery[x] = (char)(t - x -1);
                x = t;
            }
            y++;
        }
        
        lpszQuery[x]    = (char)(t-x-1);
        lpszQuery[t]    = 0;
        
        t++;
        lpszQuery[t]    = 0;
        lpszQuery[t+1]= (char)QueryType;    //qtype     question type
        
        lpszQuery[t+2]= 0;
        lpszQuery[t+3]= 1;          //qclass    internet class -- END OF Question Entry --
        
    }
}

#pragma warning( pop )
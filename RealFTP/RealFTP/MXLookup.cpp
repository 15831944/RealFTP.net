// =================================================================
//  class: CUT_MXLookup
//  File:  mxlookup.cpp
//  
//  Purpose:
//
//      Performs DNS lookups specifically on MX records
//      for a given email address 
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

#include "stdafx.h"
#include "dns_c.h"
#include "MXLookup.h"
#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

/*********************************************
CUT_MXLookup
    Constructor
Params
    None
Return
    None
**********************************************/
CUT_MXLookup::CUT_MXLookup()
{

    m_pMXListStart  = NULL;
    m_pMXListPos    = NULL;
    m_nRecordCount  = 0;

    m_nTCPRetryCount    = 1;
    m_nUDPRetryCount    = 3;

    m_nMaxRecurseCount = 3;

    m_nIncludeDefaultRecord = FALSE;
}

/*********************************************
~CUT_MXLookup
    Destructor
Params
    None
Return
    None
**********************************************/
CUT_MXLookup::~CUT_MXLookup()
{
    ClearList();
}

/*********************************************
ClearList
    Clears all MX records found during the last
    lookup operation (LookupMX)
Params
    None
Return
    None
**********************************************/
void CUT_MXLookup::ClearList(){

    while(m_pMXListStart != NULL){

        m_pMXListPos = m_pMXListStart->m_pNext;

        if(m_pMXListStart->m_szName != NULL)
            delete[] m_pMXListStart->m_szName;
        if(m_pMXListStart->m_szAddress != NULL)
            delete[] m_pMXListStart->m_szAddress;
        delete m_pMXListStart;

        m_pMXListStart = m_pMXListPos;
    }

    m_pMXListStart  = NULL;
    m_pMXListPos    = NULL;
    m_nRecordCount  = 0;

    //clear the list of name servers searched upon
    m_usedNameServerList.ClearList();
}

/*********************************************
AddItem
    Adds a entry to the MXRecord List
Params
    szName - domain name of MX record
    szAddress - IP address of MX record
    nPreference - server usage preference
Return
    TRUE - success
    FALSE - fail
**********************************************/
BOOL CUT_MXLookup::AddItem(LPSTR szName,LPSTR szAddress,long nPreference){
	// v4.2 changed to size_t
    size_t len;
    
    UT_MXLIST * pTempItem = new UT_MXLIST;

    //copy the info
    len = strlen(szName);
    pTempItem->m_szName = new char[len+1];
    strcpy(pTempItem->m_szName,szName);

    len = strlen(szAddress);
    pTempItem->m_szAddress = new char[len+1];
    strcpy(pTempItem->m_szAddress,szAddress);

    pTempItem->m_nPreference = nPreference;
    
    pTempItem->m_pNext = NULL;

    //insert the record into the list, in order of preference
    if(m_pMXListStart == NULL){
        m_pMXListStart = pTempItem;
        m_pMXListPos = pTempItem;
    }
    else{
        UT_MXLIST * pEnum = m_pMXListStart;
        UT_MXLIST * pPrev = NULL;
        while(pEnum != NULL){
            if(pEnum->m_nPreference > nPreference){
                //insert record
                pTempItem->m_pNext = pEnum;
                if(pEnum == m_pMXListStart){
                    m_pMXListStart = pTempItem;
                    m_pMXListPos = pTempItem;
                }
                else{
                    pPrev->m_pNext = pTempItem;
                }
                break;
            }
            if(pEnum->m_pNext == NULL){
                pEnum->m_pNext = pTempItem;
                break;
            }
            //move to the next item
            pPrev = pEnum;
            pEnum = pEnum->m_pNext;
        }
    }

    m_nRecordCount ++;

    return TRUE;
}

/*********************************************
GetMXRecordCount
    Returns the current number of MX records in
    the MXRecord list
Params
    None
Return
    Number of records
**********************************************/
long CUT_MXLookup::GetMXRecordCount(){
    return m_nRecordCount;
}

/*********************************************
GetMXRecord
    Performs an enumeration of the MX records in
    the MXRecord list. This function returns consecutive
    records each time the function is called or until 
    ResetEnum() is called.
Params
    szName - MX record domain name
    nNameLen - max length of the szName string
    szAddress - MX record IP address
    nAddressLen - msz length of the szAddress string
Return
    UTE_SUCCESS - success
    UTE_ERROR - fail (no more records to retrieve)
**********************************************/
#if defined _UNICODE
int CUT_MXLookup::GetMXRecord(LPWSTR szName,int nNameLen,LPWSTR szAddress,int nAddressLen,long* nPref){
	char * szNameA = (char*) alloca(nNameLen);
	char * szAddressA = (char*) alloca(nAddressLen);
	*szNameA = '\0';
	*szAddressA = '\0';
	int result = GetMXRecord(szNameA, nNameLen, szAddressA, nAddressLen, nPref);
	if(result == UTE_SUCCESS) {
		CUT_Str::cvtcpy(szName, nNameLen, szNameA);
		CUT_Str::cvtcpy(szAddress, nAddressLen, szAddressA);
	}
	return result;}
#endif
int CUT_MXLookup::GetMXRecord(LPSTR szName,int nNameLen,LPSTR szAddress,int nAddressLen,long* nPref){
    
    if(m_pMXListPos == NULL){
        return OnError(UTE_ERROR);
    }

    strncpy(szName,m_pMXListPos->m_szName,nNameLen);
    *nPref = m_pMXListPos->m_nPreference;

    if(szAddress != NULL && nAddressLen >0){
        //if the address field is empty do a lookup
        if(m_pMXListPos->m_szAddress[0] == 0){

            if(GetAddressFromName(szName,szAddress,nAddressLen) != UTE_SUCCESS){
                szAddress[0] = 0;
            }

            delete[] m_pMXListPos->m_szAddress;
			// v4.2 changed to size_t
			size_t len = strlen(szAddress);
            m_pMXListPos->m_szAddress = new char[len+1];
            strcpy(m_pMXListPos->m_szAddress,szAddress);
        }
        else{
            strncpy(szAddress,m_pMXListPos->m_szAddress,nAddressLen);
        }
    }

    m_pMXListPos = m_pMXListPos->m_pNext;
    return OnError(UTE_SUCCESS);
}

/*********************************************
GetMXRecord
    Performs an enumeration of the MX records in
    the MXRecord list. This function returns consecutive
    records each time the function is called or until 
    ResetEnum() is called.
Params
    None
Return
    A pointer to a string containing the MX record IP address
    if successful, otherwise NULL is returned.
Note: Overloads exist taking LPSTR and LPWSTR to allow for _UNICODE builds
**********************************************/
LPCSTR CUT_MXLookup::GetMXRecord(){

    if(m_pMXListPos == NULL)
        return NULL;

    //if the address field is empty do a lookup on the name to get the address
    if(m_pMXListPos->m_szAddress[0] == 0){

        char szBuf[256];
        if( GetAddressFromName(m_pMXListPos->m_szName,szBuf,sizeof(szBuf)) != UTE_SUCCESS){
            szBuf[0] = 0;
        }

        delete[] m_pMXListPos->m_szAddress;
		// v4.2 changed to size_t
		size_t nLen = strlen(szBuf);
        m_pMXListPos->m_szAddress = new char[nLen+1];
        strcpy(m_pMXListPos->m_szAddress,szBuf);
    }

    LPSTR pBuf = m_pMXListPos->m_szAddress;

    m_pMXListPos = m_pMXListPos->m_pNext;

    return pBuf;
}
/*************************************************
GetMXRecord 
These overloads allow for _UNICODE compilation.

PARAM
	record - [out] string to contain the record found
	maxSize - size (in _TCHARs) of record
	size	- [out] pointer to size_t to indicate required size
RETURN
	UTE_SUCCESS
	UTE_NULL_PARAM			- record and/or size a null pointer
	UTE_INDEX_OUTOFRANGE	- no more MX records to retrieve (see docs above)
	UTE_BUFFER_TOO_SHORT	- record too short to contain MX entry - examine size value
	UTE_OUT_OF_MEMORY		- possible in wide char overload
/*************************************************/
int CUT_MXLookup::GetMXRecord(LPSTR record, size_t maxSize, size_t *size){
	
	int retval = UTE_SUCCESS;
	
	if(record == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {

		*record = 0;
		
		LPCSTR str = GetMXRecord();
		
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
				strcpy(record, str);
			}
		}
	}
	return retval;
}
#if defined _UNICODE
int CUT_MXLookup::GetMXRecord(LPWSTR record, size_t maxSize, size_t *size){
	
	int retval;
	
	if(maxSize > 0) {
		char * recordA = new char [maxSize];
		
		if(recordA != NULL) {
			retval = GetMXRecord( recordA, maxSize, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(record, maxSize, recordA);
			}
			
			delete [] recordA;
			
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (
			retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetMXRecord();
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
ResetEnum
    Resets the enumeration for the GetMXRecord()
    functions.
Params
    None
Return
    UTE_SUCCESS - success
    UTE_ERROR - fail
**********************************************/
int CUT_MXLookup::ResetEnum(){
    m_pMXListPos = m_pMXListStart;
    return OnError(UTE_SUCCESS);
}

/*********************************************
IncludeDefaultRecord
    Enables or disables a default MX record entry,
    which is just the domain name found in the email
    address given in the LookupMX() function.
Params
    bTrueFalse - if TRUE then add the default record
        if FALSE then do not add the record (default).
Return
    UTE_SUCCESS - success
    UTE_ERROR - fail
**********************************************/
int CUT_MXLookup::IncludeDefaultRecord(BOOL bTrueFalse){
    m_nIncludeDefaultRecord = bTrueFalse;
    return OnError(UTE_SUCCESS);
}

/*********************************************
SetRetryCounts
    Sets the number of times the LookupMX() function
    will retry a query on a DNS server. The LookupMX()
    function will try TCP/IP first then if that fails
    it will switch to UDP. Default retries are 1 for TCP 
    and 3 for UDP. Setting the value to 0 will disable the
    lookup using the given method.
Params
    nTCPCount - number of times to retry DNS lookup using TCP/IP
    nUDPCount - number of times to retry DNS lookup using UDP
Return
    UTE_SUCCESS - success
    UTE_ERROR - fail
**********************************************/
int CUT_MXLookup::SetRetryCounts(int nTCPCount,int nUDPCount){
    
    if(nTCPCount >= 0 && nUDPCount >= 0){
        m_nTCPRetryCount = nTCPCount;
        m_nUDPRetryCount = nUDPCount;
        return OnError(UTE_SUCCESS);
    }

    return OnError(UTE_ERROR);
}
/*********************************************
SetMaxRecurseLevel
    Sets the maximum level of sub searches to find
    MX records for a given email address. If a DNS 
    server is set up the deepest level should be 1,
    but incorrect DNS server settings can cause searches 
    to go many levels deep, or even create an infinate 
    search loop. Default is 3 levels of sub searches.
    This value will not modify the search time for 
    properly formated DNS information.
Params
    nLevel - level of sub searches to perform.
Return
    UTE_SUCCESS - success
    UTE_ERROR - fail
**********************************************/
int CUT_MXLookup::SetMaxRecurseLevel(int nLevel){
    if(nLevel > 0){
        m_nMaxRecurseCount = nLevel;
        return OnError(UTE_SUCCESS);
    }

    return OnError(UTE_ERROR);
}
/*********************************************
LookupMX
    Performs the actual MX lookup for the given
    email address. All results are put in a 
    MXRecord list for later retrieval.
Params
    szEmailAddress - email address to lookup for.
    szNameServer - default name server to start the
        search from. If the given name server fails
        then the system will start searching using 
        the name servers added using AddNameServer().
        If this field is NULL then the searches starts
        directly from the name servers added using
        AddNameServer().
Return
    UTE_SUCCESS - success
    UTE_ERROR - fail
**********************************************/
#if defined _UNICODE
int CUT_MXLookup::LookupMX(LPCWSTR szNameServer,LPCWSTR szEmailAddress){
	return LookupMX(AC(szNameServer), AC(szEmailAddress));}
#endif
int CUT_MXLookup::LookupMX(LPCSTR szNameServer,LPCSTR szEmailAddress){

    char szDomain[256];
    int rt = UTE_ERROR;

    //parse email address
    GetDomainFromEmailAddress(szEmailAddress,szDomain,sizeof(szDomain));

    //clear any prev entries
    ClearList();

    m_nRecurseCount = 0;

    //if a name server was given then lookup using it
    if(szNameServer != NULL){
        rt = LookupMXInt(szDomain,szNameServer);
    }

    //if the first lookup was not successful then try the optional name servers if any
    if(rt != UTE_SUCCESS && m_nameServerList.GetCount() > 0){
        LPCSTR pBuf;
        int index = 0;
        while(rt != UTE_SUCCESS){
            pBuf = m_nameServerList.GetString(index);
            if(pBuf == NULL)
                break;
            index++;

           rt = LookupMXInt(szDomain,pBuf);
        }
    }
    
    //if the default record is to be added then add it
    if(m_nIncludeDefaultRecord){
        AddItem(szDomain,"",65535);
    }

    return rt;
}

/*********************************************
LookupMXInt - protected function
    This function is called from LookupMX to 
    perform the actual communication with the
    DNS servers.
Params
    szEmailAddress - email address to lookup for.
    szNameServer - default name server to start the
        search from.
Return
    UTE_SUCCESS - success
    UTE_ERROR - fail
**********************************************/
int CUT_MXLookup::LookupMXInt(LPCSTR szDomain,LPCSTR szNameServer){
    
    int nMXCount;
    char szBuf[256];
    char szBuf2[256];
    LPCSTR pBuf;
    BOOL bAuthoritative = FALSE;
    
    CUT_StringList  nameServerList;
    int             nameServerListIndex;
    
    SetUseUDP(FALSE);

    int maxRetries = m_nTCPRetryCount + m_nUDPRetryCount;

    //Main MX Lookup Loop, keeps trying to find MX records to to maxReties
    //If MX records are found then the loop is exited
    for(int retry = 0;retry < maxRetries;retry++){

        //after trying TCP try UDP
        if(retry >= m_nTCPRetryCount)
            SetUseUDP(TRUE);

        //perform a DNS lookup operation
        if(LookupName(szNameServer,szDomain) == UTE_SUCCESS){

            //store the Name servers found
            while(EnumDNSServers(szBuf,sizeof(szBuf),szBuf2,sizeof(szBuf2)) == UTE_SUCCESS){

                if(szBuf2[0] != 0)
                    pBuf = szBuf2;
                else
                    pBuf = szBuf;
                //add the name if it was not already used
                if(m_usedNameServerList.Exists(pBuf) == FALSE)
                    nameServerList.AddString(pBuf);
            }
            nameServerListIndex = 0;
            ResetEnumerations();
            
            //check to see if the lookup was authoritative
            //if it is not marked authoritative check for SOA
            //if not SOA check to see if there is a CNAME
			bool done = false;
            do{

                //check for authentication
                if(IsAuthoritative() || bAuthoritative){
                    
                    bAuthoritative = FALSE;

                    //check to see how many MX records there are
                    nMXCount = GetLookupMXRecordCount();

                    //if there are records then add them
                    if(nMXCount > 0){
                        AddMXRecords();
                        return OnError(UTE_SUCCESS);
                    }

                    //else check for CNAME
                    ResetEnumerations();
                    while(EnumDNSEntry(szBuf,sizeof(szBuf),NULL,0,CUT_DNS_CNAME) == UTE_SUCCESS){
                        pBuf = m_DNSEntries[m_nEnumIndex-1].szHost; //this is the array that holds the raw DNS data
                                                                    //the m_nEnumIndex was updated from the last
                                                                    //Enum function call
                        if(strcmp(pBuf,szDomain) == 0){
                            if(m_nRecurseCount < m_nMaxRecurseCount){
                                m_nRecurseCount++;
                                if(LookupMXInt(szBuf,szNameServer) == UTE_SUCCESS)
                                    return OnError(UTE_SUCCESS);
                                m_nRecurseCount--;
                            }
                        }
                    }   

                    //otherwise fail
                    return OnError(UTE_ERROR);
                }           

                //check for SOA or CNAME
                else{ 
                    //check for SOA, if SOA if found then it is authoritative
                    ResetEnumerations();
                    if(EnumDNSEntry(szBuf,sizeof(szBuf),NULL,0,CUT_DNS_SOA) == UTE_SUCCESS){
                        bAuthoritative = TRUE;
                        continue;
                    }   

                    //else check for CNAME
                    ResetEnumerations();
                    while(EnumDNSEntry(szBuf,sizeof(szBuf),NULL,0,CUT_DNS_CNAME) == UTE_SUCCESS){
                        pBuf = m_DNSEntries[m_nEnumIndex-1].szHost; //this is the array that holds the raw DNS data
                                                                    //the m_nEnumIndex was updated from the last
                                                                    //Enum function call
                        if(strcmp(pBuf,szDomain) == 0){
                            if(m_nRecurseCount < m_nMaxRecurseCount){
                                m_nRecurseCount++;
                                if(LookupMXInt(szBuf,szNameServer) == UTE_SUCCESS)
                                    return OnError(UTE_SUCCESS);
                                m_nRecurseCount--;
                            }
                        }
                    }   
                }

                //perform the lookup on the next name server found
                do{

                    //only go in to the max recursive level
                    if(m_nRecurseCount > m_nMaxRecurseCount){
                        return OnError(UTE_ERROR);
                    }

                    pBuf = nameServerList.GetString(nameServerListIndex);
                    
                    if(pBuf == NULL)
                        return OnError(UTE_ERROR);
                    
                    //check to see if the name server has already been used
                    if(m_usedNameServerList.Exists(pBuf) == FALSE){
                        m_nRecurseCount++;

                        m_usedNameServerList.AddString(pBuf);
                        if(LookupMXInt(szDomain,pBuf) == UTE_SUCCESS){
                            return OnError(UTE_SUCCESS);
                        }

                        m_nRecurseCount--;
                    }

                    nameServerListIndex++;

                }while(!done);

            }while(!done);
        }
    }

    return OnError(UTE_ERROR);
}

/*********************************************
GetLookupMXRecordCount - protected function
    returns the number of MX records found in the
    last DNS search
Params
    None
Return
    Number of MX records found
**********************************************/
int CUT_MXLookup::GetLookupMXRecordCount(){

    int nMXCount = 0;
    ResetEnumerations();
	// TD casts placate side effect of overload
    while(EnumMXRecords((LPSTR)NULL,0,(LPSTR)NULL,0) == UTE_SUCCESS){
        nMXCount++;
    }
    ResetEnumerations();
    return nMXCount;
}

/*********************************************
GetDomainFromEmailAddress - protected function
    Parses the given email address and returns
    the domain portion of it.
Params
    szEmailAddress - email address to parse
    szDomain - string where the domain is returned
    nLenDomain - max length of szDomain
Return
    TRUE - success
    FALSE- fail
**********************************************/
BOOL CUT_MXLookup::GetDomainFromEmailAddress(LPCSTR szEmailAddress,LPSTR szDomain,int nLenDomain){

    if(CUT_StrMethods::ParseString(szEmailAddress,"@",1,szDomain,nLenDomain) != UTE_SUCCESS) 
        strncpy(szDomain,szEmailAddress,nLenDomain);
    CUT_StrMethods::RemoveSpaces(szDomain);
	if(szDomain[strlen(szDomain)-1] == '>')
		szDomain[strlen(szDomain)-1] = 0;
    return TRUE;
}

/*********************************************
AddMXRecords - protected function
    Adds all of the MX records found during the
    last DNS lookup to the MXRecord list
Params
    None
Return
    None
**********************************************/
void CUT_MXLookup::AddMXRecords(){

    int loop,loop2;
    BOOL bAddrFound = FALSE;

    for(loop = 0; loop <m_nDNSEntryCount;loop++){

        //check to see if a MX record is found
        if( m_DNSEntries[loop].nType == CUT_DNS_MX) {
      
            //m_DNSEntries[loop].nMX_Preference;  //MX Server order preference
            //m_DNSEntries[loop].szData;          //MX Server Name
            
            //get the address for the name
            bAddrFound = FALSE;
            for(loop2=0;loop2< m_nDNSEntryCount;loop2++) {
                if(m_DNSEntries[loop2].nType == CUT_DNS_A) {
                    if(strcmp(m_DNSEntries[loop].szData,m_DNSEntries[loop2].szHost)== 0) {
                        bAddrFound = TRUE;
                        //add the entry
                        AddItem(m_DNSEntries[loop].szData,
                            m_DNSEntries[loop2].szData,
                            m_DNSEntries[loop].nMX_Preference);

                    }
                }
            }
            if(!bAddrFound){
                AddItem(m_DNSEntries[loop].szData,
                    "",
                    m_DNSEntries[loop].nMX_Preference);
            }
        }
    }    
}

/*********************************************
AddNameServer
    Adds a nameserver (domain name or IP address)
    to a list for use during the search. Add in
    order of preference. These servers will only 
    be used if the one before it fails.
    Use an IP address for better performance.
Params
    szNameServer - domain name or IP address
Return
    UTE_SUCCESS - success
    UTE_ERROR - fail    
**********************************************/
#if defined _UNICODE
int CUT_MXLookup::AddNameServer(LPCWSTR szNameServer){
	return AddNameServer(AC(szNameServer));}
#endif
int CUT_MXLookup::AddNameServer(LPCSTR szNameServer){
    if(m_nameServerList.AddString(szNameServer))
        return OnError(UTE_SUCCESS);
    return OnError(UTE_ERROR);
}
/*********************************************
ClearNameServerList
    Clears the list of domain names added with
    AddNameServer().
Params
    None
Return
    UTE_SUCCESS - success
    UTE_ERROR - fail
**********************************************/
int CUT_MXLookup::ClearNameServerList(){
    if(m_nameServerList.ClearList())
        return OnError(UTE_SUCCESS);
    return OnError(UTE_ERROR);
}

#pragma warning (pop) 
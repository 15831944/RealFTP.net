// =================================================================
//  class: CUT_DNSClient
//  File:  DNS_c.h
//
//  Purpose:
//
//      Declaration of DNS client class.
//
//      Retrieves the Domain Name Service entries for a specific domain by
//      sending a TCP query to the NS servers on port 53
//
//      RFC 1035
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

#ifndef DNS_C_H
#define DNS_C_H

// DNS entry types
typedef enum {
    CUT_DNS_A       = 1,    // address
    CUT_DNS_NS,             // name server
    CUT_DNS_MD,             // mail destination (obs)
    CUT_DNS_MF,             // mail forwarder (obs)
    CUT_DNS_CNAME,          // canonical name
    CUT_DNS_SOA,            // start of authority
    CUT_DNS_MB,             // mailbox domain
    CUT_DNS_MG,             // mail group
    CUT_DNS_MR,             // mail rename domain
    CUT_DNS_NULL,           // null 
    CUT_DNS_WKS,            // well known service description
    CUT_DNS_PTR,            // domain name pointer   
    CUT_DNS_HINFO,          // host info
    CUT_DNS_MINFO,          // mailbox or mail list info
    CUT_DNS_MX,             // mail server
    CUT_DNS_TXT             // text strings
} EntryTypes;

// Request types enumeration
// The above DNS entry types also can serve as QueryType
typedef enum {
    CUT_DNS_AXFR    = 252,  // request for transfer of an entire zone
    CUT_DNS_MAILB,          // request for mailbox related records
    CUT_DNS_MAILA,          // request for mail agent
    CUT_DNS_ALL             // request for all records
} RequestTypes;

#ifndef __midl

#include "ut_clnt.h"

#define DNS_BUFFER_SIZE     256


/***************************************************************
    DNS Entry structure
    Description of the CUT_DNSEntry members:

        szHost
            The domain name to which this resource entry pertains 

        nType:
            An integer specifies the meaning of the data in the data field of the entry. 
            It can be one of the following 

            CUT_DNS_A       1   // address
            CUT_DNS_NS      2   // name server
            CUT_DNS_MD      3   // mail destination 
            CUT_DNS_MF      4   // mail forwarder (obs)
            CUT_DNS_CNAME   5   // canonical name
            CUT_DNS_SOA     6   // start of authority
            CUT_DNS_MB      7   // mailbox domain
            CUT_DNS_MG      8   // mail group
            CUT_DNS_MR      9   // mail rename domain
            CUT_DNS_NULL    10  // null 
            CUT_DNS_WKS     11  // well known service description
            CUT_DNS_PTR     12  // domain name pointer   
            CUT_DNS_HINFO   13  // host info
            CUT_DNS_MINFO   14  // mailbox or mail list info
            CUT_DNS_MX      15  // mail server
            CUT_DNS_TXT     16  // text strings

        lTTL:
            (Time to Live) a 32 bit signed integer that specifies the 
            time interval that the resource record may be cached before
            the source of information should again be consulted. 
            0 value means that the result should not be cached.

        szdata & szData2:
            Name servers manage two kinds of data. 
            The first kind of data held in sets called zones; 
            each zone is the complete database for a particular 
            subtree of the domain space. 
            This data is called authoritative. 
            A name server periodically checks to make sure 
            that its zones are up to date, 
            and if not obtains a new copy of updated 
            zones from master files stored locally or in 
            another name server. 
            The second kind of data is cached data which was acquired by a 
            local resolver. This data may be incomplete but improves the 
            performance of the retrieval process when non-local data is 
            repeatedly accessed. 
            Cached data is eventually discarded by a timeout mechanism.

        nMX_Preference:
            16 bit integer which specifies the prefrence givin to this
             entry among others at the same host. (Mail Exchange Type entries only)

        lSOA_Serial:
            (Start of Authority Zone Saerial) version number of the origional copy of
             the zone. Zone transfer perserve this value.
             This value warps should be compared using 
             sequence space arthimetic. (SOA Type entries only)

        lSOA_Refresh:
            the time interval before the zone should be refreshed.

        lSOA_Retry:
            The time interval that should elapse before
             a failed refresh should be tried.

        lSOA_Expire:
            The upper limit on the time interval that can elapse 
            before the zone is no longer authoritative.

        lSOA_Minimum:
            minimum TTL field that should be exported with any entry from this zone.

          See rfc 1035 section 3.2 - 3.3.9

	  NOTE: As of version 5.0 szHost, szData and szData2 now _TCHAR
***************************************************/

// v4.2 maintain 2 structures - one ascii for internal entries, one _TCHAR for the EnumDNSEntry interface
typedef struct  {
    
    _TCHAR          szHost[MAX_PATH+1];
    int             nType;
    unsigned int    nMX_Preference;
    unsigned long   lTTL;
    unsigned long   lSOA_Serial;
    unsigned long   lSOA_Refresh;
    unsigned long   lSOA_Retry;
    unsigned long   lSOA_Expire;
    unsigned long   lSOA_Minimum;
    _TCHAR          szData[MAX_PATH+1];
    _TCHAR          szData2[MAX_PATH+1];
    
} CUT_DNSEntry;

typedef struct  {
    
    char            szHost[MAX_PATH+1];
    int             nType;
    unsigned int    nMX_Preference;
    unsigned long   lTTL;
    unsigned long   lSOA_Serial;
    unsigned long   lSOA_Refresh;
    unsigned long   lSOA_Retry;
    unsigned long   lSOA_Expire;
    unsigned long   lSOA_Minimum;
    char            szData[MAX_PATH+1];
    char            szData2[MAX_PATH+1];
    
} CUT_DNSEntryA;

/***************************************
CUT_DNSClient
    DNS client class
****************************************/
class CUT_DNSClient : public CUT_WSClient
{

protected: 
    unsigned int    m_nPort;                // connection port
    BOOL            m_bIncludeDefaultMX;    // if true, adds the entered domain as an MX record
    BOOL            m_bIsAuth;              // auth look-up flag
    
    // needed for domains that are not properly configured with MX Records.
    BOOL            m_bUseUDP;              // flag to use STREAM or DATAGRAM socket for the Query
    int             m_nQueryLength;         // query length
    int             m_nDNSdataLength;       // DNS data length
    int             m_nDNSEntryCount;       // number of entries
    int             m_nEnumIndex;           // enumeration index
    unsigned char   *m_lpszDNSdata;         // raw DNS data
    CUT_DNSEntryA   *m_DNSEntries;          // entry list
    char            m_szRequestedDomainName[DNS_BUFFER_SIZE]; // domain name
    int             m_nTimeOut;             // lookup timeout


    /////////////////////////////////////////////////
    // Helper functions
    /////////////////////////////////////////////////

    // Return number of DNS entries returned in the las call for the lookup function
    int     GetDNSEntryCount() const;

    int     ExtractDNSEntries();

    int     ExpandName(int startPos,int *endPos,LPSTR buf,int maxLen);

    int     GetBString(int currentPos,int *endPos,LPSTR buf,int maxLen) const;

    long    MakeLong(BYTE high,BYTE lower, BYTE evenLower, BYTE lowest);

    void    BuildQuery(char *lpszQuery, LPCSTR domain,  int QueryType );

public:

    CUT_DNSClient();
    virtual ~CUT_DNSClient();

 
    // Lookup the DNS entries for the domain using the name server provided 
    // for a specific query type
    int     LookupName(LPCSTR nameServer,LPCSTR domain, int qType = CUT_DNS_ALL);
#if defined _UNICODE
    int     LookupName(LPCWSTR nameServer,LPCWSTR domain, int qType = CUT_DNS_ALL);
#endif
	
    // Resolve the DNS entries by first resolving its own name server 
    // then use it as the server to get the DNS entries 
    // for the domain name for a specific query type
    virtual int AuthoritativeLookup(LPCSTR nameServer,LPCSTR domain, int qType = CUT_DNS_ALL);
#if defined _UNICODE
    virtual int AuthoritativeLookup(LPCWSTR nameServer,LPCWSTR domain, int qType = CUT_DNS_ALL);
#endif
    // Enumerate the name server entries for the current lookup
    virtual int EnumDNSServers(LPSTR name,int maxNameLen,LPSTR address = NULL,int maxAddrLen=0);
#if defined _UNICODE
    virtual int EnumDNSServers(LPWSTR name,int maxNameLen,LPWSTR address = NULL,int maxAddrLen=0);
#endif

    // Enumerate the name server entries for the current lookup
    virtual int EnumMXRecords(LPSTR name,int maxNameLen,LPSTR address = NULL,int maxAddrLen=0);
#if defined _UNICODE
    virtual int EnumMXRecords(LPWSTR name,int maxNameLen,LPWSTR address = NULL,int maxAddrLen=0);
#endif

    // Enumerate the name server entries for the current lookup based on type
    virtual int EnumDNSEntry(LPSTR name,int maxNameLen,LPSTR address,int maxAddrLen,int type );
#if defined _UNICODE
    virtual int EnumDNSEntry(LPWSTR name,int maxNameLen,LPWSTR address,int maxAddrLen,int type );
#endif

    // Enumerate the name server entries for the current lookup
    virtual int EnumDNSEntry(CUT_DNSEntry * entry);

    // Was the last lookup call authoritative or not?
    int     IsAuthoritative() const;

    // Reset the DNS entries index to point to the first entry avialable 
    int     ResetEnumerations();

    // Get the short name for the entry type - v4.2 using _TCHAR
    LPCTSTR   GetShortName(int index) const;
 
    // Get the long name for the entry type - v4.2 using _TCHAR
    LPCTSTR   GetLongName(int index) const;

    void    IncludeDefaultMX(BOOL bInclude);
    BOOL    GetIncludeDefaultMX() const;

    // Set/Get connect port
    int             SetPort(unsigned int newPort);
    unsigned int    GetPort() const;

    // Set lookup timeout
    int     SetLookupTimeOut(int secs); 
    int     GetLookupTimeOut() const; 

    // Switch the Socket type used when calling the look up to UDP Or TCP
    // The default type is TCP "STREAM SOCKET"
    void    SetUseUDP(BOOL flag = TRUE);
    BOOL    GetUseUDP() const;
};

#endif  //#ifndef __midl

#endif // DNS_C_H

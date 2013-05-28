// =================================================================
//  class: CUT_MXLookup
//  File:  mxlookup.h
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

#ifndef DNS_MX_LOOKUP_H
#define DNS_MX_LOOKUP_H

#include "ut_clnt.h"
#include "dns_c.h"
#include "utstrlst.h"

// ===================================
//  MX Lookup class
// ===================================
class CUT_MXLookup : public CUT_DNSClient
{

protected:
    
    typedef struct UT_MXLIST{       //MXRecord structure
        LPSTR       m_szName;
        LPSTR       m_szAddress;
        long        m_nPreference;
        UT_MXLIST*      m_pNext;
    };

    UT_MXLIST* m_pMXListStart;      //MXRecord List Start Pointer
    UT_MXLIST* m_pMXListPos;        //MXRecord List Enum Pointer
    long m_nRecordCount;            //number of MXRecords found
    
    CUT_StringList m_nameServerList;//optional name servers to search with

    CUT_StringList m_usedNameServerList;//List of name servers to already used
                                        //to perform the search

    BOOL m_nIncludeDefaultRecord;   //Include flag, for default MX record
    int  m_nTCPRetryCount;          //Number of times to try a lookup using TCP/IP
    int  m_nUDPRetryCount;          //Number of times to try a lookup using UDP

    int  m_nMaxRecurseCount;        //number of levels deep to search for MX records
    int  m_nRecurseCount;           //current search level

    virtual	void ClearList();               //clear MXRecord list
    virtual	BOOL AddItem(LPSTR szName,LPSTR szAddress,long nPreference);//Add item to MXRecord list
    virtual	void AddMXRecords();            //adds the MX records from the last lookup to the MXRecord list

    virtual	int LookupMXInt(LPCSTR szDomain,LPCSTR szNameServer); //performs the actual lookup

    virtual	int GetLookupMXRecordCount();   //retrieve the number of MX records found in the last lookup

    virtual	BOOL GetDomainFromEmailAddress(LPCSTR szEmailAddress,LPSTR szDomain,int nLenDomain); //parse email address


public:

    CUT_MXLookup();             //constructor
    virtual ~CUT_MXLookup();    //destructor

    //MX record lookup function
    int     LookupMX(LPCSTR szNameServer,LPCSTR szEmailAddress);
#if defined _UNICODE
    int     LookupMX(LPCWSTR szNameServer,LPCWSTR szEmailAddress);
#endif

    //Functions to retrieve information after a successful lookup
    long    GetMXRecordCount(); 
    int     GetMXRecord(LPSTR szName,int nNameLen,LPSTR szAddress,int nAddressLen,long* nPref);
#if defined _UNICODE
    int     GetMXRecord(LPWSTR szName,int nNameLen,LPWSTR szAddress,int nAddressLen,long* nPref);
#endif

    LPCSTR   GetMXRecord();
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetMXRecord(LPSTR record, size_t maxSize, size_t *size);
#if defined _UNICODE
	int GetMXRecord(LPWSTR record, size_t maxSize, size_t *size);
#endif

    int     ResetEnum();

    //Functions for setting lookup options - set before calling LookupMX(...)
    int     IncludeDefaultRecord(BOOL bTrueFalse = TRUE);
    int     SetRetryCounts(int nTCPCount,int nUDPCount);
    int     SetMaxRecurseLevel(int nLevel);
    int     AddNameServer(LPCSTR szNameServer);
#if defined _UNICODE
    int     AddNameServer(LPCWSTR szNameServer);
#endif
    int     ClearNameServerList();
};

#endif

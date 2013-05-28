// ===================================================================
//     Class: CUT_ICMP
//      File: UT_ICMP.h
//   Created: August 28, 1999
//   Revised: 
//   Purpose: Provide methods for ICMP 'ping' and v4 'trace route'
// Developer: 
// ===================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// ===================================================================

// ICMP message Types 
// http://www.isi.edu/in-notes/iana/assignments/icmp-parameters
enum { 
    UTT_ICMP_ECHO_REPLY     = 0,    // RFC792
    UTT_ICMP_DEST_UNRCH     = 3,    // RFC792
    UTT_ICMP_SOURCE_QUENCH  = 4,    // RFC792
    UTT_ICMP_REDIRECT       = 5,    // RFC792
    UTT_ICMP_ALT_HOST_ADDR  = 6,    
    UTT_ICMP_ECHO           = 8,    // RFC792
    UTT_ICMP_ROUTER_ADVERT  = 9,    // RFC 1256
    UTT_ICMP_ROUTER_SELECT  = 10,    // RFC 1256
    UTT_ICMP_TIME_EXCEEDED  = 11,    // RFC792
    UTT_ICMP_PARAM_PROBLEM  = 12,    // RFC792
    UTT_ICMP_TIMESTAMP      = 13,    // RFC792
    UTT_ICMP_TIMESTAMP_REPLY= 14,    // RFC792
    UTT_ICMP_INFO_REQ       = 15,    // RFC792
    UTT_ICMP_INFO_REPLY     = 16,    // RFC792
    UTT_ICMP_ADDR_MASK_REQ  = 17,    // RFC 950
    UTT_ICMP_ADDR_MASK_REPLY= 18,    // RFC 950
    UTT_ICMP_TRACEROUTE     = 30,    // RFC 1393
    UTT_ICMP_DGRAM_CONV_ERR = 31,    // RFC 1475
    UTT_ICMP_MOB_HST_REDIR  = 32,
    UTT_ICMP_IPV6_WAY       = 33,   // where are you
    UTT_ICMP_IPV6_IAH       = 34,   // i am here
    UTT_ICMP_MOB_REG_REQ    = 35,
    UTT_ICMP_MOB_REG_REPLY  = 36,
    UTT_ICMP_DOM_NAME_REQ   = 37,
    UTT_ICMP_DOM_NAME_REPLY = 38,
    UTT_ICMP_SKIP           = 39,
    UTT_ICMP_PHOTURIS       = 40
};// ICMP_TYPES;

// Message no response code
#define UTT_NO_RESPONSE         255

// ICMP message type sub-codes for Destination Unreachable
// http://www.isi.edu/in-notes/iana/assignments/icmp-parameters
enum {
    UTC_NET_UR = 0,                  // Net Unreachable
    UTC_HOST_UR = 1,                 // Host Unreachable
    UTC_PROTOCOL_UR = 2,             // Protocol Unreachable
    UTC_PORT_UR = 3,                 // Port Unreachable
    UTC_FRAG_NEEDED = 4,             // Fragmentation Needed and Don't Fragment was Set
    UTC_SOURCE_RTE_FAILED = 5,       // Source Route Failed
    UTC_DEST_NET_UNKNOWN = 6,        // Destination Network Unknown
    UTC_DEST_HOST_UNKNOWN = 7,       // Destination Host Unknown
    UTC_SOURCE_HOST_ISOLATED = 8,    // Source Host Isolated
    UTC_NET_COMM_ADMIN_PROHIBIT = 9,   // Communication with Destination Network is
                                       // Administratively Prohibited
    UTC_HOST_COMM_ADMIN_PROHIBIT = 10, // Communication with Destination Host is
                                       // Administratively Prohibited
    UTC_NET_UR_FOR_TOS = 11,         // Destination Network Unreachable for Type of Service
    UTC_HOST_UR_FOR_TOS = 12,        // Destination Host Unreachable for Type of Service
    UTC_COMM_ADMIN_PROHIBIT = 13,    // Communication Administratively Prohibited      RFC1812
    UTC_HOST_PREC_VIOLATE = 14,      // Host Precedence Violation                      RFC1812
    UTC_PREC_CUTOFF = 15             // Precedence cutoff in effect                    RFC1812
}; // DEST_UR_CODES;

// ICMP message type sub-codes for Redirect
// http://www.isi.edu/in-notes/iana/assignments/icmp-parameters
enum {
    UTC_REDIRECT_NET  = 0, // Redirect Datagram for the Network (or subnet)
    UTC_REDIRECT_HOST = 1, // Redirect Datagram for the Host
    UTC_REDIRECT_NET_TOS  = 2, // Redirect Datagram for the Type of Service and Network
    UTC_REDIRECT_HOST_TOS  = 3  // Redirect Datagram for the Type of Service and Host
}; // REDIRECT_CODES;

// ICMP message type sub-codes for Time Exceeded
// http://www.isi.edu/in-notes/iana/assignments/icmp-parameters
enum {
    UTC_TTL_EXCEEDED = 0,        // Time to Live exceeded in Transit
    UTC_FRAG_REASM_EXCEEDED = 1  // Fragment Reassembly Time Exceeded
}; // TIME_EXCEEDED_CODES;

// ICMP message type sub-codes for Parameter Problem
// http://www.isi.edu/in-notes/iana/assignments/icmp-parameters
enum {
    UTC_POINTER = 0,           // Pointer indicates the error
    UTC_OPTION_REQUIRED = 1,   // Missing a Required Option        [RFC1108]
    UTC_BAD_LENGTH = 2         // Bad Length
}; // PARAM_PROB_CODES


#ifndef __midl
#ifndef _UT_ICMP_H_
#define _UT_ICMP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ut_clnt.h"
#include "UTErr.h"

///////////////////////////////////////////////////////////////////////

#ifndef IP_TTL      
#define IP_TTL          4 /* IP time to live */
#endif 


#define DEF_PACKET_SIZE 32
#define MAX_PACKET 1024
#define ICMP_MIN 8 // minimum 8 byte icmp packet (just header)


/***********************************
 The IP header Structure 
For more details see RFC 791

    0___2___4___6___8___10__12__14__15_
    |IP     |Header |Type of Service   |// length of the header
    |Version|Length |                  |// Version of IP
    |_______|_______|__________________|// Type of service
    |Total Packet   |IP Packet ID#     |// total length of the packet
    |Length         |                  |// unique identifier
    |_____3_________|__________________|
    |Frag |IP Frag Offset              |// flags
    |Bit  |(Lcation of this frag in    |
    |     |the origional packet)       |
    |_____|____________________________|
    |Time To Live   |Transport Protocol|// Time to live
    |               |                  | // protocol (TCP, UDP etc)
    |_______________|__________________|
    |Header Checksum (For header Only) |// IP checksum
    |__________________________________|
    |(4-byte Ip address of Source host |
    |__________________________________|
    |(4-byte Ip address of Destination |
    |__________________________________|
    |IP Options (will leave it to be 0 |
    |__________________________________|
            
***********************************/
typedef struct UT_IpV4HeaderTag {
    unsigned int h_len:4;          // 4 bits length of the header
    unsigned int version:4;        // 4 bits  Version of IP
    //  IP OPTIONS
    //  bit #0, #1 & #2 = precedence, #3 = delay, #4 = throughput
    //  #5 = reliability, #6 & #7 (reserved)
    unsigned char tos;             // Type of service

    unsigned short total_len;      // total length of the packet
    unsigned short ident;          // unique identifier 
    unsigned short frag_and_flags; // flags are the first 3 bits of the LSB #0,#1 & #2
                                   // (#0 = reserverd, #1 = don't frag, #2 = more frag)
    unsigned char  ttl;  // TIME TO LIVE (maximum router hops remaining )

    // transport protocol (TCP, UDP etc)
    // 0 = IP, 1 = ICMP , 2 = IGMP , 6 = TCP , 17 = UDP
    unsigned char proto;          
    
    unsigned short checksum;       // IP checksum

    unsigned int sourceIP;
    unsigned int destIP;

} UTIpV4Header;

//
// ICMP header
//
/***********************************
The IcmpHeader header Structure 
For more details see RFC 792

    0___2___4___6___8___10__12__14__15_
    |Type   |Code   |Checksum          |  // Type sub code
    |       |       |                  |  // Type   
    |_______|_______|__________________|  // Checksum
    |       |                          |  // pointer = number of first incorrect octet 
    |pointer|Reserverd                 |  // for type PARAMETER PROBLEM (12) only
    |_______|__________________________|  // Unused 3 bytes
    |           DATA                   |
    //  (Internet header + 64 bits of  //
    |datagram header which generated   | See IP HEADER
    |   ICMP message                   |    
    +__________________________________|


***********************************/
typedef struct UTIcmpHeaderTag {
  BYTE i_type;  
  BYTE i_code; /* type sub code */
  USHORT i_cksum;
  USHORT i_id;
  USHORT i_seq;
}UTIcmpHeader;

class CUT_ICMP : public CUT_WSClient {

private:
    // private member declarations
    // copy constructor and assignment op declared private to hide.
// v4.2 removed to get rid of 4511 4512 (VC6) in derived classes
//   CUT_ICMP(const CUT_ICMP& );
//   CUT_ICMP operator=(const CUT_ICMP&);

    int     InitializeICMP(int datasize, int timeout = 0, int extra = 0);
    void    FillICMPData(int datasize = MAX_PACKET);
    USHORT  ComputeICMPChecksum();
    void    TranslateICMessage();
    void    TranslateDestUnrchMessage();
    int     SendICMPRequest(BYTE type = 0, BYTE code = 0, USHORT sequence = 0, USHORT id = 0);
    int     ReadICMPResponse();
    int     DecodeICMPResponse(unsigned int bytes);


    struct  sockaddr_in m_from;

    char    *m_recvBuf;

    int     m_bytesToSend;
    int     m_bytesToReceive;
    
    USHORT          m_nProcessID;       // for id field of ICMP header
    char            *m_icmp_data;       // buffer to hold a generic ICMP header and data
    DWORD           m_sendTime;         // time control message sent (msecs)
    DWORD           m_durationTime;     // time taken to receive response (msecs)

    BOOL            m_bCMFinished;      // is this the last control message ?
    BOOL            m_bLookupAddress;   // should we get the name of the responder?

    BYTE            m_responseType;     // set as ICM decoded
    BYTE            m_responseCode; 

    USHORT          m_responseSequence; // sequence #

    char            m_responseAddr[256];
    char            m_responseName[256];
    _TCHAR          m_responseMessage[256];

    int             m_nTimeouts;
    int             m_nMaxTimeouts;

protected:
    // protected member declarations

    virtual int     OnTimeOut();
    virtual void    OnReceiveICMP();

public:
    // public member declarations

    CUT_ICMP();
    virtual ~CUT_ICMP();
	// v4.2 function sig changed to use unsigned short
	// v4.2 Ping and TraceRoute - wide char interfaces added.
    virtual int             Ping( LPCSTR dest, int timeOut, int dataSize ,int interval, unsigned short nSendCount = 5);
#if defined _UNICODE
    virtual int             Ping( LPCWSTR dest, int timeOut, int dataSize ,int interval, unsigned short nSendCount = 5);
#endif
    virtual int             TraceRoute(LPCSTR dest, int timeout, int datasize, int nMaxHops = 30);
#if defined _UNICODE
    virtual int             TraceRoute(LPCWSTR dest, int timeout, int datasize, int nMaxHops = 30);
#endif

    // accessors - when OnRecieve is called, these will contain data on the ICMP response.
    LPCTSTR  GetResponseMessage() { return m_responseMessage; }

	// v4.2 refactored
    LPCSTR  GetResponseAddress() { return m_responseAddr; }
	int GetResponseAddress(LPSTR address, size_t maxLen, size_t *size);
#if defined _UNICODE
	int GetResponseAddress(LPWSTR address, size_t maxLen, size_t *size);
#endif

	// v4.2 refactored
    LPCSTR  GetResponseName() { return m_responseName; }
	int GetResponseName(LPSTR name, size_t maxLen, size_t *size);
#if defined _UNICODE
	int GetResponseName(LPWSTR name, size_t maxLen, size_t *size);
#endif

    const DWORD GetResponseDuration() { return m_durationTime; }
    const int GetResponseType() { return m_responseType; }
    const int GetResponseCode() { return m_responseCode; }
    const int GetResponseSequence() { return m_responseSequence; }
    const int GetResponseFinished() { return m_bCMFinished; }

    // set/get whether lookups will be done on the addresses responding 
    // to trace and ping.
    BOOL SetDoLookup(BOOL flag ) { return m_bLookupAddress = flag; }
    BOOL GetDoLookup() { return m_bLookupAddress; }

    // set get the max number of timeouts that will be tolerated 
    // (note - need not be contiguous)
    void SetMaxTimeOuts(int val) { m_nMaxTimeouts = val; }
    const int GetMaxTimeOuts() {return m_nMaxTimeouts; }
    
};

#endif
#endif  //#ifndef __midl
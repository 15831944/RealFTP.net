// ===================================================================
//     Class: CUT_ICMP
//      File: UT_ICMP.cpp
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

#ifdef _WINSOCK_2_0_
    #define _WINSOCKAPI_	/* Prevent inclusion of winsock.h in windows.h   */
	    					/* Remove this line if you are using WINSOCK 1.1 */
#endif

#include "stdafx.h"

#include "UT_ICMP.h"
#include "ut_clnt.h"

#include "ut_strop.h"

// suppress VC6 STL warnings
#pragma warning ( push, 3)
#include <iostream>
#pragma warning ( pop )

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

CUT_ICMP::CUT_ICMP() :  m_icmp_data(NULL),
                        m_bCMFinished(0),
                        m_nTimeouts(0),
                        m_nMaxTimeouts(5),
                        m_bLookupAddress(false),
                        m_recvBuf(NULL),
                        m_bytesToSend(0),
                        m_bytesToReceive(0),
                        m_sendTime(0),
                        m_durationTime(0),
                        m_nProcessID(0),
                        m_responseType(0),
                        m_responseCode(0),
                        m_responseSequence(0)
                        
{
  m_nFamily = (AF_INET);       // base class inits...
  m_nSockType=(SOCK_RAW);
  m_nProtocol=(IPPROTO_ICMP);
  strcpy(m_responseName, "NA");
  strcpy(m_responseAddr, "NA");
  _tcscpy(m_responseMessage, _T("NA"));

/*
 Create a raw socket.

	NOTE: 
 
	Raw sockets are only available in Winsock 2.0 (or better)
	and only network administrators or users with 
	administrator previleges are permitted to create raw sockets.
	i.e. If you are not an administrator (or do not have equivilant
	access, the following call to CreateSocket will fail.

	To resolve this problem ask your system administrator to create
	the following registry key:


	HKLM\System\CurrentControlSet\Services\Afd\Parameters\DisableRawSecurity DWORD 1
*/
	CreateSocket(m_socket, m_nFamily, m_nSockType, m_nProtocol);

    // set id
    m_nProcessID = (USHORT) ::GetCurrentThreadId();

}

CUT_ICMP::~CUT_ICMP(){
    // delete ICMP portion
    if( NULL != m_icmp_data ) 
    {
        delete [] m_icmp_data;
    }

    // ... and receive buffer
    if( NULL != m_recvBuf)
    {
        delete [] m_recvBuf;
    }

}

/****************************************************
ComputeICMPChecksum 
	Calculate buffer checksum as described in RFC 792  

Return
    ICMP header checksum
****************************************************/
USHORT CUT_ICMP::ComputeICMPChecksum()
{
	unsigned long		cksum = 0;
    int size = m_bytesToSend;
    USHORT * pBuf = (USHORT*)m_icmp_data;

	// Add all words in the buffer
	while(size > 1) {
		cksum += *pBuf++;
		size -= sizeof(USHORT); // WORD
		}  

	//Add the one's compliment of the total
	cksum = (cksum >> 16) + (cksum & 0xffff); 
	cksum += (cksum >> 16);

	return (USHORT)(~cksum); // produces the bitwise complement of the result    
    
}

/***************************************************
FillICMPData
	Place  ICMP header in the buffer 
Params
    datasize	- total data size
Return
    none
****************************************************/
void CUT_ICMP::FillICMPData(int datasize)
{
	UTIcmpHeader	*icmp_hdr;
	char		*datapart;

    if(m_icmp_data != NULL)
        delete [] m_icmp_data;

 	m_icmp_data = new char [datasize + 1];
	memset(m_icmp_data, 0, datasize);

	icmp_hdr = (UTIcmpHeader*)m_icmp_data;

	icmp_hdr->i_type	= UTT_ICMP_ECHO;
	icmp_hdr->i_code	= 0;
	icmp_hdr->i_id		= (USHORT)GetCurrentThreadId();
	icmp_hdr->i_cksum	= 0;
	icmp_hdr->i_seq		= 0;

	datapart = m_icmp_data + sizeof(UTIcmpHeader);

	// Place some junk in the buffer.
	memset(datapart,'ª', datasize - sizeof(UTIcmpHeader));    
    
}

/***************************************************
    InitializeICMP
    Prepare data to send and receive ICMP req/resp

  Input:    datasize - amount of data to include 
            with request.

            timeout  - socket timeouts - both 
                       send and receive
                       
            extra    - added to receive buffer size
                       if extra data expected with 
                       response.

  At exit:  m_icmp_data will be ready to send.
            m_bytesToSend will have cb to send
            m_bytesToReceive will denote what we 
                are interested in - but receive 
                buffer may need to be larger.
****************************************************/
int CUT_ICMP::InitializeICMP(int datasize, int timeout, int extra)
{
    if (datasize < DEF_PACKET_SIZE)
        datasize = DEF_PACKET_SIZE;
    
    datasize += sizeof(UTIcmpHeader);  
    FillICMPData(datasize);

	// Fixed Memory Leak
    if( NULL != m_recvBuf)
    {
        delete [] m_recvBuf;
		m_recvBuf = NULL; // in case the next line fails 
    }
    m_recvBuf = new char[(datasize+sizeof(UTIpV4Header)+extra)*2];
    
    SetSocketOption(SO_RCVTIMEO,(char*)&timeout);
    SetSocketOption(SO_SNDTIMEO,(char*)&timeout);
    
    int sz = (datasize+sizeof(UTIpV4Header)+extra)*2;
    SetSocketOption(SO_RCVBUF,&sz);

    m_bytesToReceive = (datasize+sizeof(UTIpV4Header)+extra)*2;
    m_bytesToSend = datasize;

    return 0;
}
/****************************************************
    SendICMPRequest

    Assumes packet already filled (m_icmp_data).
        and IP TTL set before call if needed.

  Input:
    type    =   ICMP type (Echo etc)
    code    =   ICMP code (def to 0)
    sequence =  ICMP seq  (def to 0)

  return:
    UTE_SUCCESS     
    UTE_SOCK_TIMEOUT
    UTE_SOCK_SEND_ERROR
    
 ****************************************************/
int CUT_ICMP::SendICMPRequest(BYTE /* type */, BYTE /* code */, USHORT sequence, USHORT id)
{
       
    if(id) // set to proc ID - caller can override w/non-0
    {
       ((UTIcmpHeader*)m_icmp_data)->i_id	= id;
    }

    ((UTIcmpHeader*)m_icmp_data)->i_cksum		= 0;
	((UTIcmpHeader*)m_icmp_data)->i_seq		= sequence;
	((UTIcmpHeader*)m_icmp_data)->i_cksum		= ComputeICMPChecksum();

	m_sendTime = GetTickCount();    // for duration calc...

    int retval = UTE_SUCCESS;
	int nSent = m_bytesToSend;
	int bWrote = SendTo(m_icmp_data, nSent);

    if (bWrote != UTE_SUCCESS) 
    {
        if (WSAGetLastError() == WSAETIMEDOUT) 
        {
            // something is not right ...
            retval = UTE_SOCK_TIMEOUT;
        }
        else{
            retval = UTE_SOCK_SEND_ERROR;
        }
    }
    else 
    {
        if (nSent < m_bytesToSend ) 
        {
            retval = UTE_SOCK_SEND_ERROR;
        }
    }
    return retval;
}

/****************************************************
  ReadICMPResponse
    The receive buffer will occasionally be filled with packets we
    don't want. We'll bypass these, and loop until we get an ICMP
    packet with our ID, as found by DecodeICMPResponse.

  Input - pointer to receive buffer
  Returns
    UTE_SOCK_TIMEOUT    - read timed out
    UTE_SOCK_RECEIVE_ERROR - hard error
    UTE_SUCCESS         - data is in buffer, addr in m_from,
                          and ICMP data has been decoded.

 ****************************************************/
int CUT_ICMP::ReadICMPResponse()
{
    // attempt to read ICMP packet intended for us...
    int bytesRead, retval;
    int fromlen = sizeof(m_from);

    do
    {
	//	if (IsAborted())
	//		break;
        // receive from whoever responds to this packet
        bytesRead = recvfrom(m_socket,m_recvBuf, m_bytesToReceive, 0, (struct sockaddr*)&m_from, &fromlen);
        
        // check for errors
        if (bytesRead == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            
            if (err == WSAETIMEDOUT) 
            {
                retval =UTE_SOCK_TIMEOUT;
                break;
            }
            if (err == WSAEMSGSIZE) {
                // While its possible we didn't allow enough space for the
                // receive buffer and m_recvBuf, more likely we have a 
                // packet not for us.  Break here when testing new ICMP code.
                continue;
            }
            retval = UTE_SOCK_RECEIVE_ERROR;
            break;


        }
    // read until we get a packet with our process ID or timeout.
    } while (UTE_SUCCESS != (retval = DecodeICMPResponse(bytesRead)));
    return OnError(retval);

}

/***************************************************
DecodeICMPResponse
	Decode response ICMP data.
Params
	bytes	- number of bytes
	from	- from address
	buf		- pointer to the response
Return
    UTE_SUCCESS if this is for us, UTE_ERROR otherwise
****************************************************/
int CUT_ICMP::DecodeICMPResponse(unsigned int bytes)
{
 
	unsigned int		iphdrlen;
	UTIpV4Header		*iphdr;
	UTIcmpHeader		*icmpResponse;
	UTIcmpHeader		*icmpOriginal;
    
	iphdr		= (UTIpV4Header *)m_recvBuf;
	iphdrlen	= iphdr->h_len * 4;			// number of 32-bit double words *4 = bytes

	if (bytes  < iphdrlen + ICMP_MIN) 
		return OnError(UTE_ERROR);


    // double check to see if the incoming ICMP header is 
    // followed by a copy of our original IP header.  Gateways
    // (routers) are known to do this for Time Exceeded control
    // messages.  If we find an 0x45 byte at the start of what should 
    // be our returned ICMP header, we can be pretty sure that our
    // stuff is displaced by the size of an IP header.
    //
	icmpOriginal = icmpResponse = (UTIcmpHeader*)(m_recvBuf + iphdrlen);


    if(*(m_recvBuf + iphdrlen+ICMP_MIN) == 0x45) 
    {
        icmpOriginal = (UTIcmpHeader*)(m_recvBuf + (iphdrlen*2) + 8);
    }
    
    // next, we need to know if this is actually a reply to 
    // our Internet Control Message... check the process ID
    if( icmpOriginal->i_id != m_nProcessID ) 
    {
        return OnError(UTE_ERROR);     // tell caller that message was not for us
    }
    else 
    {
        // ok - we know it is valid and for us 
        // lets grab the data we want...
        
        // set the duration
	    m_durationTime	= GetTickCount() - m_sendTime;

        // ICMP type of reply
        m_responseType = icmpResponse->i_type;
        m_responseCode = icmpResponse->i_code;
        
        m_responseSequence = icmpOriginal->i_seq;   

        TranslateICMessage();   // get string form of message 

    }

    return (UTE_SUCCESS);
}

/***************************************************
Ping
	Pings the address or domain name 
Params
    dest		- address or domain name to be pinged
	timeout		- max time waiting for reply (milisec.)
	datasize	- size of data to send
	interval	- interval between packets in milisec.
Return
	UTE_SUCCESS				- success
	UTE_SOCK_CREATE_FAILED	- socket creation failed
	UTE_INVALID_ADDRESS		- invalid address
	UTE_SOCK_SEND_ERROR		- socket send error
	UTE_SOCK_RECEIVE_ERROR	- socket receive error
	UTE_ABORTED				- aborted
****************************************************/
#if defined _UNICODE
int CUT_ICMP::Ping( LPCWSTR dest, int timeOut, int dataSize ,int interval, unsigned short nSendCount) {
	return Ping(AC(dest), timeOut, dataSize, interval, nSendCount); }
#endif
int	CUT_ICMP::Ping( LPCSTR dest, int timeOut, int datasize ,int interval, unsigned short nSendCount)
{
	char				addressTmp[30];
	unsigned short		seq_no = 0;
	int					result;

	if (m_socket == INVALID_SOCKET) 
		return OnError(UTE_SOCK_CREATE_FAILED);

    ClearReceiveBuffer();
	
    // Copy the address of the target to the base class 
	// target address for our SendTo function
	if (!IsIPAddress(dest)) 
    {
		if (GetAddressFromName(dest,addressTmp,sizeof(addressTmp)-1) == CUT_SUCCESS)
			strcpy(m_szAddress ,addressTmp);
		else
			return OnError(UTE_INVALID_ADDRESS); 
		}
	else
		strcpy(m_szAddress ,dest);


    InitializeICMP(datasize, timeOut);
    
    int retval;
    m_bCMFinished = 0;
    m_nTimeouts = 0;

    strcpy(m_responseName, "NA");

	// v4.2 change - was while(1)
    for(;;) 
    {

	    if(IsAborted()) {
		    retval = UTE_ABORTED;
            break;
		    }

	    // reached the maximum number 
        if (nSendCount <= seq_no) 
        {
            retval = UTE_SUCCESS;
		    break;
        }	  

 
        // SEND REQUEST
        if(UTE_SUCCESS != (retval = SendICMPRequest(0,0,seq_no++)))
            break;

        // GET RESPONSE
	    result = ReadICMPResponse();

        if(UTE_SUCCESS == result) 
        {
            // valid packet decoded
            strcpy(m_responseAddr,(LPCSTR) inet_ntoa(m_from.sin_addr));

            if(m_responseSequence == nSendCount-1)
                m_bCMFinished = true;

             // should we do a lookup?
            if(m_bLookupAddress) 
            {
                if(UTE_SUCCESS != GetNameFromAddress(m_responseAddr, m_responseName, 256))
                {
                    strcpy(m_responseName, m_responseAddr);
                }
            }
            // notify user
            OnReceiveICMP();
        }
        else 
        {
            if(UTE_SOCK_TIMEOUT == result)
            {
                m_responseSequence = (unsigned short)(seq_no - 1);
                if (!OnTimeOut() ) {
                    retval = UTE_ABORTED;
    				break;
                }
			    continue;
            }
            else 
            {
                // UTE_SOCK_READ_ERROR
                retval = result;
                break;
            }
        }

	    Sleep(interval);
  
    }

    return OnError(retval);


}

/***************************************************
TraceRoute
	Trace out the address or domain name 
Params
    dest		- address or domain name to be pinged
	timeout		- max time waiting for reply (milisec.)
	datasize	- size of data to send
	nMaxHop		- maximum hops number
Return
	UTE_SUCCESS				- success
	UTE_SOCK_CREATE_FAILED	- socket creation failed
	UTE_INVALID_ADDRESS		- invalid address
	UTE_SOCK_SEND_ERROR		- socket send error
	UTE_SOCK_RECEIVE_ERROR	- socket receive error
	UTE_ABORTED				- aborted
****************************************************/
#if defined _UNICODE
int  CUT_ICMP::TraceRoute(LPCWSTR dest, int timeout, int datasize, int nMaxHops) {
	return TraceRoute(AC(dest), timeout, datasize, nMaxHops); }
#endif
int	CUT_ICMP::TraceRoute(LPCSTR dest, int timeout, int datasize, int nMaxHops)
{
    char				addressTmp[30];
    unsigned short		seq_no = 0;
    int					result;
    
    
    if (m_socket == INVALID_SOCKET)
        return OnError(UTE_SOCK_CREATE_FAILED);

    ClearReceiveBuffer();
    
    if (!IsIPAddress(dest)) {
        if (GetAddressFromName(dest,addressTmp,sizeof(addressTmp)-1) == CUT_SUCCESS)
            strcpy(m_szAddress ,addressTmp);
        else
            return OnError(UTE_INVALID_ADDRESS); 
    }
    else
        strcpy(m_szAddress ,dest);
    
    InitializeICMP(datasize, timeout);    
    
    m_bCMFinished = 0;
    m_nTimeouts   = 0;
    int	ttl		  = 0; // we want to include the first home router  

    int retval;
    
    strcpy(m_responseName, "NA");

	// v4.2 change - was while(1)
    for(;;) {
        
        if(IsAborted()) 
        {
            retval = UTE_ABORTED;
            m_bCMFinished = true;
            break;
        }
        
        // reached the maximum number of hops or received final message
        if (nMaxHops <= seq_no  || m_bCMFinished)
        {
            retval = UTE_SUCCESS;
            break;
        }
        
        // increment the Time to live so we can get deffrent packets from each hop
        ttl ++;
        
        // set the socket time to live for the next send
        setsockopt(m_socket, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl));
        
        // SEND REQUEST
        if(UTE_SUCCESS != (retval = SendICMPRequest(0,0,seq_no++)))
            break;
        
        // get a response to our request...
        result = ReadICMPResponse();

        if(UTE_SUCCESS == result) 
        {
            // valid packet decoded
            strcpy(m_responseAddr,(LPCSTR) inet_ntoa(m_from.sin_addr));

            if(m_responseType == UTT_ICMP_ECHO_REPLY || m_responseType == UTT_ICMP_DEST_UNRCH) 
            {
                m_bCMFinished = true;
            }

             // should we do a lookup?
            if(m_bLookupAddress) 
            {
                if(UTE_SUCCESS != GetNameFromAddress(m_responseAddr, m_responseName, 256))
                {
                    strcpy(m_responseName, m_responseAddr);
                }
            }
            
            // notify user
            OnReceiveICMP();
        }
        else 
        {
            if(UTE_SOCK_TIMEOUT == result)
            {
                m_responseSequence = (unsigned short) (seq_no - 1);
                if (!OnTimeOut() ) {
                    retval = UTE_ABORTED;
    				break;
                }
			    continue;
            }
            else 
            {
                // UTE_SOCK_READ_ERROR
                retval = result;
                break;
            }
        }
      
    }
    
    return OnError(retval);
    
}


/************************************************
  TranslateICMessage
    puts a readable message string into m_responseMessage
    based on the returned ICMP type and code.

  User can later get the string with GetResponseString.
  Input: none
  Return: none
 *************************************************/
void CUT_ICMP::TranslateICMessage()
{
    switch (m_responseType) {
    case UTT_ICMP_ECHO_REPLY:     
        _tcscpy(m_responseMessage, _T("Echo Reply"));
        break;
    case UTT_ICMP_DEST_UNRCH:  
        TranslateDestUnrchMessage();
        break;        
    case UTT_ICMP_SOURCE_QUENCH:  
        _tcscpy(m_responseMessage, _T("Source Quench"));
        break;
    case UTT_ICMP_REDIRECT:
        if(m_responseCode == UTC_REDIRECT_NET) 
            _tcscpy(m_responseMessage, _T("Redirect (Redirect Datagram for the Network (or subnet))"));
        else if(m_responseCode == UTC_REDIRECT_HOST)
            _tcscpy(m_responseMessage, _T("Redirect (Redirect Datagram for the Host)"));
        else if(m_responseCode == UTC_REDIRECT_NET_TOS)
            _tcscpy(m_responseMessage, _T("Redirect (Redirect Datagram for the Type of Service and Network)"));
        else if(m_responseCode == UTC_REDIRECT_HOST_TOS)  
            _tcscpy(m_responseMessage, _T("Redirect (Redirect Datagram for the Type of Service and Host)"));
        else
            _tcscpy(m_responseMessage, _T("Redirect"));
        break;
    case UTT_ICMP_ALT_HOST_ADDR:  
        _tcscpy(m_responseMessage, _T("Alternate Host Address"));
        break;
    case UTT_ICMP_ECHO:           
        _tcscpy(m_responseMessage, _T("Echo"));
        break;
    case UTT_ICMP_ROUTER_ADVERT:  
        _tcscpy(m_responseMessage, _T("Router Advertisement"));
        break;
    case UTT_ICMP_ROUTER_SELECT:  
        _tcscpy(m_responseMessage, _T("Router Selection"));
        break;
    case UTT_ICMP_TIME_EXCEEDED:
        if (m_responseCode == 0)
           _tcscpy(m_responseMessage, _T("Time Exceeded (Time to Live exceeded in Transit)"));
        else
           _tcscpy(m_responseMessage, _T("Time Exceeded (Fragment Reassembly Time Exceeded)"));
        break;
    case UTT_ICMP_PARAM_PROBLEM:  
        if(m_responseCode == UTC_POINTER)           
            _tcscpy(m_responseMessage, _T("Parameter Problem (Pointer indicates the error)"));
        else if(m_responseCode == UTC_OPTION_REQUIRED)   
            _tcscpy(m_responseMessage, _T("Parameter Problem (Missing a Required Option)"));
        else if(m_responseCode == UTC_BAD_LENGTH)         
            _tcscpy(m_responseMessage, _T("Parameter Problem (Bad Length)"));
        else
            _tcscpy(m_responseMessage, _T("Parameter Problem"));
        break;
    case UTT_ICMP_TIMESTAMP:      
        _tcscpy(m_responseMessage, _T("Timestamp"));
        break;
    case UTT_ICMP_TIMESTAMP_REPLY:
        _tcscpy(m_responseMessage, _T("Timestamp Reply"));
        break;
    case UTT_ICMP_INFO_REQ:       
        _tcscpy(m_responseMessage, _T("Information Request"));
        break;
    case UTT_ICMP_INFO_REPLY:     
        _tcscpy(m_responseMessage, _T("Information Reply"));
        break;
    case UTT_ICMP_ADDR_MASK_REQ:  
        _tcscpy(m_responseMessage, _T("Address Mask Request"));
        break;
    case UTT_ICMP_ADDR_MASK_REPLY:
        _tcscpy(m_responseMessage, _T("Address mask reply"));
        break;
    case UTT_ICMP_TRACEROUTE:     
        _tcscpy(m_responseMessage, _T("Traceroute"));
        break;
    case UTT_ICMP_DGRAM_CONV_ERR: 
        _tcscpy(m_responseMessage, _T("Datagram Conversion Error"));
        break;
    case UTT_ICMP_MOB_HST_REDIR:  
        _tcscpy(m_responseMessage, _T("Mobile Host Redirect"));
        break;
    case UTT_ICMP_IPV6_WAY:       
        _tcscpy(m_responseMessage, _T("IPv6 Where-are-you"));
        break;
    case UTT_ICMP_IPV6_IAH:       
        _tcscpy(m_responseMessage, _T("IPv6 I-Am-Here"));
        break;
    case UTT_ICMP_MOB_REG_REQ:    
        _tcscpy(m_responseMessage, _T("Mobile Registration Request"));
        break;
    case UTT_ICMP_MOB_REG_REPLY:  
        _tcscpy(m_responseMessage, _T("Mobile Registration Reply"));
        break;
    case UTT_ICMP_DOM_NAME_REQ:   
        _tcscpy(m_responseMessage, _T("Domain Name Request"));
        break;
    case UTT_ICMP_DOM_NAME_REPLY: 
        _tcscpy(m_responseMessage, _T("Domain Name Reply"));
        break;
    case UTT_ICMP_SKIP:           
        _tcscpy(m_responseMessage, _T("SKIP"));
        break;
    case UTT_ICMP_PHOTURIS:       
        _tcscpy(m_responseMessage, _T("Photuris"));
        break;
    case UTT_NO_RESPONSE:       
        _tcscpy(m_responseMessage, _T("No Response"));
        break;
    default:
        _tcscpy(m_responseMessage, _T("Unknown Internet Control Message"));
    }
}

void CUT_ICMP::TranslateDestUnrchMessage()
{
    switch (m_responseCode) {
        case  UTC_NET_UR:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Net Unreachable)"));
           break;
        case UTC_HOST_UR: 
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Host Unreachable)"));
           break;
        case UTC_PROTOCOL_UR:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Protocol Unreachable)"));
           break;
        case UTC_PORT_UR:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Port Unreachable)"));
           break;
        case UTC_FRAG_NEEDED:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Fragmentation Needed and Don't Fragment was Set)"));
           break;
        case UTC_SOURCE_RTE_FAILED:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Source Route Failed)"));
           break;
        case UTC_DEST_NET_UNKNOWN:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Destination Network Unknown)"));
           break;
        case UTC_DEST_HOST_UNKNOWN:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Destination Host Unknown)"));
           break;
        case UTC_SOURCE_HOST_ISOLATED:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Source Host Isolated)"));
           break;
        case UTC_NET_COMM_ADMIN_PROHIBIT:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Communication with Destination Network is Administratively Prohibited)"));
           break;
        case UTC_HOST_COMM_ADMIN_PROHIBIT:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Communication with Destination Host is Administratively Prohibited)"));
           break;
        case UTC_NET_UR_FOR_TOS:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Destination Network Unreachable for Type of Service)"));
           break;
        case UTC_HOST_UR_FOR_TOS:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Destination Host Unreachable for Type of Service)"));
           break;
        case UTC_COMM_ADMIN_PROHIBIT:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Communication Administratively Prohibited)"));
           break;
        case UTC_HOST_PREC_VIOLATE:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Host Precedence Violation)"));
           break;
        case UTC_PREC_CUTOFF:
           _tcscpy(m_responseMessage, _T("Destination Unreachable (Precedence cutoff in effect)"));
           break;
        default:
           _tcscpy(m_responseMessage, _T("Destination Unreachable"));
        }
}

/*************************************************
GetResponseAddress()
Returns a string representation of the dotted IP address associated
with the reply.
PARAM
address	  - [out] pointer to buffer to receive the address
maxSize - length of buffer
size    - [out] length of address

  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- name and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - name not found
  UTE_BUFFER_TOO_SHORT  - space in name buffer indicated by maxSize insufficient, realloc 
  based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
int CUT_ICMP::GetResponseAddress(LPSTR address, size_t maxLen, size_t *size) {
	
	int retval = UTE_SUCCESS;
	
	if(address == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		
		LPCSTR str = GetResponseAddress();
		
		*size = strlen(str);
		if(*size >= maxLen) {
			++(*size);
			retval = UTE_BUFFER_TOO_SHORT;
		}
		else {
			strcpy(address, str);
		}
	}
	return retval;
}
#if defined _UNICODE
int CUT_ICMP::GetResponseAddress(LPWSTR address, size_t maxLen, size_t *size) {
	int retval;
	
	if(maxLen > 0) {
		char * addressA = new char [maxLen];
		
		if(addressA != NULL) {
			retval = GetResponseAddress( addressA, maxLen, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(address, maxLen, addressA);
			}
			delete [] addressA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetResponseAddress();
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

/*************************************************
GetResponseName()
Returns the internet name associated with the reply. - e.g. 'theultimatetoolbox.com'
PARAM
name	  - [out] pointer to buffer to receive the name
maxSize - length of buffer
size    - [out] length of name

  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- name and/or size is a null pointer
  UTE_INDEX_OUTOFRANGE  - name not found
  UTE_BUFFER_TOO_SHORT  - space in name buffer indicated by maxSize insufficient, realloc 
  based on size returned.
  UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
int CUT_ICMP::GetResponseName(LPSTR name, size_t maxLen, size_t *size) {
	
	int retval = UTE_SUCCESS;
	
	if(name == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		LPCSTR str = GetResponseName();
		
		*size = strlen(str);
		if(*size > maxLen) {
			retval = UTE_BUFFER_TOO_SHORT;
		}
		else {
			strcpy(name, str);
		}
	}
	return retval;

}
#if defined _UNICODE
int CUT_ICMP::GetResponseName(LPWSTR name, size_t maxLen, size_t *size) {

	int retval;
	
	if(maxLen > 0) {
		char * nameA = new char [maxLen];
		
		if(nameA != NULL) {
			retval = GetResponseName( nameA, maxLen, size);
			
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(name, maxLen, nameA);
			}
			delete [] nameA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetResponseName();
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
OnTimeOut

    Virtual function which is called in case of time-out.
    Default implementation sets data the user can inspect 
    then calls OnReceive.

    Note that this is called during a Ping or TraceRoute
    operation when a timeout occurs while waiting for 
    a response, not when sending.

    If a maximum number of timeouts is reached, the operation
    will be aborted.

Params
    none
Return
	TRUE	- to continue
	FALSE	- break execution
****************************************************/
int CUT_ICMP::OnTimeOut()
{
    
    m_responseCode = 0xFF;
    m_responseType = UTT_NO_RESPONSE;
    m_durationTime = 0;
    strcpy(m_responseAddr,"NA");
    _tcscpy(m_responseMessage,_T("Response timeout"));
    strcpy(m_responseName, "NA");

    ++m_nTimeouts;
    if(m_nTimeouts == m_nMaxTimeouts) {
        _tcscpy(m_responseMessage, _T("Response timeout: Maximum timeouts reached"));
        m_bCMFinished = true;
        OnReceiveICMP();
        return  FALSE;
    }

    OnReceiveICMP(); 

    return TRUE;
}

/***************************************************
OnReceiveICMP
	Virtual function is called each time we receive ICMP
    Also, the default implementation of OnTimeOut will call
    this method with data indicating that a timeout occurred.
Params
    none
Return
    none
****************************************************/
void CUT_ICMP::OnReceiveICMP()
{

}

#pragma warning ( pop )
// =================================================================
//  class: CUT_AccessControl
//  File:  UT_AccessControl.cpp
//  
//  Purpose:
//
//	  Server access control class
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
#include <process.h>
#include "UT_AccessControl.h"
#include "ut_strop.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

// these do not exist in core VC6 
#if _MSC_VER < 1400
#if !defined ULONG_PTR
#define ULONG_PTR DWORD
#define LONG_PTR DWORD
#endif
#endif

//=================================================================
//  class: CUT_AccessControl
//=================================================================

/********************************
Constructor
*********************************/
CUT_AccessControl::CUT_AccessControl() : m_bShutDown(FALSE)
{
	// Create a thread which will be removing expired items from the temp. blocked list
	m_dwCheckExpiredThread = (DWORD)_beginthread(CheckExpired, 0, (void *)this);
}

/********************************
Destructor
*********************************/
CUT_AccessControl::~CUT_AccessControl()
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);

	// Shut down the expity check thread
	m_bShutDown	= TRUE;
	if(m_dwCheckExpiredThread != -1) 
		WaitForSingleObject((HANDLE)(ULONG_PTR)m_dwCheckExpiredThread, 30000); 

}

/********************************
IsAddressAllowed
    Checks if specified IP address
	allowed to connect to the server
PARAM
    ipAddress	- IP address to check
RETURN
    TRUE if allowed
*********************************/
BOOL CUT_AccessControl::IsAddressAllowed(in_addr &ipAddress)
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);

	// Check blocked addresses list
	LIST_IP_ADDRESS_RANGE::iterator itBlocked;

	// Get current time
	time_t	CurrentTime;
	time(&CurrentTime);

	// Check temp. blocked list first
	LIST_BLOCKED_IP_ADDR::iterator itTemp;
	for(itTemp = m_listTempBlocked.begin(); itTemp != m_listTempBlocked.end(); ++itTemp)
	{
		if(ipAddress.s_addr == (*itTemp).m_ipAddress.s_addr && (*itTemp).m_timeBlockExpire >= CurrentTime)
			return FALSE;
	}

	
	
		
	long lclient = 0  ;
	long lStart = 0; 
	long lEnd  = 0;

	lclient = ipAddress.S_un.S_un_b.s_b1 * 16777216 + 
			ipAddress.S_un.S_un_b.s_b2 * 65536 +
			ipAddress.S_un.S_un_b.s_b3 * 256 +
			ipAddress.S_un.S_un_b.s_b4;


	// Check allowed addresses list
	LIST_IP_ADDRESS_RANGE::iterator itAllowed;
	for(itAllowed = m_listAllowed.begin(); itAllowed != m_listAllowed.end(); ++itAllowed)
	{

			lStart = (*itAllowed).m_ipStartAddress.S_un.S_un_b.s_b1 * 16777216 + 
			(*itAllowed).m_ipStartAddress.S_un.S_un_b.s_b2 * 65536 +
			(*itAllowed).m_ipStartAddress.S_un.S_un_b.s_b3 * 256 +
			(*itAllowed).m_ipStartAddress.S_un.S_un_b.s_b4;

			lEnd = (*itAllowed).m_ipEndAddress.S_un.S_un_b.s_b1 * 16777216 + 
			(*itAllowed).m_ipEndAddress.S_un.S_un_b.s_b2 * 65536 +
			(*itAllowed).m_ipEndAddress.S_un.S_un_b.s_b3 * 256 +
			(*itAllowed).m_ipEndAddress.S_un.S_un_b.s_b4;


		if(lclient >= lStart && lclient <= lEnd)
			return TRUE;
	}


	for(itBlocked = m_listBlocked.begin(); itBlocked != m_listBlocked.end(); ++itBlocked)
	{
	
			lStart = (*itBlocked).m_ipStartAddress.S_un.S_un_b.s_b1 * 16777216 + 
			(*itBlocked).m_ipStartAddress.S_un.S_un_b.s_b2 * 65536 +
			(*itBlocked).m_ipStartAddress.S_un.S_un_b.s_b3 * 256 +
			(*itBlocked).m_ipStartAddress.S_un.S_un_b.s_b4;

			lEnd = (*itBlocked).m_ipEndAddress.S_un.S_un_b.s_b1 * 16777216 + 
			(*itBlocked).m_ipEndAddress.S_un.S_un_b.s_b2 * 65536 +
			(*itBlocked).m_ipEndAddress.S_un.S_un_b.s_b3 * 256 +
			(*itBlocked).m_ipEndAddress.S_un.S_un_b.s_b4;


		if(lclient >= lStart && lclient <= lEnd)
			return FALSE;
	}

	return TRUE;
}

/********************************
IsAddressAllowed
    Checks if specified IP address is
	allowed to connect to the server
PARAM
    lpszAddress	- IP address to check
RETURN
    TRUE if allowed
*********************************/
BOOL CUT_AccessControl::IsAddressAllowed(LPCSTR lpszAddress)
{
	in_addr ia = StringToIP(WC(lpszAddress));
	return IsAddressAllowed(ia);
}
#if defined _UNICODE
BOOL CUT_AccessControl::IsAddressAllowed(LPCWSTR lpszAddress)
{
	in_addr ia = StringToIP(lpszAddress);
	return IsAddressAllowed(ia);
}
#endif
/********************************
AddAddress
    Add address or addresses range 
	to the allowed/blocked lists
PARAM
    Type			- Type of the access can be ACCESS_ALLOWED or ACCESS_BLOCKED
	ipStartAddress	- First IP address of the range
	[ipEndAddress]	- Last IP address of the range
RETURN
    none
*********************************/
void CUT_AccessControl::AddAddress(enumAccessType Type, in_addr &ipStartAddress, in_addr *ipEndAddress)
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);
	CUT_IPAddressRange	range;
	range.m_ipStartAddress = ipStartAddress;
	range.m_ipEndAddress = ipStartAddress;

	// Set the last range address if specified
	if(ipEndAddress != NULL)
	{
		if(ipStartAddress.s_addr > ipEndAddress->s_addr)
		{
			range.m_ipStartAddress = *ipEndAddress;
			range.m_ipEndAddress = ipStartAddress;
		}
		else
		{
			range.m_ipEndAddress = *ipEndAddress;
			range.m_ipStartAddress = ipStartAddress;
		}
	}

	// Add item into the list
	if(Type == ACCESS_ALLOWED)
		m_listAllowed.push_back(range);
	else if(Type == ACCESS_BLOCKED)
		m_listBlocked.push_back(range);
}

/********************************
AddAddress
    Add address or addresses range 
	to the allowed/blocked lists
PARAM
    Type				- Type of the access can be ACCESS_ALLOWED or ACCESS_BLOCKED
	lpszStartAddress	- First IP address of the range
	[lpszEndAddress]	- Last IP address of the range
RETURN
    none
*********************************/
void CUT_AccessControl::AddAddress(enumAccessType Type, LPCSTR lpszStartAddress, LPCSTR lpszEndAddress)
{
	in_addr sa = StringToIP(lpszStartAddress);
	in_addr ea = StringToIP(lpszEndAddress);

	if(lpszEndAddress == NULL)
		AddAddress(Type, sa);
	else
		AddAddress(Type, sa, &ea);
}
#if defined _UNICODE
void CUT_AccessControl::AddAddress(enumAccessType Type, LPCWSTR lpszStartAddress, LPCWSTR lpszEndAddress){
	in_addr sa = StringToIP(lpszStartAddress);
	in_addr ea = StringToIP(lpszEndAddress);

	if(lpszEndAddress == NULL)
		AddAddress(Type, sa);
	else
		AddAddress(Type, sa, &ea);
}
#endif
/********************************
GetAddress
    Get address or addresses range 
	from the allowed/blocked list 
	by index
PARAM
    Type			- Type of the access can be ACCESS_ALLOWED or ACCESS_BLOCKED
	lIndex			- Index of the item to get
	ipStartAddress	- Retrived first IP address of the range
	ipEndAddress	- Retrived last IP address of the range
RETURN
    TRUE if success
*********************************/
BOOL CUT_AccessControl::GetAddress(enumAccessType Type, long lIndex, in_addr &ipStartAddress, in_addr &ipEndAddress)
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);
	LIST_IP_ADDRESS_RANGE	*pList = NULL;

	// Check specified index
	if(lIndex >= GetAddressListSize(Type))
		return FALSE;

	// Select which list to use
	if(Type == ACCESS_ALLOWED)
		pList = &m_listAllowed;
	else if(Type == ACCESS_BLOCKED)
		pList = &m_listBlocked;
	else 
		return FALSE;

	// Get item from the list
	LIST_IP_ADDRESS_RANGE::iterator it;
	long	counter = 0;
	for(it = pList->begin(); it != pList->end(); ++it, counter++)
	{
		if(counter == lIndex)
		{
			ipStartAddress = (*it).m_ipStartAddress;
			ipEndAddress = (*it).m_ipEndAddress;
			return TRUE;
		}
	}

	return FALSE;
}

/********************************
GetAddressListSize
    Get the allowed/blocked/temp.blocked 
	list size
PARAM
    Type			- Type of the access can be ACCESS_ALLOWED, ACCESS_BLOCKED or ACCESS_TEMP_BLOCKED
RETURN
    size of the list
*********************************/
long CUT_AccessControl::GetAddressListSize(enumAccessType Type)
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);

	// Return specified list size
	if(Type == ACCESS_ALLOWED)
		return (long)m_listAllowed.size();
	else if(Type == ACCESS_BLOCKED)
		return (long)m_listBlocked.size();
	else if(Type == ACCESS_TEMP_BLOCKED)
		return (long)m_listTempBlocked.size();
	
	return 0;
}

/********************************
DeleteAddress
    Delete address or addresses range 
	from the allowed/blocked/temp.blocked 
	list by index
PARAM
    Type			- Type of the access can be ACCESS_ALLOWED, ACCESS_BLOCKED or ACCESS_TEMP_BLOCKED
	lIndex			- Index of the item to delete
RETURN
    TRUE if success
*********************************/
BOOL CUT_AccessControl::DeleteAddress(enumAccessType Type, long lIndex)
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);
	LIST_IP_ADDRESS_RANGE	*pList = NULL;

	// Check specified index
	if(lIndex >= GetAddressListSize(Type))
		return FALSE;

	// Select which list to use
	if(Type == ACCESS_ALLOWED)
		pList = &m_listAllowed;
	else if(Type == ACCESS_BLOCKED)
		pList = &m_listBlocked;
	else if(Type == ACCESS_TEMP_BLOCKED)
	{
		// Erase item from the list
		LIST_BLOCKED_IP_ADDR::iterator it;
		long	counter = 0;
		for(it = m_listTempBlocked.begin(); it != m_listTempBlocked.end(); ++it, counter++)
		{
			if(counter == lIndex)
			{
				m_listTempBlocked.erase(it);
				return TRUE;
			}
		}
	}
	else 
		return FALSE;

	// Erase item from the list
	LIST_IP_ADDRESS_RANGE::iterator it;
	long	counter = 0;
	for(it = pList->begin(); it != pList->end(); ++it, counter++)
	{
		if(counter == lIndex)
		{
			pList->erase(it);
			return TRUE;
		}
	}

	return FALSE;
}

/********************************
ClearAll
    Clear all addresses from the 
	allowed/blocked/temp.blocked list
PARAM
    Type			- Type of the access can be ACCESS_ALLOWED, ACCESS_BLOCKED or ACCESS_TEMP_BLOCKED
RETURN
    none
*********************************/
void CUT_AccessControl::ClearAll(enumAccessType Type)
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);

	// Clear specified list
	if(Type == ACCESS_ALLOWED)
		m_listAllowed.clear();
	else if(Type == ACCESS_BLOCKED)
		m_listBlocked.clear();
	else if(Type == ACCESS_TEMP_BLOCKED)
		m_listTempBlocked.clear();

	return;
}
	
/********************************
AddTempBlockedAddress
    Add address to the temporary 
	blocked list
PARAM
    ipAddress	- IP address
RETURN
    none
*********************************/
void CUT_AccessControl::AddTempBlockedAddress(in_addr &ipAddress)
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);

	CUT_BlockedIPAddr	address;
	BOOL					bFound = FALSE;

	// Initialize structure
	address.m_ipAddress = ipAddress;
	address.m_lBlockCount = 0;
	address.m_timeBlockExpire = 0;

	// Check if this IP address is already in the list
	LIST_BLOCKED_IP_ADDR::iterator it;
	for(it = m_listTempBlocked.begin(); it != m_listTempBlocked.end(); ++it)
	{
		if((*it).m_ipAddress.s_addr == ipAddress.s_addr)
		{
			address = (*it);
			bFound = TRUE;
			break;
		}
	}

	// Increase blocks counter
	++address.m_lBlockCount;

	// Calculate new expiry time
	address.m_timeBlockExpire = OnCalcTempBlockTime(ipAddress, address.m_lBlockCount, address.m_timeBlockExpire);

	// Add new item
	if(!bFound)
		m_listTempBlocked.push_back(address);
	// Modify existing item
	else
		(*it) = address;

	return;
}
	
/********************************
AddTempBlockedAddress
    Add address to the temporary 
	blocked list
PARAM
    lpszAddress	- IP address
RETURN
    none
*********************************/
void CUT_AccessControl::AddTempBlockedAddress(LPCSTR lpszAddress)
{
	in_addr ia = StringToIP(lpszAddress);
	AddTempBlockedAddress(ia);
}
#if defined _UNICODE
void CUT_AccessControl::AddTempBlockedAddress(LPCWSTR lpszAddress)
{
	in_addr ia = StringToIP(lpszAddress);
	AddTempBlockedAddress(ia);
}
#endif
/********************************
GetTempBlockedAddress
    Get address  from the temp.blocked 
	list by index
PARAM
    lIndex			- Index of the item to get
	ipAddress		- Retrieved IP address
	lBlockCounter	- Retrieved blocks counter
	ExpiryTime		- Retrieved expiry time
RETURN
    TRUE if success
*********************************/
BOOL CUT_AccessControl::GetTempBlockedAddress(long lIndex, in_addr &ipAddress, long &lBlockCounter, time_t &ExpiryTime)
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);

	// Check specified index
	if(lIndex >= GetAddressListSize(ACCESS_TEMP_BLOCKED))
		return FALSE;

	// Get item from the list
	LIST_BLOCKED_IP_ADDR::iterator it;
	long	counter = 0;
	for(it = m_listTempBlocked.begin(); it != m_listTempBlocked.end(); ++it, counter++)
	{
		if(counter == lIndex)
		{
			ipAddress = (*it).m_ipAddress;
			lBlockCounter = (*it).m_lBlockCount;
			ExpiryTime = (*it).m_timeBlockExpire;
			return TRUE;
		}
	}

	return FALSE;
}

/********************************
DeleteTempBlockedAddress
    Delete address from the temporary 
	blocked list
PARAM
	ipAddress		- IP address to delete
RETURN
    TRUE if success
*********************************/
BOOL CUT_AccessControl::DeleteTempBlockedAddress(in_addr &ipAddress)
{
	// Enter the critical section 
	CUT_CriticalSection	CriticalSection(m_CriticalSection);

	// Get item from the list
	LIST_BLOCKED_IP_ADDR::iterator it;
	long	counter = 0;
	for(it = m_listTempBlocked.begin(); it != m_listTempBlocked.end(); ++it, counter++)
	{
		if(ipAddress.s_addr == (*it).m_ipAddress.s_addr)
		{
			m_listTempBlocked.erase(it);
			return TRUE;
		}
	}

	return FALSE;

}

/********************************
DeleteTempBlockedAddress
    Delete address from the temporary 
	blocked list
PARAM
	lpszAddress	- IP address to delete
RETURN
    TRUE if success
*********************************/
BOOL CUT_AccessControl::DeleteTempBlockedAddress(LPCSTR lpszAddress)
{
	in_addr ia = StringToIP(lpszAddress);
	return DeleteTempBlockedAddress(ia);
}
#if defined _UNICODE
BOOL CUT_AccessControl::DeleteTempBlockedAddress(LPCWSTR lpszAddress)
{
	in_addr ia = StringToIP(lpszAddress);
	return DeleteTempBlockedAddress(ia);
}
#endif
/********************************
OnCalcTempBlockTime
    Called in the AddTempBlockedAddress 
	function to calculate the expiration 
	time of the block
PARAM
	ipAddress			- IP address to block
	lBlockCounter		- blocks counter
	timeBlockOldExpiry	- Previous expiry time
RETURN
    TRUE if success
*********************************/
time_t CUT_AccessControl::OnCalcTempBlockTime(in_addr& /* ipAddress */, long lBlockCounter, time_t /* timeBlockOldExpiry */)
{
	// Get current time
	time_t	ExpiryTime;
	time(&ExpiryTime);

	// Add seconds depending on the blocks counter
	ExpiryTime += lBlockCounter * lBlockCounter;
	
	return ExpiryTime;
}

/********************************
StringToIP
    Convert IP address string to 
	the IP adress structure
PARAM
    lpszAddress	- IP address to convert
RETURN
    IP address structure 
*********************************/
in_addr CUT_AccessControl::StringToIP(LPCSTR lpszAddress)
{
	in_addr	addr;
	addr.s_addr = 0;
	
	if(!lpszAddress)
		return addr;

	addr.s_addr = inet_addr(lpszAddress);
	return addr;
}
#if defined _UNICODE
in_addr CUT_AccessControl::StringToIP(LPCWSTR lpszAddress)
{
	in_addr	addr;
	addr.s_addr = 0;
	
	if(!lpszAddress)
		return addr;

	char szTemp[MAX_PATH+1];

	// convert to ascii
	CUT_Str::cvtcpy(szTemp, MAX_PATH, lpszAddress);

	addr.s_addr = inet_addr(szTemp);
	return addr;
}
#endif
/********************************
IPToString
    Convert IP address structure 
	to the IP adress string
PARAM
    ipAddress	- IP address to convert
RETURN
    NULL if error or pointer to
	the buffer
*********************************/
char *CUT_AccessControl::IPToString(in_addr &ipAddress)
{
	return inet_ntoa(ipAddress);
}
/*************************************************
IPToString()
Converts an IP addres to a readable string in dotted IP address format
PARAM
string	  - [out] pointer to buffer to receive title
maxSize - length of buffer
ipAddress   - address to convert
size    - [out] length of string returned, or required buffer length

  RETURN				
  UTE_SUCCES			- ok - 
  UTE_NULL_PARAM		- name and/or size is a null pointer
  UTE_BUFFER_TOO_SHORT  - space in name buffer indicated by maxSize insufficient, realloc 
  based on size returned.
**************************************************/
int CUT_AccessControl::IPToString(LPSTR string, size_t max_len, in_addr &ipAddress, size_t *size) {

	int retval = UTE_SUCCESS;
	char * addr = inet_ntoa(ipAddress);

	if(size == NULL || string == NULL ) {
		retval = UTE_NULL_PARAM;
	}
	else {
		*size = strlen(addr);
		if(*size >= max_len) {
			++(*size);
			retval = UTE_BUFFER_TOO_SHORT;
		}
		else {
			strcpy(string, addr);
		}
	}
	return retval;
}
#if defined _UNICODE
int CUT_AccessControl::IPToString(LPWSTR string, size_t max_len, in_addr &ipAddress, size_t *size) {
	int retval = UTE_SUCCESS;
	char * addr = inet_ntoa(ipAddress);

	if(size == NULL || string == NULL ) {
		retval = UTE_NULL_PARAM;
	}
	else {
		*size = strlen(addr);
		if(*size >= max_len) {
			retval = UTE_BUFFER_TOO_SHORT;
			++(*size);
		}
		else {
			CUT_Str::cvtcpy(string, max_len, addr);
		}
	}
	return retval;
}
#endif

/********************************
CheckExpired
    Thread entry function, which checks 
	and removes expired items from the
	temp. blocked list.
PARAM
    _this	- pointer to the CUT_AccessControl class
RETURN
    none
*********************************/
void CUT_AccessControl::CheckExpired(void * _this)
{
	CUT_AccessControl	*ptrAccessClass = (CUT_AccessControl *)_this;
	long				lSleepTime = 0;

	// Check if the class going to be shut down
	while(!ptrAccessClass->m_bShutDown && ptrAccessClass != NULL)
	{
		// Each 2 minutes check the list
		if(lSleepTime > 120)
		{
			lSleepTime = 0;

			// Enter the critical section 
			CUT_CriticalSection	CriticalSection(ptrAccessClass->m_CriticalSection);

			// Get current time
			time_t	CurrentTime;
			time(&CurrentTime);

			// Find expired items
			LIST_BLOCKED_IP_ADDR::iterator it;
			long	counter = 0;
			for(it = ptrAccessClass->m_listTempBlocked.begin(); it != ptrAccessClass->m_listTempBlocked.end(); counter++)
			{
				if((*it).m_timeBlockExpire < CurrentTime)
				{
					it = ptrAccessClass->m_listTempBlocked.erase(it);
					if(it == ptrAccessClass->m_listTempBlocked.end())
						break;
				}
				else
					++it;
			}
		}

		// Sleep for a second
		Sleep(1000);
		lSleepTime += 1;
	}
}

#pragma warning( pop )
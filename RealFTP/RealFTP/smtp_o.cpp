//=================================================================
//  class: CUT_SMTPOut
//  File:  smtp_o.cpp
//
//  Purpose:
//
//  Helper class for the SMTP mail server.  Implements a thread
//  that polls the que directory for incoming mail and routes
//  messages either locally or relays to the appropriate 
//  external mail server.
//
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

#include "smtp_o.h"
#include "UT_MailServer.h"

#include "ut_strop.h"

#ifdef UT_DISPLAYSTATUS
#include "uh_ctrl.h"
extern CUH_Control  *ctrlHistory;
#endif 

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

// these do not exist in core VC6 
// these do not exist in core VC6 
#if _MSC_VER < 1400
#if !defined ULONG_PTR
#define ULONG_PTR DWORD
#define LONG_PTR DWORD
#endif
#endif

/***************************************
Constructor
****************************************/
CUT_SMTPOut::CUT_SMTPOut(CUT_MailServer &ptrMailServer) :
m_ptrMailServer(&ptrMailServer),    // Initialize pointer to the Mail Server class
m_dwQuePollThread(0xFFFFFFFF),              // Initialize thread handle
m_bStartShutdown(FALSE),            // Clear shutdown flag
m_dwThreadListCount(0)              // Set thread list size to 0
{
    // Init critical section
    InitializeCriticalSection(&m_CriticalSection);
    
    // Set default address
    strcpy(m_szDefaultAddress, "");
}

/***************************************
Destructor
	Destory any allocated memory
	and shutdown all resources
****************************************/
CUT_SMTPOut::~CUT_SMTPOut(){
    
    // Set shutdown flag
    m_bStartShutdown = TRUE;
    
    // Wait for queue thread to close
    if(m_dwQuePollThread != -1)
        WaitForSingleObject((HANDLE)(ULONG_PTR)m_dwQuePollThread, 300000);
    
    // Release the critical section
    DeleteCriticalSection(&m_CriticalSection);
}

/***************************************
Start
	Starts the SMTP out thread.
Params
	none
Return
	UTE_SUCCESS - success
	UTE_ERROR   - error
****************************************/
int CUT_SMTPOut::Start(){
    
    // Run checking queue thread only once
    if(m_dwQuePollThread != (DWORD) -1 )
        return m_ptrMailServer->OnError(UTE_SUCCESS);
    
    // Spin off a que checking thread
    m_dwQuePollThread = (DWORD)_beginthread(QuePollThread,0,(VOID *)this);
    
    if (m_dwQuePollThread == (DWORD)-1)
        return m_ptrMailServer->OnError(UTE_ERROR);
    
    return m_ptrMailServer->OnError(UTE_SUCCESS);
}

/***************************************
QuePollThread
	Thread entry point. This function   
	polls the email QUE for mail to send
	out
Params
	_this - pointer to the calling CUT_SMTPOut class
Return
	none
****************************************/
void CUT_SMTPOut::QuePollThread(void *_this){
    
    // Set up the class pointers
    CUT_SMTPOut     *SMTPOut        = (CUT_SMTPOut *)_this;
    CUT_MailServer  *ptrMailServer  = SMTPOut->m_ptrMailServer;
    CUT_DataManager     *ptrDataManager = ptrMailServer->m_ptrDataManager;
    
    // Main poll loop;
    while(SMTPOut->m_bStartShutdown == FALSE) {
        
        // Check to see if the SMTP out is at its MaxOut limit
        if(SMTPOut->m_dwThreadListCount >=  ptrMailServer->Sys_GetMaxOut() ) {
            Sleep(5000);
            continue;
        }
        
        // Check to see if there is mail to send out
        long lFileHandle = ptrDataManager->Que_OpenFile();
        
        // If no mail then sleep and check again
        if(lFileHandle < 0) {
            Sleep(5000);  
            continue;
        }
        
        // Make a new instance of the SMTPOutInfo structure
        CUT_SMTPOutInfo * newInstance = new CUT_SMTPOutInfo;
        
        newInstance->SMTPOut        = SMTPOut;
        newInstance->ptrMailServer  = ptrMailServer;
        newInstance->lFileHandle    = lFileHandle;
        
        // Make a new thread to send the mail
        long dwThreadID = (DWORD)_beginthread(SendMailThread,0,(VOID *)newInstance);
        
        // If it worked then add the thread to the list
        if(dwThreadID == -1)
            delete newInstance;
    }
    
    
    // Wait for all threads to close
    while(SMTPOut->m_dwThreadListCount > 0)
        Sleep(3000);
    
    
    SMTPOut->m_dwQuePollThread = 0xFFFFFFFF;
}

/***************************************
SendMailThread
	This thread is called when there is a piece
	of mail to send out. It first checks to see
	if the mail is local or not, then passes the
	mail to the SendMailLocal or SendMailOut functions.
Params
	_this - pointer to the calling CUT_SMTPOut class        
Return
	none
****************************************/
void CUT_SMTPOut::SendMailThread(void *_this){
    
    CUT_SMTPOutInfo *info           = (CUT_SMTPOutInfo *)_this;
    CUT_SMTPOut     *SMTPOut        = info->SMTPOut;
    
    SMTPOut->SendMail(info);
    
    // Delete the information that was passed in
    delete (CUT_SMTPOutInfo *)_this;
}

/***************************************
SendMail
	Sends mail
Params
	info - pointer to the calling CUT_SMTPOut class        
Return
	none
****************************************/
void CUT_SMTPOut::SendMail(CUT_SMTPOutInfo  *info)
{
    
    CUT_MailServer  *ptrMailServer  = info->ptrMailServer;
    CUT_DataManager *ptrDataManager = ptrMailServer->m_ptrDataManager;
    long            lFileHandle     = info->lFileHandle;
    BOOLEAN         deleteQueFile   = FALSE;
    int             index, rt;
    char            buf[WSS_BUFFER_SIZE + 1];
    char            to[WSS_BUFFER_SIZE + 1];
    char            from[WSS_BUFFER_SIZE + 1];
    char            domain[WSS_BUFFER_SIZE + 1];
    char            szReturnAddress[WSS_BUFFER_SIZE + 1];
    char            addr[32];
    time_t          tm;
    DWORD           dw = 0, numRetries = 0;
    WORD			returnMail = 0;
	WORD			nReturnState = 0;		// 0 - Sending original message
											// 1 - Sending error reply to the sender
											// 2 - Sending error reply to the MailServer

	ZeroMemory( buf, WSS_BUFFER_SIZE + 1 );
	ZeroMemory( to, WSS_BUFFER_SIZE + 1 );
	ZeroMemory( from, WSS_BUFFER_SIZE + 1 );
	ZeroMemory( domain, WSS_BUFFER_SIZE + 1 );
	ZeroMemory( szReturnAddress, WSS_BUFFER_SIZE + 1 );
	ZeroMemory( addr, 32 );
    
    // Enter into a critical section
    EnterCriticalSection(&m_CriticalSection);
    
    ++m_dwThreadListCount;
    
    // Exit the critical section
    LeaveCriticalSection(&m_CriticalSection);
    to[0] = 0;

    // Initialize the return address
    strcpy(szReturnAddress, "MailServer");
    if(ptrMailServer->Sys_GetLocalName(0) != NULL) {
        strcat(szReturnAddress, "@");
		// v4.2 using AC here - local names now _TCHAR
        strcat(szReturnAddress, AC(ptrMailServer->Sys_GetLocalName(0)));
        }

    // Read the file header
    if(ptrDataManager->Que_ReadFileHeader(lFileHandle,to, WSS_BUFFER_SIZE, from, WSS_BUFFER_SIZE,&tm,&numRetries, &dw) != CUT_SUCCESS) {
        // Close & delete the file on ERROR
        ptrMailServer->OnStatus("ERROR: Que_ReadFileHeader failed.");
        ptrDataManager->Que_CloseFile(lFileHandle, TRUE);
        return;
        }

	returnMail = LOWORD(dw);
	nReturnState = HIWORD(dw);

	// Returning message to the MailServer
	if(nReturnState == 2) {
		strcpy(to, szReturnAddress);
		}

	// Check for the empty To field
    if(to[0] == 0) {
        ptrMailServer->OnStatus("To was blank");
		if(0 != m_szDefaultAddress) {
	        strcpy(to, m_szDefaultAddress);
		}
		else {
			// the idea here is that we wont be able to
			// parse a domain name - hence will force 
			// local mail, sending to MailServer...
	        strcpy(to, szReturnAddress);
		}
    }
    

	// Display the file header information 
	_snprintf(buf,sizeof(buf)-1,"Message Header. To:%s, From:%s, Retries:%d, Error:%d, State:%d", to, from, numRetries, returnMail, nReturnState);
	ptrMailServer->OnStatus(buf);


    // Get the to domain
    GetDomainFromAddress(to,domain, WSS_BUFFER_SIZE);
    
    //***** test code ***** td 1/2000
	BOOLEAN bBadDomain = FALSE;
    if(domain[0] == 0) {
	    _snprintf(buf,sizeof(buf)-1,"Domain Name not retrieved - error in processing");
		ptrMailServer->OnStatus(buf);
		bBadDomain = TRUE;
	}
	else {
		_snprintf(buf,sizeof(buf)-1,"Domain Name:%s",domain);
		ptrMailServer->OnStatus(buf);
	}
    
    // Check for max retries
    if(numRetries >= ptrMailServer->Sys_GetMaxRetries()) {
        
        // If the mail is not already being returned then return it
        if(nReturnState < 2) {
            // ReturnMail of 3 is for 'max retries reached'
            if(returnMail == 0)
                returnMail = 3;

			// Display status log message
			_snprintf(buf,sizeof(buf)-1,"Error: Max retries reached. Return Code: %d", returnMail);
			ptrMailServer->OnStatus(buf);

            numRetries = 0;
			++nReturnState;
            ptrDataManager->Que_WriteFileHeader(lFileHandle,from,to,numRetries,MAKELONG(returnMail, nReturnState));          
        }
        
        // Delete if max retries on return is reached
        else {
            deleteQueFile = TRUE;

			// Display status log message
			ptrMailServer->OnStatus("Max retries reached");
        }
    }   
    
    // Check to see if the file is local
	// v4.2 using WC here - local names now _TCHAR
    else if((ptrMailServer->Sys_IsDomainLocal(WC(domain)) == TRUE) || (bBadDomain == TRUE)) {
        
        // Display status log message
        ptrMailServer->OnStatus("Local Mail");
        
        rt = SendMailLocal(info,domain,addr,to,from,returnMail);
        if(rt != 0  && nReturnState < 2) {
			_snprintf(buf,sizeof(buf)-1,"Error: Local mail sending failed. To: %s", to);
			ptrMailServer->OnStatus(buf);

            // Don't clear the retries number
            // numRetries =0;
            returnMail = 2;
			++nReturnState;
            ptrDataManager->Que_WriteFileHeader(lFileHandle,from, to,numRetries, MAKELONG(returnMail, nReturnState));
        }
        else 
            deleteQueFile = TRUE;
    }
    
    // Send the file out if the to address is not local
    else {
        BOOLEAN         serverNotFound = TRUE;
        
        // Do a MX records lookup
        CUT_MXLookup    dns;

        // Include the default record in the MX lookup result
        dns.IncludeDefaultRecord();
        
        // Go through the DNS server list until an authorized answer is found
        // if one is not found then assume the name being looked for does not exist
        for(index = 0;index < (int)ptrMailServer->Sys_GetDNSNamesCount();index++) {
            
				// make sure we are not shutting down
				if (ptrMailServer->GetSMTPServer()->GetShutDownFlag())
				{
					break;
				}
				

            // Perform an authorized lookup
			// v4.2 using AC here - local names now _TCHAR
            if(dns.LookupMX(AC(ptrMailServer->Sys_GetDNSName(index)),domain) == UTE_SUCCESS){
                
                // Display status log message
                ptrMailServer->OnStatus("Authoritative lookup worked");
                
                // Make sure an MX record exists
                dns.ResetEnum();
                if(dns.GetMXRecord() != NULL){
                    
                    serverNotFound = FALSE;
                    break;
                }
                else 
                    // Display status log message
                    ptrMailServer->OnStatus("MX Record not found");
            }
            else {
                // Display status log message
                _snprintf(buf,sizeof(buf)-1,"Authoritative Lookup failed %s %s",domain,ptrMailServer->Sys_GetDNSName(index));
                ptrMailServer->OnStatus(buf);
            }
        }
        

            {
            serverNotFound  = TRUE;
            index           =0;
            long    nPreference;    
            
            // Go through each of the MX records until a server that works is found
            dns.ResetEnum();
            while(dns.GetMXRecord(domain, WSS_BUFFER_SIZE,addr,sizeof(addr), &nPreference) == UTE_SUCCESS){
					// make sure we are not shutting down
				if (ptrMailServer->GetSMTPServer()->GetShutDownFlag())
				{
					break;
				}
				
                
                _snprintf(buf,sizeof(buf)-1,"Trying MX %s",domain);
                ptrMailServer->OnStatus(buf);

				if (nReturnState == 0)
					returnMail = 0;
                
                // Send the mail
                rt = SendMailOut(info,domain,addr,to,from,returnMail);
                if(rt == 0) { //success
                    serverNotFound = FALSE;
                    deleteQueFile = TRUE;
                    break;
                }
                
                if(rt == UTE_RCPT_FAILED) { // User not found, return mail
                    if(nReturnState < 2) {
                        // ReturnMail of 2 is for 'user not found'
						_snprintf(buf,sizeof(buf)-1,"Error: User %s not found", to);
						ptrMailServer->OnStatus(buf);

                        numRetries =0;
						returnMail = 2;
						++nReturnState;
                        ptrDataManager->Que_WriteFileHeader(lFileHandle,from, to,numRetries,MAKELONG(returnMail, nReturnState));
                    }
                    else {
                        // If the rcpt failed and where it came from is not found
                        // then just delete it
                        deleteQueFile = TRUE;
                    }

                    serverNotFound = FALSE;
                    break;
                }
			

                
                // For other errors just repeat for each MX record
            }
            
            // If an email server was not available then retry later
            if(serverNotFound) {
                
                // ReturnMail of 1 is for 'domain not found'
                numRetries++;
				returnMail = 1;
                ptrDataManager->Que_WriteFileHeader(lFileHandle,numRetries, MAKELONG(returnMail, nReturnState));         
                
                // Display status log message
                _snprintf(buf,sizeof(buf)-1,"Retrying Mail %ld",numRetries);
                ptrMailServer->OnStatus(buf);
                
            }
        }
    }
    
    // Close the file
    ptrDataManager->Que_CloseFile(lFileHandle, deleteQueFile);
    
    // Enter into a critical section
    EnterCriticalSection(&m_CriticalSection);
    
    --m_dwThreadListCount;
    
    // Exit the critical section
    LeaveCriticalSection(&m_CriticalSection);

    // testing
	_snprintf(buf,sizeof(buf)-1,"Send mail - Exit.  m_dwThreadListCount = %ld\n", m_dwThreadListCount);

}

/***************************************
SendMailLocal
	This function is called if the mail to be
	sent to a person local to this server
Params
	info       - contains pointers to the calling 
				CUT_SMTPOut class, the CUT_DataManager class,
				and file handle for the mail
	to         - who the mail is for
	from       - who the mail is from
	returnMail - returned mail flag
Return
	UTE_SUCCESS - success
	UTE_ERROR   - error
****************************************/
int CUT_SMTPOut::SendMailLocal(CUT_SMTPOutInfo *info,LPCSTR /* domain */,LPCSTR /* addr */,
                               LPCSTR to,LPCSTR from,DWORD returnMail){
    
    CUT_MailServer  *ptrMailServer  = info->ptrMailServer;
    CUT_DataManager *ptrDataManager = info->ptrMailServer->m_ptrDataManager;
    CUT_UserManager *ptrUserManager = ptrMailServer->m_ptrUserManager;
    long            lFileHandle     = info->lFileHandle;
    char            buf[WSS_BUFFER_SIZE + 1];
    int             len;
    long            lFileHandle2;
    char            szReturnAddress[WSS_BUFFER_SIZE + 1];

    // Initialize the return address
    strcpy(szReturnAddress, "MailServer");
    if(ptrMailServer->Sys_GetLocalName(0) != NULL) {
        strcat(szReturnAddress, "@");
		// v4.2 using AC here - local names now _TCHAR")
        strcat(szReturnAddress, AC(ptrMailServer->Sys_GetLocalName(0)));
        }

	// v4.2 using WC here - user manager now using _TCHAR
    lFileHandle2 = ptrUserManager->User_CreateUserFile(WC(to));
    
    if(lFileHandle2 < 0) {
        ptrMailServer->OnStatus("Error: CreateUserFile Failed");
        return UTE_ERROR;
    }
    
    // If being returned then send the return header
    if(returnMail != 0){
        char mid[WSS_BUFFER_SIZE + 1];
        ptrMailServer->BuildUniqueID(mid, WSS_BUFFER_SIZE);
        
        _snprintf(buf,sizeof(buf)-1,"Message-ID: %s\r\n",mid);
        ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));
        _snprintf(buf,sizeof(buf)-1,"To: %s\r\n",to);
        ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));
        _snprintf(buf,sizeof(buf)-1,"From: %s\r\n", szReturnAddress);
        ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));
        GetGMTStamp(buf, WSS_BUFFER_SIZE);
        ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)"Date: ",6);
        ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));
        ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)"\r\n",2);
        
        if(returnMail == 1) { // Domain not found
			_snprintf(buf,sizeof(buf)-1, "Subject: Returned Mail- Domain Not Found (%s)\r\n\r\n", from);
            ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));                   
        }
        
        else if(returnMail == 2) { // User not found
			_snprintf(buf,sizeof(buf)-1, "Subject: Returned Mail- User Not Found (%s)\r\n\r\n", from);
            ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));                   
        }
        
        else if(returnMail == 3) { // Max retries
			_snprintf(buf,sizeof(buf)-1,"Subject: Returned Mail- Max. Send Retries Reached (%s)\r\n\r\n", from);
            ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));                   
        }
        
        strcpy(buf,"========================\r\n");
        ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));                   
        strcpy(buf," Returned Mail Follows:\r\n");
        ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));                   
        strcpy(buf,"========================\r\n");
        ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,(int)strlen(buf));                   
    }
    
    long totalSize = 0;
	for (;;) {
		len = ptrDataManager->Que_ReadFile(lFileHandle,(LPBYTE)buf, WSS_BUFFER_SIZE);
        if(len <= 0)
            break;
        
        totalSize += ptrUserManager->User_WriteFile(lFileHandle2,(LPBYTE)buf,len);
    }
    
    // Close the user file
    ptrUserManager->User_CloseFile(lFileHandle2);
    
    return UTE_SUCCESS;
}

/***************************************
SendMailOut
	This function is called to send the mail
	to another mail system.
Params
	info       - contains pointers to the calling 
				CUT_SMTPOut class, the CUT_DataManager class,
				and file handle for the mail
	domain     - domain name from the TO field
	addr       - IP address of the domain
	to         - who the mail is for
	from       - who the mail is from
	returnMail - returned mail flag
Return 
	UTE_SUCCESS         - success 
	UTE_CONNECT_FAILED  - connect failed
	UTE_HELLO_FAILED    - helo failed
	UTE_MAIL_FAILED     - mail failed
	UTE_RCPT_FAILED     - rcpt failed
	UTE_DATA_FAILED     - data failed
****************************************/
int CUT_SMTPOut::SendMailOut(CUT_SMTPOutInfo *info,LPCSTR domain,LPCSTR addr,
                             LPCSTR to,LPCSTR from,DWORD returnMail){
    
    CUT_WSClient    wsc;
    char            buf[WSS_BUFFER_SIZE + 1];
    int             len, response;
    CUT_SMTPOut     *SMTPOut        = info->SMTPOut;
    CUT_MailServer  *ptrMailServer  = info->ptrMailServer;
    CUT_DataManager     *ptrDataManager = info->ptrMailServer->m_ptrDataManager;
    long            lFileHandle     = info->lFileHandle;
    char            szReturnAddress[WSS_BUFFER_SIZE + 1];

    // Initialize the return address
    strcpy(szReturnAddress, "MailServer");
    if(ptrMailServer->Sys_GetLocalName(0) != NULL) {
        strcat(szReturnAddress, "@");
		// v4.2 using AC here - local names now _TCHAR
        strcat(szReturnAddress, AC(ptrMailServer->Sys_GetLocalName(0)));
        }
    
    // Connect
    if(addr[0] != 0) {
        if(wsc.Connect(25,(LPSTR)addr) != UTE_SUCCESS) {
            
            // Display status log message
            _snprintf(buf,sizeof(buf)-1,"Connect Failed %s",addr);
            ptrMailServer->OnStatus(buf);
            
            return ptrMailServer->OnError(UTE_CONNECT_FAILED);
        }
        
        // Display status log message
        ptrMailServer->OnStatus("Connect Succeded");
    }
    else {
        char address[WSS_BUFFER_SIZE + 1];
		ZeroMemory( address, WSS_BUFFER_SIZE + 1 );

        wsc.GetAddressFromName(domain,address, WSS_BUFFER_SIZE);
        if(wsc.Connect(25,address) != UTE_SUCCESS)
            return ptrMailServer->OnError(UTE_CONNECT_FAILED);
    }

	// Se the time out here 
	wsc.SetReceiveTimeOut (40000);
	wsc.SetSendTimeOut (40000);

   // Get the response
    response = SMTPOut->GetResponseCode(&wsc);


	if ( response ==  0 )
	{
		wsc.CloseConnection ();
		_snprintf(buf,sizeof(buf)-1,"(%s) ",domain, CUT_ERR::GetErrorString (UTE_SVR_NO_RESPONSE));
        ptrMailServer->OnStatus(buf);
       //UTE_CONNECT_REJECTED
		return ptrMailServer->OnError(UTE_SVR_NO_RESPONSE);
    }
	else if( response > 299 || response < 200  )
	{
		wsc.CloseConnection ();
		_snprintf(buf,sizeof(buf)-1,"(%s) ",domain, CUT_ERR::GetErrorString (UTE_CONNECT_REJECTED));
		ptrMailServer->OnStatus(buf);		
       //UTE_CONNECT_REJECTED
		return ptrMailServer->OnError( UTE_CONNECT_REJECTED);
	}
    
    // Send helo
    _snprintf(buf,sizeof(buf)-1,"HELO %s\r\n",ptrMailServer->Sys_GetLocalName(0));
    wsc.Send(buf);
    
    // Get the response
    response = SMTPOut->GetResponseCode(&wsc);
    if(response < 200 || response > 299) {
        ptrMailServer->OnStatus("HELO Failed");
        return ptrMailServer->OnError(UTE_HELLO_FAILED);
    }
    
    // Send mail
    _snprintf(buf,sizeof(buf)-1,"MAIL FROM: <%s>\r\n",from);
    wsc.Send(buf);
    
    // Get the response
    response = SMTPOut->GetResponseCode(&wsc);
    if(response < 200 || response > 299) {
        ptrMailServer->OnStatus("MAIL Failed");
        return ptrMailServer->OnError(UTE_MAIL_FAILED);
    }
    
    // Send rcpt
    _snprintf(buf,sizeof(buf)-1,"RCPT TO:<%s>\r\n",to);
    wsc.Send(buf);
    
    // Get the response
    response = SMTPOut->GetResponseCode(&wsc);
	if(response == 0) {
        ptrMailServer->OnStatus("RCPT Failed - timeout");
        return ptrMailServer->OnError(UTE_SOCK_TIMEOUT);
		}
    else if(response < 200 || response > 299) {
        ptrMailServer->OnStatus("RCPT Failed");
        return ptrMailServer->OnError(UTE_RCPT_FAILED);
    }
    
    // Send data
    wsc.Send("DATA\r\n");
    
    // Get the response
    response = SMTPOut->GetResponseCode(&wsc);
    if(response < 300 || response > 399) {
        ptrMailServer->OnStatus("DATA Failed");
        return ptrMailServer->OnError(UTE_DATA_FAILED);
    }
    
    // If being returned then send the return header
    if(returnMail != 0) {
        char mid[WSS_BUFFER_SIZE + 1];
        
        ptrMailServer->BuildUniqueID(mid, WSS_BUFFER_SIZE);
        _snprintf(buf,sizeof(buf)-1,"Message-ID: %s\r\n",mid);
        wsc.Send(buf);
        _snprintf(buf,sizeof(buf)-1,"To: %s\r\n",to);
        wsc.Send(buf);
        _snprintf(buf,sizeof(buf)-1,"From: %s\r\n",szReturnAddress);
        wsc.Send(buf);
        if(returnMail == 1)         // Domain not found
            wsc.Send("Subject: Returned Mail- Domain Not Found (");
        else if(returnMail == 2)    // User not found
            wsc.Send("Subject: Returned Mail- User Not Found (");
        else if(returnMail == 3)    // Max retries
            wsc.Send("Subject: Returned Mail- Max. Send Retries Reached (");

		wsc.Send(from);
		wsc.Send(")\r\n\r\n");

        
        wsc.Send("========================\r\n");       
        wsc.Send(" Returned Mail Follows:\r\n");        
        wsc.Send("========================\r\n");       
    }
    
    // Display status log message
    ptrMailServer->OnStatus("Sending Data Out");
    
    // Send the mail file
    long    total = 0;
    int     count = 0;
    
	for (;;) {
		len = ptrDataManager->Que_ReadFile(lFileHandle,(LPBYTE)buf, WSS_BUFFER_SIZE);
        if(len <= 0)
            break;
        
        wsc.Send(buf,len);
        
        total += len;
        count ++;
        
        if(count == 20){
            count = 0;
            _snprintf(buf,sizeof(buf)-1,"Total Sent: %ld",total);
            ptrMailServer->OnStatus(buf);
        }
    }   
    
    // Get the response
    response = SMTPOut->GetResponseCode(&wsc);
    if(response < 200 || response > 299) {
        
        // Display status log message
        ptrMailServer->OnStatus("Send Data Failed");
        
        wsc.CloseConnection();
        return ptrMailServer->OnError(UTE_DATA_FAILED);
    }
    
    // Display status log message
    ptrMailServer->OnStatus("Send Data Finished");
    
    wsc.CloseConnection();
    return ptrMailServer->OnError(UTE_SUCCESS);
}

/***************************************
GetDomainFromAddress
	Returns just the domain portion from
	a complete address line
Params
	addr	- complete address line
	domain	- buffer for the domain to be
			returned into
	maxSize - size of the domain buffer
Return
	UTE_SUCCESS - success
	UTE_ERROR   - error   
****************************************/
int CUT_SMTPOut::GetDomainFromAddress(LPCSTR addr,LPSTR domain,int maxSize){
    
    int len = (int)strlen(addr);
    for(int index = 0; index < len; index++) {
        if(addr[index] == '@') {
            strncpy(domain,&addr[index+1],maxSize);
            len = (int)strlen(domain) -1;
            if(domain[len]=='>')
                domain[len]=0;
            return UTE_SUCCESS;
        }
    }
    
    return UTE_ERROR;
}

/***************************************
GetResponseCode
	Returns the response code number after 
	a command is sent
Params
	pointer to the connected CUT_WSClient class
Return
	the response code - 0 for no response
****************************************/
int CUT_SMTPOut::GetResponseCode(CUT_WSClient * wsc){
    
    char buf[WSS_BUFFER_SIZE + 1];
    char response[32];
    int  len;
    int  code = 0;
    
    do {
        len = wsc->ReceiveLine(buf, WSS_BUFFER_SIZE);
        
        // Exit on an error
        if(len <=0) {
			m_ptrMailServer->OnStatus("ERROR: Response timeout.");
            return 0;
			}
        
        CUT_StrMethods::RemoveCRLF(buf);
        m_ptrMailServer->OnStatus(buf);
        
        // Parse the line
        if(CUT_StrMethods::ParseString(buf," -",0,response,sizeof(response)) != UTE_SUCCESS) {
			m_ptrMailServer->OnStatus("ERROR: Response parsing error.");
            return 0;
			}
        
    } while(buf[strlen(response)] == '-');
    
    
    code = (int)atol(response);
    
    return code;
}

/**********************************************
GetGMTStamp
	Returns a time stamp string
Params
	buf - buffer to hold the string
	maxLen - length of the buffer
Return
	none
***********************************************/
void CUT_SMTPOut::GetGMTStamp(LPSTR buf,int bufLen)
{
    time_t      t;
    struct tm   *systime;
    char        *days[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    char        *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug", "Sep","Oct","Nov","Dec"};
    
    if(bufLen < 32)
        return;
    
    _tzset();
    
    // Get the time/date
    t = time(NULL);
    systime = localtime(&t);    // fails after 2037 - t will be NULL (0)
    
    char    cSign = '-';
    int     nGmtHrDiff  = _timezone / 3600;
    int     nGmtMinDiff = abs((_timezone % 3600) / 60);
    
    if ( systime->tm_isdst > 0 )
        nGmtHrDiff -= 1;
    
    if ( nGmtHrDiff < 0 ) {
        // Because VC++ doesn't do "%+2d"
        cSign = '+';
        nGmtHrDiff *= -1;
    }
    
    _snprintf(buf,bufLen,"%s, %2.2d %s %d %2.2d:%2.2d:%2.2d %c%2.2d%2.2d",
        days[systime->tm_wday],
        systime->tm_mday, months[systime->tm_mon], systime->tm_year+1900,
        systime->tm_hour, systime->tm_min, systime->tm_sec,
        cSign, nGmtHrDiff, nGmtMinDiff );
    
}

#pragma warning ( pop )
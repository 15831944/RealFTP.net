// =================================================================
//  class: CUT_SMTPOut
//  File:  smtp_o.h
//  
//  Purpose:
//
//      Implements local mail tranfer and remote relay of 
//      messages received and waiting in queue  
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

#ifndef SMTP_OUT_H
#define SMTP_OUT_H

#include "ut_clnt.h"
#include "ut_srvr.h"
#include "fileman.h"
#include "MXLookup.h"

#include <process.h>
#include <stdio.h>
#include <time.h>


class CUT_SMTPOut;
class CUT_MailServer;

// ===================================
//  Thread info structure
// ===================================
typedef struct{
    CUT_SMTPOut     *SMTPOut;               // Pointer to the SMTP Out class
    CUT_MailServer  *ptrMailServer;         // Pointer to the Mail Server class
    long            lFileHandle;
}CUT_SMTPOutInfo;


// ===================================
//  SMTP out class
// ===================================
class CUT_SMTPOut{

private:

    DWORD               m_dwQuePollThread;              // Checking file queue thread handle
    DWORD               m_dwThreadListCount;            // Size of the thread list
    BOOL                m_bStartShutdown;               // Start shutdown flag
    CRITICAL_SECTION    m_CriticalSection;              // Critical section

public:
    char        m_szDefaultAddress[WSS_BUFFER_SIZE + 1];  // Default mail address

protected:
    CUT_MailServer      *m_ptrMailServer;               // Pointer to the Mail Server class

    // Thread entry point. This function polls the email QUE for mail to send out
    static void QuePollThread(void *_this);

    // This thread is called when there is a piece of mail to send out
    static void SendMailThread(void *_this);

    // Send mail. 
    virtual void        SendMail(CUT_SMTPOutInfo    *info);

    // This function is called to send the mail to another mail system
    virtual int         SendMailOut(CUT_SMTPOutInfo *info,LPCSTR domain,LPCSTR addr,
                            LPCSTR to,LPCSTR from,DWORD returnMail);

    // This function is called if the mail to be sent to a person local to this server
    virtual int         SendMailLocal(CUT_SMTPOutInfo *info,LPCSTR domain,LPCSTR addr,
                            LPCSTR to,LPCSTR from,DWORD returnMail);

    // Returns the response code number after a command is sent
    virtual int         GetResponseCode(CUT_WSClient * wsc);

    // Returns a time stamp string
    virtual void        GetGMTStamp(LPSTR buf,int maxLen);

    // Returns just the domain portion from a complete address line
    virtual int         GetDomainFromAddress(LPCSTR addr,LPSTR domain,int maxSize);

public:

    // Constructor/Destructor
    CUT_SMTPOut(CUT_MailServer &ptrMailServer);
    virtual ~CUT_SMTPOut();

    // Starts the SMTP out thread.
    virtual int         Start();

};

#endif

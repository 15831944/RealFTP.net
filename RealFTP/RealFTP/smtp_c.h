// =================================================================
//  class: CUT_SMTPClient
//  File:  smtp_c.h
//  
//  Purpose:
//
//  SMTP Client Class declaration.
//
//  Simple Mail Transport Protociol client.
//
//  RFC 821
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

#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

#include "ut_clnt.h"
#include "UTMessage.h"
#include <time.h>

#include "utstrlst.h"
// if the uetencode header file 
#ifndef UTBASE
    #define UTBASE
    #include "utencode.h"
#endif

#define     MAX_ADDRESS_SIZE        1000

/********************************
    SMTP client declaration
*********************************/
class CUT_SMTPClient: public CUT_WSClient {

protected: 
        
        CUT_StringList  m_listHeader;       // pointer to the Mail message header list
        unsigned int    m_nPort;            // Connect port
        int             m_nConnectTimeout;  // the wait for connect time out 
        int             m_nSMTPTimeOut;     // SMTP time out in sec.
		BOOL			m_bServerSupportTLS;
		BOOL			m_bSMTPLogin;

		// hold the user password
		char *m_lpszPassword; 
		
		// holds the user name
		char *m_lpszUserName; 

		// list supported of login mechanism
		CUT_StringList m_loginMechanism ;


		int		CRAMAuthenticate();
		int		LOGINAuthenticate();



        // Internal to find each recepient tag
        int     GetRcptName(LPCSTR rcptIn,LPSTR rcptOut,int rcptOutLen);    
        
        // Routine to parse the response from the server and return the response code
        int     GetResponseCode(int timeOut); 
        
        // Generate time stamp for the mail date
        int     GetGMTStamp(LPSTR buf,int bufLen); 

        // Remove any previously added headers such as reply and priority & X-headers
        int     ClearHeaderTags(); 

        // The actual routine that sends the mail messages to each recepient
        virtual int     ProcessMail(    LPCSTR mailto, 
                                        CUT_Msg & message,
                                        CUT_StringList &listTo,
                                        CUT_StringList &listCc,
                                        long lTotalAddressNumber,
                                        long &lNumberBytesSend,
                                        CUT_DataSource & MessageBodySource, 
                                        CUT_DataSource & AttachSource, 
                                        int nAttachNumber); 

        // Delegated function to be invoked to inform us of the message send progress
        virtual BOOL    OnSendMailProgress(long bytesSent,long totalBytes); 

		int		DoSASLAuthentication();


    public:
	    void EnableSMTPLogin(BOOL flag = TRUE);

        CUT_SMTPClient();
        virtual ~CUT_SMTPClient();
        

        // Set/Get SMTP time out
        int     SetSMTPTimeOut(int timeout);
        int     GetSMTPTimeOut() const;

        // Set/Get connect time out
        int     SetConnectTimeout(int secs);
        int     GetConnectTimeout() const;
    
        // Set/Get connect port
        int     SetPort(unsigned int newPort);
        unsigned int  GetPort() const;

        // Connect to the remote server
        virtual int     SMTPConnect(LPCSTR hostName,LPCSTR localName = "localhost"); 
#if defined _UNICODE
        virtual int     SMTPConnect(LPCWSTR hostName,LPCWSTR localName = _T("localhost")); 
#endif
        // Close the connection with the remote server 
        virtual int     SMTPClose(); 

        // Send the message to recepient
        virtual int     SendMail(CUT_Msg & message);

        // Send the message to recepient
        virtual int     SendMail(   LPCSTR to,
                            LPCSTR from,
                            LPCSTR subject,
                            LPCSTR message,
                            LPCSTR cc               = NULL, 
                            LPCSTR bcc              = NULL,
                            LPCSTR* attachFilenames = NULL,
                            int numAttach           = 0); 

#if defined _UNICODE
        virtual int     SendMail(   LPCWSTR to,
                            LPCWSTR from,
                            LPCWSTR subject,
                            LPCWSTR message,
                            LPCWSTR cc               = NULL, 
                            LPCWSTR bcc              = NULL,
                            LPCWSTR* attachFilenames = NULL,
                            int numAttach           = 0); 
#endif
		
		void SetPassword(LPCSTR lpszPass);
		void SetUserName(LPCSTR lpszName);
#if defined _UNICODE
		void SetPassword(LPCWSTR lpszPass);
		void SetUserName(LPCWSTR lpszName);
#endif

		
};
#endif  // end smtp_c.h

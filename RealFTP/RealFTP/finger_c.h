// ==================================================================
//  class:  CUT_FingerClient
//  File:   Finger_C.h
//  
//  Purpose:
//
//  Finger Client Class declaration
//
//  Finger protocol
//
//  RFC  1288
//      
// ==================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// ==================================================================

#ifndef FINGER_C_H
#define FINGER_C_H

#include "utstrlst.h"



class CUT_FingerClient : public CUT_WSClient
{

protected:  
    unsigned int m_nPort;   // Connect port
    int m_nConnectTimeout;  // the wait for connect time out 
    int m_nReceiveTimeout;  // the wait for receive time out 
    
    // Pointer to the linked list of the received lines
    CUT_StringList m_listReturnLines;   
    
    // Split the requested string into  domain and (if requested) file
    int SplitAddress(LPSTR name, LPSTR domain, LPCSTR address); 
    
    // Monitor progress and/or cancel the receive 
    virtual BOOL ReceiveFileStatus(long bytesReceived);

public:
    CUT_FingerClient();
    virtual ~CUT_FingerClient();
    
    // Set/Get connect port
    int SetPort(unsigned int newPort);
    unsigned int GetPort() const;
    
    // Set/Get connect time out
    int SetConnectTimeout(int secs);
    int GetConnectTimeout() const;
    
    // Set/Get receive time out
    int SetReceiveTimeout(int secs);
    int GetReceiveTimeout() const;
    
    // Perform the FINGER operation
    virtual int Finger(LPTSTR address, CUT_DataSource & dest, OpenMsgType fileType = (OpenMsgType)-1);
    virtual int Finger(LPTSTR address, LPTSTR filename = NULL, OpenMsgType fileType = UTM_OM_WRITING);
    
    // rertrive the nujmber of received lines
    int GetNumberReturnLines() const;           
    
    // Retrive a line by index
    LPCSTR GetReturnLine(int index) const;      
	// v4.2 refactored for wide char
	virtual int		GetReturnLine(LPSTR line, size_t maxSize, int index, size_t *size);
#if defined _UNICODE
	virtual int		GetReturnLine(LPWSTR line, size_t maxSize, int index, size_t *size);
#endif

};



#endif // FINGER_C_H

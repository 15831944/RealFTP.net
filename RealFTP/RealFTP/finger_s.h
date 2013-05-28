// ==================================================================
//  classes: CUT_FingerServer
//           CUT_FingerThread
//
//  File:  Finger_S.cpp
//  
//  Purpose:
//
//  Finger Server Class declaration
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

#ifndef FINGER_SERVER_H
#define FINGER_SERVER_H

// Include file for the base Ultimate TCP/IP Server class
#include "ut_srvr.h"
#include "UTStrLst.h"

#ifdef __BORLANDC__
    #include <dir.h>
#else
    #include <direct.h>
#endif

// VC6 warns unable to create copy ctor and assignmrnt op

class CUT_FingerThread;

// ==================================================================
//  class: CUT_FingerServer
// ==================================================================

class CUT_FingerServer: public CUT_WSServer {

friend CUT_FingerThread;


public:
        
    _TCHAR            m_szPath[MAX_PATH + 1];         // Root path

    #ifdef UT_DISPLAYSTATUS 
        // The examples of the finger server in UT2.0 use the history control.
        // Applications you will be developing may not need the functionality of the history 
        // control window.  This statement was left in as a define - to enable it from the 
        // project settings define  UT_DISPLAYSTATUS as a preprocessor defenition 
        // in the preprocessor category of the C++ tab (VC)
    #define UH_THREADSAFE
    CUH_Control *ctrlHistory;
    #endif      // #ifdef UT_DISPLAYSTATUS 

public:
    // Constructor/Destructor
    CUT_FingerServer();
    virtual ~CUT_FingerServer();

    // Returns a pointer to an instance
    // of a class derived from CUT_WSThread
    CUT_WSThread    *CreateInstance();

    // Release the instance 
    void            ReleaseInstance(CUT_WSThread *ptr);
    
    // Set/Get the root path where the search for the finger items will be
    int             SetPath(LPCTSTR);
    LPCTSTR         GetPath() const;

protected:

    // This virtual function is called each time we have any
    // status information to display.
#if defined _UNICODE
    virtual int     OnStatus(LPCSTR StatusText);
#endif
    virtual int     OnStatus(LPCTSTR StatusText);
};

//=================================================================
//  class: CUT_FingerThread
//=================================================================

class CUT_FingerThread:public CUT_WSThread {

friend CUT_FingerServer;

public:
	CUT_FingerThread(){};
private:
	CUT_FingerThread& operator=(const CUT_FingerThread&); // no implementation
	CUT_FingerThread(const CUT_FingerThread&); // no implementation

protected:
    // Over ride this to handle the max connection reached
    virtual void    OnMaxConnect();    
    
    // Override this to handle each client connection separatly
    virtual void    OnConnect();       

};


#endif

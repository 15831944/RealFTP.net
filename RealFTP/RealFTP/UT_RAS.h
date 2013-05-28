/**********************************************************
CUT_RAS
    RAS - Remote Access Services, for dial-up networking

    CUT_RAS encapsulates all of the commonly used features
    in RAS allowing developers to easily add dial-up support
    to applications.

    The class dynamically loads the RAS DLLs so that applications
    that use this class will still work on systems that have
    are not configured for dial-up networking

    Copyright 2001 The Ultimate Toolbox.
***********************************************************/

#ifndef UTRAS_H_INCLUDED
#define UTRAS_H_INCLUDED

//#define UTRAS_ENABLE_EVENTS		//define to enable disconnect events

//include RAS interfaces and definitions
#include <ras.h>
#include <raserror.h>

//include error defines
#include "uterr.h"

//function prototypes for dynamically loading RAS
typedef DWORD (APIENTRY* LPRasDial)( LPRASDIALEXTENSIONS, LPCTSTR, LPRASDIALPARAMS, DWORD, LPVOID, LPHRASCONN );
typedef DWORD (APIENTRY* LPRasHangUp)( HRASCONN );
typedef DWORD (APIENTRY* LPRasGetConnectStatus)( HRASCONN, LPRASCONNSTATUS );

typedef DWORD (APIENTRY* LPRasEnumConnections)( LPRASCONN, LPDWORD, LPDWORD );
typedef DWORD (APIENTRY* LPRasEnumEntries)( LPCTSTR, LPCTSTR, LPRASENTRYNAME, LPDWORD, LPDWORD );
typedef DWORD (APIENTRY* LPRasEnumDevices)( LPRASDEVINFO, LPDWORD, LPDWORD );

typedef DWORD (APIENTRY* LPRasCreatePhonebookEntry)( HWND, LPCSTR );
typedef DWORD (APIENTRY* LPRasEditPhonebookEntry)( HWND, LPCTSTR, LPCSTR );
typedef DWORD (APIENTRY* LPRasSetEntryDialParams)( LPCTSTR, LPRASDIALPARAMS, BOOL );
typedef DWORD (APIENTRY* LPRasGetEntryDialParams)( LPCTSTR, LPRASDIALPARAMS, LPBOOL );

typedef DWORD (APIENTRY* LPRasGetCountryInfo)( LPRASCTRYINFOA, LPDWORD );
typedef DWORD (APIENTRY* LPRasGetEntryProperties)( LPCTSTR, LPCTSTR, LPRASENTRY, LPDWORD, LPBYTE, LPDWORD );
typedef DWORD (APIENTRY* LPRasSetEntryProperties)( LPCTSTR, LPCTSTR, LPRASENTRY, DWORD, LPBYTE, DWORD );
typedef DWORD (APIENTRY* LPRasRenameEntry)( LPCTSTR, LPCTSTR, LPCTSTR );
typedef DWORD (APIENTRY* LPRasDeleteEntry)( LPCTSTR, LPCTSTR );
typedef DWORD (APIENTRY* LPRasValidateEntryName)( LPCTSTR, LPCTSTR );


/**********************************************************
***********************************************************/
class CUT_RAS  
{

protected:
    
    HINSTANCE m_hInstRASAPI;    //handle to RASAPI32.DLL
    HINSTANCE m_hInstRNAPH;     //handle to RNAPH.DLL (used by older win95 installs)
        
    //function pointers that will point to the RAS functions
    //when the library is loaded dynamically
    LPRasDial               m_rasDialFunc;
    LPRasHangUp             m_rasHangUpFunc;
    LPRasGetConnectStatus   m_rasGetConnectStatusFunc;

    LPRasEnumConnections    m_rasEnumConnectionsFunc;
    LPRasEnumEntries        m_rasEnumEntriesFunc;
    LPRasEnumDevices        m_rasEnumDevicesFunc;

    LPRasCreatePhonebookEntry m_rasCreatePhonebookEntryFunc;
    LPRasEditPhonebookEntry m_rasEditPhonebookEntryFunc;
    LPRasSetEntryDialParams m_rasSetEntryDialParamsFunc;
    LPRasGetEntryDialParams m_rasGetEntryDialParamsFunc;

    LPRasGetCountryInfo     m_rasGetCountryInfoFunc;
    LPRasGetEntryProperties m_rasGetEntryPropertiesFunc;
    LPRasSetEntryProperties m_rasSetEntryPropertiesFunc;
    LPRasRenameEntry        m_rasRenameEntryFunc;
    LPRasDeleteEntry        m_rasDeleteEntryFunc;
    LPRasValidateEntryName  m_rasValidateEntryNameFunc;

    //device info enum data
    LPRASDEVINFO    m_rdi;
    DWORD           m_rdiCount; 

    //connection info enum data
    LPRASCONN       m_rc;
    DWORD           m_rcCount;  

    //entry info enum data
    LPRASENTRYNAME  m_ren;
    DWORD           m_renCount;

    //connection handle
    HRASCONN        m_rasConn;

    //flags
    BOOL            m_bLoaded;  //set to TRUE if the RAS DLL was loaded successfully

    //dial callback
    HWND            m_hWndCallback;
    int             m_nMessageID;

    //dial attempt state
    BOOL    m_bRasCancelDial;
    BOOL    m_bDialAttemptComplete;
    BOOL    m_bDialAttemptSuccess;
    DWORD   m_dwDialState;

    //RAS dial callback function
    static VOID WINAPI RasDialStatus(UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError);

    //error handling
    DWORD   m_nLastRASError;

    //RAS DLL loading
    BOOL    InitCheck(void** pFunc);

public:
    

    //constructor, destructor
    CUT_RAS();
    virtual ~CUT_RAS();

    //initialize and cleanup functions
    BOOL    InitRAS();

    //error handling
    virtual int     OnError(int nError);

    //dialing,hangup functions
    int     Dial(LPCTSTR szEntry,LPCTSTR szUserName = NULL, LPCTSTR szPassword = NULL,LPCTSTR szNumber = NULL);
	int     DialEx(LPCTSTR szEntry,LPCTSTR szUserName = NULL, LPCTSTR szPassword = NULL,LPCTSTR szNumber = NULL);
    int     HangUp();
    int     HangUp(HRASCONN rasConn);
    int     SetDialStatusCallback(HWND hWnd, int nMessageID);
    DWORD   GetDialState();
    void    CancelDial(); // cancel the 

    BOOL    IsConnected();

    //device enumeration functions
    int     EnumDevices();
    DWORD   GetDeviceCount();
    int     GetDevice(LPRASDEVINFO rdi,DWORD index);

    //connection enumeration functions
    int     EnumConnections();
    DWORD   GetConnectionCount();
    int     GetConnection(LPRASCONN rc,DWORD index);

    //entry enumeration functions
    int     EnumEntries();
    DWORD   GetEntryCount();
    int     GetEntry(LPRASENTRYNAME ren,DWORD index);

    //phonebook entry functions
    int     GetEntryProperties(LPCTSTR szEntryName, LPRASENTRY pRasEntry,DWORD* pnRasEntryLen);
    int     SetEntryProperties(LPCTSTR szEntryName, LPRASENTRY pRasEntry,DWORD nRasEntryLen = sizeof(RASENTRY));
    
    int     SetDialEntryParams(LPCTSTR szEntryName,LPRASDIALPARAMS pRasDialParams, BOOL bClearPassword = FALSE);
    int     GetDialEntryParams(LPCTSTR szEntryName,LPRASDIALPARAMS pRasDialParams, BOOL* bClearPassword= NULL);

    int     DoesEntryExist(LPCTSTR szEntryName);
    int     ValidateEntryName(LPCTSTR szEntryName);
    int     ClearEntryPassword(LPCTSTR szEntryName);
    int     DeleteEntry(LPCTSTR szEntryName);
    int     RenameEntry(LPCTSTR szEntryName,LPCTSTR szNewEntryName);
    

    int     GetEntryPhoneNumber(LPCTSTR szEntryName, LPTSTR szPhoneNumber, long nPhoneNumberLen,
                LPTSTR szAreaCode, long nAreaCodeLen);
    int     GetEntryUserName(LPCTSTR szEntryName,LPTSTR szUserName, long nMaxLen);
    int     GetEntryPassword(LPCTSTR szEntryName,LPTSTR szPassword, long nMaxLen);

    //RAS errors        
    DWORD   GetLastRASError();
    LPCTSTR  GetRASErrorString(DWORD error);

	//events
	#ifdef UTRAS_ENABLE_EVENTS
		int				EnableDisconnectEvent(BOOL bEnable = TRUE);
		DWORD			m_disconnectThreadID;
		BOOL			m_bDisconnectThreadShutdown;
		BOOL			m_bDisconnectEventReady;
		static void		DisconnectEventThread(void *ptr);
		virtual void	OnDisconnected();
	#endif

};

#endif

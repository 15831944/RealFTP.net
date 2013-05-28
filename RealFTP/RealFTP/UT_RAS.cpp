//=================================================================
//  class: CUT_RAS
//  File:  Ut_ras.cpp
//
//  Purpose:
//
//  An easy to use RAS dialing class that provides basic and commonly
//  used RAS functionality. 
//  Create, edit and delete phonebook entries.
//  Dial phonebook entries
//  Enumerate dialup devices, connection and entries
//       
//  Note:   This class is not designed to perform multiple simultaneous
//          dialups. It has been designed to handle a single dialup
//          connection at a time. However it can handle multiple dialups
//          if each dialup is performed Sequentially.
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

#include "stdafx.h"
//#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#include "UT_RAS.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

/**********************************************************
CUT_RAS  (Constructor)
    Initializes all member variables
***********************************************************/
CUT_RAS::CUT_RAS()
{
    m_hInstRASAPI                   = NULL;
    m_hInstRNAPH                    = NULL;
    m_bLoaded                       = FALSE;

    m_rdi                           = NULL;
    m_rc                            = NULL;
    m_rasConn                       = NULL;
    m_ren                           = NULL;

    m_hWndCallback                  = NULL;
    m_nMessageID                    = 0;

    m_bDialAttemptComplete          = FALSE;
    m_bDialAttemptSuccess           = FALSE;
    m_bRasCancelDial                = FALSE;

    
    m_rasDialFunc                   = NULL;
    m_rasHangUpFunc                 = NULL;
    m_rasGetConnectStatusFunc       = NULL;
    m_rasEnumEntriesFunc            = NULL;
    m_rasEnumConnectionsFunc        = NULL;
    m_rasEnumDevicesFunc            = NULL;

    m_rasCreatePhonebookEntryFunc   = NULL;
    m_rasEditPhonebookEntryFunc     = NULL;
    m_rasSetEntryDialParamsFunc     = NULL;
    m_rasGetEntryDialParamsFunc     = NULL;

    m_rasGetCountryInfoFunc         = NULL;
    m_rasGetEntryPropertiesFunc     = NULL;
    m_rasSetEntryPropertiesFunc     = NULL;
    m_rasRenameEntryFunc            = NULL; 
    m_rasDeleteEntryFunc            = NULL; 
    m_rasValidateEntryNameFunc      = NULL;

    m_nLastRASError                 = 0;

	#ifdef UTRAS_ENABLE_EVENTS		//RAS Events
		m_disconnectThreadID			= (DWORD)-1;
		m_bDisconnectThreadShutdown		= FALSE;
	#endif

}

/**********************************************************
CUT_RAS  (Destructor)
    Cleans up all resources
***********************************************************/
CUT_RAS::~CUT_RAS()
{

    //make sure any attempted connect operation is aborted
    m_bRasCancelDial = TRUE;
    
	#ifdef UTRAS_ENABLE_EVENTS		//RAS Events
		//disable hangup event
		EnableDisconnectEvent(FALSE);
	#endif

	//hang up the connection
    HangUp();

    //free the RAS DLLs
    if(m_hInstRASAPI != NULL)
        FreeLibrary(m_hInstRASAPI);
    if(m_hInstRNAPH != NULL)
        FreeLibrary(m_hInstRNAPH);

    //free any other allocated memory
    if(m_rdi != NULL)
        delete[] m_rdi;
    if(m_rc != NULL)
        delete[] m_rc;
    if(m_ren != NULL)
        delete[] m_ren;
}

/**********************************************************
InitRAS
    This function loads in the RAS DLLs dynamically (This allows
    this class to be used in applications where RAS may or may
    not be installed). For Win95 OSR2 and later the RASAPI32.DLL
    is used. For earlier Win95 versions RNAPH.DLL is used for some
    functions. This DLL may be redistributed  since it is not part 
    of RAS, but a patch for early Win95 versions.
Params
    None
Return
    UTE_SUCCESS - success
    UTE_RAS_LOAD_ERROR - could not load the RAS DLLs
***********************************************************/
int CUT_RAS::InitRAS(){

    m_hInstRASAPI = LoadLibrary(_T("RASAPI32.DLL"));
    
    //check to see if the library loaded properly
    if(m_hInstRASAPI == NULL){
        return UTE_RAS_LOAD_ERROR;
    }

    //Load the functions from the RASAPI32.DLL
#if defined _UNICODE
	m_rasDialFunc               = (LPRasDial)GetProcAddress(m_hInstRASAPI, "RasDialW");
    m_rasHangUpFunc             = (LPRasHangUp)GetProcAddress(m_hInstRASAPI, "RasHangUpW");
    m_rasGetConnectStatusFunc   = (LPRasGetConnectStatus)GetProcAddress(m_hInstRASAPI, "RasGetConnectStatusW");

    m_rasEnumEntriesFunc        = (LPRasEnumEntries) GetProcAddress(m_hInstRASAPI, "RasEnumEntriesW");
    m_rasEnumConnectionsFunc    = (LPRasEnumConnections)GetProcAddress(m_hInstRASAPI, "RasEnumConnectionsW");
    m_rasEnumDevicesFunc        = (LPRasEnumDevices)GetProcAddress(m_hInstRASAPI, "RasEnumDevicesW");

    m_rasCreatePhonebookEntryFunc = (LPRasCreatePhonebookEntry)GetProcAddress(m_hInstRASAPI, "RasCreatePhonebookEntryW");
    m_rasEditPhonebookEntryFunc = (LPRasEditPhonebookEntry)GetProcAddress(m_hInstRASAPI, "RasEditPhonebookEntryW");
    m_rasSetEntryDialParamsFunc = (LPRasSetEntryDialParams)GetProcAddress(m_hInstRASAPI, "RasSetEntryDialParamsW");
    m_rasGetEntryDialParamsFunc = (LPRasGetEntryDialParams)GetProcAddress(m_hInstRASAPI, "RasGetEntryDialParamsW");

    m_rasGetCountryInfoFunc     = (LPRasGetCountryInfo)GetProcAddress(m_hInstRASAPI, "RasGetCountryInfoW");
    m_rasGetEntryPropertiesFunc = (LPRasGetEntryProperties)GetProcAddress(m_hInstRASAPI, "RasGetEntryPropertiesW");
    m_rasSetEntryPropertiesFunc = (LPRasSetEntryProperties)GetProcAddress(m_hInstRASAPI, "RasSetEntryPropertiesW");
    m_rasRenameEntryFunc        = (LPRasRenameEntry)GetProcAddress(m_hInstRASAPI, "RasRenameEntryW");
    m_rasDeleteEntryFunc        = (LPRasDeleteEntry)GetProcAddress(m_hInstRASAPI, "RasDeleteEntryW");
    m_rasValidateEntryNameFunc  = (LPRasValidateEntryName)GetProcAddress(m_hInstRASAPI, "RasValidateEntryNameW");

    //if RasEnumDevicesA failed then some of the functions will have to loaded from RNAPH.DLL
    //otherwise all of the functions exist inthe RASAPI32.DLL
    if(m_rasEnumDevicesFunc == NULL){
        
        m_hInstRNAPH = LoadLibrary(_T("RNAPH.DLL"));
    
        //check to see if the library loaded properly
        if(m_hInstRNAPH == NULL){
            return UTE_RAS_LOAD_ERROR;
        }

        m_rasEnumDevicesFunc            = (LPRasEnumDevices)GetProcAddress(m_hInstRNAPH, "RasEnumDevicesW");
        m_rasCreatePhonebookEntryFunc   = (LPRasCreatePhonebookEntry)GetProcAddress(m_hInstRNAPH, "RasCreatePhonebookEntryFuncW");
        m_rasEditPhonebookEntryFunc     = (LPRasEditPhonebookEntry)GetProcAddress(m_hInstRNAPH, "RasEditPhonebookEntryW");
        m_rasSetEntryDialParamsFunc     = (LPRasSetEntryDialParams)GetProcAddress(m_hInstRNAPH, "RasSetEntryDialParamsW");
        m_rasGetEntryDialParamsFunc     = (LPRasGetEntryDialParams)GetProcAddress(m_hInstRNAPH, "RasGetEntryDialParamsW");
        m_rasGetCountryInfoFunc         = (LPRasGetCountryInfo)GetProcAddress(m_hInstRNAPH, "RasGetCountryInfoW");
        m_rasGetEntryPropertiesFunc     = (LPRasGetEntryProperties)GetProcAddress(m_hInstRNAPH, "RasGetEntryPropertiesW");
        m_rasSetEntryPropertiesFunc     = (LPRasSetEntryProperties)GetProcAddress(m_hInstRNAPH, "RasSetEntryPropertiesW");
        m_rasRenameEntryFunc            = (LPRasRenameEntry)GetProcAddress(m_hInstRNAPH, "RasRenameEntryW");
        m_rasDeleteEntryFunc            = (LPRasDeleteEntry)GetProcAddress(m_hInstRNAPH, "RasDeleteEntryW");
        m_rasValidateEntryNameFunc      = (LPRasValidateEntryName)GetProcAddress(m_hInstRNAPH, "RasValidateEntryNameW");
    }

#else
	m_rasDialFunc               = (LPRasDial)GetProcAddress(m_hInstRASAPI, "RasDialA");
    m_rasHangUpFunc             = (LPRasHangUp)GetProcAddress(m_hInstRASAPI, "RasHangUpA");
    m_rasGetConnectStatusFunc   = (LPRasGetConnectStatus)GetProcAddress(m_hInstRASAPI, "RasGetConnectStatusA");

    m_rasEnumEntriesFunc        = (LPRasEnumEntries) GetProcAddress(m_hInstRASAPI, "RasEnumEntriesA");
    m_rasEnumConnectionsFunc    = (LPRasEnumConnections)GetProcAddress(m_hInstRASAPI, "RasEnumConnectionsA");
    m_rasEnumDevicesFunc        = (LPRasEnumDevices)GetProcAddress(m_hInstRASAPI, "RasEnumDevicesA");

    m_rasCreatePhonebookEntryFunc = (LPRasCreatePhonebookEntry)GetProcAddress(m_hInstRASAPI, "RasCreatePhonebookEntryA");
    m_rasEditPhonebookEntryFunc = (LPRasEditPhonebookEntry)GetProcAddress(m_hInstRASAPI, "RasEditPhonebookEntryA");
    m_rasSetEntryDialParamsFunc = (LPRasSetEntryDialParams)GetProcAddress(m_hInstRASAPI, "RasSetEntryDialParamsA");
    m_rasGetEntryDialParamsFunc = (LPRasGetEntryDialParams)GetProcAddress(m_hInstRASAPI, "RasGetEntryDialParamsA");

    m_rasGetCountryInfoFunc     = (LPRasGetCountryInfo)GetProcAddress(m_hInstRASAPI, "RasGetCountryInfoA");
    m_rasGetEntryPropertiesFunc = (LPRasGetEntryProperties)GetProcAddress(m_hInstRASAPI, "RasGetEntryPropertiesA");
    m_rasSetEntryPropertiesFunc = (LPRasSetEntryProperties)GetProcAddress(m_hInstRASAPI, "RasSetEntryPropertiesA");
    m_rasRenameEntryFunc        = (LPRasRenameEntry)GetProcAddress(m_hInstRASAPI, "RasRenameEntryA");
    m_rasDeleteEntryFunc        = (LPRasDeleteEntry)GetProcAddress(m_hInstRASAPI, "RasDeleteEntryA");
    m_rasValidateEntryNameFunc  = (LPRasValidateEntryName)GetProcAddress(m_hInstRASAPI, "RasValidateEntryNameA");

    //if RasEnumDevicesA failed then some of the functions will have to loaded from RNAPH.DLL
    //otherwise all of the functions exist inthe RASAPI32.DLL
    if(m_rasEnumDevicesFunc == NULL){
        
        m_hInstRNAPH = LoadLibrary(_T("RNAPH.DLL"));
    
        //check to see if the library loaded properly
        if(m_hInstRNAPH == NULL){
            return UTE_RAS_LOAD_ERROR;
        }

        m_rasEnumDevicesFunc            = (LPRasEnumDevices)GetProcAddress(m_hInstRNAPH, "RasEnumDevicesA");
        m_rasCreatePhonebookEntryFunc   = (LPRasCreatePhonebookEntry)GetProcAddress(m_hInstRNAPH, "RasCreatePhonebookEntryFuncA");
        m_rasEditPhonebookEntryFunc     = (LPRasEditPhonebookEntry)GetProcAddress(m_hInstRNAPH, "RasEditPhonebookEntryA");
        m_rasSetEntryDialParamsFunc     = (LPRasSetEntryDialParams)GetProcAddress(m_hInstRNAPH, "RasSetEntryDialParamsA");
        m_rasGetEntryDialParamsFunc     = (LPRasGetEntryDialParams)GetProcAddress(m_hInstRNAPH, "RasGetEntryDialParamsA");
        m_rasGetCountryInfoFunc         = (LPRasGetCountryInfo)GetProcAddress(m_hInstRNAPH, "RasGetCountryInfoA");
        m_rasGetEntryPropertiesFunc     = (LPRasGetEntryProperties)GetProcAddress(m_hInstRNAPH, "RasGetEntryPropertiesA");
        m_rasSetEntryPropertiesFunc     = (LPRasSetEntryProperties)GetProcAddress(m_hInstRNAPH, "RasSetEntryPropertiesA");
        m_rasRenameEntryFunc            = (LPRasRenameEntry)GetProcAddress(m_hInstRNAPH, "RasRenameEntryA");
        m_rasDeleteEntryFunc            = (LPRasDeleteEntry)GetProcAddress(m_hInstRNAPH, "RasDeleteEntryA");
        m_rasValidateEntryNameFunc      = (LPRasValidateEntryName)GetProcAddress(m_hInstRNAPH, "RasValidateEntryNameA");
    }

#endif

    // if we loaded the RASApi32.DLL has been loaded
    // let me make sure that it is running
    //  that will make sure that we do have RAS setp up correctly on this machine
    // Test to see if the Check if the Remote Acess service manager 
    // specially if it is NT 4.0
    // to do this we will attempt to do a test for connection
    // this will set the Erro code to 711 or ERROR_RASMAN_CANNOT_INITIALIZE

   

    //init the connection status structure
    RASCONNSTATUS rcs;
    memset(&rcs,0,sizeof(RASCONNSTATUS));
    rcs.dwSize = sizeof(RASCONNSTATUS);
	BOOL bFuncSucess = TRUE;
    //get the connection status
	try 
	{
	m_nLastRASError = (m_rasGetConnectStatusFunc)(m_rasConn,&rcs);
    }
	catch (...)
	{
		bFuncSucess = FALSE;
	}
    //return the connection status
    if(m_nLastRASError == ERROR_RASMAN_CANNOT_INITIALIZE ||!bFuncSucess)
    {
        m_bLoaded = FALSE;
        return UTE_RAS_LOAD_ERROR;
    }
    //set the loaded flag to indicate that the libraries have been loaded
    m_bLoaded = TRUE;
     //return success
    return UTE_SUCCESS;
}

/**********************************************************
InitCheck (Protected function)
    Each function that calls into the RAS DLLs makes a call
    to this function. If the RAS DLLs are not already loaded
    then this function attempts to load the DLLs and make sure
    that the given function pointer has been properly maped.
Params
    pFunc - a pointer to a function reference to be checked
Return
    TRUE - if the RAS function has been properly maped
    FALSE - if the function could not be mapped
***********************************************************/
BOOL CUT_RAS::InitCheck(void** pFunc){

    //if the RAS DLLs are not already loaded then attemp to 
    //load them
    if(!m_bLoaded)
        InitRAS();

    //check to see if the function has been properly mapped
    if(!m_bLoaded || *pFunc == NULL)
        return FALSE;

    return TRUE;
}

/**********************************************************
OnError
    All functions which have return error codes call this 
    function. Override this function to provide extended 
    error handling and debugging abilities.
Params
    nError - error code
Return
    by default returns the given error code
***********************************************************/
int CUT_RAS::OnError(int nError){

    //return the given error code
    
    return nError;
}

/**********************************************************
Dial
    Dials a given phonebook entry. When dialing an entry,
    specifying the userName, password and phoneNumber are 
    optional and can be NULL. By specifing these extra 
    parameters, they will override the information located
    in the dialup entry (but will not modify the entry).
    
    Also under Win98 and NT this function can dial without
    a phonebook entry, by using the given information and
    the first available dial-up device.

    Although a dialup connection can be made without specifying
    a phonebook entry, it is recommended to do so for purposes
    of reliability.
    
    This function does not return until the dial has completed
    successfully or with errors. To monitor the connection use
    the SetDialStatusCallback function to send dial status 
    information as a message to a given window.

    For extending information when this function returns use the
    GetDialState function.

Params
    szEntry - specifies the phonebook entry to dial. Defaults to NULL.
    szUserName - specifies the user name to use. Defaults to NULL.
    szPassword - specifies the password to use. Defaults to NULL.
    szNumber - specifies the phonenumber to dial with. Defaults to NULL.
Return
    UTE_SUCCESS - successful dialup
    UTE_RAS_DIALINIT_ERROR - error when starting the dial process
        (use GetLastRASError for more details)
    UTE_RAS_DIAL_ERROR - error during the dial process. Use the
        GetDialState function to get more detailed information.
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs

***********************************************************/
int CUT_RAS::Dial(LPCTSTR szEntry,LPCTSTR szUserName, LPCTSTR szPassword,LPCTSTR szNumber){

    //check to see if the function is loaded
    if(!InitCheck((void**)&m_rasDialFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //set the static m_this pointer so that the static RasDialStatus function can
    //use member data, this is safe since the use of the RasDialStatus function
    //is only used during the lifetime of this function call
    RasDialStatus(0xFFFFFFFF,(RASCONNSTATE)0xFFFFFFFF,(ULONG)this);

    //hangup any existing connection
    HangUp();
    
    //reset the dial variables
    m_bDialAttemptComplete  = FALSE;
    m_bDialAttemptSuccess   = FALSE;
    
    // Reset the Cancel flag to FALSE.
    m_bRasCancelDial = FALSE;

    //setup the dial parameters
    RASDIALPARAMS rdp;
    memset(&rdp,0,sizeof(RASDIALPARAMS));
    rdp.dwSize = sizeof(RASDIALPARAMS);

    //load up a phonebook entry if given
    if(szEntry != NULL){
        //get the dial params, such as the phonenumber,username and password
        GetDialEntryParams(szEntry,&rdp);
        
        //get the phone number to dial (include the area code)
        TCHAR szPhone[RAS_MaxPhoneNumber+1];
        TCHAR szArea[RAS_MaxAreaCode+1];
        GetEntryPhoneNumber(szEntry,szPhone,RAS_MaxPhoneNumber,szArea,RAS_MaxAreaCode);
		if (szArea != NULL)
		{
			_tcscpy(rdp.szPhoneNumber,szArea);
			_tcscat(rdp.szPhoneNumber,szPhone);
		}
        
        }
    //load in the phonenumber if given
    if(szNumber != NULL)
        _tcsncpy(rdp.szPhoneNumber,szNumber,RAS_MaxPhoneNumber);

    //load the password if given
    if(szUserName != NULL)
        _tcsncpy(rdp.szUserName,szUserName,UNLEN); 
    //load the username if given
    if(szPassword != NULL)
        _tcsncpy(rdp.szPassword,szPassword,PWLEN); 

    //call the dial function
    m_nLastRASError = (m_rasDialFunc)(NULL,NULL,&rdp,0,RasDialStatus,&m_rasConn);

    if(m_nLastRASError == 0){

        //wait for the connection to succeed or fail
        //the RasDialStatus function will set the m_bDialAttemptComplete variable
        //also this function will exit on a WM_QUIT or WM_CLOSE message
        MSG msg;
        while(!m_bDialAttemptComplete  && !m_bRasCancelDial){
            if(PeekMessage(&msg, NULL, 0, 0,PM_NOREMOVE)){
                if(msg.message == WM_QUIT || msg.message == WM_CLOSE)
                     break;
                GetMessage(&msg, NULL, 0, 0);
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else{
                //switch to other threads if there is nothing to do
                Sleep(50);
            }
        }
    }

    //return success if m_bDialAttemptSuccess is TRUE
    if(m_bDialAttemptSuccess){
        return UTE_SUCCESS;
    }

    //if the dialup was not completely successful, ensure proper
    //closure of the connedtion
    HangUp();

    //return the appropriate error code
    if(m_nLastRASError != 0)
        return OnError(UTE_RAS_DIALINIT_ERROR);
    else
        return OnError(UTE_RAS_DIAL_ERROR);
}
/**********************************************************
DialEx
	This function is simular to the Dial but will not hang
	up an existing connection. Once called the existing connection
	will respond to all other function called, even if the 
	connection was not originated with this class.
	See the Dial function for more dialing details
Params
    szEntry - specifies the phonebook entry to dial. Defaults to NULL.
    szUserName - specifies the user name to use. Defaults to NULL.
    szPassword - specifies the password to use. Defaults to NULL.
    szNumber - specifies the phonenumber to dial with. Defaults to NULL.
Return
    UTE_SUCCESS - successful dialup
    UTE_RAS_DIALINIT_ERROR - error when starting the dial process
        (use GetLastRASError for more details)
    UTE_RAS_DIAL_ERROR - error during the dial process. Use the
        GetDialState function to get more detailed information.
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::DialEx(LPCTSTR szEntry,LPCTSTR szUserName, LPCTSTR szPassword,LPCTSTR szNumber){

    //check to see if the function is loaded
    if(!InitCheck((void**)&m_rasDialFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //set the static m_this pointer so that the static RasDialStatus function can
    //use member data, this is safe since the use of the RasDialStatus function
    //is only used during the lifetime of this function call
    RasDialStatus(0xFFFFFFFF,(RASCONNSTATE)0xFFFFFFFF,(ULONG)this);

    //check existing connection
	if(m_rasConn == NULL){
		//check for an existing connection
		if(EnumConnections() == UTE_SUCCESS){
			if(GetConnectionCount() >0){
				RASCONN rc;
				GetConnection(&rc,0);
				m_rasConn = rc.hrasconn;
				if(IsConnected()){
				    return UTE_SUCCESS;
				}
			}
		}
	}
	else{
		if(IsConnected()){
	        return UTE_SUCCESS;
		}
	}

	//hangup any existing connection
    HangUp();
    
    //reset the dial variables
    m_bDialAttemptComplete  = FALSE;
    m_bDialAttemptSuccess   = FALSE;
    
    // Reset the Cancel flag to FALSE.
    m_bRasCancelDial = FALSE;

    //setup the dial parameters
    RASDIALPARAMS rdp;
    memset(&rdp,0,sizeof(RASDIALPARAMS));
    rdp.dwSize = sizeof(RASDIALPARAMS);

    //load up a phonebook entry if given
    if(szEntry != NULL){
        //get the dial params, such as the phonenumber,username and password
        GetDialEntryParams(szEntry,&rdp);
        
        //get the phone number to dial (include the area code)
        TCHAR szPhone[RAS_MaxPhoneNumber+1];
        TCHAR szArea[RAS_MaxAreaCode+1];
        GetEntryPhoneNumber(szEntry,szPhone,RAS_MaxPhoneNumber,szArea,RAS_MaxAreaCode);
        _tcscpy(rdp.szPhoneNumber,szPhone);
        _tcscat(rdp.szPhoneNumber,szArea);

        }
    //load in the phonenumber if given
    if(szNumber != NULL)
        // strcpyn(rdp.szPhoneNumber,szNumber,RAS_MaxPhoneNumber); //??
        _tcsncpy(rdp.szPhoneNumber,szNumber,RAS_MaxPhoneNumber);

    //load the password if given
    if(szUserName != NULL)
        // strcpyn(rdp.szUserName,szUserName,UNLEN); //??
        _tcsncpy(rdp.szUserName,szUserName,UNLEN); 
    //load the username if given
    if(szPassword != NULL)
        // strcpyn(rdp.szPassword,szPassword,PWLEN); //??
        _tcsncpy(rdp.szPassword,szPassword,PWLEN); 

    //call the dial function
    m_nLastRASError = (m_rasDialFunc)(NULL,NULL,&rdp,0,RasDialStatus,&m_rasConn);

    if(m_nLastRASError == 0){

        //wait for the connection to succeed or fail
        //the RasDialStatus function will set the m_bDialAttemptComplete variable
        //also this function will exit on a WM_QUIT or WM_CLOSE message
        MSG msg;
        while(!m_bDialAttemptComplete  && !m_bRasCancelDial){
            if(PeekMessage(&msg, NULL, 0, 0,PM_NOREMOVE)){
                if(msg.message == WM_QUIT || msg.message == WM_CLOSE)
                     break;
                GetMessage(&msg, NULL, 0, 0);
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else{
                //switch to other threads if there is nothing to do
                Sleep(50);
            }
        }
    }

    //return success if m_bDialAttemptSuccess is TRUE
    if(m_bDialAttemptSuccess){
        return UTE_SUCCESS;
    }

    //if the dialup was not completely successful, ensure proper
    //closure of the connedtion
    HangUp();

    //return the appropriate error code
    if(m_nLastRASError != 0)
        return OnError(UTE_RAS_DIALINIT_ERROR);
    else
        return OnError(UTE_RAS_DIAL_ERROR);
}

/**********************************************************
IsConnected
    Checks the current state of the current connection.
    If the connection is still good then TRUE is returned.
    If the connection has been terminated then FALSE is 
    returned.
    This function is quite useful for monitoring a connection
    throughout its lifetime, since disconnections are quite
    common for a variety of reasons (ISP timeouts, voice 
    priority over data on busy phone lines, noisy phone line,
    etc.)
Params
    none
Return
    TRUE - if the current connection is still good
    FALSE - if the current connection has been terminated.
***********************************************************/
BOOL CUT_RAS::IsConnected(){

    //check to see if the function is loaded
    if(!InitCheck((void**)&m_rasGetConnectStatusFunc) )
        return FALSE;

    //init the connection status structure
    RASCONNSTATUS rcs;
    memset(&rcs,0,sizeof(RASCONNSTATUS));
    rcs.dwSize = sizeof(RASCONNSTATUS);

    //get the connection status
    m_nLastRASError = (m_rasGetConnectStatusFunc)(m_rasConn,&rcs);
    
    //return the connection status
    if(m_nLastRASError == 0 && rcs.rasconnstate == RASCS_Connected)
        return TRUE;
    return FALSE;
}
/**********************************************************
CancelDial()
    Cancels the current dial-up process that was initiated
    with the Dial function. This function is usually used
    inside of the Dial Callback message handler . see See 
    the SetDialStatusCallback   function for more details. 
    Once this function is called    the Dial function will 
    clean up and return.
Params
    None
Return
    None
***********************************************************/
void CUT_RAS::CancelDial(){

    m_bRasCancelDial = TRUE;
}

/**********************************************************
GetDialState
    Returns the current (or last) state of the dialup process.
    Useful for determining  the state of the dialup after the
    Dial function has completed.
Params
    None
Return
    The LOWORD contains the the current (or last) state of the
    dialup process, see RASCONNSTATE for a complete list
    The HIWORD contains the error code, the error code applies
    to the current state, see RASERROR.H for a complete list
***********************************************************/
DWORD CUT_RAS::GetDialState(){

    //return the dial state (set by the RasDialStatus function)
    return m_dwDialState;
}

/**********************************************************
HangUp
    Hangs up the current connection that was initiated with
    the Dial function. See the hangup function below for
    more details
Params
    None
Return 
    UTE_SUCCESS - successfully hung up the connection, or it
        was not currently connected
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
    UTE_RAS_HANDLE_ERROR - NULL RAS handle given
***********************************************************/
int CUT_RAS::HangUp(){

    //hang up the connection initiated by this class
    DWORD dwRVal = HangUp(m_rasConn);

    //if successful then set the connection hangle to NULL
    if(dwRVal == UTE_SUCCESS)
        m_rasConn = NULL;

    //return the error code from the call to the other HangUp
    return dwRVal;
}

/**********************************************************
HangUp
    Hangs up the specified ras connection, using the given
    RAS connection handle. RAS connection handles can be 
    retrieved using the EnumConnections function.
    Once the Hangup process starts this function will wait
    until the hangup is complete, times will vary depending
    on the modem (usually 1-3 seconds). If the hangup status
    cannot be monitored (on some older versions of Win95). Then
    a 4 second wait is applied to ensure a proper hangup.
Params
    rasConn - a RAS connection handle
Return 
    UTE_SUCCESS - successfully hung up the connection, or it
        was not currently connected
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
    UTE_RAS_HANDLE_ERROR - NULL RAS handle given
***********************************************************/
int CUT_RAS::HangUp(HRASCONN rasConn){

    //check to see if the function is loaded
    if(!InitCheck((void**)&m_rasHangUpFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //check to see if the handle is NULL
    if(rasConn == NULL)
        return UTE_RAS_HANDLE_ERROR;

    //start the hang up
    m_nLastRASError = (m_rasHangUpFunc)(rasConn);

    //wait until completely hung up
    if(m_rasGetConnectStatusFunc != NULL){
        RASCONNSTATUS rcs;
        memset(&rcs,0,sizeof(RASCONNSTATUS));
        rcs.dwSize = sizeof(RASCONNSTATUS);
        //wait until the connection handle has been deleted from the handle
        //list in RAS
        while( (m_rasGetConnectStatusFunc)(rasConn,&rcs) != ERROR_INVALID_HANDLE){
            if(rcs.rasconnstate == RASCS_Disconnected)
                break;
            Sleep(500);
        }
        //wait another half second to ensure a proper hangup (some slower modems
        //need this, usually DSP modems)
        Sleep(500);
    }
    else{
        //if the hangup cannot be monitored then wait 4 seconds. This time delay
        //is needed to ensure a proper hang up since proper hang-up monitoring failed
        Sleep(4000);
    }
    OnError(m_nLastRASError);

    return UTE_SUCCESS;
}

/**********************************************************
EnumDevices
    Enumerates all of the available RAS dialup devices and
    stores them in an internal array of RASDEVINFO structures.
    Once enumerated the data can be retrieved by using the
    GetDeviceCount and GetDevice functions. This function
    must be called before the others to succeed.
Params
    none
Return
    UTE_SUCCESS - success
    UTE_RAS_ENUM_ERROR - enumeration error
        (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::EnumDevices(){

    //check to see if the function is loaded
    if(!InitCheck((void**)&m_rasEnumDevicesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);
    
    //delete the data in the internal device structure array
    if(m_rdi != NULL)
        delete[] m_rdi;

    DWORD dwSize,dwCount;
    m_rdiCount = 0;

    //find the size of the array required
    dwSize = 0;
    (m_rasEnumDevicesFunc)(NULL,&dwSize,&dwCount);

    //create the new structure array
    dwCount = (dwSize / sizeof(RASDEVINFO)) + 1;
    m_rdi = new RASDEVINFO[dwCount];
    dwSize = sizeof(RASDEVINFO) * dwCount;
    m_rdi[0].dwSize = sizeof(RASDEVINFO);

    //get the entries
    m_nLastRASError = (m_rasEnumDevicesFunc)(m_rdi,&dwSize,&dwCount);
        
    if(m_nLastRASError!= 0){
        return OnError(UTE_RAS_ENUM_ERROR);
    }
    m_rdiCount = dwCount;
    return UTE_SUCCESS;
}

/**********************************************************
GetDeviceCount
    Returns the number of devices found during the enumeration.
    The EnumDevices, needs to be called before this function
    will return any valuable information.
Params
    none
Return
    number of devices found
    -1 on error
***********************************************************/
DWORD CUT_RAS::GetDeviceCount(){

    //error checking
    if(!m_bLoaded)
        return (DWORD)-1;

    if(m_rdi == NULL)
        return (DWORD)-1;

    return (DWORD)m_rdiCount;
}

/**********************************************************
GetDevice
    Fills in the given RASDEVINFO structure with the device
    information. RASDEVINFO contains the device type and
    name.
Params
    rdi - a pointer to an existing RASDEVINFO structure.
    index - a 0 based index ( 0 - (GetDeviceCount()-1) )
return 
    UTE_SUCCESS - success
    UTE_ERROR - error, no data to return
    UTE_INDEX_OUTOFRANGE - the given index does not point to 
        a valid entry
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::GetDevice(LPRASDEVINFO rdi,DWORD index){

    //error checking
    if(!m_bLoaded)
        return OnError(UTE_RAS_LOAD_ERROR);

    //check to see if the array have been filled
    if(m_rdi == NULL)
        return OnError(UTE_ERROR);

    //check to see if the index is valid
    if(index < 0 || index >= m_rdiCount)
        return OnError(UTE_INDEX_OUTOFRANGE);

    //copy the data
    memcpy(rdi,&m_rdi[index],sizeof(RASDEVINFO));

    return UTE_SUCCESS;
}

/**********************************************************
EnumConnections
    Enumerates all of the running RAS connections and
    stores them in an internal array of RASCONN structures.
    Once enumerated the data can be retrieved by using the
    GetConnectionCount and GetConnection functions. This 
    function must be called before the others to succeed.
Params
    none
Return
    UTE_SUCCESS - success
    UTE_RAS_ENUM_ERROR - enumeration error
        (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::EnumConnections(){

    //check to see if the function is loaded
    if(!InitCheck((void**)&m_rasEnumConnectionsFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //delete the old structure array data
    if(m_rc != NULL)
        delete[] m_rc;
    m_rc = NULL;
    
    DWORD dwSize = 0;
    DWORD dwSize2 = 0;
    DWORD dwCount = 0;
    m_rcCount = 0;

    //find the size of the array required
    m_rc = new RASCONN[1];
    dwSize = sizeof(RASCONN);
    dwSize2 = sizeof(RASCONN);
    m_rc[0].dwSize = sizeof(RASCONN);
   (m_rasEnumConnectionsFunc)(m_rc,&dwSize,&dwCount);

    if(m_rc != NULL){
        delete[] m_rc;
        m_rc = NULL;
    }

    if(dwCount > 0){
        m_rc = new RASCONN[dwCount];
        dwSize = sizeof(RASCONN) * dwCount;
        m_rc[0].dwSize = sizeof(RASCONN);

        //get the entries
        m_nLastRASError= (m_rasEnumConnectionsFunc)(m_rc,&dwSize,&dwCount);
        
        if(m_nLastRASError != 0){
            return OnError(UTE_RAS_ENUM_ERROR);
        }

        m_rcCount = dwCount;
    }
    return UTE_SUCCESS;
}

/**********************************************************
GetConnectionCount
    Returns the number of connections found during the enumeration.
    The EnumConnections, needs to be called before this function
    will return any valuable information.
Params
    none
Return
    number of connections found
    -1 on error
***********************************************************/
DWORD CUT_RAS::GetConnectionCount(){

    //error checking
    if(!m_bLoaded)
        return (DWORD)-1;

    if(m_rc == NULL)
        return (DWORD)-1;

    return (DWORD)m_rcCount;
}

/**********************************************************
GetConnection
    Fills in the given RASCONN structure with the connection
    information. RASCONN contains the phonebook entry name,
    and the device type and name.
Params
    rc - a pointer to an existing RASCONN structure.
    index - a 0 based index ( 0 - (GetConnectionCount()-1) )
return 
    UTE_SUCCESS - success
    UTE_ERROR - error, no data to return
    UTE_INDEX_OUTOFRANGE - the given index does not point to 
        a valid entry
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::GetConnection(LPRASCONN rc,DWORD index){

    //error checking
    if(!m_bLoaded)
        return OnError(UTE_RAS_LOAD_ERROR);
    
    //check to see if the array has been filled
    if(m_rc == NULL)
        return OnError(UTE_ERROR);

    //check to make sure that the index is valid
    if(index < 0 || index >= m_rcCount)
        return OnError(UTE_INDEX_OUTOFRANGE);

    //copy the data
    memcpy(rc,&m_rc[index],sizeof(RASCONN));

    return UTE_SUCCESS;
}

/**********************************************************
EnumEntries
    Enumerates all of the phonebook entry names (in the main
    phonebook, the version of this class only supports the
    main phonebook) and stores them in an internal array of 
    RASENTRYNAME structures.
    Once enumerated the data can be retrieved by using the
    GetEntryCount and GetEntry functions. This function must 
    be called before the others to succeed.
Params
    none
Return
    UTE_SUCCESS - success
    UTE_RAS_ENUM_ERROR - enumeration error
        (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::EnumEntries(){

    //check to see if the function is loaded
    if(!InitCheck((void**)&m_rasEnumEntriesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //delete the old structure array
    if(m_ren != NULL)
        delete[] m_ren;

    DWORD dwSize,dwCount = 0;
    m_renCount = 0;

    //find the size of the array required
    dwSize = sizeof(RASENTRYNAME);
    m_ren = new RASENTRYNAME[1];
    m_ren[0].dwSize = sizeof(RASENTRYNAME);
   (m_rasEnumEntriesFunc)(NULL,NULL,m_ren,&dwSize,&dwCount);

    delete[] m_ren;
    m_ren = NULL;

    //create the new structure array
    dwCount = (dwSize / sizeof(RASENTRYNAME)) +1;
    m_ren = new RASENTRYNAME[dwCount];
    dwSize = sizeof(RASENTRYNAME) * dwCount;
    m_ren[0].dwSize = sizeof(RASENTRYNAME);

    //get the entries
    m_nLastRASError = (m_rasEnumEntriesFunc)(NULL,NULL,m_ren,&dwSize,&dwCount);
    if(m_nLastRASError != 0){
        return OnError(UTE_RAS_ENUM_ERROR);
    }

    m_renCount = dwCount;
    return UTE_SUCCESS;
}

/**********************************************************
GetEntryCount
    Returns the number of phonebook entries found during the 
    enumeration. The EnumEntries, needs to be called before 
    this function will return any valuable information.
Params
    none
Return
    number of connections found
    -1 on error
***********************************************************/
DWORD CUT_RAS::GetEntryCount(){

    //error checking
    if(!m_bLoaded)
        return (DWORD)-1;

    if(m_ren == NULL)
        return (DWORD)-1;

    return m_renCount;
}

/**********************************************************
GetEntry
    Fills in the given RASENTRYNAME structure with the 
    phonebook entry name. RASENTRYNAME contains the phonebook 
    entry name,
Params
    ren - a pointer to an existing RASENTRYNAME structure.
    index - a 0 based index ( 0 - (GetEntryCount()-1) )
return 
    UTE_SUCCESS - success
    UTE_ERROR - error, no data to return
    UTE_INDEX_OUTOFRANGE - the given index does not point to 
        a valid entry
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::GetEntry(LPRASENTRYNAME ren,DWORD index){

    
    //error checking
    if(!m_bLoaded)
        return OnError(UTE_RAS_LOAD_ERROR);
    
    //check to see if there is data to return
    if(m_ren == NULL)
        return OnError(UTE_ERROR);

    //check to see if the index is valid
    if(index < 0 || index >= m_renCount)
        return OnError(UTE_INDEX_OUTOFRANGE);

    //copy the data
    memcpy(ren,&m_ren[index],sizeof(RASENTRYNAME));

    return UTE_SUCCESS;
}

/**********************************************************
SetDialStatusCallback
    Sets the window to retrieve messages during dialing (see 
    the Dial function). Messages are sent each time the dialing
    state changes. This function is useful for showing the user
    the current state within a window or dialog box.

Message Format
    wParam
        The LOWORD contains the the current state of the
        dialup process, see RASCONNSTATE for a complete list
        The HIWORD contains the error code, the error code applies
        to the current state, see RASERROR.H for a complete list
    lParam
        Points to a string buffer which describes the current state

Params
    hWnd - handle to the window that will receive the messages
    nMessageID - ID of the message to send
Return
    UTE_SUCCESS - success
***********************************************************/
int CUT_RAS::SetDialStatusCallback(HWND hWnd, int nMessageID){
    
    //store the callback message params
    m_hWndCallback = hWnd;
    m_nMessageID = nMessageID;

    return UTE_SUCCESS;
}

/**********************************************************
RasDialStatus (protected function)
    Once the Dial function has been called, this function 
    is called each time the dialup state changes. This function
    co-ordinates the dialup process with the Dial function.
    
    If the SetDialStatusCallback function has been set then
    this function will send a message to the specified window
    each time the state changes. See  the SetDialStatusCallback
    function for complete details on the format of the message.
***********************************************************/
VOID WINAPI CUT_RAS::RasDialStatus(UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError){

    static CUT_RAS* _this;
    LPSTR pBuf = "";

    //if special values are sent then dwError contains a pointer to the
    //current class instance
    if(unMsg == -1 && rasconnstate == -1){
        _this = (CUT_RAS*) dwError;
        return;
    }

    //set the dial state, low word contains success code
    //high word contains error code
    _this->m_dwDialState = MAKELONG(rasconnstate,dwError);


    //return a string for specific messages
  
    
    switch(rasconnstate){

    case RASCS_OpenPort :
            pBuf = "Opening Dialup Device";
             break;
        case RASCS_PortOpened:
            pBuf = "Dialup Device Opened";
             break;
        case RASCS_ConnectDevice:
            pBuf = "Dialing";
             break;
        case RASCS_DeviceConnected:
            pBuf = "Dial Complete";
             break;
        case RASCS_Authenticate:
            pBuf = "Authorizing";
             break;
        case RASCS_Authenticated:
            pBuf = "Authorized";
             break;
        case RASCS_Connected:
            pBuf = "Connection Complete";
             break;
        default:
            pBuf = ".....";
    }

    //send the message to the specified window
    if(_this->m_hWndCallback != NULL){
        SendMessage(_this->m_hWndCallback,_this->m_nMessageID,(WPARAM)_this->m_dwDialState,(LPARAM)pBuf);
    }

    //check to see if the dialup is complete
    if(dwError!= 0 || rasconnstate == RASCS_Connected){
        //success
        if(rasconnstate == RASCS_Connected){    
            _this->m_bDialAttemptSuccess = TRUE;
        }
        //error
        else{   
            _this->m_bDialAttemptSuccess = FALSE;
            // Ghazi to TROY 
        }
        _this->m_bDialAttemptComplete = TRUE;

    }
}

/**********************************************************
SetEntryProperties
    
    Creates or modifies a phonebook entry. If the given
    entry does not exist then a new one is created. If the
    entry already exists then it is overwritten.

    This function takes a pointer to a RASENTRY structure,
    which contains all of the required fields to make a
    complete phonebook entry except for the user name and
    password (See the SetDialEntryParams).

    NOTE: the RASENTRY structure can be followed by a set of 
    followed by an array of null-terminated alternate phone 
    number strings. The last string is terminated by two 
    consecutive null characters. The dwAlternateOffset member 
    of the RASENTRY structure contains the offset to the 
    first string. Set the nRasEntryLen to the total length
    of the given data. The nRasEntryLen has a default value
    of sizeof(RASENTRY).
    
    Use the ValidateEntryName to make sure that a new entry
    name is valid.

Params
    szEntryName - phonebook entry to add or update
    pRasEntry - pointer to a RASENTRY structure which contains
        the data for the entry to add or update. The dwSize 
        member of the structure must be set to sizeof(RASENTRY).
        Note: This can be followed by an optional list of 
        alternative phone numbers.
    nRasEntryLen - defaults to sizeof(RASENTRY). Can be set if 
        a list of alternative phone numbers are also available.

Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::SetEntryProperties(LPCTSTR szEntryName, LPRASENTRY pRasEntry,DWORD nRasEntryLen){

    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasSetEntryPropertiesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //Set the entry properties
    m_nLastRASError = (m_rasSetEntryPropertiesFunc)(NULL,szEntryName,  //RasSetEntryProperties
                    pRasEntry,nRasEntryLen,NULL,NULL);

    //return the error code
    if(m_nLastRASError == 0)
        return UTE_SUCCESS;
    return OnError(UTE_ERROR);
}
    
/**********************************************************
GetEntryProperties
    
    Retrieves the data for a given phonebook entry.
    
    This function takes a pointer to a RASENTRY structure
    which must exist. 

    NOTE: the RASENTRY structure can be followed by a set of 
    followed by an array of null-terminated alternate phone 
    number strings. The last string is terminated by two 
    consecutive null characters. The dwAlternateOffset member 
    of the RASENTRY structure contains the offset to the 
    first string. Set the nRasEntryLen to the total length
    of the given data buffer. If the buffer is too small then
    the function fails.

    To help ensure success, call the function with pRasEntry set
    to NULL and nRasEntryLen set to 0. If the function returns
    with a UTE_BUFFER_TOO_SHORT, then nRasEntryLen will contain
    the size of the buffer required. Allocate a buffer of the 
    required size and call the function again.

Params
    szEntryName - phonebook entry to retrieve
    pRasEntry - pointer to an already  allocated RASENTRY structure.
        The dwSize member of the structure must be set to 
        sizeof(RASENTRY).
    nRasEntryLen - size of the pRasEntry buffer.

Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_BUFFER_TOO_SHORT - supplied buffer too small
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::GetEntryProperties(LPCTSTR szEntryName, LPRASENTRY pRasEntry,DWORD* nRasEntryLen){

    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasGetEntryPropertiesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //Get the entry properties
    m_nLastRASError = (m_rasGetEntryPropertiesFunc)(NULL,szEntryName,
                    pRasEntry,nRasEntryLen,NULL,NULL);

    //return the error code
    if(m_nLastRASError == 0)
        return UTE_SUCCESS;
    if(m_nLastRASError == 603)
        return OnError(UTE_BUFFER_TOO_SHORT);
    return OnError(UTE_ERROR);
}

/**********************************************************
SetDialEntryParams
    Sets the dialing parameters for the given phonebook entry.
    The phonebook entry must exist for this function to 
    succeed.
    This function is required to store the username and 
    password for a phonebook entry. The RASDIALPARAMS also
    contains other optional information as well.
Params
    szEntryName - a phonebook entry name
    pRasDialParams - pointer to a RASDIALPARAMS structure
    bClearPassword - set to true to remove the password from
    the given entry, set to FALSE to set the password
Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_NULL_PARAM - pRasDialParams is NULL
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::SetDialEntryParams(LPCTSTR szEntryName,LPRASDIALPARAMS pRasDialParams, BOOL bClearPassword){

    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasSetEntryPropertiesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //check to see if the pointer is NULL
    if(pRasDialParams == NULL)
        return OnError(UTE_NULL_PARAM);

    //copy the entry name into the structure
    _tcscpy(pRasDialParams->szEntryName,szEntryName);

    //Set the dial params for the entry
    m_nLastRASError = (m_rasSetEntryDialParamsFunc)(NULL,pRasDialParams,bClearPassword);

    //return the error code
    if(m_nLastRASError == 0)
        return UTE_SUCCESS;
    return OnError(UTE_ERROR);
}

/**********************************************************
GetDialEntryParams
    retrieves  the dialing parameters for the given phonebook 
    entry.
    This function is required to retrieve  the username and 
    password for a phonebook entry. The RASDIALPARAMS also
    contains other information as well.
Params
    szEntryName - a phonebook entry name
    pRasDialParams - pointer to a RASDIALPARAMS structure
    bClearPassword - on return if TRUE then the password has
        been cleared, if FALSE then the password is valid.
Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_NULL_PARAM - pRasDialParams is NULL
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::GetDialEntryParams(LPCTSTR szEntryName,LPRASDIALPARAMS pRasDialParams, BOOL* bClearPassword){

    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasGetEntryPropertiesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //check to see if the pointer is NULL
    if(pRasDialParams == NULL)
        return OnError(UTE_NULL_PARAM);

    //copy the entry name into the structure
    _tcscpy(pRasDialParams->szEntryName,szEntryName);
    
    //Get the dial entry params
    BOOL bVal;
    m_nLastRASError = (m_rasGetEntryDialParamsFunc)(NULL,pRasDialParams,&bVal);

    //return the error code
    if(m_nLastRASError == 0){
        //if successful set the bClearPassword param
        if(bClearPassword != NULL)
            *bClearPassword = bVal;
        return UTE_SUCCESS;
    }
    return OnError(UTE_ERROR);
}

/**********************************************************
ClearEntryPassword
    Clears the password from a given phonebook entry. This
    operation can also be performed using the SetDialEntryParams
    function, but this function encapsulates the operation.
Params
    szEntryName - phonebook entry to delete password from
Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::ClearEntryPassword(LPCTSTR szEntryName){
    
    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasSetEntryPropertiesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    RASDIALPARAMS rdp;

    //clear the password
    memset(&rdp,0,sizeof(RASDIALPARAMS));
    rdp.dwSize = sizeof(RASDIALPARAMS);
    _tcscpy(rdp.szEntryName,szEntryName);
    m_nLastRASError = (m_rasSetEntryDialParamsFunc)(NULL,&rdp,TRUE);

    //return the error code
    if(m_nLastRASError == 0)
        return UTE_SUCCESS;
    return OnError(UTE_ERROR);
}

/**********************************************************
DoesEntryExist
    Checks to see if the given phonebook entry exists within
    the main RAS phonebook.
Params
    szEntryName - phonebook entry to check for.
Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::DoesEntryExist(LPCTSTR szEntryName){

    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasSetEntryPropertiesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //attempt to retrieve the EntryProperties
    DWORD dwSize =0;
    m_nLastRASError = (m_rasGetEntryPropertiesFunc)(NULL,szEntryName,
                    NULL,&dwSize,NULL,NULL);

    //if the function returns success or buffer too small then the
    //entry exists
    if(m_nLastRASError == 0 || m_nLastRASError == ERROR_BUFFER_TOO_SMALL){
        return UTE_SUCCESS;
    }
    return UTE_ERROR;
}


/**********************************************************
ValidateEntryName
    Checks to see if the given phonebook entry is a valid
    name for a new entry. This function will also fail if 
    the phonebook entry already exists.
Params
    szEntryName - phonebook entry to check.
Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::ValidateEntryName(LPCTSTR szEntryName){

    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasValidateEntryNameFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //call the RAS validate function
    m_nLastRASError = (m_rasValidateEntryNameFunc)(NULL,szEntryName);

    //return the error code
    if(m_nLastRASError == 0){
        return UTE_SUCCESS;
    }
    return OnError(UTE_ERROR);
}

/**********************************************************
DeleteEntry
    Deletes the given phonebook entry.
Params
    szEntryName - phonebook entry to delete.
Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::DeleteEntry(LPCTSTR szEntryName){

    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasDeleteEntryFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //delete the entry
    m_nLastRASError = (m_rasDeleteEntryFunc)(NULL,szEntryName);

    //return the error code
    if(m_nLastRASError == 0){
        return UTE_SUCCESS;
    }
    return OnError(UTE_ERROR);
}

/**********************************************************
RenameEntry
    Renames the given phonebook entry.
Params
    szEntryName - phonebook entry to rename
    szNewEntryName - new name of phonebook entry
Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::RenameEntry(LPCTSTR szEntryName,LPCTSTR szNewEntryName){

    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasRenameEntryFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //rename the entry
    m_nLastRASError = (m_rasRenameEntryFunc)(NULL,szEntryName,szNewEntryName);

    //return the error code
    if(m_nLastRASError == 0){
        return UTE_SUCCESS;
    }
    return OnError(UTE_ERROR);
}

/**********************************************************
GetEntryPhoneNumber
    Returns the main phone number and area code for the given
    phonebook entry. This function reduces the amount of work
    required to retrieve the numbers compared to using the 
    GetEntryProperties function directly.
Params
    szEntryName - phonebook entry to retrieve the phone number for
    szPhoneNumber - pointer to a string buffer that will retrieve
        phone number
    nPhoneNumberLen - length of the szPhoneNumber buffer
    szAreaCode - pointer to a string buffer that will retrieve
        area code
    nAreaCodeLen - length of the szAreaCode buffer
Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
***********************************************************/
int CUT_RAS::GetEntryPhoneNumber(LPCTSTR szEntryName, LPTSTR szPhoneNumber, long nPhoneNumberLen,
                                 LPTSTR szAreaCode, long nAreaCodeLen){

    //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasGetEntryPropertiesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    //get the size of the buffer required
    DWORD dwSize = 0;
    m_nLastRASError = (m_rasGetEntryPropertiesFunc)(NULL,szEntryName,
                    NULL,&dwSize,NULL,NULL);

    //create a buffer of the required size
    RASENTRY* re = (RASENTRY*) new BYTE[dwSize];
    memset(re,0,dwSize);
    re->dwSize = sizeof(RASENTRY);

    //request the entry properties
    m_nLastRASError = (m_rasGetEntryPropertiesFunc)(NULL,szEntryName,
                    re,&dwSize,NULL,NULL);

    //if successful then copy the numbers into the given buffers
    if(m_nLastRASError == 0){
        if(_tcslen(re->szLocalPhoneNumber) < (size_t)nPhoneNumberLen && szPhoneNumber != NULL){
            _tcscpy(szPhoneNumber,re->szLocalPhoneNumber);
        }
        if(_tcslen(re->szAreaCode) < (size_t)nAreaCodeLen && szAreaCode != NULL){
            _tcscpy(szAreaCode,re->szAreaCode);
        }
        //return success
        delete [] re;
        return UTE_SUCCESS;
    }
delete [] re;
    //return error
    return OnError(UTE_ERROR);
}
/*********************************************
GetEntryPassword
 Returns the main phone User Nam for the given
    phonebook entry. This function reduces the amount of work
    required to retrieve the numbers compared to using the 
    GetEntryProperties function directly.
Params
    szEntryName - phonebook entry to retrieve the User Nam for
    szUserName - pointer to a string buffer that will retrieve
        username
    nMaxLen - length of the szUserName buffer
Return
    UTE_SUCCESS - function was successful
    UTE_NULL_PARAM - The passed bufferd is NULL
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_BUFFER_TOO_SHORT - The username string is larger than the nMaxLen specified
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
*********************************************/
int CUT_RAS::GetEntryUserName(LPCTSTR szEntryName,LPTSTR szUserName, long nMaxLen){

        //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasSetEntryPropertiesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    RASDIALPARAMS rdp;
    BOOL clearPassword = FALSE;

    //clear the password
    memset(&rdp,0,sizeof(RASDIALPARAMS));
    rdp.dwSize = sizeof(RASDIALPARAMS);
    _tcscpy(rdp.szEntryName,szEntryName);
    m_nLastRASError = (m_rasGetEntryDialParamsFunc)(NULL,&rdp,&clearPassword);
    //return the error code
    if(m_nLastRASError == 0)
    { 
        if (szUserName == NULL)
        {
            return OnError(UTE_NULL_PARAM);
            
        }
        if (_tcslen(rdp.szUserName) > (size_t)nMaxLen)
        {
            return OnError(UTE_BUFFER_TOO_SHORT);
        }
        _tcscpy(szUserName , rdp.szUserName);
        return UTE_SUCCESS;
    }
    return OnError(m_nLastRASError);
    
}

/*********************************************
GetEntryPassword
 Returns the main phone password  for the given
    phonebook entry. This function reduces the amount of work
    required to retrieve the numbers compared to using the 
    GetEntryProperties function directly.
Params
    szEntryName - phonebook entry to retrieve the password for
    szPassword - pointer to a string buffer that will retrieve
        password
    nMaxLen - length of the szPassword buffer
Return
    UTE_SUCCESS - function was successful
    UTE_NULL_PARAM - The passed buffer  is NULL
    UTE_ERROR - function failed (use GetLastRASError for more details)
    UTE_BUFFER_TOO_SHORT - The password string is larger than the nMaxLen specified
    UTE_RAS_LOAD_ERROR - unable to load the RAS DLLs
*********************************************/
int CUT_RAS::GetEntryPassword(LPCTSTR szEntryName,LPTSTR szPassword, long nMaxLen){

        //check to see if the function has been mapped
    if(!InitCheck((void**)&m_rasSetEntryPropertiesFunc) )
        return OnError(UTE_RAS_LOAD_ERROR);

    RASDIALPARAMS rdp;

    //don't clear the password
    BOOL clearPassword = FALSE;
    
    memset(&rdp,0,sizeof(RASDIALPARAMS));
    rdp.dwSize = sizeof(RASDIALPARAMS);
    _tcscpy(rdp.szEntryName,szEntryName);
    m_nLastRASError = (m_rasGetEntryDialParamsFunc)(NULL,&rdp,&clearPassword);
    //return the error code
    if(m_nLastRASError == 0)
    { 
        if (szPassword == NULL)
        {
            return OnError(UTE_NULL_PARAM);
            
        }
        if (_tcslen(rdp.szPassword) > (size_t)nMaxLen)
        {
            return OnError(UTE_BUFFER_TOO_SHORT);
        }
        _tcscpy(szPassword , rdp.szPassword);
        return UTE_SUCCESS;
    }
    return OnError(m_nLastRASError);
    
}
/**********************************************************
GetLastRASError
    This function returns the error code from the last RAS
    function called. For a complete list of error codes, see
    the GetRASErrorString function.
    Calling this function is useful if extended error information
    is required.
Params
    none
Return
    Last RAS error code
***********************************************************/
DWORD CUT_RAS::GetLastRASError(){

    //return the last RAS error code
    return m_nLastRASError;
}

/**********************************************************
GetLastRASError
    This function returns the error string code corresponding to 
    the passed RAS error code   function called. For a complete list of error codes, see
    the RASERROR.H file.
    Calling this function is useful if extended error information
    is required.
Params
    errror - DWORD specifying the RAS error code
Return
    a string describing the error code
***********************************************************/
LPCTSTR  CUT_RAS::GetRASErrorString(DWORD error){
    
    LPTSTR pErrorStr = _T("");
    
    switch (error)
    {
    case PENDING: //(RASBASE+0)
        pErrorStr = _T(" An operation is pending. ");
            break;
    case ERROR_INVALID_PORT_HANDLE:// (RASBASE+1)
        pErrorStr = _T(" The port handle is invalid. ") ;
        break;
    case ERROR_PORT_ALREADY_OPEN: //(RASBASE+2)
        pErrorStr = _T(" The port is already open. ");
        break;
    case ERROR_BUFFER_TOO_SMALL :  //(RASBASE+3)
        pErrorStr = _T(" Caller's buffer is too small. ");
        break;
    case ERROR_WRONG_INFO_SPECIFIED : //(RASBASE+4)
        pErrorStr = _T(" Wrong information specified. ");
        break;
    case ERROR_CANNOT_SET_PORT_INFO  : //(RASBASE+5)
        pErrorStr = _T(" Cannot set port information. "); 
        break;
    case ERROR_PORT_NOT_CONNECTED :// /(RASBASE+6)
        pErrorStr = _T(" The port is not connected. ") ;
        break;
    case ERROR_EVENT_INVALID     : //(RASBASE+7)
        pErrorStr = _T(" The event is invalid. ") ;
        break;
    case ERROR_DEVICE_DOES_NOT_EXIST : //(RASBASE+8)
        pErrorStr = _T(" The device does not exist. ") ;
        break;
    case ERROR_DEVICETYPE_DOES_NOT_EXIST ://(RASBASE+9)
        pErrorStr = _T(" The device type does not exist. ") ;
        break;
    case ERROR_BUFFER_INVALID ://(RASBASE+10)
        pErrorStr = _T(" The buffer is invalid. ") ;
        break;
    case ERROR_ROUTE_NOT_AVAILABLE ://(RASBASE+11)
        pErrorStr = _T(" The route is not available. ") ;
        break;
    case ERROR_ROUTE_NOT_ALLOCATED  ://://(RASBASE+12)
        pErrorStr = _T(" The route is not allocated. ") ; 
        break;
    case ERROR_INVALID_COMPRESSION_SPECIFIED  ://://(RASBASE+13)
        pErrorStr = _T(" Invalid compression specified. ") ; 
        break;
    case ERROR_OUT_OF_BUFFERS    ://(RASBASE+14)
        pErrorStr = _T(" Out of buffers. ") ;
        break;
    case ERROR_PORT_NOT_FOUND    ://(RASBASE+15)
        pErrorStr = _T(" The port was not found. ") ; 
        break;
    case ERROR_ASYNC_REQUEST_PENDING ://(RASBASE+16)
        pErrorStr = _T(" An asynchronous request is pending. ") ;
        break;
    case ERROR_ALREADY_DISCONNECTING ://(RASBASE+17)
        pErrorStr = _T(" The port or device is already disconnecting. ") ;
        break;
    case ERROR_PORT_NOT_OPEN         ://(RASBASE+18)
        pErrorStr = _T(" The port is not open. ") ; 
        break;
    case ERROR_PORT_DISCONNECTED     ://(RASBASE+19)
        pErrorStr = _T(" The port is disconnected. ") ;
        break;
    case ERROR_NO_ENDPOINTS          ://(RASBASE+20)
        pErrorStr = _T(" There are no endpoints. ") ; 
        break;
    case ERROR_CANNOT_OPEN_PHONEBOOK ://(RASBASE+21)
        pErrorStr = _T(" Cannot open the phone book file. ") ; 
        break;
    case ERROR_CANNOT_LOAD_PHONEBOOK ://(RASBASE+22)
        pErrorStr = _T(" Cannot load the phone book file. ") ;
        break;
    case ERROR_CANNOT_FIND_PHONEBOOK_ENTRY    ://://(RASBASE+23)
        pErrorStr = _T(" Cannot find the phone book entry. ") ;
        break;
    case ERROR_CANNOT_WRITE_PHONEBOOK://(RASBASE+24)
        pErrorStr = _T(" Cannot write the phone book file. ") ;
        break;
    case ERROR_CORRUPT_PHONEBOOK     ://(RASBASE+25)
        pErrorStr = _T(" Invalid information found in the phone book file. ") ;
        break;
    case ERROR_CANNOT_LOAD_STRING    ://(RASBASE+26)
        pErrorStr = _T(" Cannot load a string. ") ; 
        break;
    case ERROR_KEY_NOT_FOUND         ://(RASBASE+27)
        pErrorStr = _T(" Cannot find key. ") ; 
        break;
    case ERROR_DISCONNECTION         ://(RASBASE+28)
        pErrorStr = _T(" The port was disconnected. ") ;
        break;
    case ERROR_REMOTE_DISCONNECTION  ://(RASBASE+29)
        pErrorStr = _T(" The data link was terminated by the remote machine. ") ;
        break;
    case ERROR_HARDWARE_FAILURE      ://(RASBASE+30)
        pErrorStr = _T(" The port was disconnected due to hardware failure. ") ; 
        break;
    case ERROR_USER_DISCONNECTION             ://(RASBASE+31)
        pErrorStr = _T(" The port was disconnected by the user. ") ;
        break;
    case ERROR_INVALID_SIZE                   ://(RASBASE+32)
        pErrorStr = _T(" The structure size is incorrect. ") ; 
        break;
    case ERROR_PORT_NOT_AVAILABLE             ://(RASBASE+33)
        pErrorStr = _T(" The port is already in use or is not configured for Remote Access dial out. ") ; 
        break;
    case ERROR_CANNOT_PROJECT_CLIENT          ://(RASBASE+34)
        pErrorStr = _T(" Cannot register your computer on on the remote network. ") ; 
        break;
    case ERROR_UNKNOWN                        ://(RASBASE+35)
        pErrorStr = _T(" Unknown error. ") ;
        break;
    case ERROR_WRONG_DEVICE_ATTACHED          ://(RASBASE+36)
        pErrorStr = _T(" The wrong device is attached to the port. ") ;
        break;
    case ERROR_BAD_STRING                     ://(RASBASE+37)
        pErrorStr = _T(" The string could not be converted. ") ;
        break;
    case ERROR_REQUEST_TIMEOUT                ://(RASBASE+38)
        pErrorStr = _T(" The request has timed out. ") ; 
        break;
    case ERROR_CANNOT_GET_LANA                ://(RASBASE+39)
        pErrorStr = _T(" No asynchronous net available. ") ;
        break;
    case ERROR_NETBIOS_ERROR                  ://(RASBASE+40)
        pErrorStr = _T(" A NetBIOS error has occurred. ") ;
        break;
    case ERROR_SERVER_OUT_OF_RESOURCES        ://(RASBASE+41)
        pErrorStr = _T(" The server cannot allocate NetBIOS resources needed to support the client. ") ; 
        break;
    case ERROR_NAME_EXISTS_ON_NET             ://(RASBASE+42)
        pErrorStr = _T(" One of your NetBIOS names is already registered on the remote network. ") ; 
        break;
    case ERROR_SERVER_GENERAL_NET_FAILURE     ://(RASBASE+43)
        pErrorStr = _T(" A network adapter at the server failed. ") ;
        break;
    case WARNING_MSG_ALIAS_NOT_ADDED          ://(RASBASE+44)
        pErrorStr = _T(" You will not receive network message popups. ") ; 
        break;
    case ERROR_AUTH_INTERNAL                  ://(RASBASE+45)
        pErrorStr = _T(" Internal authentication error. ") ;
        break;
    case ERROR_RESTRICTED_LOGON_HOURS         ://(RASBASE+46)
        pErrorStr = _T(" The account is not permitted to logon at this time of day. ") ; 
        break;
    case ERROR_ACCT_DISABLED                  ://(RASBASE+47)
        pErrorStr = _T(" The account is disabled. ") ; 
        break;
    case ERROR_PASSWD_EXPIRED                 ://(RASBASE+48)
        pErrorStr = _T(" The password has expired. ") ;
        break;
    case ERROR_NO_DIALIN_PERMISSION           ://(RASBASE+49)
        pErrorStr = _T(" The account does not have Remote Access permission. ") ;
        break;
    case ERROR_SERVER_NOT_RESPONDING          ://(RASBASE+50)
        pErrorStr = _T(" The Remote Access server is not responding. ") ;
        break;
    case ERROR_FROM_DEVICE                    ://(RASBASE+51)
        pErrorStr = _T(" Your modem (or other connecting device) has reported an error. ") ; 
        break;
    case ERROR_UNRECOGNIZED_RESPONSE          ://(RASBASE+52)
        pErrorStr = _T(" Unrecognized response from the device. ") ; 
        break;
    case ERROR_MACRO_NOT_FOUND                ://(RASBASE+53)
        pErrorStr = _T(" A macro required by the device was not found in the device .INF file section. ") ; 
        break;
    case ERROR_MACRO_NOT_DEFINED              ://(RASBASE+54)
        pErrorStr = _T(" A command or response in the device .INF file section refers to an undefined macro. ") ;
        break;
    case ERROR_MESSAGE_MACRO_NOT_FOUND        ://(RASBASE+55)
        pErrorStr = _T(" The <message> macro was not found in the device .INF file secion. ") ;
        break;
    case ERROR_DEFAULTOFF_MACRO_NOT_FOUND     ://(RASBASE+56)
        pErrorStr = _T(" The <defaultoff> macro in the device .INF file section contains an undefined macro. ") ; 
        break;
    case ERROR_FILE_COULD_NOT_BE_OPENED       ://(RASBASE+57)
        pErrorStr = _T(" The device .INF file could not be opened. ") ;
        break;
    case ERROR_DEVICENAME_TOO_LONG            ://(RASBASE+58)
        pErrorStr = _T(" The device name in the device .INF or media .INI file is too long. ") ; 
        break;
    case ERROR_DEVICENAME_NOT_FOUND           ://(RASBASE+59)
        pErrorStr = _T(" The media .INI file refers to an unknown device name. ") ; 
        break;
    case ERROR_NO_RESPONSES                   ://(RASBASE+60)
        pErrorStr = _T(" The device .INF file contains no responses for the command. ") ; 
        break;
    case ERROR_NO_COMMAND_FOUND               ://(RASBASE+61)
        pErrorStr = _T(" The device .INF file is missing a command. ") ; 
        break;
    case ERROR_WRONG_KEY_SPECIFIED            ://(RASBASE+62)
        pErrorStr = _T(" Attempted to set a macro not listed in device .INF file section. ") ; 
        break;
    case ERROR_UNKNOWN_DEVICE_TYPE            ://(RASBASE+63)
        pErrorStr = _T(" The media .INI file refers to an unknown device type. ") ; 
        break;
    case ERROR_ALLOCATING_MEMORY              ://(RASBASE+64)
        pErrorStr = _T(" Cannot allocate memory. ") ; 
        break;
    case ERROR_PORT_NOT_CONFIGURED            ://(RASBASE+65)
        pErrorStr = _T(" The port is not configured for Remote Access. ") ; 
        break;
    case ERROR_DEVICE_NOT_READY               ://(RASBASE+66)
        pErrorStr = _T(" Your modem (or other connecting device) is not functioning. ") ; 
        break;
    case ERROR_READING_INI_FILE               ://(RASBASE+67)
        pErrorStr = _T(" Cannot read the media .INI file. ") ; 
        break;
    case ERROR_NO_CONNECTION                  ://(RASBASE+68)
        pErrorStr = _T(" The connection dropped. ") ; 
		break;
    case ERROR_BAD_USAGE_IN_INI_FILE          ://(RASBASE+69)
        pErrorStr = _T(" The usage parameter in the media .INI file is invalid. ") ; 
        break;
    case ERROR_READING_SECTIONNAME            ://(RASBASE+70)
        pErrorStr = _T(" Cannot read the section name from the media .INI file. ") ; 
        break;
    case ERROR_READING_DEVICETYPE             ://(RASBASE+71)
        pErrorStr = _T(" Cannot read the device type from the media .INI file. ") ;
        break;
    case ERROR_READING_DEVICENAME             ://(RASBASE+72)
        pErrorStr = _T(" Cannot read the device name from the media .INI file. ") ; 
        break;
    case ERROR_READING_USAGE                  ://(RASBASE+73)
        pErrorStr = _T(" Cannot read the usage from the media .INI file. ") ; 
        break;
    case ERROR_READING_MAXCONNECTBPS          ://(RASBASE+74)
        pErrorStr = _T(" Cannot read the maximum connection BPS rate from the media .INI file. ") ;
        break;
    case ERROR_READING_MAXCARRIERBPS          ://(RASBASE+75)
        pErrorStr = _T(" Cannot read the maximum carrier BPS rate from the media .INI file. ") ;
        break;
    case ERROR_LINE_BUSY                      ://(RASBASE+76)
        pErrorStr = _T(" The line is busy. ") ; 
        break;
    case ERROR_VOICE_ANSWER                   ://(RASBASE+77)
        pErrorStr = _T(" A person answered instead of a modem. ") ;
        break;
    case ERROR_NO_ANSWER                      ://(RASBASE+78)
        pErrorStr = _T(" There is no answer. ") ;
        break;
    case ERROR_NO_CARRIER                     ://(RASBASE+79)
        pErrorStr = _T(" Cannot detect carrier. ") ; 
        break;
    case ERROR_NO_DIALTONE                    ://(RASBASE+80)
        pErrorStr = _T(" There is no dial tone. ") ; 
        break;
    case ERROR_IN_COMMAND                     ://(RASBASE+81)
        pErrorStr = _T(" General error reported by device. ") ; 
        break;
    case ERROR_WRITING_SECTIONNAME            ://(RASBASE+82)
        pErrorStr = _T("ERROR_WRITING_SECTIONNAME ") ;
        break;
    case ERROR_WRITING_DEVICETYPE             ://(RASBASE+83)
        pErrorStr = _T("ERROR_WRITING_DEVICETYPE ") ; 
        break;
    case ERROR_WRITING_DEVICENAME             ://(RASBASE+84)
        pErrorStr = _T("ERROR_WRITING_DEVICENAME ") ; 
        break;
    case ERROR_WRITING_MAXCONNECTBPS          ://(RASBASE+85)
        pErrorStr = _T("ERROR_WRITING_MAXCONNECTBPS ") ;
        break;
    case ERROR_WRITING_MAXCARRIERBPS          ://(RASBASE+86)
        pErrorStr = _T("ERROR_WRITING_MAXCARRIERBPS ") ;
        break;
    case ERROR_WRITING_USAGE                  ://(RASBASE+87)
        pErrorStr = _T("ERROR_WRITING_USAGE ") ; 
        break;
    case ERROR_WRITING_DEFAULTOFF             ://(RASBASE+88)
        pErrorStr = _T("ERROR_WRITING_DEFAULTOFF ") ; 
        break;
    case ERROR_READING_DEFAULTOFF             ://(RASBASE+89)
        pErrorStr = _T("ERROR_READING_DEFAULTOFF ") ; 
        break;
    case ERROR_EMPTY_INI_FILE                 ://(RASBASE+90)
        pErrorStr = _T("ERROR_EMPTY_INI_FILE ") ; 
        break;
    case ERROR_AUTHENTICATION_FAILURE         ://(RASBASE+91)
        pErrorStr = _T("Access denied because username and/or password is invalid on the domain. ") ; 
        break;
    case ERROR_PORT_OR_DEVICE                 ://(RASBASE+92)
        pErrorStr = _T("Hardware failure in port or attached device. ") ;
        break;
    case ERROR_NOT_BINARY_MACRO               ://(RASBASE+93)
        pErrorStr = _T("ERROR_NOT_BINARY_MACRO ") ; 
        break;
    case ERROR_DCB_NOT_FOUND                  ://(RASBASE+94)
        pErrorStr = _T("ERROR_DCB_NOT_FOUND ") ;
        break;
    case ERROR_STATE_MACHINES_NOT_STARTED     ://(RASBASE+95)
        pErrorStr = _T("ERROR_STATE_MACHINES_NOT_STARTED ") ; 
        break;
    case ERROR_STATE_MACHINES_ALREADY_STARTED ://(RASBASE+96)
        pErrorStr = _T("ERROR_STATE_MACHINES_ALREADY_STARTED ") ;
        break;
    case ERROR_PARTIAL_RESPONSE_LOOPING       ://(RASBASE+97)
        pErrorStr = _T("ERROR_PARTIAL_RESPONSE_LOOPING ") ; 
        break;
    case ERROR_UNKNOWN_RESPONSE_KEY           ://(RASBASE+98)
        pErrorStr = _T("A response keyname in the device .INF file is not in the expected format. ") ; 
        break;
    case ERROR_RECV_BUF_FULL                  ://(RASBASE+99)
        pErrorStr = _T("The device response caused buffer overflow. ") ;
        break;
    case ERROR_CMD_TOO_LONG                   ://(RASBASE+100)
        pErrorStr = _T("The expanded command in the device .INF file is too long. ") ;
        break;
    case ERROR_UNSUPPORTED_BPS                ://(RASBASE+101)
        pErrorStr = _T("The device moved to a BPS rate not supported by the COM driver. ") ;
        break;
    case ERROR_UNEXPECTED_RESPONSE            ://(RASBASE+102)
        pErrorStr = _T("Device response received when none expected. ") ;
        break;
    case ERROR_INTERACTIVE_MODE               ://(RASBASE+103)
        pErrorStr = _T("The Application does not allow user interaction. The connection requires interaction with the user to complete successfully.  ") ;
        break;
    case ERROR_BAD_CALLBACK_NUMBER            ://(RASBASE+104)
        pErrorStr = _T("ERROR_BAD_CALLBACK_NUMBER") ;
        break;
    case ERROR_INVALID_AUTH_STATE             ://(RASBASE+105)
        pErrorStr = _T("ERROR_INVALID_AUTH_STATE ") ; 
        break;
    case ERROR_WRITING_INITBPS                ://(RASBASE+106)
        pErrorStr = _T("ERROR_WRITING_INITBPS ") ;
        break;
    case ERROR_X25_DIAGNOSTIC                 ://(RASBASE+107)
        pErrorStr = _T("X.25 diagnostic indication. ") ;
        break;
    case ERROR_ACCT_EXPIRED                   ://(RASBASE+108)
        pErrorStr = _T("The account has expired. ") ; 
        break;
    case ERROR_CHANGING_PASSWORD              ://(RASBASE+109)
        pErrorStr = _T("Error changing password on domain.  The password may be too short or may match a previously used password. ") ;
        break;
    case ERROR_OVERRUN                        ://(RASBASE+110)
        pErrorStr = _T("Serial overrun errors were detected while communicating with your modem. ") ; 
        break;
    case ERROR_RASMAN_CANNOT_INITIALIZE      ://(RASBASE+111)
        pErrorStr = _T("RasMan initialization failure.  Check the event log. ") ;
        break;
    case ERROR_BIPLEX_PORT_NOT_AVAILABLE      ://(RASBASE+112)
        pErrorStr = _T("Biplex port initializing.  Wait a few seconds and redial. ") ; 
        break;
    case ERROR_NO_ACTIVE_ISDN_LINES           ://(RASBASE+113)
        pErrorStr = _T("No active ISDN lines are available. ") ; 
        break;
    case ERROR_NO_ISDN_CHANNELS_AVAILABLE     ://(RASBASE+114)
        pErrorStr = _T("No ISDN channels are available to make the call. ") ;
        break;
    case ERROR_TOO_MANY_LINE_ERRORS           ://(RASBASE+115)
        pErrorStr = _T("Too many errors occurred because of poor phone line quality. ") ;
        break;
    case ERROR_IP_CONFIGURATION               ://(RASBASE+116)
        pErrorStr = _T("The Remote Access IP configuration is unusable. ") ; 
        break;
    case ERROR_NO_IP_ADDRESSES                ://(RASBASE+117)
        pErrorStr = _T("No IP addresses are available in the static pool of Remote Access IP addresses. ") ;
        break;
    case ERROR_PPP_TIMEOUT                    ://(RASBASE+118)
        pErrorStr = _T("Timed out waiting for a valid response from the remote PPP peer. ") ;
        break;
    case ERROR_PPP_REMOTE_TERMINATED          ://(RASBASE+119)
        pErrorStr = _T("PPP terminated by remote machine. ") ; 
        break;
    case ERROR_PPP_NO_PROTOCOLS_CONFIGURED    ://(RASBASE+120)
        pErrorStr = _T("No PPP control protocols configured. ");
        break;
    case ERROR_PPP_NO_RESPONSE                ://(RASBASE+121)
        pErrorStr = _T("Remote PPP peer is not responding. ") ; 
        break;
    case ERROR_PPP_INVALID_PACKET             ://(RASBASE+122)
        pErrorStr = _T("The PPP packet is invalid. ") ; 
        break;
    case ERROR_PHONE_NUMBER_TOO_LONG          ://(RASBASE+123)
        pErrorStr = _T(" The phone number including prefix and suffix is too long. ") ;
        break;
    case ERROR_IPXCP_NO_DIALOUT_CONFIGURED    ://(RASBASE+124)
        pErrorStr = _T(" The IPX protocol cannot dial-out on the port because the machine is an IPX router. ") ; 
        break;
    case ERROR_IPXCP_NO_DIALIN_CONFIGURED     ://(RASBASE+125)
        pErrorStr = _T(" The IPX protocol cannot dial-in on the port because the IPX router is not installed. ") ; 
        break;
    case ERROR_IPXCP_DIALOUT_ALREADY_ACTIVE   ://(RASBASE+126)
        pErrorStr = _T(" The IPX protocol cannot be used for dial-out on more than one port at a time. ") ;
        break;
    case ERROR_ACCESSING_TCPCFGDLL            ://(RASBASE+127)
        pErrorStr = _T("Cannot access TCPCFG.DLL. ") ; 
        break;
    case ERROR_NO_IP_RAS_ADAPTER              ://(RASBASE+128)
        pErrorStr = _T("Cannot find an IP adapter bound to Remote Access. ") ;
        break;
    case ERROR_SLIP_REQUIRES_IP               ://(RASBASE+129)
        pErrorStr = _T("SLIP cannot be used unless the IP protocol is installed. ") ;
        break;
    case ERROR_PROJECTION_NOT_COMPLETE        ://(RASBASE+130)
        pErrorStr = _T("Computer registration is not complete. ") ; 
        break;
    case ERROR_PROTOCOL_NOT_CONFIGURED        ://(RASBASE+131)
        pErrorStr = _T("The protocol is not configured. ") ; 
        break;
    case ERROR_PPP_NOT_CONVERGING             ://(RASBASE+132)
        pErrorStr = _T("The PPP negotiation is not converging. ") ; 
        break;
    case ERROR_PPP_CP_REJECTED                ://(RASBASE+133)
        pErrorStr = _T("The PPP control protocol for this network protocol is not available on the server. ") ;
        break;
    case ERROR_PPP_LCP_TERMINATED             ://(RASBASE+134)
        pErrorStr = _T("The PPP link control protocol terminated. ") ; 
        break;
    case ERROR_PPP_REQUIRED_ADDRESS_REJECTED  ://(RASBASE+135)
        pErrorStr = _T("The requested address was rejected by the server. ") ;
        break;
    case ERROR_PPP_NCP_TERMINATED             ://(RASBASE+136)
        pErrorStr = _T("The remote computer terminated the control protocol. ") ; 
        break;
    case ERROR_PPP_LOOPBACK_DETECTED          ://(RASBASE+137)
        pErrorStr = _T("Loopback detected. ") ;
        break;
    case ERROR_PPP_NO_ADDRESS_ASSIGNED        ://(RASBASE+138)
        pErrorStr = _T("The server did not assign an address. ") ;
        break;
    case ERROR_CANNOT_USE_LOGON_CREDENTIALS   ://(RASBASE+139)
        pErrorStr = _T("The authentication protocol required by the remote server cannot use the Windows NT encrypted password.  Redial, entering the password explicitly. ") ;
        break;
    case ERROR_TAPI_CONFIGURATION             ://(RASBASE+140)
        pErrorStr = _T("Invalid TAPI configuration. ") ; 
        break;
    case ERROR_NO_LOCAL_ENCRYPTION            ://(RASBASE+141)
        pErrorStr = _T("The local computer does not support the required encryption type. ") ; 
        break;
    case ERROR_NO_REMOTE_ENCRYPTION           ://(RASBASE+142)
        pErrorStr = _T("The remote computer does not support the required encryption type. ") ; 
        break;
    case ERROR_REMOTE_REQUIRES_ENCRYPTION     ://(RASBASE+143)
        pErrorStr = _T("The remote computer requires encryption. ") ; 
        break;
    case ERROR_IPXCP_NET_NUMBER_CONFLICT      ://(RASBASE+144)
        pErrorStr = _T("Cannot use the IPX network number assigned by remote server.  Check the event log. ") ; 
        break;
    case ERROR_INVALID_SMM                    ://(RASBASE+145)
        pErrorStr = _T("ERROR_INVALID_SMM ") ;
        break;
    case ERROR_SMM_UNINITIALIZED              ://(RASBASE+146)
        pErrorStr = _T("ERROR_SMM_UNINITIALIZED ") ; 
        break;
    case ERROR_NO_MAC_FOR_PORT                ://(RASBASE+147)
        pErrorStr = _T("ERROR_NO_MAC_FOR_PORT ") ; 
        break;
    case ERROR_SMM_TIMEOUT                    ://(RASBASE+148)
        pErrorStr = _T("ERROR_SMM_TIMEOUT ") ; 
        break;
    case ERROR_BAD_PHONE_NUMBER               ://(RASBASE+149)
        pErrorStr = _T("ERROR_BAD_PHONE_NUMBER ") ;
        break;
    case ERROR_WRONG_MODULE                   ://(RASBASE+150)
        pErrorStr = _T("ERROR_WRONG_MODULE ") ; 
        break;
    case ERROR_INVALID_CALLBACK_NUMBER        ://(RASBASE+151)
        pErrorStr = _T("Invalid callback number.  Only the characters 0 to 9, T, P, W, (, ), -, @, and space are allowed in the number. ") ;
        break;
    case ERROR_SCRIPT_SYNTAX                  ://(RASBASE+152)
        pErrorStr = _T("A syntax error was encountered while processing a script. ") ;
        break;
    case ERROR_HANGUP_FAILED                  ://(RASBASE+153)
        pErrorStr = _T("The connection could not be disconnected because it was created by the Multi-Protocol Router. ") ; 
        break;
    default :
        pErrorStr = _T("");
        
    }
    return pErrorStr;
}


/**********************************************************
RAS events
***********************************************************/
#ifdef UTRAS_ENABLE_EVENTS

/**********************************************************
EnableDisconnectEvent
	Enables or disables the disconnect detection callback.
	To use a new class needs to be derived from this one
	and the OnDisconnected virtual function needs to be
	overridden.
Params
	bEnable - TRUE to enable, FALSE to disable
Return
    UTE_SUCCESS - function was successful
    UTE_ERROR - function failed (use GetLastRASError for more details)
***********************************************************/
int CUT_RAS::EnableDisconnectEvent(BOOL bEnable){
	
	if(bEnable){
		if(m_disconnectThreadID == (DWORD)-1){

			m_bDisconnectEventReady = FALSE;

			m_disconnectThreadID = _beginthread(DisconnectEventThread,0,(VOID *)this);
			
			if (m_disconnectThreadID == (DWORD)-1){
				return OnError(UTE_ERROR);
			}
		}
	}
	else{
		if(m_disconnectThreadID == (DWORD)-1)
			return UTE_SUCCESS;

		m_bDisconnectThreadShutdown = TRUE;
		
		for(int loop = 0; loop < 25;loop++){
			if(m_disconnectThreadID == -1)
				break;
			Sleep(200);
		}
	}

	return UTE_SUCCESS;
}

/**********************************************************
DisconnectEventThread
	This function is only envoked if disconnect events are enabled.
	The function monitors the connection status and fires the
	disconnect event (OnDisconnected) upon detection of a disconnect.
	This method of detection is used instead of the build in event
	for a couple reasons; the build in event is not supported under
	all Windows OSs, plus the event may be fired multiple times for
	a single disconnect.
Params
	ptr - pointer to the class that created the thread
Return
	none
***********************************************************/
void CUT_RAS::DisconnectEventThread(void *ptr){

	CUT_RAS* _this = (CUT_RAS*) ptr;

	while(_this->m_bDisconnectThreadShutdown == FALSE){
		//check to see if the connection is active
		if(_this->m_rasConn != NULL){
			if( _this->IsConnected() == TRUE){
				_this->m_bDisconnectEventReady = TRUE;	//only enable once connected successfully
			}
			else if(_this->m_bDisconnectEventReady){ 
				_this->OnDisconnected();
				_this->m_bDisconnectEventReady = FALSE; //ensures only one event per connection
			}
		}
		Sleep(1000);
	}
	
	_this->m_disconnectThreadID = -1;
}

/**********************************************************
OnDisconnected
	This is the virtual function that gets called upon
	a connection disconnect, if the disconnect event is
	enabled (EnableDisconnectEvent)
Params
	none
Return
	none
***********************************************************/
void CUT_RAS::OnDisconnected(){
	
}

/**********************************************************
RAS events
***********************************************************/
#endif

#pragma warning (pop)
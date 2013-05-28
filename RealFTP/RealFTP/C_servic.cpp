// =================================================================
//  class: CUService
//  File:  C_servic.cpp
//  
//  Purpose:
//
//  Implementation of CUService to easily create NT services 
//  This class deals with a Microsoft specific operating system
//  facility. Hence much of the comments in this class may refrence 
//  (and we do encourage referring to) the MSDN Platform SDK on line help
//  for additional information
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

//MFC Users - include the following
#include "stdafx.h"
//OWL Users - include the following
//#include <owl\owlpch.h>
//SDK Users - include the following
#include <windows.h>

#include <process.h>
#include <tchar.h>
#include "c_servic.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

//===================================================================
// BACKGROUND and TERMS used in the comment of this class
//What is an NT Service ?
//      A service in Microsoft(R) Windows NT
//      is a program that runs whenever the computer 
//      is running the operating system. It does not
//      require a user to be logged on. Services are needed 
//      to perform user-independent tasks such as directory 
//      replication, process monitoring, or services to other 
//      machines on a network, such as support for the Internet HTTP protocol
// WHAT is SCM?
//      The Service Control Manager (SCM) is the component of the COM 
//      Library responsible for locating class implementations and running them. 
//      The SCM ensures that when a client request is made, the appropriate server 
//      is connected and ready to receive the request.
//      The SCM keeps a database of class information based on
//      the system registry that the client caches locally through the COM library
//SEE MSDN (Platform SDK)  for more information
//======================================================================

CUService * CUServ; // global variable to be used with threads
/******************************************
Constructor
*******************************************/
CUService::CUService(){

    //service variables
    m_ServiceName               = NULL;
    m_ExitEvent                 = NULL;
    m_PauseService              = FALSE;
    m_RunningService            = FALSE;
    m_ThreadHandle              = 0;
    m_EntryFunction             = NULL;
    m_PauseFunction             = NULL;
    m_ResumeFunction            = NULL;
    m_StopFunction              = NULL;

    //time-out variables
    m_StartTimeOut              = 10000;
    m_StopTimeOut               = 15000;
    m_PauseTimeOut              = 5000;
    m_ResumeTimeOut             = 5000;

    //install variables
    m_dwDesiredAccess           = SERVICE_ALL_ACCESS;
    m_dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    m_dwStartType               = SERVICE_AUTO_START;
    m_dwErrorControl            = SERVICE_ERROR_NORMAL;

    m_szLoadOrderGroup          = NULL;
    m_lpdwTagID                 = NULL;
    m_szDependencies            = NULL;


}
/******************************************
Destructor
*******************************************/
CUService::~CUService(){

    if(m_ServiceName != NULL)
        delete[] m_ServiceName;
}

/******************************************
InitService()
    Registers the main service thread with
    the service manager.
    Connects the main thread of a service 
    process to the service control manager,
    which causes the thread to be the service 
    control dispatcher thread for the calling process
Parameters:
    name          - internal service name
    EntryFunction - pointer to the main programs entry
                    function for when the service is started
Return value:
    If the function succeeds, the return value is nonzero
    Otherwise use GetLastError()
*******************************************/
long CUService::InitService(LPCTSTR name,void * EntryFunction){

    CUServ = this;
    m_EntryFunction = (CUENTRY)EntryFunction;
    m_ServiceName = new _TCHAR[_tcslen(name)+1];
    _tcscpy(m_ServiceName,name);

    SERVICE_TABLE_ENTRY ste[] ={
        { (_TCHAR *)name,(LPSERVICE_MAIN_FUNCTION) ServiceMain} ,{ NULL, NULL }};

    // Register with the SCM
    return StartServiceCtrlDispatcher(ste);
}
/*********************************************
SetCallBacks()
    The Service Control Handler function 
    will dispatch each request comming to the service
    from the SCM to the default or  user defined
    functions set by this function for the the following 
    notifications
        SERVICE_CONTROL_STOP
        SERVICE_CONTROL_PAUSE
        SERVICE_CONTROL_CONTINUE
    There are several cases you may want to provide
    your own implementation of these function. One of these cases is 
    writing to your service's custom log to inform you of who started,
    paused or stoped the service.
Parameters:
    void *pause  - the address of the function that will handle the 
        SERVICE_CONTROL_PAUSE notification
    void *resume - the address of the function that will handle the 
        SERVICE_CONTROL_CONTINUE notification
    void *stop - the address of the function that will handle the 
        SERVICE_CONTROL_STOP
Return value:
    VOID
***********************************************/
void CUService::SetCallBacks(void *pause,void *resume,void *stop){
    m_PauseFunction     = (CUENTRY)pause;
    m_ResumeFunction    = (CUENTRY)resume;
    m_StopFunction      = (CUENTRY)stop;
}

/******************************************
ServiceMain()
    This is the thread that is called from the
    service manager to start the service and set the 
    Entry point to handle the the Service Control
    Program(SCP, e.g Control Panel Applet)'s requests
    Internaly sets the Service Control Handler function which 
    will dispatch each request for the service to the default or 
    user defined functions set by the call to CUService::SetCallBacks()
Return value: 
    VOID    
*******************************************/
VOID CUService::ServiceMain(DWORD , LPTSTR *){

    BOOL success;
    // immediately call Registration function
    CUServ->m_ServiceStatusHandle = RegisterServiceCtrlHandler(
            CUServ->m_ServiceName,(LPHANDLER_FUNCTION )CUService::ServiceCtrlHandler);

    if (!CUServ->m_ServiceStatusHandle){
        CUServ->Exit(GetLastError());
        return;
    }

    // Notify SCM of progress
    success = CUServ->SCMStatus(SERVICE_START_PENDING,NO_ERROR, 0, 1, 8000);
    if (!success){
        CUServ->Exit(GetLastError());
        return;
    }

    // create the termination event
    CUServ->m_ExitEvent = CreateEvent (0, TRUE, FALSE,0);
    if (!CUServ->m_ExitEvent){
        CUServ->Exit(GetLastError());
        return;
    }

    // Notify SCM of progress
    success = CUServ->SCMStatus(    SERVICE_START_PENDING,NO_ERROR, 0, 3, CUServ->m_StartTimeOut);
    if (!success){
        CUServ->Exit(GetLastError());
        return;
    }

    // Start the service itself
    success = CUServ->OnStartService();
    if (!success){
        CUServ->Exit(GetLastError());
        return;
    }

    // The service is now running.
    // Notify SCM of progress
    success = CUServ->SCMStatus(SERVICE_RUNNING,NO_ERROR, 0, 0, 0);
    if (!success){
        CUServ->Exit(GetLastError());
        return;
    }

    // Wait for stop signal, and then exit
    WaitForSingleObject (CUServ->m_ExitEvent, INFINITE);

    CUServ->Exit(0);
}

/******************************************
OnStartService
    Called when the service is to be started
Parameters:
    none
Return value:
    TRUE  - successful
    FALSE - fail
*******************************************/
long CUService::OnStartService(){

    // Start the service's thread
    m_ThreadHandle = (HANDLE) _beginthread((CUENTRY)m_EntryFunction,0,(VOID *)this);

    if (m_ThreadHandle==0)
        return FALSE;
    else
    {
        m_RunningService = TRUE;
        return TRUE;
    }
}
/******************************************
OnStopService()
    A default implementation function to handle 
    the SERVICE_CONTROL_STOP command
    To provide your own implementation call CUService::SetCallBacks
Parameters:
    NONE
Return value:
    VOID
*******************************************/
void CUService::OnStopService(){
    
    m_RunningService=FALSE;

    if(m_StopFunction != NULL){
        DWORD thread;
        thread =  _beginthread((CUENTRY)m_StopFunction,0,(VOID *)this);
        if(thread != -1)
            WaitForSingleObject((HANDLE)thread,m_StopTimeOut);
    }
    
    WaitForSingleObject(m_ThreadHandle,m_StopTimeOut);

    // Set the event that is holding ServiceMain
    // so that ServiceMain can return
    SetEvent(m_ExitEvent);
}

/******************************************
OnPauseService()
    A default implementation function to handle 
    the SERVICE_CONTROL_PAUSE command
    To provide your own implementation call CUService::SetCallBacks
Parameters:
    NONE
Return value:
    VOID
*******************************************/
void CUService::OnPauseService(){

    if(m_PauseFunction != NULL){
        DWORD thread;
        thread =  _beginthread((CUENTRY)m_PauseFunction,0,(VOID *)this);
        if(thread != -1)
            WaitForSingleObject((HANDLE)thread,m_PauseTimeOut);
   }

    m_PauseService = TRUE;
    SuspendThread(m_ThreadHandle);
}

/******************************************
OnResumeService()
    A default implementation function to handle 
    the SERVICE_CONTROL_CONTINUE command
    To provide your own implementation call CUService::SetCallBacks
Parameters:
    NONE
Return value:
    VOID
******************************************/
void CUService::OnResumeService(){

    if(m_ResumeFunction != NULL){
        DWORD thread;
        thread =  _beginthread((CUENTRY)m_ResumeFunction,0,(VOID *)this);
        if(thread != -1)
            WaitForSingleObject((HANDLE)thread,m_ResumeTimeOut);
   }

    m_PauseService = FALSE;
    ResumeThread(m_ThreadHandle);
}

/******************************************
SCMStatus ()
    Update the  SERVICE_STATUS structure and 
    Inform the SCM of the status of the service.
    SERVICE_STATUS structure contains seven members that 
    reflect the current status of the service.
    All of these members must be set correctly before 
    passing the structure to SetServiceStatus.
    m_ServiceStatusHandle will be used to keep track of these 
    memebers.
Parameters:
    DWORD dwCurrentState -
        Indicates the current state of the 
        service. One of the following values is specified.
            SERVICE_STOPPED The service is not running. 
            SERVICE_START_PENDING The service is starting. 
            SERVICE_STOP_PENDING The service is stopping. 
            SERVICE_RUNNING The service is running. 
            SERVICE_CONTINUE_PENDING The service continue is pending. 
            SERVICE_PAUSE_PENDING The service pause is pending. 
            SERVICE_PAUSED The service is paused. 
    DWORD dwWin32ExitCode -
        Specifies an Win32 error code that the service uses 
        to report an error that occurs when it is starting or stopping.
    DWORD dwServiceSpecificExitCode -
        Specifies a service specific error code that the service
        returns when an error occurs while the service is 
        starting or stopping
    DWORD dwCheckPoint -
        Specifies a value that the service increments periodically
        to report its progress during a lengthy start, stop, 
        pause, or continue operation.
    DWORD dwWaitHint -
        Specifies an estimate of the amount of time, in 
        milliseconds, that the service expects a pending start,
        stop, pause, or continue operation to take before the service
        makes its next call to the SetServiceStatus function with either
        an incremented dwCheckPoint value or a change in dwCurrentState.
        SEE ALSO MSDN (SERVICE_STATUS)
RETUTN
    BOOL - TRUE if success false other wise
*******************************************/
BOOL CUService::SCMStatus (DWORD dwCurrentState,DWORD dwWin32ExitCode,
            DWORD dwServiceSpecificExitCode,DWORD dwCheckPoint,DWORD dwWaitHint){

    BOOL success;
    SERVICE_STATUS serviceStatus;

    // Fill in all of the SERVICE_STATUS fields
    serviceStatus.dwServiceType =       SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwCurrentState =  dwCurrentState;

    // If in the process of something, then accept
    // no control events, else accept anything
    if (dwCurrentState == SERVICE_START_PENDING)
        serviceStatus.dwControlsAccepted = 0;
    else
        serviceStatus.dwControlsAccepted =  SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;

    // if a specific exit code is defines, set up
    // the win32 exit code properly
    if (dwServiceSpecificExitCode == 0)
        serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
    else
        serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;

    serviceStatus.dwServiceSpecificExitCode =   dwServiceSpecificExitCode;

    serviceStatus.dwCheckPoint = dwCheckPoint;
    serviceStatus.dwWaitHint = dwWaitHint;

    // Pass the status record to the SCM
    success = SetServiceStatus (m_ServiceStatusHandle, &serviceStatus);

    if (!success)
        OnStopService();

    return success;
}

/********************************************************
    The Service Control Handler function which 
    will dispatch each request comming to the service
    from the SCM to the default or  user defined
    functions set by the call to CUService::SetCallBacks()
    And inform the SCM of the current status of the service
	Parameters:
    DWORD controlCode - SCM request or command can be one of 
	following:
	SERVICE_CONTROL_STOP
	SERVICE_CONTROL_PAUSE
	SERVICE_CONTROL_CONTINUE
	Return value:
    VOID        
*********************************************************/
VOID CUService::ServiceCtrlHandler(DWORD controlCode){
	
    DWORD  currentState = 0;
	
	// v4.2 enable exception handling if desired
//	try
//	{
		switch(controlCode){
			//AfxMessageBox("SerivceCtrlHandeler");
			// There is no START option because
			// ServiceMain gets called on a start
			
			// Stop the service
        case SERVICE_CONTROL_STOP:
			
            currentState = SERVICE_STOP_PENDING;
            
            // Tell the SCM what's happening
            CUServ->SCMStatus(currentState,NO_ERROR, 0, 1, CUServ->m_StopTimeOut);
            
            // Stop the service
            CUServ->OnStopService();
            return;
			
			// Pause the service
        case SERVICE_CONTROL_PAUSE:
			
            if (CUServ->m_RunningService && ! CUServ->m_PauseService){
				
                currentState = SERVICE_PAUSED;
                
                // Tell the SCM what's happening
                CUServ->SCMStatus(SERVICE_PAUSE_PENDING,NO_ERROR, 0, 1, CUServ->m_PauseTimeOut);
                CUServ->OnPauseService();
            }
            break;
			
			// Resume from a pause
        case SERVICE_CONTROL_CONTINUE:
			
            if (CUServ->m_RunningService && CUServ->m_PauseService){
				
                currentState = SERVICE_RUNNING;
				
                // Tell the SCM what's happening
                CUServ->SCMStatus(SERVICE_CONTINUE_PENDING,NO_ERROR, 0, 1, CUServ->m_ResumeTimeOut);
                CUServ->OnResumeService();
            }
            break;
			
		//supu -- Interogates the service when dependencies or other things are checked.
		case SERVICE_CONTROL_INTERROGATE:
			if ((currentState = CUServ->InterogateService(CUServ->m_ServiceName)) != 0)
			{
				CUServ->SCMStatus(currentState, NO_ERROR,0, 1, 0);
				return ;
			}
			break;			
		}
		
		//default
		CUServ->SCMStatus(currentState, NO_ERROR,0, 0, 0);
//	}
//	catch(CException *e)
//	{
//		e= NULL ;
//		e->Delete();
//	}	
}

/******************************************
Exit()
    Service Exit point 
Parameters:
    Error - Error code 
Return value:
    VOID
*******************************************/
VOID CUService::Exit(DWORD error){

    // if ExitEvent has been created, close it.
    if (m_ExitEvent)
        CloseHandle(m_ExitEvent);

    // Send a message to the scm to tell about
    // stopage
    if (m_ServiceStatusHandle)
        SCMStatus(SERVICE_STOPPED, error,0, 0, 0);

    // If the thread has started kill it off
    if (m_ThreadHandle)
        CloseHandle(m_ThreadHandle);

}

/******************************************
SetStartTimeOut()
    Set the time estimated for the start operation 
    to be completed
Parameters:
    milisec - estimated time in miliseconds
Return value:
    the startTime as confirmation
*******************************************/
int  CUService::SetStartTimeOut(int milisec){
    m_StartTimeOut = milisec;

    return m_StartTimeOut;
}

/******************************************
SetStopTimeOut()
    Set the time estimated for the stop operation 
    to be completed
Parameters:
    milisec - estimated time in miliseconds
Return value:
    the stop Time as confirmation
*******************************************/
int  CUService::SetStopTimeOut(int milisec){
    m_StopTimeOut = milisec;

    return m_StopTimeOut;
}

/******************************************
SetPauseTimeOut()
    Set the time estimated for the stop operation 
    to be completed
Parameters:
    milisec - estimated time in miliseconds
Return value:
    the pause Time as confirmation
*******************************************/
int  CUService::SetPauseTimeOut(int milisec){
    m_PauseTimeOut = milisec;

    return m_PauseTimeOut;
}

/******************************************
SetResumeTimeOut()
    Set the time estimated for the resume operation 
    to be completed
Parameters:
    milisec - estimated time in miliseconds
RETURN
    the resume Time as confirmation
*******************************************/
int  CUService::SetResumeTimeOut(int milisec){
    m_ResumeTimeOut = milisec;

    return m_ResumeTimeOut;
}

/********************************************
SetInstallOptions()
    Sets the Install options of the service
    to be used when a new service is created
Parameters:
    DWORD dwDesiredAccess - type of access to service can be
        one and/or any of the following
        STANDARD_RIGHTS_REQUIRED 
        SERVICE_ALL_ACCESS
        SERVICE_CHANGE_CONFIG
        SERVICE_ENUMERATE_DEPENDENTS
        SERVICE_INTERROGATE
        SERVICE_PAUSE_CONTINUE
        SERVICE_QUERY_CONFIG
        SERVICE_QUERY_STATUS
        SERVICE_START
        SERVICE_STOP
        SERVICE_USER_DEFINED_CONTROL
    DWORD dwServiceType - type of service can be one of the following
        SERVICE_WIN32_OWN_PROCESS
        SERVICE_WIN32_SHARE_PROCESS
        SERVICE_KERNEL_DRIVER
        SERVICE_FILE_SYSTEM_DRIVER
    DWORD dwStartType - Specifies when to start 
            the service must be one of the following
        SERVICE_BOOT_START 
        SERVICE_SYSTEM_START
        SERVICE_AUTO_START
        SERVICE_DEMAND_START
        SERVICE_DISABLED
    DWORD dwErrorControl - Specifies the severity 
            of the error if this service fails to 
            start during startup, and determines 
            the action taken by the startup program 
            if failure occurs. You must specify one 
            of the following error control flags
        SERVICE_ERROR_IGNORE
        SERVICE_ERROR_NORMAL
        SERVICE_ERROR_SEVERE
        SERVICE_ERROR_CRITICAL
    SEE MSDN (Platform SDK)  for more information
Return value:
    TRUE
*********************************************/
int CUService::SetInstallOptions(DWORD dwDesiredAccess, DWORD dwServiceType,
                    DWORD dwStartType,DWORD dwErrorControl){

    m_dwDesiredAccess   = dwDesiredAccess;
    m_dwServiceType     = dwServiceType;
    m_dwStartType       = dwStartType;
    m_dwErrorControl    = dwErrorControl;

    return TRUE;
}
/********************************************
SetInstallOptions()
    Sets the Install options of the service
    to be used when a new service is created
Parameters:
     LPSTR szLoadOrderGroup -
         Pointer to a null-terminated string that
         names the load ordering group of which
         this service is a member. Specify NULL 
         or an empty string if the service does 
         not belong to a group
     LPDWORD lpdwTagID  - 
         Pointer to a DWORD variable that receives
         a tag value that is unique in the group 
         specified in the lpLoadOrderGroup parameter.
         Specify NULL if you are not changing the existing tag
         Tags are only evaluated for driver services 
         that have SERVICE_BOOT_START or SERVICE_SYSTEM_START 
         start types
     LPSTR szDependencies
         Pointer to a double null-terminated array of
         null-separated names of services or load 
         ordering groups that the system must start before 
         this service. Specify NULL or an empty string if 
         the service has no dependencies. Dependency on a 
         group means that this service can run if at least 
         one member of the group is running after an attempt 
         to start all members of the group. You must prefix 
         group names with SC_GROUP_IDENTIFIER so that they 
         can be distinguished from a service name, because 
         services and service groups share the same name space
Return value:
    TRUE
*********************************************/
int CUService::SetInstallOptions(LPTSTR szLoadOrderGroup,LPDWORD lpdwTagID,
                    LPTSTR szDependencies){

    m_szLoadOrderGroup  = szLoadOrderGroup;
    m_lpdwTagID         = lpdwTagID;
    m_szDependencies    = szDependencies;

    return TRUE;
}
/********************************************
InstallService()
    Creates a service object and adds it to 
    the service control manager database
Parameters:
    LPCSTR szInternName - Service Internal name
    LPCSTR szDisplayName - Service display name 
    LPCSTR szFullPath - Full path of the service program
    LPCSTR szAccountName - User account name (Default = NULL)
    LPCSTR szPassword - User password (Default = NULL)
  Return value:
        0   success
        1   Service manager failed to open
        2  Failed to create service
*********************************************/
int CUService::InstallService(LPCTSTR szInternName,LPCTSTR szDisplayName,LPCTSTR szFullPath,
        LPCTSTR szAccountName,LPCTSTR szPassword){

    SC_HANDLE newService, scm;
    int rt = 0;

    // open a connection to the SCM
    scm = OpenSCManager(0, 0,SC_MANAGER_CREATE_SERVICE);
	if (!scm){
        return 1;
    }

   // Install the new service

    newService = CreateService(
        scm,
        szInternName,               //internal name
        szDisplayName,              //display name
        m_dwDesiredAccess,        //desired access def: SERVICE_ALL_ACCESS
        m_dwServiceType,          //service type def: SERVICE_WIN32_OWN_PROCESS
        m_dwStartType,            //start type def: SERVICE_AUTOSTART
        m_dwErrorControl,         //error control def: SERVICE_ERROR_NORMAL
        szFullPath,                 // exec name and full path
        m_szLoadOrderGroup,       //load order group def: none
        m_lpdwTagID,              //group tag ID def: none
        m_szDependencies,         //dependencies def: none
        szAccountName,          // user account name def:Local System
        szPassword);            // user account password

    if (!newService){
        rt =2;
    }
    else{
        // clean up
        CloseServiceHandle(newService);
    }

    // clean up
    CloseServiceHandle(scm);

    return rt;
}
/********************************************
RemoveService()
    Opens up the SCM database and locate the requested 
    service then Marks the specified service for deletion
    from the service control manager database
Parameters:
     LPCSTR szInternName - Internal name of 
        the service to be removed from the SCM
Return value:
        0           success
        1           failed to open service manager
        2           failed to locate the specified service
        3           failed to delete service
*********************************************/
int CUService::RemoveService(LPCTSTR szInternName){

    SC_HANDLE service, scm;
    int  rt;
   SERVICE_STATUS ss;

    // open a connection to the SCM
    scm = OpenSCManager(0, 0,SC_MANAGER_ALL_ACCESS);
    if (!scm){
        return 1;
    }

   //open the service
    service = OpenService(scm,szInternName,SERVICE_ALL_ACCESS|DELETE );
    if(service==NULL){
        MessageBox(NULL,_T("Failed to Locate Installed Service"),_T("Error"),MB_OK);
        // clean up
        CloseServiceHandle(scm);
        return 2;
    }

   ControlService(service,SERVICE_CONTROL_STOP,&ss);
    //delete the service
    if(DeleteService(service))
        rt = 0;
    else
        rt = 3;

    // clean up
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

   return rt;
}


DWORD CUService::InterogateService(LPCTSTR szInternName)
{
    SC_HANDLE service, scm;
    DWORD  rt = 0;
	_SERVICE_STATUS ss;

    // open a connection to the SCM
    scm = OpenSCManager(0, 0,SC_MANAGER_ALL_ACCESS);
    if (!scm){
        return FALSE;
    }

   //open the service
    service = OpenService(scm,szInternName,SERVICE_ALL_ACCESS | SERVICE_INTERROGATE );
    if(service==NULL){
        MessageBox(NULL,_T("Failed to Locate Installed Service"),_T("Error"),MB_OK);
        // clean up
        CloseServiceHandle(scm);
        return FALSE;
    }

	if (!QueryServiceStatus(service,&ss))
	{
		CloseServiceHandle(service);
		CloseServiceHandle(scm);
		return FALSE;
	}

	rt = ss.dwCurrentState ;

	// clean up
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

   return rt;
}

#pragma warning(pop)
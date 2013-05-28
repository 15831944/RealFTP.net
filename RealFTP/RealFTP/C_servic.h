// ===================================================================
//  class: CUService
//  File:  C_servic.h
//  
//  Purpose:
//
//  Class declaration of service class for creation of NT services 
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

#ifndef C_SERVICE_H
#define C_SERVICE_H

typedef void (* CUENTRY)(void *);

//=================================================================
//  class: CUService

class CUService{

    private:
        // the service's name.
        // The string has a maximum length of 256 characters. 
        LPTSTR                  m_ServiceName;  

        // Handle to an event so we can set it later to be flagged
        // for latter use with WaitForXobject() functions
        HANDLE                  m_ExitEvent;

        //====================================
        //install variables
        //desired access def: SERVICE_ALL_ACCESS
        DWORD m_dwDesiredAccess;
        
        //service type def: SERVICE_WIN32_OWN_PROCESS
        DWORD m_dwServiceType;
        
        // start type def: SERVICE_AUTOSTART
        DWORD m_dwStartType;

        //error control def: SERVICE_ERROR_NORMAL
        DWORD m_dwErrorControl;

        //load order group def: none
        LPTSTR   m_szLoadOrderGroup;

        //group tag ID def: none
        LPDWORD m_lpdwTagID;

          //dependencies def: none
        LPTSTR   m_szDependencies;
 
        // The SERVICE_STATUS structure contains seven members that 
        // reflect the current status of the service.
        // All of these members must be set correctly before 
        // passing the structure to SetServiceStatus.
        // m_ServiceStatusHandle will be used to keep track of these 
        // memebers
        SERVICE_STATUS_HANDLE   m_ServiceStatusHandle;

        // Flag if the Service is currently paused or not
        BOOL                    m_PauseService;

        // Flag if the Service is currently running or not
        BOOL                    m_RunningService;

        // THread handle 
        HANDLE                  m_ThreadHandle;

        // Pointer to the entry point 
        CUENTRY                 m_EntryFunction;

        // Pointer to the Pause function
        CUENTRY                 m_PauseFunction;

        // Pointer to the Resume function
        CUENTRY                 m_ResumeFunction;

        // Pointer to the Stop function
        CUENTRY                 m_StopFunction;

        // Service start time out def= 10000
        int m_StartTimeOut;
        // Service stop time out def. = 15000
        int m_StopTimeOut;
        // Service pause time out  def. = 5000
        int m_PauseTimeOut;
        // Servicwe Resume time out def. = 5000
        int m_ResumeTimeOut;
    
        //============================================
        // service control manager Call back functions

        // handles  SERVICE_CONTROL_PAUSE from the service control manager  or SCP
        // or the NET.EXE command (e.g. NET PAUSE servicname)
    
        void OnPauseService();

        // handles  SERVICE_CONTROL_CONTINUE from the service control manager  or SCP
        // or the NET.EXE commands (e.g. NET CONTINUE servicname)
    
        void OnResumeService();

        // handles  SERVICE_CONTROL_STOP from the service control manager  or SCP
        // or the NET.EXE commands (e.g. NET STOP servicname)
    
        void OnStopService();

        // Start the service (e.g. NET START servicname)
    
        long OnStartService();

        // Service main entry 
    
        static VOID ServiceMain(DWORD argc, LPTSTR *argv);

        // update service status to the SCM 

        BOOL SCMStatus (DWORD dwCurrentState,DWORD dwWin32ExitCode,
            DWORD dwServiceSpecificExitCode,DWORD dwCheckPoint,DWORD dwWaitHint);

        // Entry point to handle the the Service Control Program
        // (SCP, e.g Control Panel Applet)'s requests
    
        static VOID ServiceCtrlHandler (DWORD controlCode);

        // Service Exit point
    
        VOID Exit(DWORD error);

    public:
	    DWORD InterogateService(LPCTSTR szInternName);
        CUService();
        virtual ~CUService();

        // Registers the main service thread with
        // the service manager

        long InitService(LPCTSTR name,void * EntryFunction);
    
        void SetCallBacks(void *pause,void *resume,void *stop);
      
        // service timeout settings

        // Set the time to wait for a successful start

        int  SetStartTimeOut(int milisec);
    
        // Set the time to wait for a successful stop
        
        int  SetStopTimeOut(int milisec);
    
        // Set the time to wait for a successful Pause
        
        int  SetPauseTimeOut(int milisec);
    
        // Set the time to wait for a successful Resume
        
        int  SetResumeTimeOut(int milisec);
    
        // service install / un-install

        // modify the install options (1)
        
        int SetInstallOptions(DWORD dwDesiredAccess, DWORD dwServiceType,
                    DWORD dwStartType,DWORD dwErrorControl);

        // modify the install options (2)
        
        int SetInstallOptions(LPTSTR szLoadOrderGroup,LPDWORD lpdwTagID,
                    LPTSTR szDependencies);

        int InstallService(LPCTSTR szInternName,LPCTSTR szDisplayName,LPCTSTR szFullPath,
                    LPCTSTR szAccountName = NULL ,LPCTSTR szPassword = NULL);

       int RemoveService(LPCTSTR szInternName);

};
#endif
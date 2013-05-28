#include "StdAfx.h"
#include "RunCommand.h"
//#include "Logger.h"

/**
* run
*
* Description: runs a command and waits for it to return.
*
* Parameters:
*		CString &command. String command to run.
*/
bool RunCommand::run(CString &command, DWORD &retCode, CString &error)
{
	bool result = true;

	//Logger l;
	//l.log(CString(_T("Run command: ")) + command);

	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
	DWORD code;
	
	memset(&StartupInfo, 0, sizeof(StartupInfo));
	StartupInfo.cb = sizeof(STARTUPINFO);
	//StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	//StartupInfo.wShowWindow = SW_HIDE;  // don't hide windows - created deadlock on gui installers.

	if (!CreateProcess( NULL, command.AllocSysString(), NULL, NULL, TRUE,
		CREATE_NEW_CONSOLE, 
		NULL, 
		NULL,
		&StartupInfo,
		&ProcessInfo))
	{
		DWORD err = GetLastError();
		char buffer[32];
		ltoa((long)err, buffer, 10);
		/*l.webLog(CString(_T("")), CString(_T("error")), 
				CString(_T("installer run cmd process")), 
				command + 
				CString(_T(" code: ")) +
				CString(buffer) );*/
		error = error + CString(_T(" CreateProcess() failed with code: ")) + CString(buffer);
		result = false;	
	}

	WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
	if(!GetExitCodeProcess(ProcessInfo.hProcess, &code)){
		DWORD err = GetLastError();
		char buffer[32];
		ltoa((long)err, buffer, 10);
		//l.webLog(CString(_T("")), CString(_T("error")), CString(_T("installer run process get exit code failed")), CString(buffer) );
		error = error + CString(_T(" GetExitCodeProcess() failed with code: ")) + CString(buffer);
		code = 0;
		result = false;
	}

	if(code != 0){
		//char buffer[32];
		//ltoa((long)code, buffer, 10);
		//l.webLog(CString(_T("")), CString(_T("error")), CString(_T("installer run process returned error ")), CString(buffer) );
	}
	retCode = code;

	CloseHandle(ProcessInfo.hThread);
	CloseHandle(ProcessInfo.hProcess);

	return result;
}

/**
* run
*
* Description: runs a command and returns.
*
* Parameters:
*		CString &command. String command to run.
*/
bool RunCommand::runAndForget(CString &command)
{
	bool result = true;

	//Logger l;
	//l.log(CString(_T("Run and forget command: ")) + command);

	STARTUPINFO StartupInfo;
	//PROCESS_INFORMATION ProcessInfo;
	ULONG rc;
	
	memset(&StartupInfo, 0, sizeof(StartupInfo));
	StartupInfo.cb = sizeof(STARTUPINFO);
	//StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	//StartupInfo.wShowWindow = SW_HIDE;  // don't hide windows - created deadlock on gui installers.

	if (!CreateProcess(NULL, command.AllocSysString(), NULL, NULL, TRUE,
		CREATE_NEW_CONSOLE, 
		NULL, 
		NULL,
		&StartupInfo,
		&mProcessInfo))
	{
		DWORD err = GetLastError();
		char buffer[32];
		ltoa((long)err, buffer, 10);
		/*l.webLog(CString(_T("")), CString(_T("error")), 
				CString(_T("installer run cmd process")), 
				command + 
				CString(_T(" code: ")) +
				CString(buffer), 1 );
		*/
		result = false;		
	}

	//::Sleep(50);

	return result;
}

bool RunCommand::waitForCommandToFinish(DWORD &retCode, CString &error){
	bool result = true;
	DWORD code;

	WaitForSingleObject(mProcessInfo.hProcess, INFINITE);
	if(!GetExitCodeProcess(mProcessInfo.hProcess, &code)){
		DWORD err = GetLastError();
		char buffer[32];
		ltoa((long)err, buffer, 10);
		//l.webLog(CString(_T("")), CString(_T("error")), CString(_T("installer run process get exit code failed")), CString(buffer) );
		error = error + CString(_T(" GetExitCodeProcess() failed with code: ")) + CString(buffer);
		code = 0;
		result = false;
	}

	retCode = code;

	CloseHandle(mProcessInfo.hThread);
	CloseHandle(mProcessInfo.hProcess);

	return result;
}


/**
* This is a bad function to use.
*
*/
bool RunCommand::shellExec(CString &command, CString &arguments)
{
	bool result = true;
	//Logger l;
	SHELLEXECUTEINFO shExInfo = {0};
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	shExInfo.lpVerb = _T("runas");                // Operation to perform
	shExInfo.lpFile = command.AllocSysString();       // Application to start    
	shExInfo.lpParameters = arguments.AllocSysString();   // Additional parameters
	shExInfo.lpDirectory = 0;
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;  

	if (ShellExecuteEx(&shExInfo))
	{
		WaitForSingleObject(shExInfo.hProcess, INFINITE);
		CloseHandle(shExInfo.hProcess);
		//CloseHandle(shExInfo.hThread);
	} else {
		DWORD err = GetLastError();
		char buffer[32];
		ltoa((long)err, buffer, 10);
		/*l.webLog(CString(_T("")), CString(_T("error")), 
				CString(_T("installer shellExec process")), 
				command + 
				CString(_T(" code: ")) +
				CString(buffer), 1 );*/
		result = false;
	}
	return result;
}


/**
* system - DEPRICATED
*
* Description: Runs command by the system method. Displays a command window.
*				
*/
bool RunCommand::systemCommand(CString &command)
{
	bool result = true;
	int i = 0;
	if (system(NULL)){ 
		// Convert to char * for execution.
		char * cProgramFile;
		int wlen = wcslen(command.AllocSysString());
		cProgramFile = (char *)malloc( wlen + 1 );
		wtc((char *)cProgramFile, command.AllocSysString());
		
		i = system(cProgramFile);
		
		free(cProgramFile);
		char resultBuff[10];
		itoa(i, resultBuff, 10);
		if(i == 1){
			// Error
			result = false;
		} else {
			// Ok
		}			
	}
	return result;
}

/*
* wtc
* Converts wide tchar * (unicode) to char *. 
*/
void RunCommand::wtc(char* to, wchar_t* from)
{
	size_t n = 999999;
	for(size_t i = 0; i < n; i++)
	{
		if(from[i] != 0){
			to[i] = from[i];
		} else {
			n = 0; // done loop
			to[i] = 0; // terminate string
		}
	}
}

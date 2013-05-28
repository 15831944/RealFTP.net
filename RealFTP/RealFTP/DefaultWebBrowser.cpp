#include "StdAfx.h"
#include "DefaultWebBrowser.h"
#include <AtlBase.h>				// atl -> registry access
//#include "Logger.h"
#include "RunCommand.h"


/**
* launch
* 
* Default: Launch a default web browser to a given URL.
*			Query the registry to find out what browser is default, 
*			Add the given url to the command argument then
*			run it using a command.
*
* Parameters:
*			CString &url. Given a reference to a cstring containing the url to load.
*/
void DefaultWebBrowser::launch(CString &url)
{
	bool ieIsDefault = true;
	CRegKey defaultBrowserKey;
	long defaultKeyError = defaultBrowserKey.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice"), KEY_READ);
	if(defaultKeyError == ERROR_SUCCESS)
    {
		CString sDefaultPath, sPathEntry = _T("Progid"); 
		DWORD dwBufferSize = MAX_PATH;
		defaultBrowserKey.QueryValue( sDefaultPath.GetBuffer( dwBufferSize ), sPathEntry, &dwBufferSize );
		sDefaultPath.ReleaseBuffer();
		if(sDefaultPath.Compare(_T("IE.HTTP")) != 0){
			ieIsDefault = false;
		}
		defaultBrowserKey.Close();
	}

	if(!ieIsDefault){
		CRegKey key;
		long nError = key.Open(HKEY_CLASSES_ROOT, _T("http\\shell\\open\\command"), KEY_READ);
		if(nError == ERROR_SUCCESS)
		{
			CString sDefaultPath, sPathEntry = _T(""); 
			DWORD dwBufferSize = MAX_PATH;
			key.QueryValue( sDefaultPath.GetBuffer( dwBufferSize ), sPathEntry, &dwBufferSize );
			sDefaultPath.ReleaseBuffer();
		
			TRACE(_T(" default browser found  %s \n"), sDefaultPath.AllocSysString()  );

			if(sDefaultPath.Replace(_T("%1"), url) == 0){				// replace %1 with argument
				sDefaultPath = sDefaultPath + CString(_T(" ") + url);	// %1 does not exist (IE browser) append
			}

			RunCommand cmd;
			//cmd.run(sDefaultPath);
			cmd.runAndForget(sDefaultPath);

			key.Close();
		} else {
			ieIsDefault = true;
		}
	}

	if(ieIsDefault) {
		// No default browser defined. Use IE? 
		ShellExecute(NULL, _T("open"), url.AllocSysString(), NULL, NULL, SW_SHOWNORMAL);
	}
}
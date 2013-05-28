#include "StdAfx.h"
#include "Upgrade.h"
#include "WebGrab.h"
#include "Version.h"
#include "resource.h"
#include "DownloadManager.h"
#include "RunCommand.h"

CUpgrade::CUpgrade(void)
{
}

CUpgrade::~CUpgrade(void)
{
}



void CUpgrade::CheckForUpgrade()
{
	THREADPARAM * _param = new THREADPARAM;
	//_param->parameters.insert(std::pair<CString,CString>( CString(_T("c")), clickID));
	
	CWinThread *m_pCheckForUpgradeThread = AfxBeginThread(CheckForUpgradeThread, _param); 
	if (m_pCheckForUpgradeThread != NULL)
	{
	} 
}


/**
* CheckForUpgradeThread
*
* Description: 
*/
UINT CUpgrade::CheckForUpgradeThread(LPVOID pParam)
{
	THREADPARAM* ts = (THREADPARAM*)pParam;
	CString strBuffer;
	CWebGrab grab;
	CString webUrl(_T("http://realftp.net/app/version.php"));
	CString current_version(_T(""));
	CGlobalFunctions::GetFileVersionX(current_version);

	if (grab.GetFile(webUrl, strBuffer, _T("RealFTP"), NULL)) {
		if(current_version.Compare(strBuffer) != 0)
		{
			// Notify new version.
			PostMessage( AfxGetMainWnd()->GetSafeHwnd(), ID_UPGRADE_AVAILABLE, 0, 0);
		}
	}
	return 1;
}



/**
* Upgrade
*
*/
bool CUpgrade::Upgrade()
{
	int r = AfxMessageBox(L"RealFTP will restart while upgrading to the latest version.\n"
	          L"Do you want to continue?",
        	          MB_YESNO);
	if(r == MB_OK || r == 6){

		AfxGetMainWnd()->ShowWindow(SW_HIDE);

		THREADPARAM * _param = new THREADPARAM;
		CWinThread *m_pDownloadUpgraderThread = AfxBeginThread(DownloadUpgraderThread, _param); 
		if (m_pDownloadUpgraderThread != NULL)
		{
		} 
	}
	return true;
}


bool CUpgrade::UpgraderPresent()
{

	return false;
}


/**
*
*
*/
UINT CUpgrade::DownloadUpgraderThread(LPVOID pParam)
{
	THREADPARAM* ts = (THREADPARAM*)pParam;

	// Location of upgrader
	CString upgraderPath(_T(""));
	TCHAR programPath[_MAX_PATH];
	if(::GetModuleFileName(NULL, programPath, _MAX_PATH))
	{
		upgraderPath = CString(programPath);
		if(upgraderPath.ReverseFind('\\') != -1)
		{
			upgraderPath = upgraderPath.Mid(0, upgraderPath.ReverseFind('\\') + 1);
			upgraderPath = upgraderPath + CString(_T("Upgrader.exe"));
		}
	}

	// Download upgrader
	CFileStatus status;
	if(!CFile::GetStatus(upgraderPath, status))
	{
		// Download
		DownloadManager dm;
		CString e(_T(""));
		dm.DownloadFile(CString(_T("http://realftp.net/Upgrader.exe")), upgraderPath, e);
	}

	// Run upgrader
	RunCommand cmd;
	DWORD retCode;
	CString error(_T(""));
	if(cmd.runAndForget( upgraderPath )){
		Sleep(100);
		PostMessage( AfxGetMainWnd()->GetSafeHwnd(), ID_EXIT, 0, 0); 
		//AfxGetMainWnd()->SendMessage(WM_CLOSE);
	}

	return 1;
}
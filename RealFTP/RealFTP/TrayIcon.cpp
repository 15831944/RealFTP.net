#include "stdafx.h"
#include "TrayIcon.h"
#include "Resource.h"

CTrayIcon::CTrayIcon(void)
{
	m_installer = NULL;
}

CTrayIcon::~CTrayIcon(void)
{
}

void CTrayIcon::SetInstaller(HWND wnd)
{
	m_installer = wnd;
}

void CTrayIcon::OnLButtonDown(UINT nIconID, CPoint ptMouse)
{
    // restore installer window.
	RestoreInstallerDialog();
}

void CTrayIcon::OnRButtonDown(UINT nIconID, CPoint ptMouse)
{
    // restore installer window.
	RestoreInstallerDialog();
}

void CTrayIcon::OnLButtonDblClk(UINT nIconID, CPoint ptMouse)
{
	// restore installer window.
	RestoreInstallerDialog();
}


/**
* RestoreInstallerDialog
*
* Description: brings up the installer window from the dock.
*/
void CTrayIcon::RestoreInstallerDialog()
{
	if(m_installer != NULL)
	{
		bool r = ShowWindow(m_installer, SW_RESTORE);
		if(r){
			int i = 0;
		}
	}
}

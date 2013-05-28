
#pragma once

#include "SysTrayIcon.h"
//#include "AirInstallerDlg.h"

class CTrayIcon : public CSysTrayIcon
{
public:

	HWND m_installer;

    CTrayIcon(void);
    ~CTrayIcon(void);
    
	virtual void OnLButtonDown(UINT nIconID, CPoint ptMouse);
    virtual void OnRButtonDown(UINT nIconID, CPoint ptMouse);
    virtual void OnLButtonDblClk(UINT nIconID, CPoint ptMouse);

	void SetInstaller(HWND wnd);
	void RestoreInstallerDialog();
};

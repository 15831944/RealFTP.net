#include "StdAfx.h"
#include "ToolTip.h"


CToolTip::CToolTip()
{
	// http://msdn.microsoft.com/en-US/library/czada357(v=vs.80).aspx

	Create(NULL, _T("XXX"),
		WS_DLGFRAME   | WS_CAPTION, // WS_POPUPWINDOW WS_OVERLAPPEDWINDOW
		CRect(400, 280, 580, 520), NULL, NULL,
		WS_EX_TOOLWINDOW);
}
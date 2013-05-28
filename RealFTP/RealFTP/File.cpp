#include "StdAfx.h"
#include "File.h"
//#include "resource.h"


CFileContainer::CFileContainer(void)
{
	localStore = false;
	permissions = CString(_T(""));
	owner = CString(_T(""));
	group = CString(_T(""));
	host = CString(_T(""));
}

CFileContainer::~CFileContainer(void)
{
}


long CFileContainer::GetAge()
{
	// remoteDate
	CString remote_date(remoteDate); // 2012/08/26 13:32
	if(remote_date.Find(_T(" ")) != -1){
		remote_date = remote_date.Mid(0, remote_date.Find(_T(" ")));
	}
	remote_date.Remove(':');
	remote_date.Remove(' ');
	remote_date.Remove('/');
	long remoteVal = _wtol((LPCWSTR)remote_date);
	
	CString curr_date = CTime::GetCurrentTime().Format("%Y%m%d");  // %H%M
	long currVal = _wtol((LPCWSTR)curr_date);

	long difference = currVal - remoteVal;

	return difference;
}

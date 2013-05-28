#pragma once
#include "afx.h"

class CFileContainer 
{
public:
	CFileContainer(void);
	~CFileContainer(void);
	
	int dir;
	CString name;
	CString path;
	CString localPath;
	CString extension;
	CString remoteDate;
	CString localDate;
	CString permissions;
	CString host;
	CString owner;
	CString group;
	long size;
	bool localStore;

	int view_x;
	int view_y;
	
	long GetAge();
};
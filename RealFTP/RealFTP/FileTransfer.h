#pragma once
#include "Connection.h"

class CFileTransfer 
{
public:
	CFileTransfer(void);
	CFileTransfer(CFileTransfer * t);
	~CFileTransfer(void);

	CConnection * connection;
	CString fileName;
	CString path;
	bool isDir;
	long size;
	bool sending;
	int progress;
	bool cancel;
	bool can_delete;

	void Clone(CFileTransfer * t);
};
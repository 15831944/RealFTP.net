#include "StdAfx.h"
#include "FileTransfer.h"


CFileTransfer::CFileTransfer(void)
{
	fileName = CString(_T(""));
	path = CString(_T(""));
	isDir = false;
	size = 0;
	connection = NULL;
}

CFileTransfer::CFileTransfer(CFileTransfer * t)
{
	Clone(t);
}

CFileTransfer::~CFileTransfer(void)
{
	if(connection)
	{
		delete connection;
	}
}


/**
* Clone the given object
*
*/
void CFileTransfer::Clone(CFileTransfer * t)
{
	fileName = CString(t->fileName);
	path = CString(t->path);
	sending = t->sending;
	progress = t->progress;
	cancel = t->cancel;
	can_delete = t->can_delete;
	connection = new CConnection();
	connection->Clone(t->connection);
}
#include "StdAfx.h"
#include "Connection.h"
#include "resource.h"


IMPLEMENT_SERIAL(CConnection, CObject, 0)


CConnection::CConnection(void)
{
	name = CString(_T(""));
	host = CString(_T(""));
	user = CString(_T(""));
	password = CString(_T(""));
	key = CString(_T(""));
	progress = 100;
	m_path = CString(_T(""));
}

CConnection::CConnection(CConnection * con)
{
	Clone(con);
}

CConnection::~CConnection(void)
{
}



void CConnection::Serialize(CArchive& ar)
{
    CObject::Serialize(ar);
    
    if( ar.IsStoring() )
	ar << name << host << user << password << m_path;
    else
	ar >> name >> host >> user >> password >> m_path;
}


/**
* Clone
*
*/
void CConnection::Clone(CConnection * con)
{
	name = CString(con->name);
	host = CString(con->host);
	user = CString(con->user);
	password = CString(con->password);
	key = CString(con->key);
	m_path = CString(con->m_path);
	progress = con->progress;
}

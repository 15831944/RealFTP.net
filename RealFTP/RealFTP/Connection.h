#pragma once
#include "afx.h"
//#include "RealFtpClient.h"

class CConnection : public CObject
{
public:
	CConnection(void);
	CConnection(CConnection * con);
	~CConnection(void);
	
	CString name;
	CString host;
	CString user;
	CString password;
	CString key;

	int progress;
	CString m_path; // Makes connection a bookmark

	//CRealFtpClient * ftpClient;

public:
	void Clone(CConnection * con);
	void Serialize(CArchive& ar);
	DECLARE_SERIAL(CConnection);
//private:
	
};
#pragma once
#include "Connection.h"
#include "afxcoll.h"

class CConnectionManager : public CObject
{
public:
	CConnectionManager(void);
	~CConnectionManager(void);
	
	
public:

	void Add(CConnection &con);
	void RemoveAt(int index);
	CConnection* GetAt(int index);
	void Update(int index, CConnection &con);
	void Swap(int index, int with_index);
	void Load();
	void Store();
	CObArray * GetConnections();
	virtual void Serialize(CArchive& ar);
	
private:
	CObArray m_listOfConnections;
};
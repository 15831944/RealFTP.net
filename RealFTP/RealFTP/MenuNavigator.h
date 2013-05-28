#pragma once
#include "Connection.h"
#include <vector>

class CMenuNavigator 
{
public:
	CMenuNavigator(void);
	~CMenuNavigator(void);

	// TODO: Cache tree in memory for performant loading.

	void Reset();
	void BuildMenu(CConnection * m_connection, CString &dir, CMenu *tracker, int maxLevels);
	bool DirHasChildren(CString &dir);

	int MENU_ID;
	static std::vector<CString> menu_id_paths;
	void GetPathForID(int id, CString &path);
	static CString m_localRoot;
};

// Define bytecount in file scope.
//std::vector<CString> CMenuNavigator::menu_id_paths;
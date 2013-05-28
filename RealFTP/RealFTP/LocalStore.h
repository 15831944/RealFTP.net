#pragma once
#include "File.h"


class CLocalStore 
{
public:
	CLocalStore(void);
	~CLocalStore(void);

	void SetStore(CFileContainer & file, bool stored);
	void ToggleStore(CFileContainer & file);
	bool IsFolderStored(CFileContainer & file);


protected:


};
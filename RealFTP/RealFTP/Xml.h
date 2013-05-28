#pragma once

class CXml
{
public:
	CXml(void);
	~CXml(void);
	
public:
	bool GetTag(CString & xml, CString & tag, CString & value);
	
	
};
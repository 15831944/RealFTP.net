#include "StdAfx.h"
#include "Xml.h"


CXml::CXml(void)
{
}

CXml::~CXml(void)
{
}


/**
* GetTag
*
* Description: Extract tag from xml element.
*/
bool CXml::GetTag(CString & xml, CString & tag, CString & value)
{
	bool result = false;
	int start = xml.Find( CString(_T("<") + tag) );
	int end = xml.Find( CString(_T("</") + tag) );
	if(start != -1 && end != -1)
	{
		int start_close = xml.Find(_T(">"), start);
		if(start_close != -1){
			value = CString( xml.Mid(start_close + 1, end - start_close - 1 ) );
			result = true;
		}
	}
	return result;
}
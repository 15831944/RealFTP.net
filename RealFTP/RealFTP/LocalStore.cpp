#include "stdafx.h"
#include "LocalStore.h"
#include "Xml.h"


CLocalStore::CLocalStore(void)
{
}

CLocalStore::~CLocalStore(void)
{
}

void CLocalStore::SetStore(CFileContainer & file, bool stored)
{
}


/**
* ToggleStore
*
* Description: 
*/
void CLocalStore::ToggleStore(CFileContainer & file)
{
	if(file.name.GetLength() == 0)
	{
		TRACE0(" ERROR --- CLocalStore.ToggleStore(No FILE) \n");
		return;
	}

	CString configPath(_T(""));
	TCHAR programPath[_MAX_PATH];
	if(::GetModuleFileName(NULL, programPath, _MAX_PATH))
	{
		configPath = CString(programPath);
		if(configPath.ReverseFind('\\') != -1)
		{
			configPath = configPath.Mid(0, configPath.ReverseFind('\\') + 1);
		}
		configPath = CString(configPath + _T(".localstore"));
	}

	CString contents;
	bool store_locally = false;
	bool found = false;
	int found_path_length = 0;
	CString filePath(file.path + file.name + _T("/"));

	// Read
	CFileStatus status;
	if(CFile::GetStatus(configPath, status))
	{
		CStdioFile stdFile (configPath, CStdioFile::modeRead | CFile::shareDenyNone);
		CString buffer;
		//CString string;
		while (stdFile.ReadString(buffer))
		{
			CXml xml;
			CString con(_T(""));
			CString path(_T(""));
			CString local(_T(""));
			xml.GetTag(buffer, CString(_T("con")), con);
			xml.GetTag(buffer, CString(_T("path")), path);
			xml.GetTag(buffer, CString(_T("local")), local);
			//store_locally = false;

			//TRACE2(" buffer: %s   bla: %s  \n", buffer, CString(file.path + file.name));
			
			// Search for this folder
			if(path.Compare(filePath) == 0 && con.Compare(file.host) == 0)
			{
				// Found exact folder  
				found = true;
				found_path_length = filePath.GetLength(); // will be largest match
				if(local.Compare(_T("true")) == 0) 
				{
					store_locally = true;
				} 
			} else { 
				// line is not file given, not affected
				contents = CString(contents + _T("<con>") + con + _T("</con>") + 
					_T("<path>") + path + _T("</path><local>") + local + _T("</local>\r\n"));
			}

			// Search for close parent folder (if not exact match, this will set local_store status)
			if(path.GetLength() > 0 && filePath.Find(path) == 0 && con.Compare(file.host) == 0)
			{
				// found close path. May be decendent folder.
				if(local.Compare(_T("true")) == 0 && found_path_length <= path.GetLength()) // true if largest matching path 
				{
					store_locally = true;
					found_path_length = path.GetLength(); 
				} 
				if(local.Compare(_T("false")) == 0 && found_path_length <= path.GetLength())
				{
					store_locally = false;
					found_path_length = path.GetLength();
				}
			} 
		}
	}

	// Set store local
	if(filePath.GetLength() > 0 && store_locally == false) // 
	{
		contents = CString(contents + _T("<con>") + file.host + _T("</con>") + 
					_T("<path>") + filePath + _T("</path><local>") + _T("true") + _T("</local>\r\n"));
	}

	// Do not dtore local 
	if(filePath.GetLength() > 0 && store_locally == true) // 
	{
		contents = CString(contents + _T("<con>") + file.host + _T("</con>") + 
					_T("<path>") + filePath + _T("</path><local>") + _T("false") + _T("</local>\r\n"));
	}


	// Write
	CFile theFile(configPath, CFile::modeReadWrite | CFile::modeCreate);
	char buffer[200000];
	sprintf(buffer, "%S", contents); // CString to char *
	int len = strlen(buffer);
	//theFile.Write( (LPCTSTR) dirContentListing, dirContentListing.GetLength() * sizeof(TCHAR));
	theFile.Write(buffer, len);
}


/**
* IsFolderStored
*
* Description:
*/
bool CLocalStore::IsFolderStored(CFileContainer & file)
{
	bool result = false;

	//CString dirPath(file.localPath + file.name + _T("\\.files.rftp"));
	
	CString configPath(_T(""));
	TCHAR programPath[_MAX_PATH];
	if(::GetModuleFileName(NULL, programPath, _MAX_PATH))
	{
		configPath = CString(programPath);
		if(configPath.ReverseFind('\\') != -1)
		{
			configPath = configPath.Mid(0, configPath.ReverseFind('\\') + 1);
		}
		configPath = CString(configPath + _T("\\.localstore"));
	}
	
	int found_path_length = 0;
	CString filePath(file.path + file.name + _T("/"));

	CFileStatus status;
	if(CFile::GetStatus(configPath, status))
	{
		CStdioFile stdFile (configPath, CStdioFile::modeRead | CFile::shareDenyNone);
		CString buffer;
		//CString string;
		while (stdFile.ReadString(buffer))
		{
			CXml xml;
			CString con(_T(""));
			CString path(_T(""));
			CString local(_T(""));
			xml.GetTag(buffer, CString(_T("con")), con);
			xml.GetTag(buffer, CString(_T("path")), path);
			xml.GetTag(buffer, CString(_T("local")), local);

			//TRACE2(" folder: %s - %s \n", buffer, CString(file.path + file.name));

			// Close match
			if(path.GetLength() > 0 && filePath.Find(path) == 0 && con.Compare(file.host) == 0)
			{
				// found close path. May be decendent folder.
				if(local.Compare(_T("true")) == 0 && found_path_length <= path.GetLength()) // true if largest matching path 
				{
					result = true;
					found_path_length = path.GetLength(); 
				} 
				if(local.Compare(_T("false")) == 0 && found_path_length <= path.GetLength())
				{
					result = false; 
					found_path_length = path.GetLength(); 
				}
			} 

			// Exact Match - Overides close match
			if(path.Compare(filePath) == 0 && con.Compare(file.host) == 0 && local.Compare(_T("true")) == 0)
			{
				result = true;
			}
			if(path.Compare(filePath) == 0 && con.Compare(file.host) == 0 && local.Compare(_T("false")) == 0)
			{
				result = false;
			}
		}
	}
	
	return result;
}
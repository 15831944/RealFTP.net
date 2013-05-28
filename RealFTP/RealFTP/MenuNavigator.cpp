/**
*
*
* ON_COMMAND_RANGE 
*/

#include "StdAfx.h"
#include "MenuNavigator.h"
#include "FtpManager.h"
#include "File.h"

std::vector<CString> CMenuNavigator::menu_id_paths;
CString CMenuNavigator::m_localRoot;

CMenuNavigator::CMenuNavigator(void)
{
	MENU_ID = 5000;
	//CMenuNavigator::m_localRoot = CString(_T("a"));
}

CMenuNavigator::~CMenuNavigator(void){}


void CMenuNavigator::Reset()
{
	MENU_ID = 5000;
	CMenuNavigator::menu_id_paths.clear();
	//CMenuNavigator::m_localRoot = CString(_T("a"));
}


/**
* BuildMenu
*
* Description: Load menu from folders for navigation.
*
* Parameters:
*				connection: specifies server and local file directory location.
*				dir: directory viewed.
*				menu: CMenu object to add sub folder entries to.
*				maxLevels: max number of subfolders to add recursivly.
*/
void CMenuNavigator::BuildMenu(CConnection * connection, CString &dir, CMenu *menu, int maxLevels)
{
	maxLevels = maxLevels - 1;
	if(maxLevels < 1){
		return;
	}

	// get files for folder
	std::vector<CFileContainer> subFolders;
	CFtpManager ftp;

	CString localStorePath;
	ftp.GetLocalStorePath(connection, localStorePath);
	CMenuNavigator::m_localRoot = CString(localStorePath);
	CString localPath(localStorePath + dir);
	localPath.Replace('/', '\\');

	ftp.ReadLocalDirectory(connection, dir, subFolders);
	int i = 0;
	for(i = 0; i < subFolders.size(); i++)
	{
		CFileContainer f;
		f = (CFileContainer)subFolders[i];
		CString subFolderPath(localPath + f.name + _T("\\"));
		if(f.name.Trim().Compare(_T(".")) != 0 && 
			f.name.Trim().Compare(_T("")) != 0 
			&& f.dir 
			){
			if( !DirHasChildren(subFolderPath) ){

				CMenuNavigator::menu_id_paths.push_back(subFolderPath);
				menu->AppendMenu(MF_STRING, MENU_ID++, f.name);  

				//TRACE1(" no child  %s \n", subFolderPath);

			} else {
				CMenu *subMenu = new CMenu();
				subMenu->CreatePopupMenu();

				// Hack - CMenu popups can't be clicked - so add a child entry.
				CMenuNavigator::menu_id_paths.push_back(subFolderPath);
				subMenu->AppendMenu(MF_STRING, MENU_ID++, f.name); 
				subMenu->AppendMenu(MF_SEPARATOR, 1, _T("")); 
				// End hack

				//CMenuNavigator menuNavigator;
				//menuNavigator.
				BuildMenu(connection, CString(dir + f.name + _T("/")), subMenu, maxLevels);
				menu->AppendMenu(MF_POPUP|MF_STRING, (UINT)subMenu->m_hMenu,  f.name );

				//TRACE1("has child  %s \n", dir + f.name);
			}
			//TRACE1("is  a dir  %s \n", f.name);
		} else {
			//TRACE1("Not a dir  %s \n", f.name);
		}
	}	
}



/**
*
*
*/
bool CMenuNavigator::DirHasChildren(CString &directory)
{
	bool result = false;
	CString dir(directory);
	dir.Replace('/', '\\');
	dir = dir + CString(_T("*.*"));

	CFileFind finder; 
	BOOL bWorking = finder.FindFile(dir);
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		CFileContainer f;
		f.name = finder.GetFileTitle();
		f.path = finder.GetFilePath();
		if(f.name.Trim().Compare(_T(".")) != 0 && f.name.Trim().Compare(_T("")) != 0 ){
			//files.push_back(f);
			result = true;
			bWorking = false;
		}
	}
	finder.Close();

	return result;
}

void CMenuNavigator::GetPathForID(int id, CString &path)
{
	id = id - 5000;
	path = CString(_T("/"));
	if(id >= 0 && id < CMenuNavigator::menu_id_paths.size()){
		path = CMenuNavigator::menu_id_paths[id];
	}

	// strip out local dir
	if(path.Find(CMenuNavigator::m_localRoot) != -1){
		path = path.Mid(CMenuNavigator::m_localRoot.GetLength(), path.GetLength() - CMenuNavigator::m_localRoot.GetLength());
	}

	path.Replace('\\', '/');
}
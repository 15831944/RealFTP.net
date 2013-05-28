#include "StdAfx.h"
#include "Version.h"


CString CGlobalFunctions::m_csFileVersion;
CString CGlobalFunctions::m_csProductVersion;

CGlobalFunctions::CGlobalFunctions()
{

}

CGlobalFunctions::~CGlobalFunctions()
{

}
// This is the key method
CString CGlobalFunctions::GetVersionInfo(HMODULE hLib, CString csEntry, CString & value)
{
  CString csRet;

  if (hLib == NULL)
    hLib = AfxGetResourceHandle();
  
  HRSRC hVersion = FindResource( hLib, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION );
  if (hVersion != NULL)
  {
    HGLOBAL hGlobal = LoadResource( hLib, hVersion ); 
    if ( hGlobal != NULL)  
    {  
  
      LPVOID versionInfo  = LockResource(hGlobal);  
      if (versionInfo != NULL)
      {
		  
        DWORD vLen,langD;
        BOOL retVal;    
    
        LPVOID retbuf=NULL;
    
        static char fileEntry[256];
		//static LPCWSTR fileEntry[256];
		static CString sFileEntry(_T("\\VarFileInfo\\Translation"));

        sprintf(fileEntry,"\\VarFileInfo\\Translation");
        retVal = VerQueryValue(versionInfo, sFileEntry, &retbuf,(UINT *)&vLen);
        if (retVal && vLen==4) 
        {
          memcpy(&langD,retbuf,4);            
          sprintf(fileEntry, "\\StringFileInfo\\%02X%02X%02X%02X\\",
                  (langD & 0xff00)>>8,langD & 0xff,(langD & 0xff000000)>>24, 
                  (langD & 0xff0000)>>16  );   // \StringFileInfo\040904B0\F   

		  sFileEntry = CString(fileEntry) + csEntry;
        }
        else 
		{
          sprintf(fileEntry, "\\StringFileInfo\\%04X04B0\\%s", 
            GetUserDefaultLangID(), csEntry);
		}

        if (VerQueryValue(versionInfo, sFileEntry, &retbuf, (UINT *)&vLen)) 
		{
          //csRet = (char*)retbuf;
		 
			int i = 0;
			for(int i = 0; i < vLen*2; i++ ){
				csRet = csRet + CString((char*)retbuf + i);
				i++;
			}
		} else {
			int no_dice = 1;
		}
      }
    }

    UnlockResource( hGlobal );  
    FreeResource( hGlobal );  
  }
  value = CString(csRet);
  return csRet;
}

// Re-formats a string formatted as "m,n,o,p" to format as "m.nop"
CString CGlobalFunctions::FormatVersion(CString cs)
{
  CString csRet;
  if (!cs.IsEmpty())
  {
    cs.TrimRight();
    int iPos = cs.Find(',');
    if (iPos == -1)
      return "";
    cs.TrimLeft();
    cs.TrimRight();
    //csRet.Format("%s.", cs.Left(iPos));
	csRet = CString(cs.Left(iPos) + _T(".")); //  Format("%s.", cs.Left(iPos));

    while (1)
    {
      cs = cs.Mid(iPos + 1);
      cs.TrimLeft();
      iPos = cs.Find(',');
      if (iPos == -1)
      {
        csRet +=cs;
        break;
      }
      csRet += cs.Left(iPos);
    }
  }

  return csRet;
}

// Loads "FileVersion" from resource formats it and keeps it in mind
CString CGlobalFunctions::GetFileVersionX(CString & value)
{
  if (m_csFileVersion.IsEmpty())
  {
    CString csVersion = CString(_T("")); //FormatVersion(GetVersionInfo(NULL, "FileVersion"));
	GetVersionInfo(NULL, "FileVersion", csVersion);
    //m_csFileVersion.Format("Version %s (Build %s)", csVersion, GetVersionInfo(NULL, "SpecialBuild"));
	m_csFileVersion = CString(csVersion);
	value = csVersion;
  }

  value = m_csFileVersion;

  return m_csFileVersion;
}

// Loads "ProductVersion" from resource formats it and keeps it in mind
CString CGlobalFunctions::GetProductVersionX()
{
  if (m_csProductVersion.IsEmpty())
  {
	CString value(_T(""));
	GetVersionInfo(NULL, "ProductVersion", value);
    m_csProductVersion = value;
  }
  return m_csProductVersion;
}
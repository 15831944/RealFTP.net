// Properties.h: interface for the CProperties class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROPERTIES_H__FC2CE3B6_ADD5_45B9_B814_5630DDECE3D3__INCLUDED_)
#define AFX_PROPERTIES_H__FC2CE3B6_ADD5_45B9_B814_5630DDECE3D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable: 4786)
#pragma warning(disable: 4710)

#include <vector>

// forward declaration
class t_props;

class CProperties  
{
public:
    CString GetIniFilePath();
    CString GetProgramDir();
    bool Load();
	bool Save();
    bool Create();
	CProperties();
	virtual ~CProperties();

    CString m_serverIp;
    int     m_useEthernet;
    int     m_serverPort;
    int     m_serverTimeout;
private:

    std::vector<t_props> m_proplist;
};

class CSplitPath
{ 
// Construction
public: 
    CSplitPath( LPCTSTR lpszPath = NULL );

// Operations
    BOOL    Split(LPCTSTR lpszPath );
    CString GetPath( void ) { return path_buffer; }
    CString GetDrive( void ) { return drive; }
    CString GetDirectory( void ) { return dir; }
    CString GetFileName( void ) { return fname; }
    CString GetExtension( void ) { return ext; }

// Attributes
protected:
    TCHAR path_buffer[_MAX_PATH];
    TCHAR drive[_MAX_DRIVE];
    TCHAR dir[_MAX_DIR];
    TCHAR fname[_MAX_FNAME];
    TCHAR ext[_MAX_EXT];
}; 

enum type{T_STRING, T_INT, T_FLOAT};
class t_props
{
public:
    t_props(CString key, CString valname, CString def, CString * str)
    {
        this->key = key;
        this->valname = valname;
        this->defStr = def;
        type = T_STRING;
        value = (void*)str;
    }

    t_props(CString key, CString valname, int def, int * val)
    {
        this->key = key;
        this->valname = valname;
        this->defInt = def;
        type = T_INT;
        value = (void*)val;
    }

    t_props(CString key, CString valname, double def, double * val)
    {
        this->key = key;
        this->valname = valname;
        this->defFloat = def;
        type = T_FLOAT;
        value = (void*)val;
    }

    CString key;
    CString valname;
    int     type;
    CString defStr;
    int     defInt;
    double  defFloat;
    void *  value;
};


#endif // !defined(AFX_PROPERTIES_H__FC2CE3B6_ADD5_45B9_B814_5630DDECE3D3__INCLUDED_)

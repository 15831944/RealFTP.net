#pragma once

class CGlobalFunctions  : public CObject
{
public:
    CGlobalFunctions();
    virtual ~CGlobalFunctions();
public:
    static CString GetFileVersionX(CString & value);
    static CString GetProductVersionX();
    static CString GetVersionInfo(HMODULE hLib, CString csEntry, CString & value);
    static CString FormatVersion(CString cs);

	
private:
    static CString m_csFileVersion;
    static CString m_csProductVersion;
};
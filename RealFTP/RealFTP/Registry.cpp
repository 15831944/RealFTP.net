#include "StdAfx.h"
#include "Registry.h"
#include <AtlBase.h>			// atl -> registry access
#include <intrin.h>				// cpuid
#include <stdint.h>


/**
* IsPurchased
*
* Description: Is this app purchased. 
* Return: bool, true if purchased. False otherwise.
*/
bool CRegistry::IsPurchased()
{
	bool result = false;
	
	CString key(_T("HKEY_CURRENT_USER/Software/RealFTP"));
	CString value(_T(""));
	if(!doesRegistryKeyExist(key, value))
	{
		CString p(_T("HKEY_CURRENT_USER/Software/"));
		CString k(_T("RealFTP"));
		CString v(_T(""));
		bool r = addRegistryEntry(p, k, v);
	}

	/*
	key = CString(_T("HKEY_CURRENT_USER/Software/RealFTP/guid"));
	if(!doesRegistryKeyExist(key, value))
	{
		CString p(_T("HKEY_CURRENT_USER/Software/RealFTP/"));
		CString k(_T("guid"));
		CString v(_T(""));
		GetRandomID(v);
		bool r = addRegistryEntry(p, k, v);
	}
	*/

	key = CString(_T("HKEY_CURRENT_USER/Software/RealFTP/pchs"));
	if(!doesRegistryKeyExist(key, value))
	{
		CString p(_T("HKEY_CURRENT_USER/Software/RealFTP/"));
		CString k(_T("pchs"));
		value = CString(_T("320"));
		bool r = addRegistryEntry(p, k, value);
	}

	if(value.Compare(_T("250")) == 0)
	{
		result = true;
	}

	return result;
}


bool CRegistry::Purchase()
{
	bool result = true;

	CString key(_T("HKEY_CURRENT_USER/Software/RealFTP"));
	CString value(_T(""));
	if(!doesRegistryKeyExist(key, value))
	{
		CString p(_T("HKEY_CURRENT_USER/Software/"));
		CString k(_T("RealFTP"));
		CString v(_T(""));
		bool r = addRegistryEntry(p, k, v);
	}

	key = CString(_T("HKEY_CURRENT_USER/Software/RealFTP/pchs"));
	CString p(_T("HKEY_CURRENT_USER/Software/RealFTP/"));
	CString k(_T("pchs"));
	value = CString(_T("250"));
	bool r = addRegistryEntry(p, k, value);
	
	return result;
}





void CRegistry::GetGUID(CString & guid)
{
	CString key(_T("HKEY_CURRENT_USER/Software/RealFTP"));
	CString value(_T(""));
	if(!doesRegistryKeyExist(key, value))
	{
		CString p(_T("HKEY_CURRENT_USER/Software/"));
		CString k(_T("RealFTP"));
		CString v(_T(""));
		bool r = addRegistryEntry(p, k, v);
	}

	key = CString(_T("HKEY_CURRENT_USER/Software/RealFTP/guid"));
	if(!doesRegistryKeyExist(key, guid))
	{
		CString p(_T("HKEY_CURRENT_USER/Software/RealFTP/"));
		CString k(_T("guid"));
		GetRandomID(guid);
		bool r = addRegistryEntry(p, k, guid);
	}
}


bool CRegistry::SetUpgradeKey(CString & activation_key)
{
	CString key(_T("HKEY_CURRENT_USER/Software/RealFTP"));
	CString value(_T(""));
	if(!doesRegistryKeyExist(key, value))
	{
		CString p(_T("HKEY_CURRENT_USER/Software/"));
		CString k(_T("RealFTP"));
		CString v(_T(""));
		bool r = addRegistryEntry(p, k, v);
	}

	key = CString(_T("HKEY_CURRENT_USER/Software/RealFTP/key"));
	CString p(_T("HKEY_CURRENT_USER/Software/RealFTP/"));
	CString k(_T("key"));
	bool r = addRegistryEntry(p, k, activation_key);
	return true;
}



void CRegistry::GetUpgradeKey(CString & activation_key)
{
	CString key(_T("HKEY_CURRENT_USER/Software/RealFTP"));
	CString value(_T(""));
	if(!doesRegistryKeyExist(key, value))
	{
		CString p(_T("HKEY_CURRENT_USER/Software/"));
		CString k(_T("RealFTP"));
		CString v(_T(""));
		bool r = addRegistryEntry(p, k, v);
	}

	key = CString(_T("HKEY_CURRENT_USER/Software/RealFTP/key"));
	if(!doesRegistryKeyExist(key, activation_key))
	{
	
	}
}




/**
* doesRegistryKeyExist
*
* Description: Checks system registry for a key. returns true if it exists.
*			Detect hkey type from string.
*/
bool CRegistry::doesRegistryKeyExist(CString &registryKey, CString &value)
{
	bool result = false;

	int bitPlatform = 0;
	if(is64BitOS()){
		bitPlatform = KEY_WOW64_64KEY;
	} else {
		bitPlatform = KEY_WOW64_32KEY;
	}

	//CString k(_T("HKEY_CURRENT_USER\\Software\\DealPly\\InstallStatus"));
	CString fullRegistryKey(registryKey);
	CString workRegistryKey(registryKey);

	HKEY HKEY_TYPE = HKEY_CURRENT_USER; 
	if( workRegistryKey.Find(_T("HKEY_LOCAL_MACHINE")) > -1 ){ HKEY_TYPE = HKEY_LOCAL_MACHINE; }
	if( workRegistryKey.Find(_T("HKEY_USERS")) != -1){ HKEY_TYPE = HKEY_USERS; }
	if( workRegistryKey.Find(_T("HKEY_CURRENT_CONFIG")) != -1){ HKEY_TYPE = HKEY_CURRENT_CONFIG; }
	if( workRegistryKey.Find(_T("HKEY_CLASSES_ROOT")) != -1){ HKEY_TYPE = HKEY_CLASSES_ROOT; }

	workRegistryKey.Replace(_T("/"), _T("\\"));		// replace any signe forward slashes with one backslash
	workRegistryKey.Replace(_T("\\\\"), _T("\\"));

	workRegistryKey.Replace(_T("HKEY_CURRENT_USER\\"), _T("")); // cut out hkey section
	workRegistryKey.Replace(_T("HKEY_LOCAL_MACHINE\\"), _T("")); // cut out hkey section
	workRegistryKey.Replace(_T("HKEY_USERS\\"), _T("")); // cut out hkey section
	workRegistryKey.Replace(_T("HKEY_CURRENT_CONFIG\\"), _T("")); // cut out hkey section
	workRegistryKey.Replace(_T("HKEY_CLASSES_ROOT\\"), _T("")); // cut out hkey section
	int endPos = workRegistryKey.ReverseFind( '\\' );
	CString attributeKeyName( workRegistryKey.Mid(endPos + 1,  workRegistryKey.GetLength() - endPos - 1) );
	CString attributePath( workRegistryKey.Mid(0, endPos) );

	// Wow6432Node
	//attributePath = CString(_T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));
	//attributeKeyName = CString(_T("{889DF117-14D1-44EE-9F31-C5FB5D47F68B}"));

	// This is how you'd get access to the registry for reading information
    CRegKey key;
	long nError = key.Open(HKEY_TYPE, attributePath.AllocSysString(), KEY_READ|bitPlatform);
    if(nError == ERROR_SUCCESS)
    {
		CString sDefaultPath, sPathEntry = attributeKeyName.AllocSysString(); 
		DWORD dwBufferSize = MAX_PATH;
		if(key.QueryValue( NULL, attributeKeyName, &dwBufferSize ) == ERROR_SUCCESS){
			key.QueryValue( sDefaultPath.GetBuffer(dwBufferSize), sPathEntry, &dwBufferSize );
			sDefaultPath.ReleaseBuffer();	
			value = CString(sDefaultPath);
			result = true;
		}
		key.Close();
    }

	HKEY hKey;
	LONG lRes = RegOpenKeyExW(HKEY_TYPE, workRegistryKey.AllocSysString(), 0, KEY_READ|bitPlatform, &hKey);
	if(lRes == ERROR_SUCCESS){ 
		result = true;
	}

	return result;
}

bool CRegistry::GetStringRegKey(HKEY hKey, CString &strValueName, CString &strValue, CString &strDefaultValue)
{
	bool result = false;
	strValue = strDefaultValue;
	WCHAR szBuffer[512];
	DWORD dwBufferSize = sizeof(szBuffer);
	ULONG nError;
	nError = RegQueryValueExW(hKey, strValueName.AllocSysString(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
	if (ERROR_SUCCESS == nError)
	{
		strValue = szBuffer;
		result = true;
	}

	// If not set, try to read double value.
	if(strValue.GetLength() == 0){
		DWORD nResult(0);
		nError = RegQueryValueExW(hKey, strValueName.AllocSysString(),
					0, NULL, reinterpret_cast<LPBYTE>(&nResult), &dwBufferSize);
		if (ERROR_SUCCESS == nError)
		{
			char buff[10];
			ltoa(nResult, buff, 10);
			strValue = CString(buff);
			result = true;
		}
	}

	return result;
}


/**
* addRegistryEntry
*
* Description: adds a registry key/value to a location.
*/
bool CRegistry::addRegistryEntry(CString &sLocation, CString &sKey, CString &sValue)
{
	bool result = true;
	

	int bitPlatform = 0;
	if(is64BitOS()){
		bitPlatform = KEY_WOW64_64KEY;
	} else {
		bitPlatform = KEY_WOW64_32KEY;
	}
		
	CString location(sLocation);

	CString sFullLocation(location);

	HKEY HKEY_TYPE = HKEY_CURRENT_USER; 
	if( location.Find(_T("HKEY_LOCAL_MACHINE")) > -1 ){
		HKEY_TYPE = HKEY_LOCAL_MACHINE;
	}

	location.Replace(_T("/"), _T("\\"));		// replace any signe forward slashes with one backslash
	location.Replace(_T("\\\\"), _T("\\"));
	location.Replace(_T("HKEY_CURRENT_USER\\"), _T("")); // cut out hkey section
	location.Replace(_T("HKEY_LOCAL_MACHINE\\"), _T("")); // cut out hkey section

	/*
	DWORD lRv;
	HKEY hKey;
	CString bad(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AirInstaller"));
	lRv = RegOpenKeyEx(HKEY_TYPE, bad.AllocSysString(), 0, KEY_WRITE, &hKey);
	if (lRv != ERROR_SUCCESS)
	{
		DWORD dwDisposition;
		// Create a key if it did not exist
		lRv = RegCreateKeyEx(HKEY_TYPE, location.AllocSysString(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
	}
	*/


	CRegKey key;
	long nError = key.Open(HKEY_TYPE, location.AllocSysString(), KEY_READ | KEY_WRITE | bitPlatform ); // KEY_READ
    if(nError != ERROR_SUCCESS){
		if ( key.Create( HKEY_TYPE, location.AllocSysString(), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | bitPlatform, NULL, NULL ) != S_OK ){
			/*
			l.log(CString(_T("Error adding registry entry. the location does not exist. Location: ")) +
				sFullLocation +
				CString(_T(" Key: ")) + sKey + 
				CString(_T(" value: ")) + sValue);
			*/
			return false;
		}
	}

	if ( key.SetStringValue( sKey.AllocSysString(), sValue.AllocSysString(), REG_SZ ) != ERROR_SUCCESS ) {
	//if ( key.SetValue( sKey.AllocSysString(), sValue.AllocSysString() ) != ERROR_SUCCESS ) {
		// error message, couldn't set value..
		
		TRACE(_T("CRegistry Error adding registry entry: %s with: %s\n"), sKey.AllocSysString(), sValue.AllocSysString());

		result = false;
	} else {
		// success message..

		result = true;
	}

	key.Close();
	
	return result;
}



/**
* is64BitOS
*
* Description: checks the system for a 32 or 64 bit OS. returns true if OS is 64 bit.
*/
int CRegistry::is64BitOS()
{
	BOOL bIs64BitOS = false;
	typedef BOOL (WINAPI * LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");
	if(NULL != fnIsWow64Process){
		if(!fnIsWow64Process(GetCurrentProcess(), &bIs64BitOS)){
			// error
			int i = 0;
		}
	}
	int result = 0;
	if(bIs64BitOS){
		result = 1;
	}
	return result;
}


/**
* GetRandomID
*
*/
void CRegistry::GetRandomID(CString &uid)
{
	// Random Number
	{
		srand((unsigned)time(0)); 
		int random_integer = rand(); 
		char buff[10];
		itoa(random_integer, buff, 10);
		
		char c[MAX_PATH];
		wtc(c, CString(buff).AllocSysString());
		uint32_t hash = hash_string( c );
		char hashBuff[100];
		sprintf(hashBuff,"%x",hash);
		
		uid = CString(hashBuff);
	}

	//
	// user
	//
	TCHAR acUserName[1000];
    DWORD nUserName = sizeof(acUserName);
	if (GetUserName( (LPWSTR)acUserName , &nUserName)) {
		char c[MAX_PATH];
		wtc(c, CString(acUserName).AllocSysString());
		uint32_t hash = hash_string( c );
		char hashBuff[100];
		sprintf(hashBuff,"%x",hash); // %x hex   - %u
		uid = uid + CString(_T("")) + CString( hashBuff );
	}

	// Get and display the name of the computer. 
	TCHAR  infoBuf[MAX_PATH];
	DWORD  bufCharCount = MAX_PATH;
	bufCharCount = MAX_PATH;
	if( GetComputerName( infoBuf, &bufCharCount ) )
	{
		char c[MAX_PATH];
		wtc(c, CString(acUserName).AllocSysString());
		uint32_t hash = hash_string( c );
		char hashBuff[100];
		sprintf(hashBuff,"%x",hash);
		uid = uid + CString( hashBuff ); 
	}

	/*
	while(uid.GetLength() < 32){
		srand((unsigned)time(0)); 
		random_integer = rand(); 
		uid = uid + CString( _T("x") );
	}
	*/

	//
	// Hash unique string
	//
	//char c[MAX_PATH];
	//wtc(c, uid.GetBuffer());
	//uint32_t hash = hash_string( c );

	//char hashBuff[100];
	//sprintf(hashBuff,"%x",hash); // %x hex   - %u
	//uid = CString( hashBuff );

}


uint32_t CRegistry::hash_string(const char * s)
{
    uint32_t hash = 0;
    for(; *s; ++s)
    {
        hash += *s;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}


/*
* wtc
* Converts wide tchar * (unicode) to char *. 
*/
void CRegistry::wtc(char* to, wchar_t* from)
{
	size_t n = 999999;
	for(size_t i = 0; i < n; i++)
	{
		if(from[i] != 0){
			to[i] = from[i];
		} else {
			n = 0; // done loop
			to[i] = 0; // terminate string
		}
	}
}
#pragma once
#include <stdint.h>

class CRegistry {
public:
	bool IsPurchased();
	bool Purchase();
	void GetGUID(CString & guid);

	bool SetUpgradeKey(CString & key);
	void GetUpgradeKey(CString & key);

	bool doesRegistryKeyExist(CString &registryKey, CString &value);
	bool addRegistryEntry(CString &location, CString &key, CString &value);
	
	int is64BitOS();
	//int getOSVersion(CString &osVersion);
	
	//BOOL IsCurrentUserLocalAdministrator(void);
	//void GetUID(CString &uid);

	bool GetStringRegKey(HKEY hKey, CString &strValueName, CString &strValue, CString &strDefaultValue);
	void GetRandomID(CString &uid);
	//bool GetRegistryKeyValue(CString &registryKey, CString &value);

	//bool IsVistaOrLater();

	uint32_t hash_string(const char * s);
	void wtc(char* to, wchar_t* from);
};
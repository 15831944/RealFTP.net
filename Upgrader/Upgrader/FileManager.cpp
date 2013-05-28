#include "StdAfx.h"
#include "FileManager.h"
#include "DownloadCallback.h"
#include <stdio.h>
#include <windows.h>
#include <Wincrypt.h>
#include <fstream>
#include <vector>
#include "dirent.h"

#define BUFSIZE 1024
#define MD5LEN  16

/**
* FileInfo
*
* Description: get information about a file including the following details:
*  exists, bytes, permissions...
*/
void FileManager::FileInfo(CString &file, CString &info)
{
	DWORD fileAttr = GetFileAttributes(file);
	if (0xFFFFFFFF == fileAttr) 
	{
		// File does not exist
		info = CString(_T("File does not exist: ")) + CString(file);
	}  
	if (fileAttr & FILE_ATTRIBUTE_READONLY) {
		info = info + CString(_T(" Read Only "));
	}

	BOOL                        fOk;
	WIN32_FILE_ATTRIBUTE_DATA   fileInfo;
	fOk = GetFileAttributesEx(file, GetFileExInfoStandard, (void*)&fileInfo);
	if(fOk)
	{
		DWORD size = (DWORD)fileInfo.nFileSizeLow;
		char sizeBuff[100];
		//sprintf(sizeBuff, "Size: %ul");
		
		ltoa((long)size, sizeBuff, 10);
		info = info + CString(sizeBuff);
	}
}

/**
* FileExists
*
* Description: Does a file exist?
*/
bool FileManager::FileExists(CString &file)
{
	bool result = true;
	DWORD fileAttr = GetFileAttributes(file);
	if (0xFFFFFFFF == fileAttr) 
	{
		result = false;
	}

	// is not zero bytes
	if(result){
		BOOL                        fOk;
		WIN32_FILE_ATTRIBUTE_DATA   fileInfo;
		fOk = GetFileAttributesEx(file, GetFileExInfoStandard, (void*)&fileInfo);
		if(fOk)
		{
			DWORD size = (DWORD)fileInfo.nFileSizeLow;
			if(size == 0){
				result = false;
			}
		}
	}

	return result;
}


bool FileManager::FolderExists(CString &strFolderName)
{   
    return GetFileAttributes(strFolderName) != INVALID_FILE_ATTRIBUTES;   
}


/**
* CreateTempFolder
*
* Description: Finds a temp file location and creates a random directory.
*/
bool FileManager::CreateTempFolder(CString &sPath)
{
	DWORD dwRetVal = 0;
	TCHAR lpTempPathBuffer[MAX_PATH];
	dwRetVal = GetTempPath(MAX_PATH,			// length of the buffer
						lpTempPathBuffer);		// buffer for path 
	if (dwRetVal > MAX_PATH || (dwRetVal == 0))
	{
		return false;  // return error
	}

	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	char tempDir[11];
	srand (time(NULL));
	for (int i = 0; i < 10; ++i) 
	{
		tempDir[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	tempDir[10] = 0;

	sPath = CString(tempDir);
	sPath = lpTempPathBuffer + sPath + _T("\\");

	if(GetFileAttributes(sPath.AllocSysString()) == INVALID_FILE_ATTRIBUTES)
	{
		if(!CreateDirectory( sPath.AllocSysString(), NULL))
		{
			return false;
		}
	}

	return true;
}

/**
* DeleteFolder
*
* Description: Deletes a folder and its contents.
*/
bool FileManager::DeleteFolder(CString &sFolderPath)
{
	CString fileFound(_T("*.*"));
	fileFound = sFolderPath + fileFound;
	WIN32_FIND_DATA info;
	HANDLE hp; 
	hp = FindFirstFile(fileFound, &info);
	do
	{
		if(CString(_T(".")).Compare(info.cFileName) != 0 &&
			CString(_T("..")).Compare(info.cFileName) != 0)
		{
			if((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==FILE_ATTRIBUTE_DIRECTORY)
			{
				CString subFolder(sFolderPath);
				subFolder = subFolder + _T("");
				subFolder = subFolder + CString( info.cFileName ) + _T("\\");
				DeleteFolder(subFolder);
				RemoveDirectory(subFolder.AllocSysString());
			}
			else
			{
				fileFound = sFolderPath + info.cFileName;
				BOOL retVal = DeleteFile(fileFound);
			}
		}
 
	} while(FindNextFile(hp, &info)); 
	FindClose(hp);
	return true;
}


/**
* CopyToTempFile
*
* Description: Copy a given file to a temp file with the same extension.
*/
bool FileManager::CopyToTempFile(CString &sFilePath)
{
	bool result = false;

	CString fileExt(_T(""));
	int extPos = sFilePath.ReverseFind('.');
	if( extPos != -1 ){
		fileExt = sFilePath.Right( sFilePath.GetLength() - extPos - 1 );
	}

	HRESULT getPathHr;
	LPWSTR pwszCacheDir = NULL;
	TCHAR szTempFileName[MAX_PATH];  
	TCHAR lpTempPathBuffer[MAX_PATH];
	DWORD dwRetVal = 0;
	dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer); // buffer for path 
	if (dwRetVal > MAX_PATH || (dwRetVal == 0))
	{
		// Error
	} else {
		UINT uRetVal   = 0;
		uRetVal = GetTempFileName(lpTempPathBuffer, // directory for tmp files
                            TEXT("air"),     // temp file name prefix 
                            0,                // create unique name 
                            szTempFileName);  // buffer for name 
		CString sTempFileWithExtension(szTempFileName);
		sTempFileWithExtension = sTempFileWithExtension + CString(_T(".")) + fileExt;
		sTempFileWithExtension.Replace(_T(".tmp"), _T(""));

		if (uRetVal != 0){
			// Copy does not include security attributes.
			bool copied = CopyFile( sFilePath.GetBuffer(), sTempFileWithExtension.GetBuffer(), false ); // existing, new , failifExists
			if(copied){
				BOOL del = DeleteFile( sFilePath ); // If file is bad, clear for next try (193)
				// only use file copy if it is valid
				sFilePath = CString(sTempFileWithExtension);

				result = true;
			}
			//// empty the filename string if failed or canceled
			//sFilePath.ReleaseBuffer(SUCCEEDED(hr) ? -1 : 0);
		}
	}
	return result;
}


/**
* DownloadUrlToTempFile
* 
* Description: download a file from a url.
*/
bool FileManager::DownloadUrlToTempFile(CString &url, CString &sFilePath, CString &error)
{
	bool result = false;

	HRESULT getPathHr;
	LPWSTR pwszCacheDir = NULL;
	TCHAR szTempFileName[MAX_PATH];  
	TCHAR lpTempPathBuffer[MAX_PATH];
	DWORD dwRetVal = 0;
	dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer); // buffer for path 
	if (dwRetVal > MAX_PATH || (dwRetVal == 0))
	{
		DWORD err = GetLastError();
		char errCodeBuffer[32];
		ltoa((long)err, errCodeBuffer, 10);
		error = error + CString(_T(" GetTempPath() failed: ")) + CString(errCodeBuffer);
	} else {

		// does folder exist
		if(!FolderExists(CString(lpTempPathBuffer))){
			error = error + CString(_T(" GetTempPath() does not exist "));
		}

		UINT uRetVal   = 0;
		uRetVal = GetTempFileName(lpTempPathBuffer, // directory for tmp files
                            TEXT("air"),     // temp file name prefix 
                            0,                // create unique name 
                            szTempFileName);  // buffer for name 
		if(uRetVal == 0){
			DWORD err = GetLastError();
			char errCodeBuffer[32];
			ltoa((long)err, errCodeBuffer, 10);
			error = error + CString(_T(" GetTempFileName failed: ")) + CString(errCodeBuffer);
		}
		CString sTempFileWithExtension(szTempFileName);
		sTempFileWithExtension = sTempFileWithExtension + CString(_T(".exe"));
		sTempFileWithExtension.Replace(_T(".tmp"), _T(""));
		
		if (uRetVal != 0){
			DownloadCallback bsc(NULL, NULL, CString(_T("offer")));

			HRESULT downloadHr;
			downloadHr = ::URLDownloadToFile(NULL, url, sTempFileWithExtension, 0, &bsc);   
			if(downloadHr != S_OK){
				DWORD err = GetLastError();
				char errCodeBuffer[32];
				ltoa((long)err, errCodeBuffer, 10);
				error = error + CString(_T(" URLDownloadToFile failed: ")) + CString(errCodeBuffer);
				if(downloadHr == E_OUTOFMEMORY){
					error = error + CString(_T(" E_OUTOFMEMORY "));
				}
				if(downloadHr == INET_E_DOWNLOAD_FAILURE){
					error = error + CString(_T(" INET_E_DOWNLOAD_FAILURE "));
				}
			}

			if(downloadHr == S_OK){
				int limit = 600;
				while(!bsc.Done() && limit > 0){ // wait for download
					::Sleep(100);
					//TRACE(_T(" sleeping \n") );
					//limit--;
					error = error + CString(_T("."));
				}
				::Sleep(700);
				if(FileExists(sTempFileWithExtension)){
					result = true;
				} else {
					error = error + CString(_T(" file doesn't exist "));
				}
				sFilePath = sTempFileWithExtension;
			}
		}
	}
	return result;
}


/**
* GetFileMD5
*
* Description: get the md5 from a file.
*/
bool FileManager::GetFileMD5(CString &filePath, CString &md5)
{
	DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = NULL;
    BYTE rgbFile[BUFSIZE];
    DWORD cbRead = 0;
    BYTE rgbHash[MD5LEN];
    DWORD cbHash = 0;
    CHAR rgbDigits[] = "0123456789abcdef";

	hFile = CreateFile(filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwStatus = GetLastError();
        //printf("Error opening file %s\nError: %d\n", filePath, dwStatus); 
		return false;
    }

    // Get handle to the crypto provider
    if (!CryptAcquireContext(&hProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        //printf("CryptAcquireContext failed: %d\n", dwStatus); 
        CloseHandle(hFile);
        //return dwStatus;
		return false;
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        dwStatus = GetLastError();
        //printf("CryptAcquireContext failed: %d\n", dwStatus); 
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        //return dwStatus;
		return false;
    }

    while (bResult = ReadFile(hFile, rgbFile, BUFSIZE, 
        &cbRead, NULL))
    {
        if (0 == cbRead)
        {
            break;
        }

        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            //printf("CryptHashData failed: %d\n", dwStatus); 
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            //return dwStatus;
			return false;
        }
    }

    if (!bResult)
    {
        dwStatus = GetLastError();
        //printf("ReadFile failed: %d\n", dwStatus); 
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        //return dwStatus;
		return false;
    }

    cbHash = MD5LEN;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        //printf("MD5 hash of file %s is: ", filePath);
        for (DWORD i = 0; i < cbHash; i++)
        {
            printf("%c%c", rgbDigits[rgbHash[i] >> 4], rgbDigits[rgbHash[i] & 0xf]);
			char a = rgbDigits[rgbHash[i] >> 4];
			char b = rgbDigits[rgbHash[i] & 0xf];

			md5 = md5 + CString(a) + CString(b);
        }
    }
    else
    {
        dwStatus = GetLastError();
        //printf("CryptGetHashParam failed: %d\n", dwStatus); 
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

	return true;
}



/**
* GetFolders
*
* Description: 
*/
bool FileManager::FindFolder(CString &in_folder_path, CString &findFolder, std::vector< CString > &found_folders)
{
	in_folder_path.Replace(_T("\\\\"), _T("\\"));

	DIR *dirp;
	struct dirent *entry;

	char dir[1000];
	wtc(dir, in_folder_path.AllocSysString());

	if(dirp = opendir( dir ))
	{
		while(entry = readdir(dirp))
		{
			//printf("entry: %s  type: %d \n", entry->d_name, entry->d_type);
			CString currDir(entry->d_name);
			if(currDir.Compare(_T(".")) != 0 && currDir.Compare(_T("..")) != 0)
			{	
				CString fullPath(_T("")); fullPath = in_folder_path + CString(_T("\\")) + currDir;
				//printf(" full: %s \n", fullPath);

				if(currDir.Compare( findFolder ) == 0){
					//foundFolder = fullPath;
					found_folders.push_back(in_folder_path); // fullPath
					//return true;
				}
				if(entry->d_type == 16384){ // Is Dir
					bool result = FindFolder(fullPath, findFolder, found_folders);
					if(result){
						//return true;
					}
				}
			}
		}
		closedir(dirp);
	}  
	return false;
}


/**
* GetDrives
*
* Description: Get a vector of drive leters in CStrings.
*/
bool FileManager::GetDrives(std::vector< CString > &drive_vector){
	DWORD mydrives = 100;
	char lpBuffer[100];
	DWORD result = GetLogicalDriveStrings(mydrives, (LPWSTR)lpBuffer );
	int i;
	if(result != 0)
	{
		CString currDrive(_T(""));
		for(i = 0; i<mydrives; i+=2) {
			char c = lpBuffer[i];
			if(c == 0){
				drive_vector.push_back( currDrive );
				currDrive = CString(_T(""));
			} else {
				currDrive = currDrive + CString(lpBuffer[i]);
			}
		}
	} else {
		//printf("GetLogicalDriveStrings() is failed lor!!! Error code: %d\n", GetLastError());
		return false;
	}
	return true;
}



/*
* etc
* Converts wide tchar * (unicode) to char *. 
*/
void FileManager::wtc(char* to, wchar_t* from)
{
	size_t n = 999999;
	for(size_t i = 0; i < n; i++)
	{
		if(from[i] != 0){
			to[i] = (char)from[i];
		} else {
			n = 0; // done loop
			to[i] = 0; // terminate string
		}
	}
}
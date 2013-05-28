#include "StdAfx.h"
#include "DownloadManager.h"
#include <afxinet.h>
#include <fstream>
//#include "FileManager.h"
//#include "Logger.h"
//#include "RemoteFileSize.h"
#include "unzip.h"

/**
* Constructor
*/
DownloadManager::DownloadManager()
{
	m_callback = NULL;
	m_dnldCallback = NULL;
	m_session = CString(_T(""));
}

/**
* SetSession
*
*/
void DownloadManager::SetSession(CString &ssn)
{
	m_session = CString(ssn);
}


/**
* SetLauncherID
*
*/
void DownloadManager::SetLauncherID(CString &lid)
{
	m_launcher_id = CString(lid);
}


/**
* DownloadFile
*
* Description: Download a file from a url to a temp file location. Uses WinINet.
*/
bool DownloadManager::DownloadFile(CString &url /* in */, CString &filePath /* out */, CString &error /* out */)
{
	//Logger l;
	//l.webLogAsync(m_session, CString(_T("log")), CString(_T("core")), CString(_T("DownloadManager.DownloadFile() start ")) + url, 9);
	
	bool result = false;

	CString server(url);
	CString uri(_T(""));

	int comPos = url.Find(_T(".com/"));
	if(comPos != -1){
		uri = url.Right( url.GetLength() - comPos - 4);
		server = url.Left(comPos + 4);
	}

	CString fileName(_T(""));
	int filePos = url.ReverseFind('/');
	if(filePos != -1 ){ fileName = url.Right( url.GetLength() - filePos -1); }

	// Get temp path file name
	


	// Get current path  
	TCHAR programPath[_MAX_PATH];
	CString path(_T(""));
	if(::GetModuleFileName(NULL, programPath, _MAX_PATH))
	{
		path = CString(programPath);
		if(path.ReverseFind('\\') != -1)
		{
			
			path = path.Mid(0, path.ReverseFind('\\') + 1);

			filePath = CString(path + fileName);
		}
	}

	//l.webLogAsync(m_session, CString(_T("log")), CString(_T("core")), CString(_T("DownloadManager.DownloadFile file ")) + filePath, 9);
	

	//
	// InternetOpen
	// 
	HINTERNET connect = InternetOpen(_T("Upgrader"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(!connect){
		error += CString(_T(" Connection Failed "));
		return false;
	}
	//INTERNET_STATUS_CALLBACK callback = InternetSetStatusCallback( connect, (INTERNET_STATUS_CALLBACK)InternetCallbackFunc );

	//
	// InternetOpenUrl
	//
	HINTERNET OpenAddress = InternetOpenUrl(connect, url.AllocSysString(), NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_KEEP_CONNECTION, 0);
	if ( !OpenAddress )
	{
		DWORD ErrorNum = GetLastError();
		char errorBuffer[32];
		ltoa((long)ErrorNum, errorBuffer, 10);
		error += CString(_T(" Failed to open URL Error: ")) + CString(errorBuffer);

		InternetCloseHandle(connect);
		return false;
	}


	//
	// Get file size
	//
	long len = 0; 
	char szSizeBuffer[64];
	DWORD dwLengthSizeBuffer = sizeof(szSizeBuffer); 
	BOOL bQuery = HttpQueryInfo(OpenAddress, HTTP_QUERY_CONTENT_LENGTH, szSizeBuffer, &dwLengthSizeBuffer, NULL); 
	if(bQuery){
		char sBuff[64];
		wtc(sBuff, (wchar_t*)szSizeBuffer);
		len = atol(sBuff);
	} else {
		// No header
	}
	

	// Clear existing file.
	std::ofstream clearfile(filePath, std::ios_base::trunc);
	if(clearfile.is_open()){
		clearfile.close();
	}

	std::ofstream outfile (filePath, std::ios::out | std::ios::app | std::ios::binary); 
	if(outfile.is_open()){
		long written = 0;
		long postFreq = 0;

		SetLastError(0);

		char DataReceived[4096];
		DWORD NumberOfBytesRead = 0;
		while(InternetReadFile(OpenAddress, DataReceived, 4096, &NumberOfBytesRead) && NumberOfBytesRead )
		{
			outfile.write(DataReceived,NumberOfBytesRead); 
			written += NumberOfBytesRead;

			//Sleep(4); // TEMP DEBUG

			// Update progress
			if(m_callback != NULL && postFreq++ == 40){
				//OnProgress(ULONG ulProgress,ULONG ulProgressMax,ULONG ulStatusCode,LPCWSTR szStatusText)
				m_callback->OnProgress(written, len, 0, _T(""));
				postFreq = 0;
			}
			if(m_dnldCallback != NULL ){ // && postFreq++ == 40

				long progress = 0;
				if(written > 0 && len > 0){
					progress = (written / len) * 100;
				}
				//m_dnldCallback->OnProgress(written, len, 0, _T(""));
				m_dnldCallback->OnProgress(progress, len, 0, _T(""));
				postFreq = 0;
			}
		}

		DWORD ErrorNum = GetLastError();
		char errorBuffer[32];
		ltoa((long)ErrorNum, errorBuffer, 10);

		char lenBuffer[32]; itoa(len, lenBuffer, 10);
		char writtenBuffer[32]; itoa(written, writtenBuffer, 10);

		if(ErrorNum != 0){
			/*l.webLog(m_session, CString(_T("error")), CString(_T("core")), 
			CString(_T("DownloadManager.DownloadFile2() url: ")) + url +
			CString(_T(" received: ")) +  writtenBuffer +
			CString(_T(" total: ")) +  CString(lenBuffer) +
			CString(_T(" error: ")) + errorBuffer, 
			1);
			*/
			result = false;
		}

		/*l.webLog(m_session, CString(_T("debug")), CString(_T("core")), 
			CString(_T("DownloadManager.DownloadFile2() url: ")) + url +
			CString(_T(" received: ")) +  writtenBuffer +
			CString(_T(" total: ")) +  CString(lenBuffer) +
			CString(_T(" error: ")) + errorBuffer, 
			9);
			*/
		outfile.close();
		result = true;

		if(written < len){
			error += CString(_T("DownloadFile() file size is less than expected. "));
			//result = true;
		}

	} else {
		result = false;
		error += CString(_T(" DownloadFile() failed to write to file. "));
	}
	
	
	//
	// Status code
	//
	int statusCode = 0;
	char responseText[256]; // change to wchar_t for unicode
	DWORD responseTextSize = sizeof(responseText);
	if(HttpQueryInfo(OpenAddress,
                  HTTP_QUERY_STATUS_CODE,
                  &responseText,
                  &responseTextSize,
                  NULL)){
		char sBuff[64];
		wtc(sBuff, (wchar_t*)responseText);
		statusCode = atoi(sBuff);
	}
	if(statusCode == 404){
		result = false;
		error += CString(_T(" DownloadFile() failed 404. "));
	}
	

	
	InternetCloseHandle(OpenAddress);
	InternetCloseHandle(connect);

	//
	// Copy file to release handle (share violation 32)
	//
	/*
	FileManager fm;
	bool copied = fm.CopyToTempFile(filePath);
	if(!copied)
	{
		result = false;
		error += CString(_T(" DownloadFile() copy file failed. "));
	}
	*/
	return result;
}



/*
// __stdcall
void  DownloadManager::CallMaster(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength)
{
    UNREFERENCED_PARAMETER(hInternet);
    UNREFERENCED_PARAMETER(lpvStatusInformation);
    UNREFERENCED_PARAMETER(dwStatusInformationLength);

    REQUEST_CONTEXT *cpContext;
    cpContext = (REQUEST_CONTEXT*)dwContext;
    char szStatusText[80];

}
*/

/**
* DownloadFile
*
* Description: Download a URL resource to a file.
* Uses WinInet.
*
* Example: http://launcher.airinstaller.com/downloads/LinkSuggestorSetup.exe
*			c:\\Temp
*/
bool DownloadManager::DownloadFileHttp(CString &url, CString &filePath, CString &error)
{
	//Logger l;
	//l.webLogAsync(m_session, CString(_T("log")), CString(_T("core")), CString(_T("DownloadManager.DownloadFile() start ")) + url, 10);
	
	bool result = false;

	CString server(url);
	CString uri(_T(""));

	int comPos = url.Find(_T(".com/"));
	if(comPos != -1){
		uri = url.Right( url.GetLength() - comPos -4);
		server = url.Left(comPos + 4);
	}

	if(filePath.GetLength() == 0){  
		TCHAR lpTempPathBuffer[MAX_PATH];
		DWORD dwRetVal = 0;
		dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer); // buffer for path 
		if (dwRetVal > MAX_PATH || (dwRetVal == 0))
		{
			error += CString(_T(" DownloadFile() failed to get temp path. "));
		} else {
			filePath = CString(lpTempPathBuffer);
		}
	}

	CString fileName(_T(""));
	int filePos = url.ReverseFind('/');
	if(filePos != -1){
		fileName = url.Right( url.GetLength() - filePos -1);
		if( filePath.Find(fileName) == -1  ){
			filePath = filePath + _T("\\") + fileName;
			filePath.Replace(_T("\\\\"), _T("\\"));
		}
	}

	CString sessionID(url);
	CInternetSession session(sessionID);
	CHttpConnection* pServer = NULL;
	CHttpFile* pFile = NULL;
	try
	{
		CString strServerName;
		INTERNET_PORT nPort = INTERNET_DEFAULT_HTTP_PORT;
		pServer = session.GetHttpConnection(url.AllocSysString(), nPort);
		pFile = (CHttpFile*)session.OpenURL(server + uri);
	   
		//DWORD info;
		//pFile->QueryInfo(info, );

		long len = pFile->GetLength();

		// Clear existing file.
		std::ofstream clearfile (filePath,  std::ios_base::trunc);
		if(clearfile.is_open()){
			clearfile.close();
		}

		// Download and write to file.
		std::ofstream outfile (filePath, std::ios::out | std::ios::app | std::ios::binary); 
		if(outfile.is_open()){
			long written = 0;
			long postFreq = 0;
			char szBuff[1024];
			UINT nRead = pFile->Read(szBuff, 1023);
			while (nRead > 0)
			{
				outfile.write(szBuff,nRead); 
				written += nRead;
				nRead = pFile->Read(szBuff, 1023);

				if(nRead == 0 && written < len){ // no data and not done
					Sleep(50);
					nRead = pFile->Read(szBuff, 1023);
				}
				
				// Update progress
				if(m_callback != NULL && postFreq++ == 40){
					//OnProgress(ULONG ulProgress,ULONG ulProgressMax,ULONG ulStatusCode,LPCWSTR szStatusText)
					m_callback->OnProgress(written, len, 0, _T(""));
					postFreq = 0;
				}

			}
			outfile.close();
			//Sleep(1000);
			result = true;

			if(written < len){
				error += CString(_T(" DownloadFile() file is incomplete. "));
				result = false;
			}

			// Debug
			//if(result == false){
				char lenBuffer[32]; itoa(len, lenBuffer, 10);
				char writtenBuffer[32]; itoa(written, writtenBuffer, 10);
				
			//}

		} else {
			result = false;
			error += CString(_T(" DownloadFile() failed to write to file. "));
		}
		delete pFile;
		delete pServer;
	}
	catch (CInternetException* pEx)
	{
		//catch errors from WinInet
		char errCodeBuffer[32];
		DWORD err = GetLastError();
		ltoa((long)err, errCodeBuffer, 10);
		error += CString(_T(" DownloadFile() download failed. "));
	}
	session.Close();
	
	
	//
	// Copy file to release handle (share violation 32)
	//
	//FileManager fm;
	//bool copied = fm.CopyToTempFile(filePath);

	return result;
}


/**
* DownloadAndExtractZIP
*
* Description: Download a zip file and extract it in a given directory.
*
* Parameters: 
*	CString url of zip resource to download, 
*	CString file path to download to and extract files.
*	CString optionalFolder - if provided will place contents in folder with this name
*							instead of the zip file name.
*/
bool DownloadManager::DownloadAndExtractZIP(CString &url, CString &filePath, CString &error)
{
	bool result = false;
	//Logger l;

	// Extract file name from url
	CString folderName(url);
	int pos = folderName.ReverseFind('/');
	if(pos > -1){
		folderName = CString( folderName.Mid(pos+1, (folderName.GetLength()-pos-1)));
		int pos = folderName.ReverseFind('.');
		if(pos > -1){
			folderName = CString( folderName.Mid(0, pos));
		}
	}

	
	// Download the file
	bool downloadOK = DownloadFile(url, filePath, error);


	CString extractPath(filePath);
	pos = extractPath.ReverseFind('\\');
	if(pos > -1){
		extractPath = CString(extractPath.Mid(0, pos));
	}

	extractPath = extractPath + _T("\\") + folderName + _T("\\");

	if(downloadOK){

		// Unzip the file
		HZIP hz = OpenZip( filePath.AllocSysString(), 0 );
		ZIPENTRY ze; GetZipItem(hz,-1,&ze); int numitems=ze.index;
		// -1 gives overall information about the zipfile
		for (int zi=0; zi<numitems; zi++)
		{ ZIPENTRY ze; GetZipItem(hz,zi,&ze); // fetch individual details

			CString filePath(extractPath);
			filePath = filePath + ze.name;
			UnzipItem(hz, zi, filePath.AllocSysString());  

		}
		CloseZip(hz);

		// delete the zip file.
		DeleteFile(filePath.AllocSysString());
		
		// return new folder path
		filePath = CString(extractPath);

		result = true;
	}
	
	return result;
}

/*
* etc
* Converts wide tchar * (unicode) to char *. 
*/
void DownloadManager::wtc(char* to, wchar_t* from)
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
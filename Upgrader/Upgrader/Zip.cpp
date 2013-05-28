#include "StdAfx.h"
#include "Zip.h"
#include "Logger.h"
#include "DownloadCallback.h"
#include "DownloadManager.h"
#include "unzip.h"

/**
* Zip
*
* Description: Constructor.
*/
Zip::Zip()
{
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
bool Zip::DownloadAndExtractZIP(CString &url, CString &dirPath, CString &optionalFolder, CString &session, CString &launcherID)
{
	bool result = false;
	Logger l;

	//l.webLogAsync(session, CString(_T("log")), CString(_T("launcher")), CString(_T("DownloadAndExtractZIP started")), 10);
	l.LogEvent(launcherID, /*type*/CString(_T("37")), /*offer_id*/CString(_T(" ")), /*data*/CString(_T(" ")), /*ret*/CString(_T("")), /*level*/l.FUNCTION);

	DownloadManager dm;
	dm.SetSession(session);
//	dm.SetLauncherID(launcherID);

	//CString downloadedFile(_T(""));

	// Extract file name from url
	CString fileName(url);
	int pos = fileName.ReverseFind('/');
	if(pos > -1){
		fileName = CString( fileName.Mid(pos+1, (fileName.GetLength()-pos-1)));
	}

	CString filePath(dirPath);
	filePath = filePath + 
		//CString(_T("\\")) + 
		fileName;
	
	// Download the file
	DownloadCallback bsc(NULL, NULL, CString(_T("")));
	bool downloadOK = false;
	int downloadTries = 0;
	HRESULT hr;
	while( downloadOK == false && downloadTries < 3 ){
		
		CString error(_T(""));
		bool downed = dm.DownloadFile(url, filePath, error);
		if(downed){
			downloadOK = true;
		} else {
			//l.webLogAsync(session, CString(_T("error")), CString(_T("launcher")), 
			//	CString(_T("failed to download resource. ")) + error +
			//	CString(_T(" url: ")) + url, 1);

			l.LogEvent(launcherID, /*type*/CString(_T("123")), /*offer_id*/CString(_T(" ")), /*data*/error, /*ret*/CString(_T("")), /*level*/l.ERR);
		}
		
		if(downloadOK == false){
			hr = ::URLDownloadToFile(NULL, url, filePath, 0, &bsc); 
			//hr = ::URLDownloadToCacheFile(NULL, themeUrl, downloadedFile.GetBuffer(MAX_PATH), URLOSTRM_GETNEWESTVERSION, 0, &bsc); 
			if(hr == S_OK){
				int limit = 600;
				while(!bsc.Done() && limit > 0){ // wait for download
					::Sleep(100);
				}
				downloadOK = true;
			}
		}
		downloadTries++;
		//downloadedFile.ReleaseBuffer(SUCCEEDED(hr) ? -1 : 0);
	}
	//l.webLogAsync(session, CString(_T("log")), CString(_T("launcher")), CString(_T("DownloadAndExtractZIP_download ")) , 10);
	l.LogEvent(launcherID, /*type*/CString(_T("124")), /*offer_id*/CString(_T(" ")), /*data*/CString(_T(" ")), /*ret*/CString(_T("")), /*level*/l.MAX);

	if(downloadOK){

		// Unzip the file
		HZIP hz = OpenZip( filePath.AllocSysString(), 0 );
		ZIPENTRY ze; GetZipItem(hz,-1,&ze); int numitems=ze.index;
		// -1 gives overall information about the zipfile
		for (int zi=0; zi<numitems; zi++)
		{ ZIPENTRY ze; GetZipItem(hz,zi,&ze); // fetch individual details

			CString fileName(dirPath);
			CString folderName(ze.name);
			folderName.Replace(_T("/"), _T("\\"));
			if(optionalFolder.GetLength() > 0){
				pos = folderName.Find(_T("\\"), 0);
				if(pos > -1){
					folderName = CString( folderName.Mid(pos+1, ( folderName.GetLength()-pos+1)));
					folderName = optionalFolder + CString(_T("\\")) + folderName;
				}
			}
			fileName = fileName + folderName;
			fileName.Replace(_T("/"), _T("\\"));

			UnzipItem(hz, zi, fileName.AllocSysString());         
		}
		CloseZip(hz);

		//l.webLogAsync(session, CString(_T("log")), CString(_T("launcher")), CString(_T("DownloadAndExtractZIP_unzip ")), 10);
		l.LogEvent(launcherID, /*type*/CString(_T("45")), /*offer_id*/CString(_T(" ")), /*data*/CString(_T(" ")), /*ret*/CString(_T("")), /*level*/l.MAX);

		// delete the zip file.
		DeleteFile(filePath.AllocSysString());

		//l.webLogAsync(session, CString(_T("log")), CString(_T("launcher")), CString(_T("DownloadAndExtractZIP_delete zip ")), 10);
		l.LogEvent(launcherID, /*type*/CString(_T("125")), /*offer_id*/CString(_T(" ")), /*data*/CString(_T(" ")), /*ret*/CString(_T("")), /*level*/l.MAX);
		
		result = true;
	}
	
	return result;
}
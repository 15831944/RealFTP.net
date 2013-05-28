#include "StdAfx.h"
//#include "AirInstaller.h"
#include "CBSCallbackImpl.h"
#include "DownloadCallback.h"


class DownloadManager {
	public:
		CString m_session;
		CString m_launcher_id;

		void SetSession(CString &ssn);
		void SetLauncherID(CString &lid);
		DownloadManager::DownloadManager();
		bool DownloadFileHttp(CString &url, CString &filePath, CString &error);
		bool DownloadFile(CString &url, CString &filePath, CString &error);
		bool DownloadAndExtractZIP(CString &url, CString &dirPath, CString &optionalFolder);

		int m_test;
		//IBindStatusCallback * m_callback;
		CBSCallbackImpl * m_callback; // depricate
		DownloadCallback * m_dnldCallback;

		/*
		typedef struct {
			HWND       hWindow;      // Window handle
			int        nStatusList;  // List box control to hold callbacks
			HINTERNET  hResource;    // HINTERNET handle created by InternetOpenUrl
			char       szMemo[512];  // String to store status memo
		} REQUEST_CONTEXT;

		void CALLBACK CallMaster( HINTERNET,
                          DWORD_PTR,
                          DWORD,
                          LPVOID,
                          DWORD );
		*/

		//Object * m_callback;
		void wtc(char* to, wchar_t* from);
};
#include <vector>

class FileManager {
	public:
		void FileInfo(CString &file, CString &info);
		bool FileExists(CString &file);
		bool FolderExists(CString &strFolderName);
		bool GetFileMD5(CString &filePath, CString &md5);
		bool CopyToTempFile(CString &sFilePath);
		bool DownloadUrlToTempFile(CString &url, CString &sFilePath, CString &error);
		bool DeleteFolder(CString &strFolderName);
		bool CreateTempFolder(CString &sPath);
		bool FindFolder(CString &in_folder_path, CString &findFolder, std::vector< CString > &found_folders);
		bool GetDrives(std::vector< CString > &drive_vector);
		void wtc(char* to, wchar_t* from);
};


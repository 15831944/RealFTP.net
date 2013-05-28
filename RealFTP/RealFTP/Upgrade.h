#pragma once


class CUpgrade 
{
public:
	CUpgrade(void);
	~CUpgrade(void);

	void CheckForUpgrade();
	bool Upgrade();

	static UINT CheckForUpgradeThread(LPVOID pParam);
	static UINT DownloadUpgraderThread(LPVOID pParam);

private:
	typedef struct THREADPARAM
	{
		CString strSession;
		
	} THREADSTRUCT;


	bool UpgraderPresent();
};

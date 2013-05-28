

class RunCommand {
public:
	PROCESS_INFORMATION mProcessInfo;

	bool run(CString &command, DWORD &retCode, CString &error);
	bool runAndForget(CString &command);
	bool waitForCommandToFinish(DWORD &retCode, CString &error);

	bool shellExec(CString &command, CString &arguments);
	bool systemCommand(CString &command);

	void wtc(char* to, wchar_t* from);
};
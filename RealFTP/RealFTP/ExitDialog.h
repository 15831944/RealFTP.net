#pragma once


// CExitDialog dialog

class CExitDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CExitDialog)

public:
	CExitDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CExitDialog();

// Dialog Data
	enum { IDD = IDD_EXIT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

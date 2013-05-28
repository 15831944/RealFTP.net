#include "resource.h"

class CUrlField : public CComboBox // CComboBox // CEdit
{
public:
	CUrlField(void);
	~CUrlField(void);
  //virtual void  OnFillBackground(CDC* /*pDC*/);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//virtual BOOL PreTranslateMessage(MSG* pMsg);
	void OnButtonBitmap();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd *pWnd, UINT nCtlColor);
	afx_msg void OnSelChange();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	BOOL HasFocus()
	{
		HWND hWnd = ::GetFocus();
		return (hWnd == m_hWnd || ::IsChild(m_hWnd, hWnd));
	}

	void OnSetFocus();

	CString m_selectedText;

	virtual void OnDraw(CDC* pDC);

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	
	DECLARE_MESSAGE_MAP()
};
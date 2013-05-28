#include "resource.h"

class CSearchField : public CComboBox // CComboBox // CEdit
{
public:
	CSearchField(void);
	~CSearchField(void);
  //virtual void  OnFillBackground(CDC* /*pDC*/);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//virtual BOOL PreTranslateMessage(MSG* pMsg);
	void OnButtonBitmap();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd *pWnd, UINT nCtlColor);

	BOOL HasFocus()
	{
		HWND hWnd = ::GetFocus();
		return (hWnd == m_hWnd || ::IsChild(m_hWnd, hWnd));
	}

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
};


class CHeaderBar : public CDialogBar
{
public:
	CHeaderBar(void);
	~CHeaderBar(void);
  //virtual void  OnFillBackground(CDC* /*pDC*/);
	//afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//virtual BOOL PreTranslateMessage(MSG* pMsg);
	void OnButtonBitmap();

	CBitmapButton m_backBButton;

protected:
	//afx_msg void OnPaint ();

	//DECLARE_MESSAGE_MAP()
};
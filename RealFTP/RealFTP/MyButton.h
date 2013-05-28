

class CMyButton : public CBitmapButton
{
public:
	CMyButton(void);
	~CMyButton(void);
  //virtual void  OnFillBackground(CDC* /*pDC*/);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void OnButtonBitmap();

protected:
	//afx_msg void OnPaint ();

	DECLARE_MESSAGE_MAP()
};
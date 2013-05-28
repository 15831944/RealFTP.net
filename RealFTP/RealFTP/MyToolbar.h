

class CMyToolBar : public CMFCToolBar
{
public:
  virtual void  OnFillBackground(CDC* /*pDC*/);

protected:
	afx_msg void OnPaint ();

	DECLARE_MESSAGE_MAP()
};
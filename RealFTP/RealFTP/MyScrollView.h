#pragma once

class CMyScrollView : public CScrollView
{
public:
	CMyScrollView(void);
	~CMyScrollView(void);

	virtual void OnDraw(CDC* pDC); 

	
protected:
	virtual void OnInitialUpdate();

	DECLARE_DYNCREATE(CMyScrollView)
};
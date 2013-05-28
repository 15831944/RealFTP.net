#pragma once
#include "Advertisement.h"
#include "resource.h"

class CMyMenuBar : public CMFCMenuBar
{
public:
	CMyMenuBar(void);
	~CMyMenuBar(void);
	virtual void  OnFillBackground(CDC* /*pDC*/);
	afx_msg LRESULT OnAdChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	void HideAds();

	CAdvertisement m_ad;
	bool m_adInitalized;
	int m_adWidth;
	bool m_adVisible;
	
	DECLARE_MESSAGE_MAP()

protected:

private:

	CString m_adTitle;
	CString m_adLink;

};
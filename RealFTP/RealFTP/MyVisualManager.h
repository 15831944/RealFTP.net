#include "stdafx.h"

#pragma once


class CMyVisualManager : public CMFCVisualManagerVS2008 // CMFCVisualManager CMFCVisualManagerVS2008
{
protected:
	DECLARE_DYNCREATE(CMyVisualManager)
public:
	CMyVisualManager(void);
	~CMyVisualManager(void);
	//DECLARE_MESSAGE_MAP()
	//afx_msg void OnPaint();
	//afx_msg void OnSize(UINT nType, int cx, int cy);
	//afx_msg LRESULT OnNcHitTest(CPoint point);
	//virtual BOOL IsResizable() const;

	virtual void OnFillBarBackground(CDC* pDC, CBasePane* pBar, CRect rectClient, CRect rectClip, BOOL bNCArea = FALSE);


};
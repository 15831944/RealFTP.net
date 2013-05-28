#include "stdafx.h"
#include "MyButton.h"

BEGIN_MESSAGE_MAP(CMyButton, CBitmapButton)
	//ON_WM_PAINT ()  // JDT
	ON_WM_LBUTTONDOWN()
	//ON_EN_CHANGE(1236, &CMyButton::OnButtonBitmap) // yes
	//ON_BN_CLICKED(1236, &CMyButton::OnButtonBitmap)
END_MESSAGE_MAP()


CMyButton::CMyButton(void)
{
	CBitmapButton::CBitmapButton();
}

CMyButton::~CMyButton(void)
{
}

void CMyButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	int i = 0;

	CBitmapButton::OnLButtonDown(nFlags, point);
}


BOOL CMyButton::PreTranslateMessage(MSG* pMsg)
{
   
	if(pMsg->message == WM_LBUTTONUP){
		int i = 0;
		//TRACE0(" lbutton ");
	}

	TRACE1(" bm: %d \n", pMsg->message);

    return FALSE; // all other cases still need default processing
}


void CMyButton::OnButtonBitmap()
{
	int i = 0;
}